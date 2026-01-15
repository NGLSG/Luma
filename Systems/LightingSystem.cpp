#include "LightingSystem.h"
#include "AreaLightSystem.h"
#include "ShadowRenderer.h"
#include "QualityManager.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/PointLightComponent.h"
#include "../Components/SpotLightComponent.h"
#include "../Components/DirectionalLightComponent.h"
#include "../Components/Transform.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/LightingRenderer.h"
#include "../Renderer/Camera.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <functional>

#include "LightingSettingsComponent.h"

namespace Systems
{
    LightingSystem::LightingSystem()
        : m_settings()
        , m_debugMode(false)
        , m_buffersCreated(false)
        , m_isDirty(true)
        , m_lastLightDataHash(0)
        , m_lastLightCount(0)
        , m_lastSettingsHash(0)
        , m_forceUpdate(true)
        , m_frameUpdateStartIndex(0)
        , m_enableFrameDistributedUpdate(true)
        , m_frameUpdateComplete(true)
    {
    }

    void LightingSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        // 从场景获取光照设置
        UpdateSettingsFromScene(scene);

        // 创建 GPU 缓冲区
        CreateBuffers(engineCtx);

        // 设置 LightingRenderer 的引用
        auto& lightingRenderer = LightingRenderer::GetInstance();
        if (lightingRenderer.IsInitialized())
        {
            lightingRenderer.SetLightingSystem(this);
        }

        // 注册到 QualityManager
        QualityManager::GetInstance().SetLightingSystem(this);

        LogInfo("LightingSystem initialized");
    }

    void LightingSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        // 1. 从场景更新光照设置
        UpdateSettingsFromScene(scene);

        // 2. 收集所有光源
        CollectLights(scene);

        // 3. 获取相机信息进行视锥剔除
        const Camera& camera = GetActiveCamera();
        CameraProperties props = camera.GetProperties();
        
        // 计算视图尺寸
        float viewWidth = props.viewport.width() / props.GetEffectiveZoom().x();
        float viewHeight = props.viewport.height() / props.GetEffectiveZoom().y();
        glm::vec2 cameraPos(props.position.x(), props.position.y());

        // 4. 执行视锥剔除
        CullLights(cameraPos, viewWidth, viewHeight);

        // 5. 检查是否需要更新 GPU 缓冲区（脏标记优化）
        bool needsUpdate = m_forceUpdate || HasLightDataChanged();
        
        if (needsUpdate)
        {
            m_isDirty = true;
            m_forceUpdate = false;
            
            // 更新哈希值
            m_lastLightDataHash = CalculateLightDataHash();
            m_lastLightCount = static_cast<uint32_t>(m_visibleLights.size());
            m_lastSettingsHash = CalculateSettingsHash();
            
            // 6. 更新 GPU 缓冲区（使用分帧更新或完整更新）
            if (m_enableFrameDistributedUpdate && m_visibleLights.size() > MAX_LIGHTS_PER_FRAME_UPDATE)
            {
                // 启动分帧更新
                m_pendingLightUpdates = m_visibleLights;
                m_frameUpdateStartIndex = 0;
                m_frameUpdateComplete = false;
                PerformFrameDistributedUpdate();
            }
            else
            {
                // 完整更新
                UpdateLightBuffer();
                m_frameUpdateComplete = true;
            }
        }
        else if (!m_frameUpdateComplete)
        {
            // 继续分帧更新
            PerformFrameDistributedUpdate();
        }
        else
        {
            m_isDirty = false;
        }

        // 7. 每帧更新面光源 GPU 缓冲区（面光源数据可能独立变化）
        UpdateAreaLightBuffer();
    }

    void LightingSystem::OnDestroy(RuntimeScene* scene)
    {
        // 从 QualityManager 注销
        QualityManager::GetInstance().SetLightingSystem(nullptr);

        // 清除 LightingRenderer 的引用
        auto& lightingRenderer = LightingRenderer::GetInstance();
        lightingRenderer.SetLightingSystem(nullptr);

        m_allLights.clear();
        m_visibleLights.clear();
        m_lightBuffer.reset();
        m_globalBuffer.reset();
        
        // 清理面光源相关资源
        m_areaLightSystem = nullptr;
        m_areaLightBuffer.reset();
        m_areaLightGlobalBuffer.reset();
        
        m_buffersCreated = false;
        
        // 重置优化相关状态
        m_lastLightDataHash = 0;
        m_lastLightCount = 0;
        m_lastSettingsHash = 0;
        m_forceUpdate = true;
        m_frameUpdateStartIndex = 0;
        m_frameUpdateComplete = true;
        m_pendingLightUpdates.clear();
        m_isDirty = true;

        LogInfo("LightingSystem destroyed");
    }

    void LightingSystem::CollectLights(RuntimeScene* scene)
    {
        m_allLights.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();
        const Camera& camera = GetActiveCamera();
        CameraProperties props = camera.GetProperties();
        glm::vec2 cameraPos(props.position.x(), props.position.y());

        // 收集点光源
        auto pointLightView = registry.view<ECS::PointLightComponent, ECS::TransformComponent>();
        for (auto entity : pointLightView)
        {
            auto& pointLight = pointLightView.get<ECS::PointLightComponent>(entity);
            auto& transform = pointLightView.get<ECS::TransformComponent>(entity);

            // 检查组件是否启用
            if (!pointLight.Enable)
                continue;

            // 检查游戏对象是否激活
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            LightInfo info;
            info.data = pointLight.ToLightData(glm::vec2(transform.position.x, transform.position.y));
            info.priority = pointLight.priority;
            info.isDirectional = false;
            
            // 计算到相机的距离
            glm::vec2 lightPos(transform.position.x, transform.position.y);
            info.distanceToCamera = glm::length(lightPos - cameraPos);

            m_allLights.push_back(info);
        }

        // 收集聚光灯
        auto spotLightView = registry.view<ECS::SpotLightComponent, ECS::TransformComponent>();
        for (auto entity : spotLightView)
        {
            auto& spotLight = spotLightView.get<ECS::SpotLightComponent>(entity);
            auto& transform = spotLightView.get<ECS::TransformComponent>(entity);

            if (!spotLight.Enable)
                continue;

            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            // 计算光源方向（基于旋转）
            float angle = transform.rotation;
            glm::vec2 direction(std::sin(angle), -std::cos(angle));

            LightInfo info;
            info.data = spotLight.ToLightData(
                glm::vec2(transform.position.x, transform.position.y),
                direction
            );
            info.priority = spotLight.priority;
            info.isDirectional = false;

            glm::vec2 lightPos(transform.position.x, transform.position.y);
            info.distanceToCamera = glm::length(lightPos - cameraPos);

            m_allLights.push_back(info);
        }

        // 收集方向光
        auto dirLightView = registry.view<ECS::DirectionalLightComponent>();
        for (auto entity : dirLightView)
        {
            auto& dirLight = dirLightView.get<ECS::DirectionalLightComponent>(entity);

            if (!dirLight.Enable)
                continue;

            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            LightInfo info;
            info.data = dirLight.ToLightData();
            info.priority = 1000; // 方向光优先级最高
            info.isDirectional = true;
            info.distanceToCamera = 0.0f; // 方向光没有距离概念

            m_allLights.push_back(info);
        }
    }

    void LightingSystem::CullLights(const glm::vec2& cameraPosition, float viewWidth, float viewHeight)
    {
        // 计算视锥边界框（添加一些边距以包含部分可见的光源）
        float margin = 100.0f; // 边距
        LightBounds viewBounds;
        viewBounds.minX = cameraPosition.x - viewWidth / 2.0f - margin;
        viewBounds.maxX = cameraPosition.x + viewWidth / 2.0f + margin;
        viewBounds.minY = cameraPosition.y - viewHeight / 2.0f - margin;
        viewBounds.maxY = cameraPosition.y + viewHeight / 2.0f + margin;

        // 过滤不在视锥内的光源
        std::vector<LightInfo> culledLights;
        culledLights.reserve(m_allLights.size());

        for (const auto& light : m_allLights)
        {
            // 方向光始终可见
            if (light.isDirectional)
            {
                culledLights.push_back(light);
                continue;
            }

            // 计算光源边界框
            LightBounds lightBounds = CalculateLightBounds(light.data);

            // 检查是否与视锥相交
            if (IsLightInView(lightBounds, viewBounds))
            {
                culledLights.push_back(light);
            }
        }

        // 按优先级排序
        SortLightsByPriority(culledLights);

        // 限制光源数量
        LimitLightCount(culledLights, MAX_LIGHTS_PER_FRAME);

        // 提取光源数据
        m_visibleLights.clear();
        m_visibleLights.reserve(culledLights.size());
        for (const auto& light : culledLights)
        {
            m_visibleLights.push_back(light.data);
        }

        // 如果光源数量超过限制，记录警告
        if (m_allLights.size() > MAX_LIGHTS_PER_FRAME && m_debugMode)
        {
            LogWarn("Light count {} exceeds limit {}, culling by priority",
                    m_allLights.size(), MAX_LIGHTS_PER_FRAME);
        }
    }

    LightBounds LightingSystem::CalculateLightBounds(const ECS::LightData& light)
    {
        LightBounds bounds;

        // 方向光没有边界
        if (light.lightType == static_cast<uint32_t>(ECS::LightType::Directional))
        {
            bounds.minX = -std::numeric_limits<float>::max();
            bounds.minY = -std::numeric_limits<float>::max();
            bounds.maxX = std::numeric_limits<float>::max();
            bounds.maxY = std::numeric_limits<float>::max();
            return bounds;
        }

        // 点光源和聚光灯使用半径计算边界
        float radius = light.radius;
        bounds.minX = light.position.x - radius;
        bounds.maxX = light.position.x + radius;
        bounds.minY = light.position.y - radius;
        bounds.maxY = light.position.y + radius;

        return bounds;
    }

    bool LightingSystem::IsLightInView(const LightBounds& lightBounds, const LightBounds& viewBounds)
    {
        return lightBounds.Intersects(viewBounds);
    }

    void LightingSystem::SortLightsByPriority(std::vector<LightInfo>& lights)
    {
        std::stable_sort(lights.begin(), lights.end(),
            [](const LightInfo& a, const LightInfo& b)
            {
                // 首先按优先级排序（高优先级在前）
                if (a.priority != b.priority)
                {
                    return a.priority > b.priority;
                }
                // 优先级相同时，按距离排序（近的在前）
                return a.distanceToCamera < b.distanceToCamera;
            });
    }

    void LightingSystem::LimitLightCount(std::vector<LightInfo>& lights, uint32_t maxCount)
    {
        if (lights.size() > maxCount)
        {
            lights.resize(maxCount);
        }
    }

    void LightingSystem::UpdateLightBuffer()
    {
        if (!m_buffersCreated)
            return;

        // 更新光照全局数据
        ECS::LightingGlobalData globalData = GetGlobalData();
        if (m_globalBuffer)
        {
            m_globalBuffer->WriteBuffer(&globalData, sizeof(ECS::LightingGlobalData));
        }

        // 更新光源数据
        if (m_lightBuffer && !m_visibleLights.empty())
        {
            size_t dataSize = m_visibleLights.size() * sizeof(ECS::LightData);
            m_lightBuffer->WriteBuffer(m_visibleLights.data(), static_cast<uint32_t>(dataSize));
        }
    }

    void LightingSystem::UpdateSettingsFromScene(RuntimeScene* scene)
    {
        if (!scene)
            return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::LightingSettingsComponent>();

        for (auto entity : view)
        {
            auto& settings = view.get<ECS::LightingSettingsComponent>(entity);
            if (settings.Enable)
            {
                m_settings = settings.ToSettingsData();
                return;
            }
        }

        // 如果没有找到光照设置组件，使用默认值
        m_settings = ECS::LightingSettingsData();
    }

    void LightingSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create light buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create light buffers");
            return;
        }

        // 创建光照全局数据缓冲区
        Nut::BufferLayout globalLayout;
        globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        globalLayout.size = sizeof(ECS::LightingGlobalData);
        globalLayout.mapped = false;

        m_globalBuffer = std::make_shared<Nut::Buffer>(globalLayout, nutContext);

        // 创建光源数据存储缓冲区
        Nut::BufferLayout lightLayout;
        lightLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        lightLayout.size = MAX_LIGHTS_PER_FRAME * sizeof(ECS::LightData);
        lightLayout.mapped = false;

        m_lightBuffer = std::make_shared<Nut::Buffer>(lightLayout, nutContext);

        // 写入初始数据以设置缓冲区大小
        ECS::LightingGlobalData initialGlobalData(m_settings);
        initialGlobalData.lightCount = 0;
        m_globalBuffer->WriteBuffer(&initialGlobalData, sizeof(ECS::LightingGlobalData));

        // 写入空的光源数据（至少一个占位）
        ECS::LightData emptyLight{};
        m_lightBuffer->WriteBuffer(&emptyLight, sizeof(ECS::LightData));

        // ==================== 面光源缓冲区 ====================
        
        // 创建面光源全局数据缓冲区（包含面光源数量等信息）
        Nut::BufferLayout areaLightGlobalLayout;
        areaLightGlobalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        areaLightGlobalLayout.size = 16; // 4 个 uint32_t: areaLightCount + padding
        areaLightGlobalLayout.mapped = false;

        m_areaLightGlobalBuffer = std::make_shared<Nut::Buffer>(areaLightGlobalLayout, nutContext);

        // 创建面光源数据存储缓冲区
        Nut::BufferLayout areaLightLayout;
        areaLightLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        areaLightLayout.size = AreaLightSystem::MAX_AREA_LIGHTS_PER_FRAME * sizeof(ECS::AreaLightData);
        areaLightLayout.mapped = false;

        m_areaLightBuffer = std::make_shared<Nut::Buffer>(areaLightLayout, nutContext);

        // 写入初始面光源全局数据
        uint32_t initialAreaLightGlobalData[4] = {0, 0, 0, 0}; // areaLightCount = 0
        m_areaLightGlobalBuffer->WriteBuffer(initialAreaLightGlobalData, sizeof(initialAreaLightGlobalData));

        // 写入空的面光源数据（至少一个占位）
        ECS::AreaLightData emptyAreaLight{};
        m_areaLightBuffer->WriteBuffer(&emptyAreaLight, sizeof(ECS::AreaLightData));

        m_buffersCreated = true;
        LogInfo("Light buffers created successfully (including area light buffers)");
    }

    ECS::LightingGlobalData LightingSystem::GetGlobalData() const
    {
        ECS::LightingGlobalData data(m_settings);
        data.lightCount = static_cast<uint32_t>(m_visibleLights.size());
        return data;
    }

    void LightingSystem::SetAmbientLight(const ECS::Color& color, float intensity)
    {
        m_settings.ambientColor = color;
        m_settings.ambientIntensity = intensity;
        m_isDirty = true;
    }

    void LightingSystem::EnableShadows(bool enable)
    {
        m_settings.enableShadows = enable;
        m_isDirty = true;
    }

    void LightingSystem::SetMaxLightsPerPixel(int maxLights)
    {
        m_settings.maxLightsPerPixel = maxLights;
        m_isDirty = true;
    }



    // ==================== 脏标记优化实现 ====================

    size_t LightingSystem::CalculateLightDataHash() const
    {
        size_t hash = 0;
        
        // 简单的哈希计算：组合光源数量和关键属性
        for (const auto& light : m_visibleLights)
        {
            // 使用位置、颜色、强度等关键属性计算哈希
            hash ^= std::hash<float>{}(light.position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.intensity) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.radius) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.color.r) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.color.g) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(light.color.b) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<uint32_t>{}(light.lightType) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        
        return hash;
    }

    size_t LightingSystem::CalculateSettingsHash() const
    {
        size_t hash = 0;
        
        hash ^= std::hash<float>{}(m_settings.ambientColor.r) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(m_settings.ambientColor.g) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(m_settings.ambientColor.b) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(m_settings.ambientIntensity) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>{}(m_settings.maxLightsPerPixel) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<bool>{}(m_settings.enableShadows) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        
        return hash;
    }

    bool LightingSystem::HasLightDataChanged() const
    {
        // 检查光源数量是否变化
        if (m_visibleLights.size() != m_lastLightCount)
        {
            return true;
        }
        
        // 检查光源数据哈希是否变化
        size_t currentHash = CalculateLightDataHash();
        if (currentHash != m_lastLightDataHash)
        {
            return true;
        }
        
        // 检查设置哈希是否变化
        size_t currentSettingsHash = CalculateSettingsHash();
        if (currentSettingsHash != m_lastSettingsHash)
        {
            return true;
        }
        
        return false;
    }

    // ==================== 分帧更新实现 ====================

    bool LightingSystem::PerformFrameDistributedUpdate()
    {
        if (!m_buffersCreated || m_pendingLightUpdates.empty())
        {
            m_frameUpdateComplete = true;
            return true;
        }

        // 计算本帧要更新的光源范围
        uint32_t totalLights = static_cast<uint32_t>(m_pendingLightUpdates.size());
        uint32_t endIndex = std::min(m_frameUpdateStartIndex + MAX_LIGHTS_PER_FRAME_UPDATE, totalLights);
        uint32_t lightsToUpdate = endIndex - m_frameUpdateStartIndex;

        if (lightsToUpdate == 0)
        {
            m_frameUpdateComplete = true;
            return true;
        }

        // 更新全局数据（每帧都更新，因为 lightCount 可能变化）
        ECS::LightingGlobalData globalData = GetGlobalData();
        if (m_globalBuffer)
        {
            m_globalBuffer->WriteBuffer(&globalData, sizeof(ECS::LightingGlobalData));
        }

        // 分帧更新光源数据
        if (m_lightBuffer)
        {
            // 计算偏移量和数据大小
            size_t offset = m_frameUpdateStartIndex * sizeof(ECS::LightData);
            size_t dataSize = lightsToUpdate * sizeof(ECS::LightData);
            
            // 写入部分数据
            // 注意：这里假设 Buffer 支持带偏移的写入，如果不支持则需要完整更新
            // 为了简化，我们在第一帧写入所有数据
            if (m_frameUpdateStartIndex == 0)
            {
                // 第一帧：写入完整数据
                size_t totalDataSize = m_pendingLightUpdates.size() * sizeof(ECS::LightData);
                m_lightBuffer->WriteBuffer(m_pendingLightUpdates.data(), static_cast<uint32_t>(totalDataSize));
            }
        }

        // 更新索引
        m_frameUpdateStartIndex = endIndex;

        // 检查是否完成
        if (m_frameUpdateStartIndex >= totalLights)
        {
            m_frameUpdateComplete = true;
            m_pendingLightUpdates.clear();
            m_frameUpdateStartIndex = 0;
            
            if (m_debugMode)
            {
                LogInfo("Frame-distributed light update complete: {} lights", totalLights);
            }
            
            return true;
        }

        return false;
    }

    float LightingSystem::GetFrameUpdateProgress() const
    {
        if (m_frameUpdateComplete || m_pendingLightUpdates.empty())
        {
            return 1.0f;
        }
        
        return static_cast<float>(m_frameUpdateStartIndex) / 
               static_cast<float>(m_pendingLightUpdates.size());
    }

    // ==================== 面光源集成实现 ====================

    uint32_t LightingSystem::GetAreaLightCount() const
    {
        if (m_areaLightSystem)
        {
            return m_areaLightSystem->GetAreaLightCount();
        }
        return 0;
    }

    void LightingSystem::UpdateAreaLightBuffer()
    {
        if (!m_buffersCreated || !m_areaLightSystem)
            return;

        const auto& areaLights = m_areaLightSystem->GetVisibleAreaLights();
        uint32_t areaLightCount = static_cast<uint32_t>(areaLights.size());

        // 更新面光源全局数据
        if (m_areaLightGlobalBuffer)
        {
            uint32_t globalData[4] = {areaLightCount, 0, 0, 0};
            m_areaLightGlobalBuffer->WriteBuffer(globalData, sizeof(globalData));
        }

        // 更新面光源数据
        if (m_areaLightBuffer && !areaLights.empty())
        {
            size_t dataSize = areaLights.size() * sizeof(ECS::AreaLightData);
            m_areaLightBuffer->WriteBuffer(areaLights.data(), static_cast<uint32_t>(dataSize));
        }
    }

    // ==================== 阴影方法控制实现 (Requirements: 7.5) ====================

    void LightingSystem::SetShadowMethod(ECS::ShadowMethod method)
    {
        auto* shadowRenderer = ShadowRenderer::GetInstance();
        if (shadowRenderer)
        {
            shadowRenderer->SetShadowMethod(method);
            LogInfo("LightingSystem: Shadow method set to {}", static_cast<int>(method));
        }
        else
        {
            LogWarn("LightingSystem: ShadowRenderer not available, cannot set shadow method");
        }
    }

    ECS::ShadowMethod LightingSystem::GetShadowMethod() const
    {
        auto* shadowRenderer = ShadowRenderer::GetInstance();
        if (shadowRenderer)
        {
            return shadowRenderer->GetShadowMethod();
        }
        return ECS::ShadowMethod::Basic;
    }

    bool LightingSystem::IsShadowMethodSupported(ECS::ShadowMethod method) const
    {
        auto* shadowRenderer = ShadowRenderer::GetInstance();
        if (shadowRenderer)
        {
            return shadowRenderer->IsShadowMethodSupported(method);
        }
        return method == ECS::ShadowMethod::Basic;
    }
}
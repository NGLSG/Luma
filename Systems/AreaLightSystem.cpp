#include "AreaLightSystem.h"
#include "LightingSystem.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/Transform.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/Camera.h"
#include "LightingMath.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Systems
{
    AreaLightSystem::AreaLightSystem()
        : m_debugMode(false)
        , m_buffersCreated(false)
        , m_isDirty(true)
    {
    }

    void AreaLightSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        // 创建 GPU 缓冲区
        CreateBuffers(engineCtx);

        // 注意：此时 LightingSystem 可能还没有创建，连接将在 OnUpdate 中延迟进行
        m_connectedToLightingSystem = false;

        LogInfo("AreaLightSystem initialized");
    }

    void AreaLightSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        // 延迟连接到 LightingSystem（因为 AreaLightSystem 在 LightingSystem 之前创建）
        if (!m_connectedToLightingSystem && scene)
        {
            auto* lightingSystem = scene->GetSystem<LightingSystem>();
            if (lightingSystem)
            {
                lightingSystem->SetAreaLightSystem(this);
                m_connectedToLightingSystem = true;
                LogInfo("AreaLightSystem connected to LightingSystem");
            }
        }

        // 1. 收集所有面光源
        CollectAreaLights(scene);

        // 2. 获取相机信息进行视锥剔除
        const Camera& camera = GetActiveCamera();
        CameraProperties props = camera.GetProperties();
        
        // 计算视图尺寸
        float viewWidth = props.viewport.width() / props.GetEffectiveZoom().x();
        float viewHeight = props.viewport.height() / props.GetEffectiveZoom().y();
        glm::vec2 cameraPos(props.position.x(), props.position.y());

        // 3. 执行视锥剔除
        CullAreaLights(cameraPos, viewWidth, viewHeight);

        // 4. 更新 GPU 缓冲区
        UpdateAreaLightBuffer();
    }

    void AreaLightSystem::OnDestroy(RuntimeScene* scene)
    {
        // 断开与 LightingSystem 的连接
        if (m_connectedToLightingSystem && scene)
        {
            auto* lightingSystem = scene->GetSystem<LightingSystem>();
            if (lightingSystem && lightingSystem->GetAreaLightSystem() == this)
            {
                lightingSystem->SetAreaLightSystem(nullptr);
                LogInfo("AreaLightSystem disconnected from LightingSystem");
            }
        }

        m_allAreaLights.clear();
        m_visibleAreaLights.clear();
        m_areaLightBuffer.reset();
        m_buffersCreated = false;
        m_isDirty = true;
        m_connectedToLightingSystem = false;

        LogInfo("AreaLightSystem destroyed");
    }

    void AreaLightSystem::CollectAreaLights(RuntimeScene* scene)
    {
        m_allAreaLights.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();
        const Camera& camera = GetActiveCamera();
        CameraProperties props = camera.GetProperties();
        glm::vec2 cameraPos(props.position.x(), props.position.y());

        // 收集面光源
        auto areaLightView = registry.view<ECS::AreaLightComponent, ECS::TransformComponent>();
        for (auto entity : areaLightView)
        {
            auto& areaLight = areaLightView.get<ECS::AreaLightComponent>(entity);
            auto& transform = areaLightView.get<ECS::TransformComponent>(entity);

            // 检查组件是否启用
            if (!areaLight.Enable)
                continue;

            // 检查游戏对象是否激活
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            AreaLightInfo info;
            info.data = areaLight.ToAreaLightData(glm::vec2(transform.position.x, transform.position.y));
            info.priority = areaLight.priority;
            
            // 计算到相机的距离
            glm::vec2 lightPos(transform.position.x, transform.position.y);
            info.distanceToCamera = glm::length(lightPos - cameraPos);

            m_allAreaLights.push_back(info);
        }
    }

    void AreaLightSystem::CullAreaLights(const glm::vec2& cameraPosition, float viewWidth, float viewHeight)
    {
        // 计算视锥边界框（添加一些边距以包含部分可见的面光源）
        float margin = 100.0f;
        AreaLightBounds viewBounds;
        viewBounds.minX = cameraPosition.x - viewWidth / 2.0f - margin;
        viewBounds.maxX = cameraPosition.x + viewWidth / 2.0f + margin;
        viewBounds.minY = cameraPosition.y - viewHeight / 2.0f - margin;
        viewBounds.maxY = cameraPosition.y + viewHeight / 2.0f + margin;

        // 过滤不在视锥内的面光源
        std::vector<AreaLightInfo> culledLights;
        culledLights.reserve(m_allAreaLights.size());

        for (const auto& light : m_allAreaLights)
        {
            // 计算面光源边界框
            AreaLightBounds lightBounds = CalculateAreaLightBounds(light.data);

            // 检查是否与视锥相交
            if (IsAreaLightInView(lightBounds, viewBounds))
            {
                culledLights.push_back(light);
            }
        }

        // 按优先级排序
        SortAreaLightsByPriority(culledLights);

        // 限制面光源数量
        LimitAreaLightCount(culledLights, MAX_AREA_LIGHTS_PER_FRAME);

        // 提取面光源数据
        m_visibleAreaLights.clear();
        m_visibleAreaLights.reserve(culledLights.size());
        for (const auto& light : culledLights)
        {
            m_visibleAreaLights.push_back(light.data);
        }

        // 如果面光源数量超过限制，记录警告
        if (m_allAreaLights.size() > MAX_AREA_LIGHTS_PER_FRAME && m_debugMode)
        {
            LogWarn("Area light count {} exceeds limit {}, culling by priority",
                    m_allAreaLights.size(), MAX_AREA_LIGHTS_PER_FRAME);
        }
    }

    AreaLightBounds AreaLightSystem::CalculateAreaLightBounds(const ECS::AreaLightData& areaLight)
    {
        AreaLightBounds bounds;

        // 面光源使用影响半径计算边界
        float radius = areaLight.radius;
        
        // 考虑面光源自身的尺寸
        float halfWidth = areaLight.size.x / 2.0f;
        float halfHeight = areaLight.size.y / 2.0f;
        
        // 边界 = 位置 ± (影响半径 + 自身尺寸的一半)
        bounds.minX = areaLight.position.x - radius - halfWidth;
        bounds.maxX = areaLight.position.x + radius + halfWidth;
        bounds.minY = areaLight.position.y - radius - halfHeight;
        bounds.maxY = areaLight.position.y + radius + halfHeight;

        return bounds;
    }

    bool AreaLightSystem::IsAreaLightInView(const AreaLightBounds& lightBounds, const AreaLightBounds& viewBounds)
    {
        return lightBounds.Intersects(viewBounds);
    }

    void AreaLightSystem::SortAreaLightsByPriority(std::vector<AreaLightInfo>& lights)
    {
        std::stable_sort(lights.begin(), lights.end(),
            [](const AreaLightInfo& a, const AreaLightInfo& b)
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

    void AreaLightSystem::LimitAreaLightCount(std::vector<AreaLightInfo>& lights, uint32_t maxCount)
    {
        if (lights.size() > maxCount)
        {
            lights.resize(maxCount);
        }
    }

    void AreaLightSystem::UpdateAreaLightBuffer()
    {
        if (!m_buffersCreated)
            return;

        // 更新面光源数据
        if (m_areaLightBuffer && !m_visibleAreaLights.empty())
        {
            size_t dataSize = m_visibleAreaLights.size() * sizeof(ECS::AreaLightData);
            m_areaLightBuffer->WriteBuffer(m_visibleAreaLights.data(), static_cast<uint32_t>(dataSize));
        }
    }

    void AreaLightSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create area light buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create area light buffers");
            return;
        }

        // 创建面光源数据存储缓冲区
        Nut::BufferLayout areaLightLayout;
        areaLightLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        areaLightLayout.size = MAX_AREA_LIGHTS_PER_FRAME * sizeof(ECS::AreaLightData);
        areaLightLayout.mapped = false;

        m_areaLightBuffer = std::make_shared<Nut::Buffer>(areaLightLayout, nutContext);

        // 写入空的面光源数据（至少一个占位）
        ECS::AreaLightData emptyLight{};
        m_areaLightBuffer->WriteBuffer(&emptyLight, sizeof(ECS::AreaLightData));

        m_buffersCreated = true;
        LogInfo("Area light buffers created successfully");
    }


    // ==================== 面光源光照计算 ====================

    float AreaLightSystem::CalculateRectangleLightContribution(
        const ECS::AreaLightData& areaLight,
        const glm::vec2& targetPosition)
    {
        // 计算目标点到面光源中心的距离
        glm::vec2 toTarget = targetPosition - areaLight.position;
        float distance = glm::length(toTarget);

        // 如果超出影响半径，返回 0
        if (distance >= areaLight.radius)
        {
            return 0.0f;
        }

        // 计算目标点到矩形面光源的最近点
        float halfWidth = areaLight.size.x / 2.0f;
        float halfHeight = areaLight.size.y / 2.0f;

        // 将目标点转换到面光源的局部坐标系
        glm::vec2 localPos = toTarget;

        // 计算到矩形边界的最近点
        float closestX = std::clamp(localPos.x, -halfWidth, halfWidth);
        float closestY = std::clamp(localPos.y, -halfHeight, halfHeight);
        glm::vec2 closestPoint(closestX, closestY);

        // 计算到最近点的距离
        float distToSurface = glm::length(localPos - closestPoint);

        // 如果目标点在矩形内部，距离为 0
        if (distToSurface < Lighting::MIN_RADIUS)
        {
            distToSurface = Lighting::MIN_RADIUS;
        }

        // 计算衰减
        float attenuation = Lighting::CalculateAttenuation(
            distToSurface, 
            areaLight.radius, 
            static_cast<ECS::AttenuationType>(static_cast<int>(areaLight.attenuation)));

        // 返回光照贡献 = 强度 * 衰减
        return areaLight.intensity * attenuation;
    }

    float AreaLightSystem::CalculateCircleLightContribution(
        const ECS::AreaLightData& areaLight,
        const glm::vec2& targetPosition)
    {
        // 计算目标点到面光源中心的距离
        glm::vec2 toTarget = targetPosition - areaLight.position;
        float distance = glm::length(toTarget);

        // 如果超出影响半径，返回 0
        if (distance >= areaLight.radius)
        {
            return 0.0f;
        }

        // 圆形面光源的半径（使用 width 作为直径）
        float lightRadius = areaLight.size.x / 2.0f;

        // 计算到圆形表面的距离
        float distToSurface = std::max(0.0f, distance - lightRadius);

        // 如果目标点在圆形内部，距离为 0
        if (distToSurface < Lighting::MIN_RADIUS)
        {
            distToSurface = Lighting::MIN_RADIUS;
        }

        // 计算衰减
        float attenuation = Lighting::CalculateAttenuation(
            distToSurface, 
            areaLight.radius, 
            static_cast<ECS::AttenuationType>(static_cast<int>(areaLight.attenuation)));

        // 返回光照贡献 = 强度 * 衰减
        return areaLight.intensity * attenuation;
    }

    float AreaLightSystem::CalculateAreaLightContribution(
        const ECS::AreaLightData& areaLight,
        const glm::vec2& targetPosition)
    {
        // 根据形状类型选择计算方法
        if (areaLight.shape == static_cast<uint32_t>(ECS::AreaLightShape::Circle))
        {
            return CalculateCircleLightContribution(areaLight, targetPosition);
        }
        else
        {
            return CalculateRectangleLightContribution(areaLight, targetPosition);
        }
    }

    glm::vec3 AreaLightSystem::CalculateAreaLightColorContribution(
        const ECS::AreaLightData& areaLight,
        const glm::vec2& targetPosition,
        uint32_t spriteLayerMask)
    {
        // 检查光照层
        if (!Lighting::LightAffectsLayer(areaLight.layerMask, spriteLayerMask))
        {
            return glm::vec3(0.0f);
        }

        // 计算光照贡献
        float contribution = CalculateAreaLightContribution(areaLight, targetPosition);

        if (contribution <= 0.0f)
        {
            return glm::vec3(0.0f);
        }

        // 返回颜色贡献
        return glm::vec3(areaLight.color.r, areaLight.color.g, areaLight.color.b) * contribution;
    }

    // ==================== 面光源转换为点光源 ====================

    std::vector<ECS::LightData> AreaLightSystem::ConvertToPointLights(
        const ECS::AreaLightData& areaLight, 
        int sampleCount)
    {
        std::vector<ECS::LightData> pointLights;
        
        // 限制采样数量
        sampleCount = std::clamp(sampleCount, 1, static_cast<int>(MAX_SAMPLES_PER_AREA_LIGHT));
        
        // 每个点光源的强度 = 总强度 / 采样数
        float intensityPerSample = areaLight.intensity / static_cast<float>(sampleCount);

        if (areaLight.shape == static_cast<uint32_t>(ECS::AreaLightShape::Circle))
        {
            // 圆形面光源：在圆周上均匀采样
            float lightRadius = areaLight.size.x / 2.0f;
            
            for (int i = 0; i < sampleCount; ++i)
            {
                float angle = (2.0f * static_cast<float>(M_PI) * i) / static_cast<float>(sampleCount);
                
                // 在圆周上的采样点（使用 0.7 倍半径以获得更好的分布）
                float sampleRadius = lightRadius * 0.7f;
                glm::vec2 offset(std::cos(angle) * sampleRadius, std::sin(angle) * sampleRadius);
                
                ECS::LightData pointLight;
                pointLight.position = areaLight.position + offset;
                pointLight.direction = glm::vec2(0.0f, 0.0f);
                pointLight.color = areaLight.color;
                pointLight.intensity = intensityPerSample;
                pointLight.radius = areaLight.radius;
                pointLight.innerAngle = 0.0f;
                pointLight.outerAngle = 0.0f;
                pointLight.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                pointLight.layerMask = areaLight.layerMask;
                pointLight.attenuation = areaLight.attenuation;
                pointLight.castShadows = 0; // 转换后的点光源不投射阴影
                
                pointLights.push_back(pointLight);
            }
        }
        else
        {
            // 矩形面光源：在矩形内均匀采样
            float halfWidth = areaLight.size.x / 2.0f;
            float halfHeight = areaLight.size.y / 2.0f;
            
            // 计算网格尺寸
            int gridSize = static_cast<int>(std::sqrt(static_cast<float>(sampleCount)));
            if (gridSize < 1) gridSize = 1;
            
            float stepX = (gridSize > 1) ? (areaLight.size.x * 0.8f) / (gridSize - 1) : 0.0f;
            float stepY = (gridSize > 1) ? (areaLight.size.y * 0.8f) / (gridSize - 1) : 0.0f;
            float startX = -halfWidth * 0.8f;
            float startY = -halfHeight * 0.8f;
            
            int count = 0;
            for (int y = 0; y < gridSize && count < sampleCount; ++y)
            {
                for (int x = 0; x < gridSize && count < sampleCount; ++x)
                {
                    glm::vec2 offset;
                    if (gridSize > 1)
                    {
                        offset = glm::vec2(startX + x * stepX, startY + y * stepY);
                    }
                    else
                    {
                        offset = glm::vec2(0.0f, 0.0f);
                    }
                    
                    ECS::LightData pointLight;
                    pointLight.position = areaLight.position + offset;
                    pointLight.direction = glm::vec2(0.0f, 0.0f);
                    pointLight.color = areaLight.color;
                    pointLight.intensity = intensityPerSample;
                    pointLight.radius = areaLight.radius;
                    pointLight.innerAngle = 0.0f;
                    pointLight.outerAngle = 0.0f;
                    pointLight.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                    pointLight.layerMask = areaLight.layerMask;
                    pointLight.attenuation = areaLight.attenuation;
                    pointLight.castShadows = 0;
                    
                    pointLights.push_back(pointLight);
                    ++count;
                }
            }
        }

        return pointLights;
    }

    std::vector<ECS::LightData> AreaLightSystem::GetConvertedPointLights(int sampleCount) const
    {
        std::vector<ECS::LightData> allPointLights;
        
        for (const auto& areaLight : m_visibleAreaLights)
        {
            auto pointLights = ConvertToPointLights(areaLight, sampleCount);
            allPointLights.insert(allPointLights.end(), pointLights.begin(), pointLights.end());
        }
        
        return allPointLights;
    }
}

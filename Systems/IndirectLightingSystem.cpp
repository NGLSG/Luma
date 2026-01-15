#include "IndirectLightingSystem.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/Transform.h"
#include "../Components/Sprite.h"
#include "../Components/LightingSettingsComponent.h"
#include "../Renderer/GraphicsBackend.h"
#include "LightingSystem.h"
#include "Logger.h"
#include <algorithm>

namespace Systems
{
    IndirectLightingSystem* IndirectLightingSystem::s_instance = nullptr;

    IndirectLightingSystem::IndirectLightingSystem()
        : m_enabled(true)
        , m_buffersCreated(false)
        , m_indirectRadius(200.0f)
        , m_engineCtx(nullptr)
    {
    }

    void IndirectLightingSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        m_engineCtx = &engineCtx;
        s_instance = this;

        // 创建 GPU 缓冲区
        CreateBuffers(engineCtx);

        LogInfo("IndirectLightingSystem initialized");
    }

    void IndirectLightingSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!m_enabled)
            return;

        // 1. 从场景更新设置
        UpdateSettingsFromScene(scene);

        // 2. 收集反射体
        CollectReflectors(scene);

        // 3. 更新 GPU 缓冲区
        UpdateBuffers();
    }

    void IndirectLightingSystem::OnDestroy(RuntimeScene* scene)
    {
        m_reflectors.clear();
        m_reflectorBuffer.reset();
        m_globalBuffer.reset();
        m_buffersCreated = false;
        m_engineCtx = nullptr;

        if (s_instance == this)
        {
            s_instance = nullptr;
        }

        LogInfo("IndirectLightingSystem destroyed");
    }

    void IndirectLightingSystem::CollectReflectors(RuntimeScene* scene)
    {
        m_reflectors.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // 收集所有有颜色的精灵作为反射体
        auto view = registry.view<ECS::SpriteComponent, ECS::TransformComponent>();
        for (auto entity : view)
        {
            auto& sprite = view.get<ECS::SpriteComponent>(entity);
            auto& transform = view.get<ECS::TransformComponent>(entity);

            // 检查游戏对象是否激活
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            // 只有有颜色的物体才能反射光
            // 跳过完全透明或接近黑色的物体
            const auto& color = sprite.color;
            // 使用最大颜色分量来判断，确保纯色（如纯红、纯绿、纯蓝）也能反射
            float maxComponent = std::max({color.r, color.g, color.b});
            if (color.a < 0.1f || maxComponent < 0.05f)
                continue;

            // 计算物体尺寸
            float width = sprite.sourceRect.z * std::abs(transform.scale.x);
            float height = sprite.sourceRect.w * std::abs(transform.scale.y);
            
            // 跳过太小的物体
            if (width < 1.0f || height < 1.0f)
                continue;

            ECS::IndirectLightData reflector;
            reflector.position = glm::vec2(transform.position.x, transform.position.y);
            reflector.size = glm::vec2(width, height);
            reflector.color = glm::vec4(color.r, color.g, color.b, color.a);
            // 反射强度：使用颜色的最大分量，确保纯色（如纯红）也有足够的反射强度
            maxComponent = std::max({color.r, color.g, color.b});
            reflector.intensity = maxComponent * color.a;
            reflector.radius = m_indirectRadius;
            reflector.layerMask = sprite.lightLayer.value;

            // 限制反射体数量
            if (m_reflectors.size() < MAX_REFLECTORS)
            {
                m_reflectors.push_back(reflector);
            }
        }
    }

    void IndirectLightingSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create indirect lighting buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create indirect lighting buffers");
            return;
        }

        // 创建全局数据缓冲区
        Nut::BufferLayout globalLayout;
        globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        globalLayout.size = sizeof(ECS::IndirectLightingGlobalData);
        globalLayout.mapped = false;

        m_globalBuffer = std::make_shared<Nut::Buffer>(globalLayout, nutContext);

        // 创建反射体数据缓冲区
        Nut::BufferLayout reflectorLayout;
        reflectorLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        reflectorLayout.size = MAX_REFLECTORS * sizeof(ECS::IndirectLightData);
        reflectorLayout.mapped = false;

        m_reflectorBuffer = std::make_shared<Nut::Buffer>(reflectorLayout, nutContext);

        // 写入初始数据
        ECS::IndirectLightingGlobalData initialGlobal{};
        m_globalBuffer->WriteBuffer(&initialGlobal, sizeof(ECS::IndirectLightingGlobalData));

        ECS::IndirectLightData emptyReflector{};
        m_reflectorBuffer->WriteBuffer(&emptyReflector, sizeof(ECS::IndirectLightData));

        m_buffersCreated = true;
        LogInfo("Indirect lighting buffers created");
    }

    void IndirectLightingSystem::UpdateBuffers()
    {
        if (!m_buffersCreated)
            return;

        // 更新全局数据
        m_globalData.reflectorCount = static_cast<uint32_t>(m_reflectors.size());

        if (m_globalBuffer)
        {
            m_globalBuffer->WriteBuffer(&m_globalData, sizeof(ECS::IndirectLightingGlobalData));
        }

        // 更新反射体数据
        if (m_reflectorBuffer && !m_reflectors.empty())
        {
            size_t dataSize = m_reflectors.size() * sizeof(ECS::IndirectLightData);
            m_reflectorBuffer->WriteBuffer(m_reflectors.data(), static_cast<uint32_t>(dataSize));
        }
    }

    void IndirectLightingSystem::UpdateSettingsFromScene(RuntimeScene* scene)
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
                m_enabled = settings.enableIndirectLighting;
                m_globalData.indirectIntensity = settings.indirectIntensity;
                m_globalData.bounceDecay = settings.bounceDecay;
                m_globalData.enableIndirect = settings.enableIndirectLighting ? 1u : 0u;
                m_indirectRadius = settings.indirectRadius;
                return;
            }
        }
    }
}

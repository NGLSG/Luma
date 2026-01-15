#include "LightProbeSystem.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/Transform.h"
#include "../Components/PointLightComponent.h"
#include "../Components/AreaLightComponent.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/Camera.h"
#include "LightingMath.h"
#include "LightingSystem.h"
#include "AreaLightSystem.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Systems
{
    // Static instance
    LightProbeSystem* LightProbeSystem::s_instance = nullptr;

    LightProbeSystem::LightProbeSystem()
        : m_debugMode(false)
        , m_buffersCreated(false)
        , m_hasGeneratedGrid(false)
        , m_updateFrequency(DEFAULT_UPDATE_FREQUENCY)
        , m_timeSinceLastUpdate(0.0f)
        , m_realtimeUpdateEnabled(true)
        , m_engineCtx(nullptr)
    {
        s_instance = this;
    }

    void LightProbeSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        m_engineCtx = &engineCtx;
        
        // Create GPU buffers
        CreateBuffers(engineCtx);

        LogInfo("LightProbeSystem initialized");
    }

    void LightProbeSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        // 1. Collect all light probes from scene
        CollectLightProbes(scene);

        // 2. Update realtime probes if enabled
        if (m_realtimeUpdateEnabled)
        {
            UpdateRealtimeProbes(scene, deltaTime);
        }

        // 3. Update GPU buffer
        UpdateProbeBuffer();
    }

    void LightProbeSystem::OnDestroy(RuntimeScene* scene)
    {
        m_probes.clear();
        m_probeData.clear();
        m_probeBuffer.reset();
        m_buffersCreated = false;
        m_hasGeneratedGrid = false;
        m_engineCtx = nullptr;

        if (s_instance == this)
        {
            s_instance = nullptr;
        }

        LogInfo("LightProbeSystem destroyed");
    }

    void LightProbeSystem::CollectLightProbes(RuntimeScene* scene)
    {
        m_probes.clear();
        m_probeData.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // Collect light probes
        auto probeView = registry.view<ECS::LightProbeComponent, ECS::TransformComponent>();
        for (auto entity : probeView)
        {
            auto& probe = probeView.get<ECS::LightProbeComponent>(entity);
            auto& transform = probeView.get<ECS::TransformComponent>(entity);

            // Check if component is enabled
            if (!probe.Enable)
                continue;

            // Check if game object is active
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            glm::vec2 position(transform.position.x, transform.position.y);

            LightProbeInfo info;
            info.data = probe.ToLightProbeData(position);
            info.position = position;
            info.influenceRadius = probe.influenceRadius;
            info.isBaked = probe.isBaked;
            info.needsUpdate = !probe.isBaked; // Non-baked probes need updates

            m_probes.push_back(info);
            m_probeData.push_back(info.data);
        }

        // Limit probe count
        if (m_probes.size() > MAX_LIGHT_PROBES)
        {
            LogWarn("Light probe count {} exceeds limit {}, truncating",
                    m_probes.size(), MAX_LIGHT_PROBES);
            m_probes.resize(MAX_LIGHT_PROBES);
            m_probeData.resize(MAX_LIGHT_PROBES);
        }
    }


    void LightProbeSystem::UpdateRealtimeProbes(RuntimeScene* scene, float deltaTime)
    {
        m_timeSinceLastUpdate += deltaTime;

        // Check if it's time to update
        if (m_timeSinceLastUpdate < m_updateFrequency)
            return;

        m_timeSinceLastUpdate = 0.0f;

        // Update all non-baked probes
        for (size_t i = 0; i < m_probes.size(); ++i)
        {
            if (!m_probes[i].isBaked || m_probes[i].needsUpdate)
            {
                glm::vec3 color;
                float intensity;
                SampleIndirectLightAtProbe(scene, m_probes[i].position, color, intensity);

                m_probes[i].data.sampledColor = color;
                m_probes[i].data.sampledIntensity = intensity;
                m_probes[i].needsUpdate = false;

                // Update probe data array
                if (i < m_probeData.size())
                {
                    m_probeData[i].sampledColor = color;
                    m_probeData[i].sampledIntensity = intensity;
                }
            }
        }
    }

    void LightProbeSystem::SampleIndirectLightAtProbe(
        RuntimeScene* scene,
        const glm::vec2& position,
        glm::vec3& outColor,
        float& outIntensity)
    {
        outColor = glm::vec3(0.0f);
        outIntensity = 0.0f;

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // Sample from point lights
        auto pointLightView = registry.view<ECS::PointLightComponent, ECS::TransformComponent>();
        for (auto entity : pointLightView)
        {
            auto& light = pointLightView.get<ECS::PointLightComponent>(entity);
            auto& transform = pointLightView.get<ECS::TransformComponent>(entity);

            if (!light.Enable)
                continue;

            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            glm::vec2 lightPos(transform.position.x, transform.position.y);
            float distance = CalculateDistance(position, lightPos);

            if (distance < light.radius)
            {
                float attenuation = Lighting::CalculateAttenuation(
                    distance, light.radius, light.attenuation);

                // Indirect light is a fraction of direct light (bounce simulation)
                float indirectFactor = 0.3f;
                glm::vec3 lightColor(light.color.r, light.color.g, light.color.b);
                outColor += lightColor * light.intensity * attenuation * indirectFactor;
                outIntensity += light.intensity * attenuation * indirectFactor;
            }
        }

        // Sample from area lights
        auto areaLightView = registry.view<ECS::AreaLightComponent, ECS::TransformComponent>();
        for (auto entity : areaLightView)
        {
            auto& light = areaLightView.get<ECS::AreaLightComponent>(entity);
            auto& transform = areaLightView.get<ECS::TransformComponent>(entity);

            if (!light.Enable)
                continue;

            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            glm::vec2 lightPos(transform.position.x, transform.position.y);
            ECS::AreaLightData areaData = light.ToAreaLightData(lightPos);
            
            float contribution = AreaLightSystem::CalculateAreaLightContribution(areaData, position);
            
            if (contribution > 0.0f)
            {
                float indirectFactor = 0.3f;
                glm::vec3 lightColor(light.color.r, light.color.g, light.color.b);
                outColor += lightColor * contribution * indirectFactor;
                outIntensity += contribution * indirectFactor;
            }
        }

        // Clamp intensity
        outIntensity = std::min(outIntensity, 1.0f);
    }

    // ==================== Probe Grid Generation ====================

    void LightProbeSystem::GenerateProbeGrid(RuntimeScene* scene, const ECS::LightProbeGridConfig& config)
    {
        if (!scene)
            return;

        m_gridConfig = config;

        // Calculate probe spacing
        float spacingX = config.gridSize.x / static_cast<float>(std::max(1, config.probeCount.x - 1));
        float spacingY = config.gridSize.y / static_cast<float>(std::max(1, config.probeCount.y - 1));

        // Handle single probe case
        if (config.probeCount.x <= 1) spacingX = 0.0f;
        if (config.probeCount.y <= 1) spacingY = 0.0f;

        auto& registry = scene->GetRegistry();

        // Generate probes in grid pattern
        for (int y = 0; y < config.probeCount.y; ++y)
        {
            for (int x = 0; x < config.probeCount.x; ++x)
            {
                glm::vec2 position(
                    config.gridOrigin.x + x * spacingX,
                    config.gridOrigin.y + y * spacingY
                );

                // Create entity with LightProbeComponent
                auto entity = registry.create();
                
                ECS::TransformComponent transform;
                transform.position = ECS::Vector2f(position.x, position.y);
                registry.emplace<ECS::TransformComponent>(entity, transform);

                ECS::LightProbeComponent probe;
                probe.Enable = true;
                probe.influenceRadius = std::max(spacingX, spacingY) * 1.5f;
                probe.isBaked = false;
                registry.emplace<ECS::LightProbeComponent>(entity, probe);
            }
        }

        m_hasGeneratedGrid = true;
        m_updateFrequency = config.updateFrequency;

        LogInfo("Generated light probe grid: {}x{} probes", 
                config.probeCount.x, config.probeCount.y);
    }

    void LightProbeSystem::ClearProbeGrid()
    {
        m_probes.clear();
        m_probeData.clear();
        m_hasGeneratedGrid = false;
    }

    // ==================== Probe Baking ====================

    void LightProbeSystem::BakeAllProbes(RuntimeScene* scene)
    {
        if (!scene)
            return;

        for (size_t i = 0; i < m_probes.size(); ++i)
        {
            BakeProbe(scene, static_cast<uint32_t>(i));
        }

        LogInfo("Baked {} light probes", m_probes.size());
    }

    void LightProbeSystem::BakeProbe(RuntimeScene* scene, uint32_t probeIndex)
    {
        if (probeIndex >= m_probes.size())
            return;

        glm::vec3 color;
        float intensity;
        SampleIndirectLightAtProbe(scene, m_probes[probeIndex].position, color, intensity);

        m_probes[probeIndex].data.sampledColor = color;
        m_probes[probeIndex].data.sampledIntensity = intensity;
        m_probes[probeIndex].isBaked = true;
        m_probes[probeIndex].needsUpdate = false;

        if (probeIndex < m_probeData.size())
        {
            m_probeData[probeIndex].sampledColor = color;
            m_probeData[probeIndex].sampledIntensity = intensity;
        }
    }

    bool LightProbeSystem::AreAllProbesBaked() const
    {
        for (const auto& probe : m_probes)
        {
            if (!probe.isBaked)
                return false;
        }
        return true;
    }


    // ==================== Indirect Light Interpolation ====================

    glm::vec3 LightProbeSystem::InterpolateIndirectLightAt(const glm::vec2& position) const
    {
        float intensity;
        return InterpolateIndirectLightAt(position, intensity);
    }

    glm::vec3 LightProbeSystem::InterpolateIndirectLightAt(const glm::vec2& position, float& outIntensity) const
    {
        if (m_probes.empty())
        {
            outIntensity = 0.0f;
            return glm::vec3(0.0f);
        }

        // Find all probes that affect this position
        std::vector<uint32_t> affectingProbes = GetProbesAffecting(position);

        if (affectingProbes.empty())
        {
            outIntensity = 0.0f;
            return glm::vec3(0.0f);
        }

        // Distance-weighted interpolation
        glm::vec3 totalColor(0.0f);
        float totalIntensity = 0.0f;
        float totalWeight = 0.0f;

        for (uint32_t idx : affectingProbes)
        {
            const auto& probe = m_probes[idx];
            float distance = CalculateDistance(position, probe.position);
            float weight = CalculateDistanceWeight(distance, probe.influenceRadius);

            if (weight > 0.0f)
            {
                totalColor += probe.data.sampledColor * weight;
                totalIntensity += probe.data.sampledIntensity * weight;
                totalWeight += weight;
            }
        }

        if (totalWeight > 0.0f)
        {
            outIntensity = totalIntensity / totalWeight;
            return totalColor / totalWeight;
        }

        outIntensity = 0.0f;
        return glm::vec3(0.0f);
    }

    std::vector<uint32_t> LightProbeSystem::GetProbesAffecting(const glm::vec2& position) const
    {
        std::vector<uint32_t> result;
        result.reserve(16); // Pre-allocate for typical case

        for (uint32_t i = 0; i < m_probes.size(); ++i)
        {
            const auto& probe = m_probes[i];
            float distance = CalculateDistance(position, probe.position);

            if (distance < probe.influenceRadius)
            {
                result.push_back(i);
            }
        }

        return result;
    }

    void LightProbeSystem::MarkAllProbesDirty()
    {
        for (auto& probe : m_probes)
        {
            probe.needsUpdate = true;
        }
    }

    // ==================== Static Utility Functions ====================

    float LightProbeSystem::CalculateDistance(const glm::vec2& a, const glm::vec2& b)
    {
        return glm::length(b - a);
    }

    float LightProbeSystem::CalculateDistanceWeight(float distance, float influenceRadius)
    {
        if (distance >= influenceRadius || influenceRadius <= 0.0f)
        {
            return 0.0f;
        }

        // Inverse square falloff with smooth edge
        float normalizedDist = distance / influenceRadius;
        float falloff = 1.0f - normalizedDist;
        
        // Smoothstep for smoother transition
        falloff = falloff * falloff * (3.0f - 2.0f * falloff);
        
        return falloff;
    }

    glm::vec3 LightProbeSystem::BilinearInterpolate(
        const glm::vec3& topLeft,
        const glm::vec3& topRight,
        const glm::vec3& bottomLeft,
        const glm::vec3& bottomRight,
        float tx,
        float ty)
    {
        // Clamp interpolation factors
        tx = std::clamp(tx, 0.0f, 1.0f);
        ty = std::clamp(ty, 0.0f, 1.0f);

        // Interpolate along X axis
        glm::vec3 top = topLeft * (1.0f - tx) + topRight * tx;
        glm::vec3 bottom = bottomLeft * (1.0f - tx) + bottomRight * tx;

        // Interpolate along Y axis
        return top * (1.0f - ty) + bottom * ty;
    }

    glm::vec3 LightProbeSystem::BarycentricInterpolate(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        const glm::vec3& barycentricCoords)
    {
        // Ensure barycentric coordinates sum to 1
        float sum = barycentricCoords.x + barycentricCoords.y + barycentricCoords.z;
        glm::vec3 normalizedCoords = barycentricCoords;
        if (sum > 0.0f)
        {
            normalizedCoords /= sum;
        }

        return v0 * normalizedCoords.x + v1 * normalizedCoords.y + v2 * normalizedCoords.z;
    }

    // ==================== GPU Buffer Management ====================

    void LightProbeSystem::UpdateProbeBuffer()
    {
        if (!m_buffersCreated)
            return;

        if (m_probeBuffer && !m_probeData.empty())
        {
            size_t dataSize = m_probeData.size() * sizeof(ECS::LightProbeData);
            m_probeBuffer->WriteBuffer(m_probeData.data(), static_cast<uint32_t>(dataSize));
        }
    }

    void LightProbeSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create light probe buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create light probe buffers");
            return;
        }

        // Create probe data storage buffer
        Nut::BufferLayout probeLayout;
        probeLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        probeLayout.size = MAX_LIGHT_PROBES * sizeof(ECS::LightProbeData);
        probeLayout.mapped = false;

        m_probeBuffer = std::make_shared<Nut::Buffer>(probeLayout, nutContext);

        // Write empty probe data (at least one placeholder)
        ECS::LightProbeData emptyProbe{};
        m_probeBuffer->WriteBuffer(&emptyProbe, sizeof(ECS::LightProbeData));

        m_buffersCreated = true;
        LogInfo("Light probe buffers created successfully");
    }
}

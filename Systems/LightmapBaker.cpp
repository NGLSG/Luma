#include "LightmapBaker.h"
#include "LightingMath.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/PointLightComponent.h"
#include "../Components/SpotLightComponent.h"
#include "../Components/DirectionalLightComponent.h"
#include "../Components/LightingSettingsComponent.h"
#include "../Components/Transform.h"
#include "../Renderer/GraphicsBackend.h"
#include "Logger.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <Nut/TextureA.h>

namespace Systems
{
    LightmapBaker::LightmapBaker()
        : m_lightmap(nullptr)
          , m_currentConfig()
          , m_defaultConfig()
          , m_bakedLightCount(0)
          , m_isInitialized(false)
    {
    }

    LightmapBaker::~LightmapBaker()
    {
        ClearLightmap();
    }

    LightmapBakeResult LightmapBaker::BakeLightmap(
        RuntimeScene* scene,
        EngineContext& engineCtx,
        const LightmapBakeConfig& config,
        BakeProgressCallback progressCallback)
    {
        LightmapBakeResult result;
        auto startTime = std::chrono::high_resolution_clock::now();

        if (!scene)
        {
            result.success = false;
            result.errorMessage = "Scene is null";
            return result;
        }

        // 收集静态光源
        if (progressCallback)
        {
            progressCallback(0.0f, "Collecting static lights...");
        }

        std::vector<StaticLightInfo> staticLights = CollectStaticLights(scene);

        // 获取环境光设置
        ECS::Color ambientColor(0.1f, 0.1f, 0.15f, 1.0f);
        float ambientIntensity = 0.2f;

        auto& registry = scene->GetRegistry();
        auto settingsView = registry.view<ECS::LightingSettingsComponent>();
        for (auto entity : settingsView)
        {
            auto& settings = settingsView.get<ECS::LightingSettingsComponent>(entity);
            if (settings.Enable)
            {
                ambientColor = settings.ambientColor;
                ambientIntensity = settings.ambientIntensity;
                break;
            }
        }

        // 调用从光源列表烘焙的方法
        return BakeLightmapFromLights(
            staticLights,
            engineCtx,
            config,
            ambientColor,
            ambientIntensity,
            progressCallback);
    }


    LightmapBakeResult LightmapBaker::BakeLightmapFromLights(
        const std::vector<StaticLightInfo>& lights,
        EngineContext& engineCtx,
        const LightmapBakeConfig& config,
        const ECS::Color& ambientColor,
        float ambientIntensity,
        BakeProgressCallback progressCallback)
    {
        LightmapBakeResult result;
        auto startTime = std::chrono::high_resolution_clock::now();

        // 验证配置
        if (config.resolution < MIN_LIGHTMAP_RESOLUTION ||
            config.resolution > MAX_LIGHTMAP_RESOLUTION)
        {
            result.success = false;
            result.errorMessage = "Invalid lightmap resolution";
            return result;
        }

        if (config.worldWidth <= 0.0f || config.worldHeight <= 0.0f)
        {
            result.success = false;
            result.errorMessage = "Invalid world dimensions";
            return result;
        }

        // 过滤静态光源
        std::vector<StaticLightInfo> staticLights;
        for (const auto& light : lights)
        {
            if (light.isStatic)
            {
                staticLights.push_back(light);
            }
        }

        if (progressCallback)
        {
            progressCallback(0.1f, "Computing lighting...");
        }

        // 计算光照
        std::vector<uint8_t> pixelData = ComputeLightingCPU(
            staticLights,
            config,
            ambientColor,
            ambientIntensity,
            progressCallback);

        if (pixelData.empty())
        {
            result.success = false;
            result.errorMessage = "Failed to compute lighting";
            return result;
        }

        if (progressCallback)
        {
            progressCallback(0.9f, "Uploading to GPU...");
        }

        // 创建并上传纹理
        if (!CreateLightmapTexture(engineCtx, config.resolution))
        {
            result.success = false;
            result.errorMessage = "Failed to create lightmap texture";
            return result;
        }

        if (!UploadToGPU(engineCtx, pixelData, config.resolution, config.resolution))
        {
            result.success = false;
            result.errorMessage = "Failed to upload lightmap to GPU";
            return result;
        }

        // 更新状态
        m_currentConfig = config;
        m_bakedLightCount = static_cast<uint32_t>(staticLights.size());
        m_isInitialized = true;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (progressCallback)
        {
            progressCallback(1.0f, "Baking complete!");
        }

        result.success = true;
        result.lightmap = m_lightmap;
        result.bakedLightCount = m_bakedLightCount;
        result.bakeTimeMs = static_cast<float>(duration.count());

        LogInfo("Lightmap baked successfully: {} lights, {}x{} resolution, {:.2f}ms",
                m_bakedLightCount, config.resolution, config.resolution, result.bakeTimeMs);

        return result;
    }

    void LightmapBaker::ClearLightmap()
    {
        m_lightmap.reset();
        m_bakedLightCount = 0;
        m_isInitialized = false;
    }

    std::vector<StaticLightInfo> LightmapBaker::CollectStaticLights(RuntimeScene* scene)
    {
        std::vector<StaticLightInfo> staticLights;

        if (!scene)
            return staticLights;

        auto& registry = scene->GetRegistry();

        // 收集点光源（假设所有光源都是静态的，实际应用中可以添加 isStatic 标志）
        auto pointLightView = registry.view<ECS::PointLightComponent, ECS::TransformComponent>();
        for (auto entity : pointLightView)
        {
            auto& pointLight = pointLightView.get<ECS::PointLightComponent>(entity);
            auto& transform = pointLightView.get<ECS::TransformComponent>(entity);

            if (!pointLight.Enable)
                continue;

            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            StaticLightInfo info;
            info.lightData = pointLight.ToLightData(glm::vec2(transform.position.x, transform.position.y));
            info.isStatic = true; // 默认所有光源为静态
            info.entity = entity;

            staticLights.push_back(info);
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

            float angle = transform.rotation;
            glm::vec2 direction(std::sin(angle), -std::cos(angle));

            StaticLightInfo info;
            info.lightData = spotLight.ToLightData(
                glm::vec2(transform.position.x, transform.position.y),
                direction);
            info.isStatic = true;
            info.entity = entity;

            staticLights.push_back(info);
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

            StaticLightInfo info;
            info.lightData = dirLight.ToLightData();
            info.isStatic = true;
            info.entity = entity;

            staticLights.push_back(info);
        }

        return staticLights;
    }


    bool LightmapBaker::CreateLightmapTexture(EngineContext& engineCtx, uint32_t resolution)
    {
        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available for lightmap creation");
            return false;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available for lightmap creation");
            return false;
        }

        // 创建光照贴图纹理
        m_lightmap = Nut::TextureBuilder()
                     .SetSize(resolution, resolution)
                     .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
                     .SetUsage(Nut::TextureUsageFlags::GetCommonTextureUsage().GetUsage())
                     .Build(nutContext);

        if (!m_lightmap)
        {
            LogError("Failed to create lightmap texture");
            return false;
        }

        return true;
    }

    std::vector<uint8_t> LightmapBaker::ComputeLightingCPU(
        const std::vector<StaticLightInfo>& lights,
        const LightmapBakeConfig& config,
        const ECS::Color& ambientColor,
        float ambientIntensity,
        BakeProgressCallback progressCallback)
    {
        uint32_t resolution = config.resolution;
        std::vector<uint8_t> pixelData(resolution * resolution * 4);

        float progressStep = 0.8f / static_cast<float>(resolution);
        float currentProgress = 0.1f;

        for (uint32_t y = 0; y < resolution; ++y)
        {
            for (uint32_t x = 0; x < resolution; ++x)
            {
                // 计算纹理坐标
                glm::vec2 texCoord(
                    (static_cast<float>(x) + 0.5f) / static_cast<float>(resolution),
                    (static_cast<float>(y) + 0.5f) / static_cast<float>(resolution));

                // 转换为世界坐标
                glm::vec2 worldPos = TextureCoordToWorld(texCoord, config);

                // 计算光照
                glm::vec4 lighting = CalculatePixelLighting(
                    worldPos, lights, ambientColor, ambientIntensity);

                // 转换为 8 位颜色
                uint32_t pixelIndex = (y * resolution + x) * 4;
                pixelData[pixelIndex + 0] = static_cast<uint8_t>(std::clamp(lighting.r * 255.0f, 0.0f, 255.0f));
                pixelData[pixelIndex + 1] = static_cast<uint8_t>(std::clamp(lighting.g * 255.0f, 0.0f, 255.0f));
                pixelData[pixelIndex + 2] = static_cast<uint8_t>(std::clamp(lighting.b * 255.0f, 0.0f, 255.0f));
                pixelData[pixelIndex + 3] = static_cast<uint8_t>(std::clamp(lighting.a * 255.0f, 0.0f, 255.0f));
            }

            // 更新进度
            if (progressCallback && (y % 32 == 0))
            {
                currentProgress = 0.1f + progressStep * static_cast<float>(y);
                progressCallback(currentProgress, "Computing lighting...");
            }
        }

        return pixelData;
    }

    bool LightmapBaker::UploadToGPU(
        EngineContext& engineCtx,
        const std::vector<uint8_t>& pixelData,
        uint32_t width,
        uint32_t height)
    {
        if (!m_lightmap)
            return false;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
            return false;

        auto device = graphicsBackend->GetDevice();
        if (!device)
            return false;

        // 计算数据布局
        uint32_t bytesPerRow = width * 4;
        // WebGPU 要求 bytesPerRow 对齐到 256 字节
        uint32_t alignedBytesPerRow = (bytesPerRow + 255) & ~255;

        // 如果需要对齐，创建对齐的数据
        std::vector<uint8_t> alignedData;
        const uint8_t* uploadData = pixelData.data();

        if (alignedBytesPerRow != bytesPerRow)
        {
            alignedData.resize(alignedBytesPerRow * height);
            for (uint32_t y = 0; y < height; ++y)
            {
                std::memcpy(
                    alignedData.data() + y * alignedBytesPerRow,
                    pixelData.data() + y * bytesPerRow,
                    bytesPerRow);
            }
            uploadData = alignedData.data();
        }

        // 上传数据到纹理
        wgpu::TexelCopyTextureInfo  destination;
        destination.texture = m_lightmap->GetTexture();
        destination.mipLevel = 0;
        destination.origin = {0, 0, 0};
        destination.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferLayout dataLayout;
        dataLayout.offset = 0;
        dataLayout.bytesPerRow = alignedBytesPerRow;
        dataLayout.rowsPerImage = height;

        wgpu::Extent3D writeSize = {width, height, 1};

        device.GetQueue().WriteTexture(
            &destination,
            uploadData,
            alignedBytesPerRow * height,
            &dataLayout,
            &writeSize);

        return true;
    }


    // ==================== 静态工具函数实现 ====================

    glm::vec4 LightmapBaker::CalculatePixelLighting(
        const glm::vec2& worldPos,
        const std::vector<StaticLightInfo>& lights,
        const ECS::Color& ambientColor,
        float ambientIntensity)
    {
        // 从环境光开始
        glm::vec3 totalLight(
            ambientColor.r * ambientIntensity,
            ambientColor.g * ambientIntensity,
            ambientColor.b * ambientIntensity);

        // 累加所有光源贡献
        for (const auto& lightInfo : lights)
        {
            if (!lightInfo.isStatic)
                continue;

            const auto& light = lightInfo.lightData;
            glm::vec3 contribution(0.0f);

            switch (static_cast<ECS::LightType>(light.lightType))
            {
            case ECS::LightType::Point:
                contribution = CalculatePointLightContribution(worldPos, light);
                break;
            case ECS::LightType::Spot:
                contribution = CalculateSpotLightContribution(worldPos, light);
                break;
            case ECS::LightType::Directional:
                contribution = CalculateDirectionalLightContribution(light);
                break;
            }

            totalLight += contribution;
        }

        // 钳制到 [0, 1] 范围
        return glm::vec4(
            std::clamp(totalLight.r, 0.0f, 1.0f),
            std::clamp(totalLight.g, 0.0f, 1.0f),
            std::clamp(totalLight.b, 0.0f, 1.0f),
            1.0f);
    }

    glm::vec3 LightmapBaker::CalculatePointLightContribution(
        const glm::vec2& worldPos,
        const ECS::LightData& lightData)
    {
        glm::vec2 lightPos = lightData.position;
        float distance = glm::length(worldPos - lightPos);

        // 超出半径范围
        if (distance >= lightData.radius)
            return glm::vec3(0.0f);

        // 计算衰减
        float attenuation = Lighting::CalculateAttenuation(
            distance,
            lightData.radius,
            static_cast<ECS::AttenuationType>(static_cast<int>(lightData.attenuation)));

        // 计算光照贡献
        return glm::vec3(
            lightData.color.r * lightData.intensity * attenuation,
            lightData.color.g * lightData.intensity * attenuation,
            lightData.color.b * lightData.intensity * attenuation);
    }

    glm::vec3 LightmapBaker::CalculateSpotLightContribution(
        const glm::vec2& worldPos,
        const ECS::LightData& lightData)
    {
        glm::vec2 lightPos = lightData.position;
        glm::vec2 toPoint = worldPos - lightPos;
        float distance = glm::length(toPoint);

        // 超出半径范围
        if (distance >= lightData.radius)
            return glm::vec3(0.0f);

        // 计算距离衰减
        float distanceAttenuation = Lighting::CalculateAttenuation(
            distance,
            lightData.radius,
            static_cast<ECS::AttenuationType>(static_cast<int>(lightData.attenuation)));

        // 计算角度衰减
        glm::vec2 lightDir = glm::normalize(lightData.direction);
        glm::vec2 toPointDir = glm::normalize(toPoint);
        float cosAngle = glm::dot(lightDir, toPointDir);
        float angle = std::acos(std::clamp(cosAngle, -1.0f, 1.0f));

        float angleAttenuation = Lighting::CalculateSpotAngleAttenuationFromAngles(
            angle,
            lightData.innerAngle,
            lightData.outerAngle);

        float totalAttenuation = distanceAttenuation * angleAttenuation;

        return glm::vec3(
            lightData.color.r * lightData.intensity * totalAttenuation,
            lightData.color.g * lightData.intensity * totalAttenuation,
            lightData.color.b * lightData.intensity * totalAttenuation);
    }

    glm::vec3 LightmapBaker::CalculateDirectionalLightContribution(
        const ECS::LightData& lightData)
    {
        // 方向光对所有点的贡献相同
        return glm::vec3(
            lightData.color.r * lightData.intensity,
            lightData.color.g * lightData.intensity,
            lightData.color.b * lightData.intensity);
    }

    glm::vec2 LightmapBaker::WorldToTextureCoord(
        const glm::vec2& worldPos,
        const LightmapBakeConfig& config)
    {
        return glm::vec2(
            (worldPos.x - config.worldOrigin.x) / config.worldWidth,
            (worldPos.y - config.worldOrigin.y) / config.worldHeight);
    }

    glm::vec2 LightmapBaker::TextureCoordToWorld(
        const glm::vec2& texCoord,
        const LightmapBakeConfig& config)
    {
        return glm::vec2(
            config.worldOrigin.x + texCoord.x * config.worldWidth,
            config.worldOrigin.y + texCoord.y * config.worldHeight);
    }
}

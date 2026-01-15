#include "AmbientZoneSystem.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/Transform.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/LightingRenderer.h"
#include "../Renderer/Camera.h"
#include "LightingMath.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Systems
{
    AmbientZoneSystem::AmbientZoneSystem()
        : m_debugMode(false)
        , m_buffersCreated(false)
    {
    }

    void AmbientZoneSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        // 创建 GPU 缓冲区
        CreateBuffers(engineCtx);

        // 设置 LightingRenderer 的引用
        auto& lightingRenderer = LightingRenderer::GetInstance();
        if (lightingRenderer.IsInitialized())
        {
            lightingRenderer.SetAmbientZoneSystem(this);
        }

        LogInfo("AmbientZoneSystem initialized");
    }

    void AmbientZoneSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        // 1. 收集所有环境光区域
        CollectAmbientZones(scene);

        // 2. 更新 GPU 缓冲区
        UpdateAmbientZoneBuffer();
    }

    void AmbientZoneSystem::OnDestroy(RuntimeScene* scene)
    {
        // 清除 LightingRenderer 的引用
        auto& lightingRenderer = LightingRenderer::GetInstance();
        lightingRenderer.SetAmbientZoneSystem(nullptr);

        m_allZones.clear();
        m_allZoneData.clear();
        m_ambientZoneBuffer.reset();
        m_ambientZoneGlobalBuffer.reset();
        m_buffersCreated = false;

        LogInfo("AmbientZoneSystem destroyed");
    }

    void AmbientZoneSystem::CollectAmbientZones(RuntimeScene* scene)
    {
        m_allZones.clear();
        m_allZoneData.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();
        const Camera& camera = GetActiveCamera();
        CameraProperties props = camera.GetProperties();
        glm::vec2 cameraPos(props.position.x(), props.position.y());

        // 收集环境光区域
        auto zoneView = registry.view<ECS::AmbientZoneComponent, ECS::TransformComponent>();
        for (auto entity : zoneView)
        {
            auto& zone = zoneView.get<ECS::AmbientZoneComponent>(entity);
            auto& transform = zoneView.get<ECS::TransformComponent>(entity);

            // 检查组件是否启用
            if (!zone.Enable)
                continue;

            // 检查游戏对象是否激活
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            AmbientZoneInfo info;
            info.data = zone.ToAmbientZoneData(glm::vec2(transform.position.x, transform.position.y));
            info.priority = zone.priority;
            
            // 计算到相机的距离
            glm::vec2 zonePos(transform.position.x, transform.position.y);
            info.distanceToCamera = glm::length(zonePos - cameraPos);

            m_allZones.push_back(info);
        }

        // 按优先级排序
        SortZonesByPriority(m_allZones);

        // 限制区域数量
        if (m_allZones.size() > MAX_AMBIENT_ZONES_PER_FRAME)
        {
            if (m_debugMode)
            {
                LogWarn("Ambient zone count {} exceeds limit {}, truncating",
                        m_allZones.size(), MAX_AMBIENT_ZONES_PER_FRAME);
            }
            m_allZones.resize(MAX_AMBIENT_ZONES_PER_FRAME);
        }

        // 提取区域数据
        m_allZoneData.reserve(m_allZones.size());
        for (const auto& zone : m_allZones)
        {
            m_allZoneData.push_back(zone.data);
        }
    }


    void AmbientZoneSystem::UpdateAmbientZoneBuffer()
    {
        if (!m_buffersCreated)
            return;

        // 更新环境光区域全局数据
        if (m_ambientZoneGlobalBuffer)
        {
            uint32_t globalData[4] = {static_cast<uint32_t>(m_allZoneData.size()), 0, 0, 0};
            m_ambientZoneGlobalBuffer->WriteBuffer(globalData, sizeof(globalData));
        }

        // 更新环境光区域数据
        if (m_ambientZoneBuffer && !m_allZoneData.empty())
        {
            size_t dataSize = m_allZoneData.size() * sizeof(ECS::AmbientZoneData);
            m_ambientZoneBuffer->WriteBuffer(m_allZoneData.data(), static_cast<uint32_t>(dataSize));
        }
    }

    void AmbientZoneSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create ambient zone buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create ambient zone buffers");
            return;
        }

        // 创建环境光区域全局数据缓冲区
        Nut::BufferLayout globalLayout;
        globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        globalLayout.size = 16; // ambientZoneCount + 3 padding
        globalLayout.mapped = false;

        m_ambientZoneGlobalBuffer = std::make_shared<Nut::Buffer>(globalLayout, nutContext);

        // 写入初始全局数据
        uint32_t initialGlobalData[4] = {0, 0, 0, 0};
        m_ambientZoneGlobalBuffer->WriteBuffer(initialGlobalData, sizeof(initialGlobalData));

        // 创建环境光区域数据存储缓冲区
        Nut::BufferLayout zoneLayout;
        zoneLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        zoneLayout.size = MAX_AMBIENT_ZONES_PER_FRAME * sizeof(ECS::AmbientZoneData);
        zoneLayout.mapped = false;

        m_ambientZoneBuffer = std::make_shared<Nut::Buffer>(zoneLayout, nutContext);

        // 写入空的区域数据（至少一个占位）
        ECS::AmbientZoneData emptyZone{};
        m_ambientZoneBuffer->WriteBuffer(&emptyZone, sizeof(ECS::AmbientZoneData));

        m_buffersCreated = true;
        LogInfo("Ambient zone buffers created successfully");
    }

    // ==================== 环境光计算 ====================

    ECS::Color AmbientZoneSystem::CalculateAmbientAt(const glm::vec2& position) const
    {
        // 获取影响该位置的所有区域
        std::vector<const ECS::AmbientZoneData*> affectingZones = GetZonesAt(position);

        if (affectingZones.empty())
        {
            // 没有区域影响，返回默认环境光（黑色/透明）
            return ECS::Color(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // 混合所有区域的颜色
        return BlendZoneColors(affectingZones, position);
    }

    std::vector<const ECS::AmbientZoneData*> AmbientZoneSystem::GetZonesAt(const glm::vec2& position) const
    {
        std::vector<const ECS::AmbientZoneData*> result;

        for (const auto& zone : m_allZoneData)
        {
            if (IsPointInZone(position, zone))
            {
                result.push_back(&zone);
            }
        }

        return result;
    }

    // ==================== 静态工具函数 ====================

    bool AmbientZoneSystem::IsPointInRectangle(
        const glm::vec2& position,
        const glm::vec2& zonePosition,
        float width,
        float height)
    {
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        return position.x >= zonePosition.x - halfWidth &&
               position.x <= zonePosition.x + halfWidth &&
               position.y >= zonePosition.y - halfHeight &&
               position.y <= zonePosition.y + halfHeight;
    }

    bool AmbientZoneSystem::IsPointInCircle(
        const glm::vec2& position,
        const glm::vec2& zonePosition,
        float radius)
    {
        float distSq = glm::dot(position - zonePosition, position - zonePosition);
        return distSq <= radius * radius;
    }

    bool AmbientZoneSystem::IsPointInZone(
        const glm::vec2& position,
        const ECS::AmbientZoneData& zone)
    {
        if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
        {
            // 圆形区域使用 width/2 作为半径
            return IsPointInCircle(position, zone.position, zone.size.x / 2.0f);
        }
        else
        {
            // 矩形区域
            return IsPointInRectangle(position, zone.position, zone.size.x, zone.size.y);
        }
    }

    float AmbientZoneSystem::CalculateRectangleEdgeFactor(
        const glm::vec2& position,
        const glm::vec2& zonePosition,
        float width,
        float height,
        float edgeSoftness)
    {
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        // 计算相对位置
        glm::vec2 localPos = position - zonePosition;

        // 计算到各边的距离（归一化）
        float distToEdgeX = halfWidth - std::abs(localPos.x);
        float distToEdgeY = halfHeight - std::abs(localPos.y);

        // 如果在区域外，返回 0
        if (distToEdgeX < 0.0f || distToEdgeY < 0.0f)
        {
            return 0.0f;
        }

        // 计算边缘柔和度区域
        float softWidth = halfWidth * edgeSoftness;
        float softHeight = halfHeight * edgeSoftness;

        // 确保柔和度区域不超过区域的一半
        softWidth = std::min(softWidth, halfWidth);
        softHeight = std::min(softHeight, halfHeight);

        // 计算边缘因子
        float factorX = 1.0f;
        float factorY = 1.0f;

        if (softWidth > 0.0f && distToEdgeX < softWidth)
        {
            factorX = distToEdgeX / softWidth;
        }

        if (softHeight > 0.0f && distToEdgeY < softHeight)
        {
            factorY = distToEdgeY / softHeight;
        }

        // 使用最小值作为最终因子
        float factor = std::min(factorX, factorY);

        // 应用平滑插值 (smoothstep)
        return factor * factor * (3.0f - 2.0f * factor);
    }

    float AmbientZoneSystem::CalculateCircleEdgeFactor(
        const glm::vec2& position,
        const glm::vec2& zonePosition,
        float radius,
        float edgeSoftness)
    {
        float distance = glm::length(position - zonePosition);

        // 如果在区域外，返回 0
        if (distance >= radius)
        {
            return 0.0f;
        }

        // 计算边缘柔和度区域
        float softRadius = radius * edgeSoftness;
        softRadius = std::min(softRadius, radius);

        // 计算到边缘的距离
        float distToEdge = radius - distance;

        // 计算边缘因子
        float factor = 1.0f;
        if (softRadius > 0.0f && distToEdge < softRadius)
        {
            factor = distToEdge / softRadius;
        }

        // 应用平滑插值 (smoothstep)
        return factor * factor * (3.0f - 2.0f * factor);
    }

    float AmbientZoneSystem::CalculateEdgeFactor(
        const glm::vec2& position,
        const ECS::AmbientZoneData& zone)
    {
        if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
        {
            return CalculateCircleEdgeFactor(
                position, zone.position, zone.size.x / 2.0f, zone.edgeSoftness);
        }
        else
        {
            return CalculateRectangleEdgeFactor(
                position, zone.position, zone.size.x, zone.size.y, zone.edgeSoftness);
        }
    }


    ECS::Color AmbientZoneSystem::CalculateGradientColor(
        const ECS::AmbientZoneData& zone,
        const glm::vec2& localPosition)
    {
        ECS::AmbientGradientMode gradientMode = 
            static_cast<ECS::AmbientGradientMode>(zone.gradientMode);

        // 如果没有渐变，直接返回主颜色
        if (gradientMode == ECS::AmbientGradientMode::None)
        {
            return ECS::Color(
                zone.primaryColor.r,
                zone.primaryColor.g,
                zone.primaryColor.b,
                zone.primaryColor.a);
        }

        // 计算插值因子
        float t = 0.0f;

        if (gradientMode == ECS::AmbientGradientMode::Vertical)
        {
            // 垂直渐变：从顶部（primaryColor）到底部（secondaryColor）
            float halfHeight = zone.size.y / 2.0f;
            if (halfHeight > 0.0f)
            {
                // 将 localPosition.y 从 [-halfHeight, halfHeight] 映射到 [0, 1]
                t = (localPosition.y + halfHeight) / zone.size.y;
                t = std::clamp(t, 0.0f, 1.0f);
            }
        }
        else if (gradientMode == ECS::AmbientGradientMode::Horizontal)
        {
            // 水平渐变：从左侧（primaryColor）到右侧（secondaryColor）
            float halfWidth = zone.size.x / 2.0f;
            if (halfWidth > 0.0f)
            {
                // 将 localPosition.x 从 [-halfWidth, halfWidth] 映射到 [0, 1]
                t = (localPosition.x + halfWidth) / zone.size.x;
                t = std::clamp(t, 0.0f, 1.0f);
            }
        }

        // 线性插值颜色
        ECS::Color result;
        result.r = zone.primaryColor.r + t * (zone.secondaryColor.r - zone.primaryColor.r);
        result.g = zone.primaryColor.g + t * (zone.secondaryColor.g - zone.primaryColor.g);
        result.b = zone.primaryColor.b + t * (zone.secondaryColor.b - zone.primaryColor.b);
        result.a = zone.primaryColor.a + t * (zone.secondaryColor.a - zone.primaryColor.a);

        return result;
    }

    AmbientZoneBounds AmbientZoneSystem::CalculateZoneBounds(const ECS::AmbientZoneData& zone)
    {
        AmbientZoneBounds bounds;

        if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
        {
            // 圆形区域使用 width/2 作为半径
            float radius = zone.size.x / 2.0f;
            bounds.minX = zone.position.x - radius;
            bounds.maxX = zone.position.x + radius;
            bounds.minY = zone.position.y - radius;
            bounds.maxY = zone.position.y + radius;
        }
        else
        {
            // 矩形区域
            float halfWidth = zone.size.x / 2.0f;
            float halfHeight = zone.size.y / 2.0f;
            bounds.minX = zone.position.x - halfWidth;
            bounds.maxX = zone.position.x + halfWidth;
            bounds.minY = zone.position.y - halfHeight;
            bounds.maxY = zone.position.y + halfHeight;
        }

        return bounds;
    }

    void AmbientZoneSystem::SortZonesByPriority(std::vector<AmbientZoneInfo>& zones)
    {
        std::stable_sort(zones.begin(), zones.end(),
            [](const AmbientZoneInfo& a, const AmbientZoneInfo& b)
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

    ECS::Color AmbientZoneSystem::BlendZoneColors(
        const std::vector<const ECS::AmbientZoneData*>& zones,
        const glm::vec2& position)
    {
        if (zones.empty())
        {
            return ECS::Color(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // 按优先级排序（创建临时副本）
        std::vector<const ECS::AmbientZoneData*> sortedZones = zones;
        std::sort(sortedZones.begin(), sortedZones.end(),
            [](const ECS::AmbientZoneData* a, const ECS::AmbientZoneData* b)
            {
                return a->priority > b->priority;
            });

        // 计算加权混合
        float totalWeight = 0.0f;
        glm::vec4 blendedColor(0.0f);

        for (const auto* zone : sortedZones)
        {
            // 计算边缘因子
            float edgeFactor = CalculateEdgeFactor(position, *zone);

            // 计算局部坐标
            glm::vec2 localPos = position - zone->position;

            // 计算渐变颜色
            ECS::Color gradientColor = CalculateGradientColor(*zone, localPos);

            // 计算最终权重 = 边缘因子 * 混合权重 * 强度
            float weight = edgeFactor * zone->blendWeight * zone->intensity;

            if (weight > 0.0f)
            {
                blendedColor.r += gradientColor.r * weight;
                blendedColor.g += gradientColor.g * weight;
                blendedColor.b += gradientColor.b * weight;
                blendedColor.a += gradientColor.a * weight;
                totalWeight += weight;
            }
        }

        // 归一化
        if (totalWeight > 0.0f)
        {
            blendedColor /= totalWeight;
        }

        return ECS::Color(blendedColor.r, blendedColor.g, blendedColor.b, blendedColor.a);
    }
}

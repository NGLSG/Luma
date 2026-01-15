#include "ShadowRenderer.h"
#include "QualityManager.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Components/Transform.h"
#include "../Components/Sprite.h"
#include "../Renderer/GraphicsBackend.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Systems
{
    ShadowRenderer* ShadowRenderer::s_instance = nullptr;

    ShadowRenderer::ShadowRenderer()
        : m_shadowMapResolution(DEFAULT_SHADOW_MAP_RESOLUTION)
        , m_shadowSoftness(1.0f)
        , m_enabled(true)
        , m_texturesCreated(false)
        , m_buffersCreated(false)
        , m_shadowMethod(ECS::ShadowMethod::Basic)
        , m_previousShadowMethod(ECS::ShadowMethod::Basic)
        , m_shadowCacheEnabled(true)
        , m_currentFrame(0)
        , m_cacheHits(0)
        , m_cacheMisses(0)
        , m_engineCtx(nullptr)
    {
    }

    void ShadowRenderer::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        m_engineCtx = &engineCtx;
        s_instance = this;
        
        // 创建阴影贴图纹理
        CreateShadowMapTextures(engineCtx);
        
        // 创建 GPU 缓冲区
        CreateGPUBuffers(engineCtx);

        // 创建 SDF 缓冲区
        CreateSDFBuffer(engineCtx);

        // 注册到 QualityManager
        QualityManager::GetInstance().SetShadowRenderer(this);

        LogInfo("ShadowRenderer initialized with method: {}", 
                static_cast<int>(m_shadowMethod));
    }

    void ShadowRenderer::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!m_enabled)
            return;

        // 增加帧计数
        m_currentFrame++;

        // 0. 处理待失效的缓存
        ProcessCacheInvalidations(scene);

        // 1. 收集阴影投射器
        CollectShadowCasters(scene);

        // 2. 更新阴影缓存
        if (m_shadowCacheEnabled)
        {
            UpdateShadowCaches(scene);
        }

        // 3. 根据阴影方法更新数据
        switch (m_shadowMethod)
        {
            case ECS::ShadowMethod::SDF:
                UpdateSDFData(scene);
                break;
            case ECS::ShadowMethod::ScreenSpace:
                // 屏幕空间阴影在着色器中计算
                break;
            case ECS::ShadowMethod::Basic:
            default:
                // 提取所有边缘
                m_allEdges.clear();
                for (const auto& caster : m_shadowCasters)
                {
                    auto edges = ExtractEdges(caster.vertices);
                    m_allEdges.insert(m_allEdges.end(), edges.begin(), edges.end());
                }
                break;
        }

        // 4. 更新 GPU 缓冲区
        UpdateGPUBuffers();

        // 5. 更新合并阴影贴图（实际渲染在着色器中完成）
        UpdateCombinedShadowMap();

        // 记录阴影方法切换
        if (m_shadowMethod != m_previousShadowMethod)
        {
            LogInfo("Shadow method changed from {} to {}", 
                    static_cast<int>(m_previousShadowMethod),
                    static_cast<int>(m_shadowMethod));
            m_previousShadowMethod = m_shadowMethod;
        }
    }

    void ShadowRenderer::OnDestroy(RuntimeScene* scene)
    {
        // 从 QualityManager 注销
        QualityManager::GetInstance().SetShadowRenderer(nullptr);

        m_shadowCasters.clear();
        m_allEdges.clear();
        m_lightShadowData.clear();
        m_combinedShadowMap.reset();
        m_shadowVertexBuffer.reset();
        m_edgeBuffer.reset();
        m_paramsBuffer.reset();
        m_texturesCreated = false;
        m_buffersCreated = false;
        m_engineCtx = nullptr;
        
        if (s_instance == this)
        {
            s_instance = nullptr;
        }

        LogInfo("ShadowRenderer destroyed");
    }

    void ShadowRenderer::CollectShadowCasters(RuntimeScene* scene)
    {
        m_shadowCasters.clear();

        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // 收集所有阴影投射器
        auto view = registry.view<ECS::ShadowCasterComponent, ECS::TransformComponent>();
        for (auto entity : view)
        {
            auto& caster = view.get<ECS::ShadowCasterComponent>(entity);
            auto& transform = view.get<ECS::TransformComponent>(entity);

            // 检查组件是否启用
            if (!caster.Enable)
                continue;

            // 检查游戏对象是否激活
            RuntimeGameObject go = scene->FindGameObjectByEntity(entity);
            if (!go.IsActive())
                continue;

            ShadowCasterInfo info;
            info.position = glm::vec2(transform.position.x, transform.position.y);
            info.opacity = caster.opacity;
            info.selfShadow = caster.selfShadow;
            info.entity = entity;

            // 生成顶点
            glm::vec2 scale(transform.scale.x, transform.scale.y);
            info.vertices = GenerateVertices(caster, info.position, scale, transform.rotation);

            // 限制阴影投射器数量
            if (m_shadowCasters.size() < MAX_SHADOW_CASTERS)
            {
                m_shadowCasters.push_back(info);
            }
        }
    }

    std::vector<glm::vec2> ShadowRenderer::GenerateVertices(
        const ECS::ShadowCasterComponent& caster,
        const glm::vec2& position,
        const glm::vec2& scale,
        float rotation)
    {
        std::vector<glm::vec2> localVertices;

        switch (caster.shape)
        {
            case ECS::ShadowShape::Rectangle:
                localVertices = GenerateRectangleVertices(
                    glm::vec2(caster.rectangleSize.x, caster.rectangleSize.y));
                break;

            case ECS::ShadowShape::Circle:
                localVertices = GenerateCircleVertices(caster.circleRadius);
                break;

            case ECS::ShadowShape::Polygon:
                // 使用自定义顶点
                for (const auto& v : caster.vertices)
                {
                    localVertices.push_back(glm::vec2(v.x, v.y));
                }
                break;

            case ECS::ShadowShape::Auto:
            default:
                // 默认使用矩形
                localVertices = GenerateRectangleVertices(
                    glm::vec2(caster.rectangleSize.x, caster.rectangleSize.y));
                break;
        }

        // 应用偏移
        glm::vec2 offset(caster.offset.x, caster.offset.y);
        for (auto& v : localVertices)
        {
            v += offset;
        }

        // 变换到世界坐标
        return TransformVertices(localVertices, position, scale, rotation);
    }

    std::vector<glm::vec2> ShadowRenderer::GenerateRectangleVertices(const glm::vec2& size)
    {
        float halfW = size.x * 0.5f;
        float halfH = size.y * 0.5f;

        return {
            glm::vec2(-halfW, -halfH),
            glm::vec2( halfW, -halfH),
            glm::vec2( halfW,  halfH),
            glm::vec2(-halfW,  halfH)
        };
    }

    std::vector<glm::vec2> ShadowRenderer::GenerateCircleVertices(float radius, uint32_t segments)
    {
        std::vector<glm::vec2> vertices;
        vertices.reserve(segments);

        float angleStep = 2.0f * 3.14159265359f / static_cast<float>(segments);
        for (uint32_t i = 0; i < segments; ++i)
        {
            float angle = static_cast<float>(i) * angleStep;
            vertices.push_back(glm::vec2(
                radius * std::cos(angle),
                radius * std::sin(angle)
            ));
        }

        return vertices;
    }

    std::vector<glm::vec2> ShadowRenderer::TransformVertices(
        const std::vector<glm::vec2>& localVertices,
        const glm::vec2& position,
        const glm::vec2& scale,
        float rotation)
    {
        std::vector<glm::vec2> worldVertices;
        worldVertices.reserve(localVertices.size());

        float cosR = std::cos(rotation);
        float sinR = std::sin(rotation);

        for (const auto& v : localVertices)
        {
            // 缩放
            glm::vec2 scaled = v * scale;

            // 旋转
            glm::vec2 rotated(
                scaled.x * cosR - scaled.y * sinR,
                scaled.x * sinR + scaled.y * cosR
            );

            // 平移
            worldVertices.push_back(rotated + position);
        }

        return worldVertices;
    }

    std::vector<ShadowEdge> ShadowRenderer::ExtractEdges(const std::vector<glm::vec2>& vertices)
    {
        std::vector<ShadowEdge> edges;
        
        if (vertices.size() < 2)
            return edges;

        edges.reserve(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            size_t nextIdx = (i + 1) % vertices.size();
            ShadowEdge edge;
            edge.start = vertices[i];
            edge.end = vertices[nextIdx];
            edges.push_back(edge);
        }

        return edges;
    }

    bool ShadowRenderer::RayEdgeIntersection(
        const glm::vec2& rayOrigin,
        const glm::vec2& rayDir,
        const ShadowEdge& edge,
        float& outT)
    {
        glm::vec2 edgeDir = edge.end - edge.start;
        glm::vec2 originToStart = edge.start - rayOrigin;

        // 计算行列式
        float det = rayDir.x * edgeDir.y - rayDir.y * edgeDir.x;

        // 平行检测
        if (std::abs(det) < 1e-6f)
            return false;

        float invDet = 1.0f / det;

        // 计算射线参数 t
        float t = (originToStart.x * edgeDir.y - originToStart.y * edgeDir.x) * invDet;

        // 计算边缘参数 u
        float u = (originToStart.x * rayDir.y - originToStart.y * rayDir.x) * invDet;

        // 检查交点是否在射线正方向且在边缘上
        if (t > 0.0f && u >= 0.0f && u <= 1.0f)
        {
            outT = t;
            return true;
        }

        return false;
    }

    bool ShadowRenderer::IsPointInShadow(
        const glm::vec2& point,
        const glm::vec2& lightPos,
        const std::vector<ShadowEdge>& edges)
    {
        // 从光源到点的方向
        glm::vec2 toPoint = point - lightPos;
        float distToPoint = glm::length(toPoint);

        if (distToPoint < 1e-6f)
            return false;

        glm::vec2 rayDir = toPoint / distToPoint;

        // 检查是否有边缘阻挡
        for (const auto& edge : edges)
        {
            float t;
            if (RayEdgeIntersection(lightPos, rayDir, edge, t))
            {
                // 如果交点在光源和目标点之间，则点在阴影中
                if (t < distToPoint - 1e-4f)
                {
                    return true;
                }
            }
        }

        return false;
    }

    Nut::TextureAPtr ShadowRenderer::GetShadowMap(uint32_t lightIndex) const
    {
        auto it = m_lightShadowData.find(lightIndex);
        if (it != m_lightShadowData.end())
        {
            return it->second.shadowMap;
        }
        return nullptr;
    }

    void ShadowRenderer::SetShadowMapResolution(uint32_t resolution)
    {
        if (m_shadowMapResolution != resolution)
        {
            m_shadowMapResolution = resolution;
            // 需要重新创建纹理
            m_texturesCreated = false;
            if (m_engineCtx)
            {
                CreateShadowMapTextures(*m_engineCtx);
            }
        }
    }

    void ShadowRenderer::SetShadowSoftness(float softness)
    {
        m_shadowSoftness = std::clamp(softness, 0.0f, 1.0f);
    }

    void ShadowRenderer::RenderShadowMapForLight(
        const glm::vec2& lightPos,
        float lightRadius,
        uint32_t lightIndex)
    {
        // 阴影贴图渲染将在 GPU 着色器中完成
        // 这里只更新光源数据
        auto& shadowData = m_lightShadowData[lightIndex];
        shadowData.lightPosition = lightPos;
        shadowData.lightRadius = lightRadius;
        shadowData.isDirty = true;
    }

    void ShadowRenderer::UpdateCombinedShadowMap()
    {
        // 合并阴影贴图的更新将在渲染管线中完成
        // 这里可以标记需要更新
    }

    void ShadowRenderer::CreateShadowMapTextures(EngineContext& engineCtx)
    {
        if (m_texturesCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create shadow map textures");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create shadow map textures");
            return;
        }

        // 创建合并阴影贴图
        Nut::TextureDescriptor shadowMapDesc;
        shadowMapDesc.SetSize(m_shadowMapResolution, m_shadowMapResolution)
                     .SetFormat(wgpu::TextureFormat::R32Float)
                     .SetUsage(Nut::TextureUsageFlags::GetRenderTargetUsage());

        Nut::TextureBuilder builder;
        m_combinedShadowMap = builder
            .SetDescriptor(shadowMapDesc)
            .Build(nutContext);

        if (m_combinedShadowMap)
        {
            m_texturesCreated = true;
            LogInfo("Shadow map textures created (resolution: {})", m_shadowMapResolution);
        }
        else
        {
            LogError("Failed to create shadow map textures");
        }
    }

    void ShadowRenderer::CreateGPUBuffers(EngineContext& engineCtx)
    {
        if (m_buffersCreated)
            return;

        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("GraphicsBackend not available, cannot create shadow buffers");
            return;
        }

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
        {
            LogError("NutContext not available, cannot create shadow buffers");
            return;
        }

        // 创建阴影参数缓冲区
        Nut::BufferLayout paramsLayout;
        paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        paramsLayout.size = sizeof(ShadowParams);
        paramsLayout.mapped = false;

        m_paramsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, nutContext);

        // 创建阴影边缘缓冲区（预分配最大容量）
        Nut::BufferLayout edgeLayout;
        edgeLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        edgeLayout.size = MAX_SHADOW_CASTERS * 8 * sizeof(GPUShadowEdge); // 每个投射器最多8条边
        edgeLayout.mapped = false;

        m_edgeBuffer = std::make_shared<Nut::Buffer>(edgeLayout, nutContext);

        // 写入初始数据
        ShadowParams initialParams{};
        initialParams.edgeCount = 0;
        initialParams.shadowSoftness = m_shadowSoftness;
        initialParams.shadowBias = 0.005f;
        m_paramsBuffer->WriteBuffer(&initialParams, sizeof(ShadowParams));

        // 写入空的边缘数据（占位）
        GPUShadowEdge emptyEdge{};
        m_edgeBuffer->WriteBuffer(&emptyEdge, sizeof(GPUShadowEdge));

        m_buffersCreated = true;
        LogInfo("Shadow GPU buffers created");
    }

    void ShadowRenderer::UpdateGPUBuffers()
    {
        if (!m_buffersCreated)
            return;

        // 更新阴影参数
        ShadowParams params{};
        params.edgeCount = static_cast<uint32_t>(m_allEdges.size());
        params.shadowSoftness = m_shadowSoftness;
        params.shadowBias = 0.005f;

        if (m_paramsBuffer)
        {
            m_paramsBuffer->WriteBuffer(&params, sizeof(ShadowParams));
        }

        // 更新阴影边缘数据
        if (m_edgeBuffer && !m_allEdges.empty())
        {
            // 转换为 GPU 格式，包含投射器包围盒和自阴影信息
            std::vector<GPUShadowEdge> gpuEdges;
            gpuEdges.reserve(m_allEdges.size());
            
            // 为每个投射器的边缘计算包围盒
            size_t edgeIndex = 0;
            for (const auto& caster : m_shadowCasters)
            {
                // 计算投射器的包围盒
                glm::vec2 boundsMin(std::numeric_limits<float>::max());
                glm::vec2 boundsMax(std::numeric_limits<float>::lowest());
                
                for (const auto& vertex : caster.vertices)
                {
                    boundsMin.x = std::min(boundsMin.x, vertex.x);
                    boundsMin.y = std::min(boundsMin.y, vertex.y);
                    boundsMax.x = std::max(boundsMax.x, vertex.x);
                    boundsMax.y = std::max(boundsMax.y, vertex.y);
                }
                
                // 为该投射器的每条边缘添加包围盒信息
                auto edges = ExtractEdges(caster.vertices);
                for (const auto& edge : edges)
                {
                    GPUShadowEdge gpuEdge;
                    gpuEdge.start = edge.start;
                    gpuEdge.end = edge.end;
                    gpuEdge.boundsMin = boundsMin;
                    gpuEdge.boundsMax = boundsMax;
                    gpuEdge.selfShadow = caster.selfShadow ? 1u : 0u;
                    gpuEdge.opacity = caster.opacity;
                    gpuEdge.padding[0] = 0.0f;
                    gpuEdge.padding[1] = 0.0f;
                    gpuEdges.push_back(gpuEdge);
                }
            }

            size_t dataSize = gpuEdges.size() * sizeof(GPUShadowEdge);
            m_edgeBuffer->WriteBuffer(gpuEdges.data(), static_cast<uint32_t>(dataSize));
        }
    }

    // ==================== 阴影方法切换 ====================

    void ShadowRenderer::SetShadowMethod(ECS::ShadowMethod method)
    {
        if (m_shadowMethod != method)
        {
            m_previousShadowMethod = m_shadowMethod;
            m_shadowMethod = method;
            
            // 使所有缓存失效，因为阴影方法改变了
            InvalidateAllShadowCaches();
            
            LogInfo("Shadow method set to: {}", static_cast<int>(method));
        }
    }

    bool ShadowRenderer::IsShadowMethodSupported(ECS::ShadowMethod method) const
    {
        // 所有方法都支持
        return true;
    }

    // ==================== SDF 阴影生成 ====================

    ECS::SDFData ShadowRenderer::GenerateSDF(
        const ECS::ShadowCasterComponent& caster,
        const std::vector<glm::vec2>& vertices)
    {
        ECS::SDFData sdfData;

        if (vertices.empty())
            return sdfData;

        // 计算包围盒
        glm::vec2 boundsMin(std::numeric_limits<float>::max());
        glm::vec2 boundsMax(std::numeric_limits<float>::lowest());

        for (const auto& v : vertices)
        {
            boundsMin.x = std::min(boundsMin.x, v.x);
            boundsMin.y = std::min(boundsMin.y, v.y);
            boundsMax.x = std::max(boundsMax.x, v.x);
            boundsMax.y = std::max(boundsMax.y, v.y);
        }

        // 添加填充
        boundsMin -= glm::vec2(caster.sdfPadding);
        boundsMax += glm::vec2(caster.sdfPadding);

        // 计算单元格大小
        glm::vec2 size = boundsMax - boundsMin;
        float cellSize = std::max(size.x, size.y) / static_cast<float>(caster.sdfResolution);

        // 初始化 SDF 数据
        int width = static_cast<int>(std::ceil(size.x / cellSize));
        int height = static_cast<int>(std::ceil(size.y / cellSize));

        // 限制最大分辨率
        width = std::min(width, 256);
        height = std::min(height, 256);

        sdfData.Initialize(width, height, cellSize, 
                           ECS::Vector2f(boundsMin.x, boundsMin.y));

        // 为每个网格点计算有符号距离
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                // 计算世界坐标
                glm::vec2 worldPos(
                    boundsMin.x + (x + 0.5f) * cellSize,
                    boundsMin.y + (y + 0.5f) * cellSize
                );

                // 计算有符号距离
                float distance = CalculateSignedDistance(worldPos, vertices);
                sdfData.SetDistance(x, y, distance);
            }
        }

        return sdfData;
    }

    float ShadowRenderer::CalculateSignedDistance(
        const glm::vec2& point,
        const std::vector<glm::vec2>& vertices)
    {
        if (vertices.size() < 3)
            return 1e10f;

        // 计算到多边形边缘的最小距离
        float minDistance = std::numeric_limits<float>::max();

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            size_t j = (i + 1) % vertices.size();
            float dist = PointToSegmentDistance(point, vertices[i], vertices[j]);
            minDistance = std::min(minDistance, dist);
        }

        // 确定点是否在多边形内部（使用射线投射法）
        bool inside = false;
        size_t n = vertices.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
                (point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) / 
                           (vertices[j].y - vertices[i].y) + vertices[i].x))
            {
                inside = !inside;
            }
        }

        // 内部为负，外部为正
        return inside ? -minDistance : minDistance;
    }

    float ShadowRenderer::PointToSegmentDistance(
        const glm::vec2& point,
        const glm::vec2& lineStart,
        const glm::vec2& lineEnd)
    {
        glm::vec2 line = lineEnd - lineStart;
        float lineLength = glm::length(line);

        if (lineLength < 1e-6f)
            return glm::length(point - lineStart);

        // 计算投影参数
        float t = glm::dot(point - lineStart, line) / (lineLength * lineLength);
        t = std::clamp(t, 0.0f, 1.0f);

        // 计算最近点
        glm::vec2 closest = lineStart + t * line;

        return glm::length(point - closest);
    }

    float ShadowRenderer::CalculateSDFShadow(
        const glm::vec2& point,
        const glm::vec2& lightPos,
        const ECS::SDFData& sdfData,
        float softness)
    {
        if (!sdfData.isValid)
            return 0.0f;

        glm::vec2 rayDir = point - lightPos;
        float rayLength = glm::length(rayDir);

        if (rayLength < 1e-6f)
            return 0.0f;

        rayDir /= rayLength;

        // 使用球体追踪（sphere tracing）计算软阴影
        float shadow = 1.0f;
        float t = 0.0f;
        const float minStep = 0.01f;
        const int maxSteps = 64;

        for (int i = 0; i < maxSteps && t < rayLength; ++i)
        {
            glm::vec2 samplePos = lightPos + rayDir * t;
            ECS::Vector2f samplePosVec(samplePos.x, samplePos.y);
            float distance = sdfData.SampleWorld(samplePosVec);

            // 如果在物体内部，完全阴影
            if (distance < 0.0f)
            {
                return 1.0f;
            }

            // 计算软阴影因子
            // 距离越小，阴影越硬；softness 控制柔和度
            float penumbra = softness * distance / t;
            shadow = std::min(shadow, penumbra);

            // 前进
            t += std::max(distance, minStep);
        }

        // 反转：0 = 完全照亮，1 = 完全阴影
        return 1.0f - std::clamp(shadow, 0.0f, 1.0f);
    }

    // ==================== 屏幕空间阴影 ====================

    float ShadowRenderer::CalculateScreenSpaceShadow(
        const glm::vec2& screenPos,
        const glm::vec2& lightScreenPos,
        const std::vector<float>& depthBuffer,
        int width, int height)
    {
        if (depthBuffer.empty() || width <= 0 || height <= 0)
            return 0.0f;

        // 从光源到目标点的方向
        glm::vec2 rayDir = screenPos - lightScreenPos;
        float rayLength = glm::length(rayDir);

        if (rayLength < 1e-6f)
            return 0.0f;

        rayDir /= rayLength;

        // 沿射线采样深度缓冲区
        const int numSamples = 16;
        float stepSize = rayLength / static_cast<float>(numSamples);
        float shadow = 0.0f;

        for (int i = 1; i < numSamples; ++i)
        {
            glm::vec2 samplePos = lightScreenPos + rayDir * (stepSize * i);

            // 转换到像素坐标
            int px = static_cast<int>(samplePos.x * width);
            int py = static_cast<int>(samplePos.y * height);

            // 边界检查
            if (px < 0 || px >= width || py < 0 || py >= height)
                continue;

            // 采样深度
            float depth = depthBuffer[py * width + px];

            // 如果有遮挡物，增加阴影
            if (depth > 0.0f)
            {
                shadow += 1.0f / numSamples;
            }
        }

        return std::clamp(shadow, 0.0f, 1.0f);
    }

    // ==================== 阴影缓存 ====================

    void ShadowRenderer::InvalidateAllShadowCaches()
    {
        // 重置缓存统计
        m_cacheHits = 0;
        m_cacheMisses = 0;

        // 清空待失效列表
        m_entitiesToInvalidate.clear();

        LogInfo("All shadow caches invalidated");
    }

    void ShadowRenderer::ResetCacheStatistics()
    {
        m_cacheHits = 0;
        m_cacheMisses = 0;
    }

    void ShadowRenderer::InvalidateShadowCache(entt::entity entity)
    {
        // 在下一次更新时，该实体的缓存将被标记为脏
        // 需要通过场景访问组件
        // 注意：这个方法需要在有场景上下文时调用
        // 实际的缓存失效会在 UpdateShadowCaches 中处理
        
        // 记录需要失效的实体
        m_entitiesToInvalidate.push_back(entity);
    }

    void ShadowRenderer::ProcessCacheInvalidations(RuntimeScene* scene)
    {
        if (!scene || m_entitiesToInvalidate.empty())
            return;

        auto& registry = scene->GetRegistry();

        for (auto entity : m_entitiesToInvalidate)
        {
            if (registry.valid(entity) && registry.all_of<ECS::ShadowCasterComponent>(entity))
            {
                auto& caster = registry.get<ECS::ShadowCasterComponent>(entity);
                caster.cacheData.Invalidate();
                
                // 如果启用了 SDF，也使 SDF 数据失效
                if (caster.enableSDF)
                {
                    caster.InvalidateSdf();
                }
            }
        }

        m_entitiesToInvalidate.clear();
    }

    float ShadowRenderer::GetCacheHitRate() const
    {
        uint64_t total = m_cacheHits + m_cacheMisses;
        if (total == 0)
            return 0.0f;
        return static_cast<float>(m_cacheHits) / static_cast<float>(total);
    }

    void ShadowRenderer::UpdateSDFData(RuntimeScene* scene)
    {
        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // 更新需要 SDF 的阴影投射器
        auto view = registry.view<ECS::ShadowCasterComponent, ECS::TransformComponent>();
        for (auto entity : view)
        {
            auto& caster = view.get<ECS::ShadowCasterComponent>(entity);
            auto& transform = view.get<ECS::TransformComponent>(entity);

            if (!caster.Enable || !caster.enableSDF)
                continue;

            // 检查是否需要重新生成 SDF
            ECS::Vector2f position(transform.position.x, transform.position.y);
            ECS::Vector2f scale(transform.scale.x, transform.scale.y);

            if (caster.NeedsCacheUpdate(position, transform.rotation, scale))
            {
                // 生成顶点
                glm::vec2 pos(transform.position.x, transform.position.y);
                glm::vec2 scl(transform.scale.x, transform.scale.y);
                auto vertices = GenerateVertices(caster, pos, scl, transform.rotation);

                // 生成 SDF
                caster.sdfData = GenerateSDF(caster, vertices);

                // 更新缓存
                caster.cacheData.UpdateCache(position, transform.rotation, scale, m_currentFrame);

                m_cacheMisses++;
            }
            else
            {
                m_cacheHits++;
            }
        }
    }

    void ShadowRenderer::UpdateShadowCaches(RuntimeScene* scene)
    {
        if (!scene)
            return;

        auto& registry = scene->GetRegistry();

        // 更新阴影缓存状态
        auto view = registry.view<ECS::ShadowCasterComponent, ECS::TransformComponent>();
        for (auto entity : view)
        {
            auto& caster = view.get<ECS::ShadowCasterComponent>(entity);
            auto& transform = view.get<ECS::TransformComponent>(entity);

            if (!caster.Enable)
                continue;

            // 获取当前变换
            ECS::Vector2f position(transform.position.x, transform.position.y);
            ECS::Vector2f scale(transform.scale.x, transform.scale.y);

            // 如果缓存未启用，每帧都需要更新
            if (!caster.enableCache)
            {
                m_cacheMisses++;
                continue;
            }

            // 静态物体：只在首次或脏标记时更新
            if (caster.isStatic)
            {
                if (!caster.cacheData.isCached || caster.cacheData.isDirty)
                {
                    // 需要更新缓存
                    caster.cacheData.UpdateCache(position, transform.rotation, scale, m_currentFrame);
                    m_cacheMisses++;
                }
                else
                {
                    // 使用缓存
                    m_cacheHits++;
                }
            }
            else
            {
                // 动态物体：检查变换是否改变
                if (caster.cacheData.HasTransformChanged(position, transform.rotation, scale))
                {
                    // 变换改变，需要更新缓存
                    caster.cacheData.MarkDirty();
                    caster.cacheData.UpdateCache(position, transform.rotation, scale, m_currentFrame);
                    m_cacheMisses++;
                }
                else
                {
                    // 变换未改变，使用缓存
                    m_cacheHits++;
                }
            }
        }
    }

    void ShadowRenderer::CreateSDFBuffer(EngineContext& engineCtx)
    {
        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
            return;

        auto nutContext = graphicsBackend->GetNutContext();
        if (!nutContext)
            return;

        // 创建 SDF 数据缓冲区
        // 预分配足够的空间存储多个 SDF 纹理的数据
        const size_t maxSDFSize = 256 * 256 * sizeof(float) * MAX_SHADOW_CASTERS;

        Nut::BufferLayout sdfLayout;
        sdfLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
        sdfLayout.size = maxSDFSize;
        sdfLayout.mapped = false;

        m_sdfBuffer = std::make_shared<Nut::Buffer>(sdfLayout, nutContext);

        LogInfo("SDF buffer created");
    }
}

/**
 * @file DeferredLightingPass.cpp
 * @brief 延迟光照 Pass 实现
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 8.1, 8.3
 */

#include "DeferredLightingPass.h"
#include "LightingSystem.h"
#include "../Renderer/Nut/NutContext.h"
#include "../Renderer/Nut/ShaderRegistry.h"
#include "../Renderer/LightingRenderer.h"
#include "Logger.h"

namespace Systems
{

DeferredLightingPass* DeferredLightingPass::s_instance = nullptr;

DeferredLightingPass::DeferredLightingPass()
{
}

DeferredLightingPass::~DeferredLightingPass()
{
    Shutdown();
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

DeferredLightingPass& DeferredLightingPass::GetInstance()
{
    if (!s_instance)
    {
        static DeferredLightingPass instance;
        s_instance = &instance;
    }
    return *s_instance;
}

bool DeferredLightingPass::Initialize(const std::shared_ptr<Nut::NutContext>& context)
{
    if (m_initialized)
        return true;

    if (!context)
    {
        LogError("DeferredLightingPass::Initialize - NutContext is null");
        return false;
    }

    m_context = context;

    // 创建全屏四边形缓冲区
    CreateFullscreenQuadBuffer();

    // 创建渲染管线
    if (!CreatePipelines())
    {
        LogError("DeferredLightingPass::Initialize - Failed to create pipelines");
        return false;
    }

    m_initialized = true;
    LogInfo("DeferredLightingPass initialized");
    return true;
}

void DeferredLightingPass::Shutdown()
{
    m_fullscreenLightingPipeline.reset();
    m_lightVolumePipeline.reset();
    m_fullscreenQuadBuffer.reset();
    m_lightVolumes.clear();
    m_lightingSystem = nullptr;
    m_deferredRenderer = nullptr;
    m_context.reset();
    m_initialized = false;
}

void DeferredLightingPass::SetLightingSystem(LightingSystem* lightingSystem)
{
    m_lightingSystem = lightingSystem;
}

void DeferredLightingPass::SetDeferredRenderer(DeferredRenderer* deferredRenderer)
{
    m_deferredRenderer = deferredRenderer;
}

bool DeferredLightingPass::CreatePipelines()
{
    // 注意：实际的管线创建需要着色器模块
    // 这里提供框架，实际实现需要在着色器注册后完成
    
    // 管线创建将在着色器系统初始化后进行
    // 目前返回 true 表示框架已准备好
    
    return true;
}

void DeferredLightingPass::CreateFullscreenQuadBuffer()
{
    if (!m_context)
        return;

    // 全屏四边形顶点数据
    // 使用三角形带：左下、右下、左上、右上
    struct FullscreenVertex
    {
        float position[2];
        float uv[2];
    };

    FullscreenVertex vertices[] = {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},  // 左下
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},  // 右下
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},  // 左上
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},  // 右上
    };

    Nut::BufferLayout layout;
    layout.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    layout.size = sizeof(vertices);
    layout.mapped = false;

    m_fullscreenQuadBuffer = std::make_shared<Nut::Buffer>(layout, m_context);
    m_fullscreenQuadBuffer->WriteBuffer(vertices, sizeof(vertices));
}

void DeferredLightingPass::Execute(Nut::RenderTarget* outputTarget)
{
    if (!m_initialized || !outputTarget)
        return;

    if (!m_deferredRenderer || !m_deferredRenderer->IsGBufferValid())
    {
        LogWarn("DeferredLightingPass::Execute - G-Buffer not valid");
        return;
    }

    // 获取光源数量
    uint32_t lightCount = 0;
    if (m_lightingSystem)
    {
        lightCount = m_lightingSystem->GetLightCount();
    }

    // 根据光源数量选择渲染方式
    if (ShouldUseLightVolumeRendering(lightCount))
    {
        ExecuteLightVolumeRendering();
    }
    else
    {
        ExecuteFullscreenLighting();
    }
}

void DeferredLightingPass::ExecuteFullscreenLighting()
{
    if (!m_context || !m_deferredRenderer)
        return;

    // 绑定 G-Buffer 纹理
    BindGBufferTextures();

    // 绑定光照数据
    BindLightingData();

    // 执行全屏光照计算
    // 注意：实际的渲染命令需要在渲染 Pass 中执行
    // 这里提供框架，实际实现需要与渲染系统集成

    LogDebug("DeferredLightingPass: Executing fullscreen lighting");
}

void DeferredLightingPass::ExecuteLightVolumeRendering()
{
    if (!m_context || !m_lightingSystem || !m_deferredRenderer)
        return;

    // 生成光源体积数据
    const auto& lights = m_lightingSystem->GetVisibleLights();
    m_lightVolumes = GenerateLightVolumes(lights);

    // 绑定 G-Buffer 纹理
    BindGBufferTextures();

    // 对每个光源体积执行渲染
    for (const auto& volume : m_lightVolumes)
    {
        // 根据体积类型选择渲染方式
        switch (volume.type)
        {
            case LightVolumeType::Fullscreen:
                // 方向光使用全屏四边形
                break;
            case LightVolumeType::Sphere:
                // 点光源使用球体
                break;
            case LightVolumeType::Cone:
                // 聚光灯使用圆锥体
                break;
            case LightVolumeType::Rectangle:
                // 面光源使用矩形
                break;
        }
    }

    LogDebug("DeferredLightingPass: Executing light volume rendering with {} volumes", 
             m_lightVolumes.size());
}

std::vector<LightVolumeData> DeferredLightingPass::GenerateLightVolumes(
    const std::vector<ECS::LightData>& lights)
{
    std::vector<LightVolumeData> volumes;
    volumes.reserve(lights.size());

    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& light = lights[i];
        LightVolumeData volume;
        volume.lightIndex = static_cast<uint32_t>(i);
        volume.position = glm::vec2(light.position[0], light.position[1]);
        volume.radius = light.radius;
        volume.direction = glm::vec2(light.direction[0], light.direction[1]);
        volume.innerAngle = light.innerAngle;
        volume.outerAngle = light.outerAngle;

        // 根据光源类型确定体积类型
        switch (static_cast<ECS::LightType>(light.lightType))
        {
            case ECS::LightType::Point:
                volume.type = LightVolumeType::Sphere;
                break;
            case ECS::LightType::Spot:
                volume.type = LightVolumeType::Cone;
                break;
            case ECS::LightType::Directional:
                volume.type = LightVolumeType::Fullscreen;
                break;
            default:
                volume.type = LightVolumeType::Fullscreen;
                break;
        }

        volumes.push_back(volume);
    }

    return volumes;
}

bool DeferredLightingPass::ShouldUseLightVolumeRendering(uint32_t lightCount) const
{
    // 当光源数量超过阈值时使用光源体积渲染
    // 光源体积渲染可以减少不必要的像素着色
    return lightCount > m_lightVolumeThreshold;
}

void DeferredLightingPass::BindGBufferTextures()
{
    if (!m_deferredRenderer || !m_fullscreenLightingPipeline)
        return;

    // 绑定 G-Buffer 纹理到管线
    // 注意：实际绑定需要在管线创建后进行
    
    // Position buffer
    auto positionView = m_deferredRenderer->GetGBufferView(GBufferType::Position);
    // Normal buffer
    auto normalView = m_deferredRenderer->GetGBufferView(GBufferType::Normal);
    // Albedo buffer
    auto albedoView = m_deferredRenderer->GetGBufferView(GBufferType::Albedo);
    // Material buffer
    auto materialView = m_deferredRenderer->GetGBufferView(GBufferType::Material);
}

void DeferredLightingPass::BindLightingData()
{
    if (!m_lightingSystem || !m_fullscreenLightingPipeline)
        return;

    // 使用 LightingRenderer 绑定光照数据
    auto& lightingRenderer = LightingRenderer::GetInstance();
    lightingRenderer.BindLightingData(m_fullscreenLightingPipeline.get(), 1);
}

} // namespace Systems

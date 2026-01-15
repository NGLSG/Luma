/**
 * @file DeferredRenderer.cpp
 * @brief 延迟渲染器实现
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 8.1, 8.2, 8.3, 8.4, 8.5
 */

#include "DeferredRenderer.h"
#include "../Systems/LightingSystem.h"
#include "Nut/NutContext.h"
#include "Logger.h"

DeferredRenderer* DeferredRenderer::s_instance = nullptr;

DeferredRenderer::DeferredRenderer()
{
}

DeferredRenderer::~DeferredRenderer()
{
    Shutdown();
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

DeferredRenderer& DeferredRenderer::GetInstance()
{
    if (!s_instance)
    {
        static DeferredRenderer instance;
        s_instance = &instance;
    }
    return *s_instance;
}

bool DeferredRenderer::Initialize(const std::shared_ptr<Nut::NutContext>& context)
{
    if (m_initialized)
        return true;

    if (!context)
    {
        LogError("DeferredRenderer::Initialize - NutContext is null");
        return false;
    }

    m_context = context;
    CreateGBufferGlobalBuffer();
    CreateDeferredLightingParamsBuffer();
    m_initialized = true;

    LogInfo("DeferredRenderer initialized");
    return true;
}

void DeferredRenderer::Shutdown()
{
    DestroyGBuffer();
    m_gBufferGlobalBuffer.reset();
    m_deferredLightingParamsBuffer.reset();
    m_compositeTarget.reset();
    m_lightingSystem = nullptr;
    m_context.reset();
    m_initialized = false;
}

// ============================================================================
// G-Buffer 管理
// Requirements: 8.2
// ============================================================================

wgpu::TextureFormat DeferredRenderer::GetGBufferFormat(GBufferType type)
{
    switch (type)
    {
        case GBufferType::Position:
            return wgpu::TextureFormat::RGBA16Float;  // 位置需要高精度
        case GBufferType::Normal:
            return wgpu::TextureFormat::RGBA8Snorm;   // 法线使用有符号归一化
        case GBufferType::Albedo:
            return wgpu::TextureFormat::RGBA8Unorm;   // 颜色使用标准格式
        case GBufferType::Material:
            return wgpu::TextureFormat::RGBA8Unorm;   // 材质属性
        default:
            return wgpu::TextureFormat::RGBA8Unorm;
    }
}

bool DeferredRenderer::CreateGBuffer(uint32_t width, uint32_t height)
{
    if (!m_context || width == 0 || height == 0)
    {
        LogError("DeferredRenderer::CreateGBuffer - Invalid parameters");
        return false;
    }

    // 如果尺寸相同且缓冲区已存在，不需要重新创建
    if (IsGBufferValid() && m_gBufferWidth == width && m_gBufferHeight == height)
    {
        return true;
    }

    // 销毁旧的 G-Buffer
    DestroyGBuffer();

    // 创建所有 G-Buffer 纹理
    const char* bufferNames[] = { "GBuffer_Position", "GBuffer_Normal", "GBuffer_Albedo", "GBuffer_Material" };

    for (size_t i = 0; i < static_cast<size_t>(GBufferType::Count); ++i)
    {
        GBufferType type = static_cast<GBufferType>(i);
        wgpu::TextureFormat format = GetGBufferFormat(type);

        wgpu::TextureDescriptor textureDesc{};
        textureDesc.label = bufferNames[i];
        textureDesc.size = { width, height, 1 };
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = format;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                            wgpu::TextureUsage::TextureBinding |
                            wgpu::TextureUsage::CopySrc;

        wgpu::Texture texture = m_context->GetWGPUDevice().CreateTexture(&textureDesc);
        if (!texture)
        {
            LogError("DeferredRenderer::CreateGBuffer - Failed to create {} texture", bufferNames[i]);
            DestroyGBuffer();
            return false;
        }

        m_gBuffers[i] = std::make_shared<Nut::RenderTarget>(
            texture,
            static_cast<uint16_t>(width),
            static_cast<uint16_t>(height)
        );
    }

    // 创建合成渲染目标
    {
        wgpu::TextureDescriptor textureDesc{};
        textureDesc.label = "DeferredComposite";
        textureDesc.size = { width, height, 1 };
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = wgpu::TextureFormat::RGBA16Float;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                            wgpu::TextureUsage::TextureBinding |
                            wgpu::TextureUsage::CopySrc;

        wgpu::Texture texture = m_context->GetWGPUDevice().CreateTexture(&textureDesc);
        if (!texture)
        {
            LogError("DeferredRenderer::CreateGBuffer - Failed to create composite texture");
            DestroyGBuffer();
            return false;
        }

        m_compositeTarget = std::make_shared<Nut::RenderTarget>(
            texture,
            static_cast<uint16_t>(width),
            static_cast<uint16_t>(height)
        );
    }

    m_gBufferWidth = width;
    m_gBufferHeight = height;

    // 更新全局数据
    UpdateGBufferGlobalData();

    LogInfo("G-Buffer created: {}x{}", width, height);
    return true;
}

void DeferredRenderer::DestroyGBuffer()
{
    for (auto& buffer : m_gBuffers)
    {
        buffer.reset();
    }
    m_compositeTarget.reset();
    m_gBufferWidth = 0;
    m_gBufferHeight = 0;
}

bool DeferredRenderer::IsGBufferValid() const
{
    for (const auto& buffer : m_gBuffers)
    {
        if (!buffer)
            return false;
    }
    return m_gBufferWidth > 0 && m_gBufferHeight > 0;
}

std::shared_ptr<Nut::RenderTarget> DeferredRenderer::GetGBuffer(GBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index < m_gBuffers.size())
    {
        return m_gBuffers[index];
    }
    return nullptr;
}

wgpu::TextureView DeferredRenderer::GetGBufferView(GBufferType type) const
{
    auto buffer = GetGBuffer(type);
    if (buffer)
    {
        return buffer->GetView();
    }
    return nullptr;
}

// ============================================================================
// 渲染模式控制
// Requirements: 8.5
// ============================================================================

void DeferredRenderer::SetRenderMode(RenderMode mode)
{
    m_renderMode = mode;
    
    // 如果不是自动模式，直接设置有效模式
    if (mode != RenderMode::Auto)
    {
        m_effectiveRenderMode = mode;
    }
    
    UpdateGBufferGlobalData();
}

void DeferredRenderer::UpdateRenderMode(uint32_t lightCount)
{
    // 只有在自动模式下才根据光源数量切换
    if (m_renderMode == RenderMode::Auto)
    {
        // 使用滞后（hysteresis）防止快速切换
        // 切换到延迟模式需要超过阈值
        // 切换回前向模式需要低于阈值的 80%
        const uint32_t switchToDeferred = m_autoSwitchThreshold;
        const uint32_t switchToForward = static_cast<uint32_t>(m_autoSwitchThreshold * 0.8f);
        
        RenderMode newMode = m_effectiveRenderMode;
        
        if (m_effectiveRenderMode == RenderMode::Forward && lightCount >= switchToDeferred)
        {
            newMode = RenderMode::Deferred;
        }
        else if (m_effectiveRenderMode == RenderMode::Deferred && lightCount < switchToForward)
        {
            newMode = RenderMode::Forward;
        }
        
        if (newMode != m_effectiveRenderMode)
        {
            m_effectiveRenderMode = newMode;
            UpdateGBufferGlobalData();
            UpdateCompositeParams();
            LogInfo("Auto-switched to {} rendering (light count: {}, threshold: {})",
                    newMode == RenderMode::Deferred ? "deferred" : "forward",
                    lightCount, m_autoSwitchThreshold);
        }
    }
}

// ============================================================================
// 延迟光照 Pass
// Requirements: 8.1, 8.3
// ============================================================================

void DeferredRenderer::SetLightingSystem(Systems::LightingSystem* lightingSystem)
{
    m_lightingSystem = lightingSystem;
}

void DeferredRenderer::CreateGBufferGlobalBuffer()
{
    if (!m_context)
        return;

    Nut::BufferLayout layout;
    layout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    layout.size = sizeof(GBufferGlobalData);
    layout.mapped = false;

    m_gBufferGlobalBuffer = std::make_shared<Nut::Buffer>(layout, m_context);

    // 写入默认数据
    m_gBufferGlobalBuffer->WriteBuffer(&m_gBufferGlobalData, sizeof(GBufferGlobalData));
}

void DeferredRenderer::CreateDeferredLightingParamsBuffer()
{
    if (!m_context)
        return;

    Nut::BufferLayout layout;
    layout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    layout.size = sizeof(DeferredLightingParams);
    layout.mapped = false;

    m_deferredLightingParamsBuffer = std::make_shared<Nut::Buffer>(layout, m_context);

    // 写入默认数据
    m_deferredLightingParamsBuffer->WriteBuffer(&m_deferredLightingParams, sizeof(DeferredLightingParams));
}

void DeferredRenderer::UpdateGBufferGlobalData()
{
    m_gBufferGlobalData.bufferWidth = m_gBufferWidth;
    m_gBufferGlobalData.bufferHeight = m_gBufferHeight;
    m_gBufferGlobalData.renderMode = static_cast<uint32_t>(m_effectiveRenderMode);
    m_gBufferGlobalData.enableDeferred = (m_effectiveRenderMode == RenderMode::Deferred) ? 1 : 0;

    if (m_gBufferGlobalBuffer)
    {
        m_gBufferGlobalBuffer->WriteBuffer(&m_gBufferGlobalData, sizeof(GBufferGlobalData));
    }
}

std::shared_ptr<Nut::Buffer> DeferredRenderer::GetDeferredLightingParamsBuffer() const
{
    return m_deferredLightingParamsBuffer;
}

std::shared_ptr<Nut::Buffer> DeferredRenderer::GetGBufferGlobalBuffer() const
{
    return m_gBufferGlobalBuffer;
}

void DeferredRenderer::UpdateDeferredLightingParams(
    uint32_t lightCount,
    uint32_t maxLightsPerPixel,
    bool enableShadows,
    float ambientIntensity)
{
    m_deferredLightingParams.lightCount = lightCount;
    m_deferredLightingParams.maxLightsPerPixel = maxLightsPerPixel;
    m_deferredLightingParams.enableShadows = enableShadows ? 1 : 0;
    m_deferredLightingParams.ambientIntensity = ambientIntensity;
    m_deferredLightingParams.debugMode = m_debugMode;

    if (m_deferredLightingParamsBuffer)
    {
        m_deferredLightingParamsBuffer->WriteBuffer(&m_deferredLightingParams, sizeof(DeferredLightingParams));
    }
}

void DeferredRenderer::BindGBufferData(Nut::RenderPipeline* pipeline, uint32_t groupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    auto globalBuffer = GetGBufferGlobalBuffer();
    if (globalBuffer)
    {
        pipeline->SetBinding(groupIndex, 0, *globalBuffer);
    }

    // 绑定 G-Buffer 纹理（如果有效）
    // 注意：纹理绑定需要在着色器中定义相应的绑定点

    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

// ============================================================================
// 延迟/前向混合
// Requirements: 8.4
// ============================================================================

bool DeferredRenderer::ShouldUseForwardRendering(bool isTransparent) const
{
    // 透明物体始终使用前向渲染
    if (isTransparent)
        return true;

    // 如果当前使用前向渲染模式，返回 true
    return m_effectiveRenderMode == RenderMode::Forward;
}

std::shared_ptr<Nut::RenderTarget> DeferredRenderer::GetCompositeTarget() const
{
    return m_compositeTarget;
}

std::shared_ptr<Nut::RenderTarget> DeferredRenderer::GetForwardTarget() const
{
    return m_forwardTarget;
}

bool DeferredRenderer::CreateForwardTarget(uint32_t width, uint32_t height)
{
    if (!m_context || width == 0 || height == 0)
    {
        LogError("DeferredRenderer::CreateForwardTarget - Invalid parameters");
        return false;
    }

    // 创建前向渲染目标（用于透明物体）
    wgpu::TextureDescriptor textureDesc{};
    textureDesc.label = "ForwardTarget";
    textureDesc.size = { width, height, 1 };
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.format = wgpu::TextureFormat::RGBA16Float;
    textureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                        wgpu::TextureUsage::TextureBinding |
                        wgpu::TextureUsage::CopySrc;

    wgpu::Texture texture = m_context->GetWGPUDevice().CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("DeferredRenderer::CreateForwardTarget - Failed to create texture");
        return false;
    }

    m_forwardTarget = std::make_shared<Nut::RenderTarget>(
        texture,
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height)
    );

    // 创建合成参数缓冲区
    if (!m_compositeParamsBuffer)
    {
        Nut::BufferLayout layout;
        layout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        layout.size = sizeof(CompositeParams);
        layout.mapped = false;

        m_compositeParamsBuffer = std::make_shared<Nut::Buffer>(layout, m_context);
        UpdateCompositeParams();
    }

    LogInfo("Forward target created: {}x{}", width, height);
    return true;
}

std::shared_ptr<Nut::Buffer> DeferredRenderer::GetCompositeParamsBuffer() const
{
    return m_compositeParamsBuffer;
}

void DeferredRenderer::UpdateCompositeParams()
{
    m_compositeParams.enableDeferred = (m_effectiveRenderMode == RenderMode::Deferred) ? 1 : 0;
    m_compositeParams.enableForward = 1;  // 前向渲染始终启用（用于透明物体）
    m_compositeParams.blendMode = static_cast<uint32_t>(m_blendMode);
    m_compositeParams.debugMode = m_debugMode;
    m_compositeParams.deferredWeight = m_deferredWeight;
    m_compositeParams.forwardWeight = m_forwardWeight;

    if (m_compositeParamsBuffer)
    {
        m_compositeParamsBuffer->WriteBuffer(&m_compositeParams, sizeof(CompositeParams));
    }
}

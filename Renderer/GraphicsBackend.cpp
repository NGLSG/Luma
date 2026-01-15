#include "GraphicsBackend.h"

#include <iostream>

#include "Nut/NutContext.h"
#include "Nut/TextureA.h"
#include "Nut/Buffer.h"
#include "Nut/ShaderCache.h"
#include "Logger.h"
#include "Resources/RuntimeAsset/RuntimeWGSLMaterial.h"
#include "LightingRenderer.h"
#include "include/gpu/graphite/Image.h"
#include "include/core/SkCanvas.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#endif


GraphicsBackend::GraphicsBackend() = default;

GraphicsBackend::~GraphicsBackend()
{
    shutdown();
}


Nut::TextureAPtr GraphicsBackend::LoadTextureFromFile(const std::string& filename)
{
    if (!nutContext)
    {
        LogError("LoadTextureFromFile 失败: nutContext为空");
        return nullptr;
    }

    return nutContext->LoadTextureFromFile(filename);
}

Nut::TextureAPtr GraphicsBackend::LoadTextureFromData(const unsigned char* data, size_t size)
{
    if (!nutContext)
    {
        LogError("LoadTextureFromData 失败: nutContext为空");
        return nullptr;
    }

    return nutContext->CreateTextureFromMemory(data, size);
}


void GraphicsBackend::updateQualitySettings()
{
    uint32_t previousSampleCount = m_msaaSampleCount;

    switch (options.qualityLevel)
    {
    case QualityLevel::Low: m_msaaSampleCount = 1;
        break;
    case QualityLevel::Medium: m_msaaSampleCount = 2;
        break;
    case QualityLevel::High: m_msaaSampleCount = 4;
        break;
    case QualityLevel::Ultra: m_msaaSampleCount = 8;
        break;
    }

    if (m_msaaSampleCount > 1)
    {
        m_msaaSampleCount = 4;
    }

    LogInfo("更新质量设置 - MSAA 样本数: {} -> {}", previousSampleCount, m_msaaSampleCount);
}

void GraphicsBackend::initialize(const GraphicsBackendOptions& opts)
{
    this->options = opts;
    isDeviceLost = false;
    enableVSync = options.enableVSync;
    updateQualitySettings();

    try
    {
        Nut::NutContextDescriptor nutDesc;
        nutDesc.width = opts.width;
        nutDesc.height = opts.height;
        nutDesc.enableVSync = opts.enableVSync;


#if defined(_WIN32)
        nutDesc.windowHandle.hWnd = opts.windowHandle.hWnd;
        nutDesc.windowHandle.hInst = opts.windowHandle.hInst;
#elif defined(__APPLE__)
        nutDesc.windowHandle.metalLayer = opts.windowHandle.metalLayer;
#elif defined(__linux__) && !defined(__ANDROID__)
        nutDesc.windowHandle.x11Display = opts.windowHandle.x11Display;
        nutDesc.windowHandle.x11Window = opts.windowHandle.x11Window;
#elif defined(__ANDROID__)
        nutDesc.windowHandle.aNativeWindow = opts.windowHandle.aNativeWindow;
#endif


        nutDesc.backendTypePriority = opts.backendTypePriority;


        switch (opts.qualityLevel)
        {
        case QualityLevel::Low:
            nutDesc.qualityLevel = Nut::QualityLevel::Low;
            break;
        case QualityLevel::Medium:
            nutDesc.qualityLevel = Nut::QualityLevel::Medium;
            break;
        case QualityLevel::High:
        case QualityLevel::Ultra:
            nutDesc.qualityLevel = Nut::QualityLevel::High;
            break;
        }


        nutContext = Nut::NutContext::Create(nutDesc);
        if (!nutContext)
        {
            LogError("创建 NutContext 失败");
            throw std::runtime_error("创建 NutContext 失败");
        }


        Nut::ShaderCache::Initialize();

        skgpu::graphite::DawnBackendContext backendContext;
        backendContext.fInstance = nutContext->GetWGPUInstance();
        backendContext.fDevice = nutContext->GetWGPUDevice();
        backendContext.fQueue = nutContext->GetWGPUDevice().GetQueue();

        graphiteContext = skgpu::graphite::ContextFactory::MakeDawn(backendContext, {});
        if (!graphiteContext)
        {
            LogError("创建 Skia Graphite 上下文失败");
            throw std::runtime_error("创建 Skia Graphite 上下文失败");
        }

        graphiteRecorder = graphiteContext->makeRecorder();
        if (!graphiteRecorder)
        {
            LogError("创建 Graphite 记录器失败");
            throw std::runtime_error("创建 Graphite 记录器失败");
        }

        this->currentWidth = options.width;
        this->currentHeight = options.height;


        if (m_msaaSampleCount > 1)
        {
            Nut::TextureDescriptor msaaDesc;
            msaaDesc.SetSize(currentWidth, currentHeight)
                    .SetFormat(GetSurfaceFormat())
                    .SetSampleCount(m_msaaSampleCount)
                    .SetUsage(wgpu::TextureUsage::RenderAttachment)
                    .SetLabel("MSAA Texture");

            m_msaaTexture = nutContext->CreateTexture(msaaDesc);

            if (!m_msaaTexture)
            {
                LogError("创建 MSAA 纹理失败 (样本数: {})", m_msaaSampleCount);
            }
            else
            {
                LogInfo("创建 MSAA 纹理 (样本数: {})", m_msaaSampleCount);
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("图形后端初始化过程中发生异常: {}", e.what());
        throw;
    }
}

void GraphicsBackend::ResolveMSAA()
{
    if (m_msaaSampleCount <= 1 || !nutContext || !m_msaaTexture)
    {
        return;
    }

    if (m_activeRenderTarget)
    {
        return;
    }

    auto currentTexture = nutContext->GetCurrentTexture();
    if (!currentTexture || !currentTexture->GetTexture())
    {
        return;
    }

    if (!nutContext->ResolveTexture(m_msaaTexture, currentTexture))
    {
        LogError("ResolveMSAA: 调用 NutContext 解析 MSAA 失败");
    }
}

std::shared_ptr<RenderTarget> GraphicsBackend::CreateOrGetRenderTarget(const std::string& name, uint16_t width,
                                                                       uint16_t height)
{
    if (name.empty())
    {
        LogError("CreateOrGetRenderTarget 失败: 渲染目标名称为空");
        return nullptr;
    }

    if (width == 0 || height == 0)
    {
        LogError("CreateOrGetRenderTarget 失败: 无效的尺寸 ({}x{})", width, height);
        return nullptr;
    }

    if (!nutContext)
    {
        LogError("CreateOrGetRenderTarget 失败: nutContext为空");
        return nullptr;
    }

    auto target = nutContext->CreateOrGetRenderTarget(name, width, height);
    if (!target)
    {
        LogError("CreateOrGetRenderTarget 失败: NutContext 创建渲染目标 {} 失败", name);
        return nullptr;
    }

    return target;
}

void GraphicsBackend::SetActiveRenderTarget(std::shared_ptr<RenderTarget> target)
{
    m_activeRenderTarget = std::move(target);
    if (nutContext)
    {
        nutContext->SetActiveRenderTarget(m_activeRenderTarget);
    }
}

sk_sp<SkImage> GraphicsBackend::CreateImageFromCompressedData(const unsigned char* data, size_t size,
                                                              int width, int height, wgpu::TextureFormat format)
{
    if (!data || size == 0)
    {
        LogError("CreateImageFromCompressedData 失败: 数据指针为空或大小为 0");
        return nullptr;
    }

    if (width <= 0 || height <= 0)
    {
        LogError("CreateImageFromCompressedData 失败: 无效的尺寸 ({}x{})", width, height);
        return nullptr;
    }

    if (!nutContext)
    {
        LogError("CreateImageFromCompressedData 失败: nutContext为空");
        return nullptr;
    }

    auto texture = nutContext->CreateTextureFromCompressedData(data, size, width, height, format);
    if (!texture)
    {
        LogError("CreateImageFromCompressedData 失败: NutContext 创建纹理失败");
        return nullptr;
    }

    auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(texture->GetTexture().Get());
    if (!backendTex.isValid())
    {
        LogError("从 WGPU 纹理创建有效的 BackendTexture 失败");
        return nullptr;
    }

    sk_sp<SkImage> image = SkImages::WrapTexture(
        graphiteRecorder.get(),
        backendTex,
        kUnknown_SkColorType,
        kPremul_SkAlphaType,
        SkColorSpace::MakeSRGB()
    );

    if (!image)
    {
        LogError("包装后端纹理为压缩数据图像失败");
        return nullptr;
    }

    return image;
}

sk_sp<SkSurface> GraphicsBackend::GetSurface()
{
    if (m_activeRenderTarget)
    {
        skgpu::graphite::BackendTexture backendTex = skgpu::graphite::BackendTextures::MakeDawn(
            m_activeRenderTarget->GetTexture().Get());

        if (!backendTex.isValid())
        {
            LogError("GetSurface: 活动渲染目标的后端纹理无效");
            return nullptr;
        }

        auto surface = SkSurfaces::WrapBackendTexture(graphiteRecorder.get(),
                                                      backendTex,
                                                      kRGBA_8888_SkColorType,
                                                      SkColorSpace::MakeSRGB(),
                                                      nullptr);
        if (!surface)
        {
            LogError("GetSurface: 包装活动渲染目标失败");
        }
        return surface;
    }


    if (nutContext)
    {
        if (m_msaaSampleCount > 1 && m_msaaTexture)
        {
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_msaaTexture->GetTexture().Get());
            if (!backendTex.isValid()) return nullptr;

            return SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                  kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
        }


        auto currentTexture = nutContext->GetCurrentTexture();
        if (!currentTexture || !currentTexture->GetTexture())
        {
            LogError("GetSurface: 获取当前纹理失败");
            return nullptr;
        }


        auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(currentTexture->GetTexture().Get());
        if (!backendTex.isValid())
        {
            LogError("GetSurface: 当前表面纹理的后端纹理无效");
            return nullptr;
        }


        auto skSurface = SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                        kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);


        m_currentSwapChainSurface = skSurface;

        return skSurface;
    }
    else
    {
        if (!offscreenSurface || offscreenSurface->width() != currentWidth || offscreenSurface->height() !=
            currentHeight)
        {
            SkImageInfo imageInfo = SkImageInfo::Make(
                currentWidth, currentHeight,
                kRGBA_8888_SkColorType,
                kPremul_SkAlphaType,
                SkColorSpace::MakeSRGB()
            );

            offscreenSurface = SkSurfaces::RenderTarget(graphiteRecorder.get(), imageInfo);
            if (!offscreenSurface)
            {
                LogError("GetSurface: 创建离屏表面失败 (尺寸: {}x{})", currentWidth, currentHeight);
                return nullptr;
            }
        }
        return offscreenSurface;
    }

    return nullptr;
}

sk_sp<SkImage> GraphicsBackend::CreateSpriteImageFromFile(const char* filePath) const
{
    if (!filePath || strlen(filePath) == 0)
    {
        LogError("CreateSpriteImageFromFile 失败: 文件路径为空");
        return nullptr;
    }

    sk_sp<SkData> encodedData = SkData::MakeFromFileName(filePath);
    if (!encodedData)
    {
        LogError("从文件读取数据失败: {}", filePath);
        return nullptr;
    }

    return CreateSpriteImageFromData(encodedData);
}

sk_sp<SkImage> GraphicsBackend::CreateSpriteImageFromData(const sk_sp<SkData>& data) const
{
    if (!data)
    {
        LogError("CreateSpriteImageFromData 失败: 数据为空");
        return nullptr;
    }

    sk_sp<SkImage> cpuImage = SkImages::DeferredFromEncodedData(data);
    if (!cpuImage)
    {
        LogError("从编码数据解码图像失败");
        return nullptr;
    }

    if (GetRecorder())
    {
        sk_sp<SkImage> gpuImage = SkImages::TextureFromImage(GetRecorder(), cpuImage.get());
        if (!gpuImage)
        {
            LogError("将图像转换为纹理失败");
            return nullptr;
        }
        return gpuImage;
    }

    LogWarn("记录器为空,返回 CPU 图像");
    return nullptr;
}

void GraphicsBackend::Resize(uint16_t width, uint16_t height)
{
    if (width == 0 || height == 0)
    {
        LogError("Resize 失败: 无效的尺寸 ({}x{})", width, height);
        return;
    }

    if (width == currentWidth && height == currentHeight)
    {
        return;
    }

    if (nutContext)
    {
        nutContext->Resize(width, height);
        nutContext->SetActiveRenderTarget(nullptr);
        m_activeRenderTarget = nullptr;


        if (m_msaaSampleCount > 1)
        {
            if (m_msaaTexture)
            {
                m_msaaTexture->GetTexture().Destroy();
            }
            Nut::TextureDescriptor msaaDesc;
            msaaDesc.SetSize(width, height)
                    .SetFormat(GetSurfaceFormat())
                    .SetSampleCount(m_msaaSampleCount)
                    .SetUsage(wgpu::TextureUsage::RenderAttachment)
                    .SetLabel("MSAA Texture");

            m_msaaTexture = nutContext->CreateTexture(msaaDesc);

            if (!m_msaaTexture)
            {
                LogError("创建 MSAA 纹理失败 (样本数: {})", m_msaaSampleCount);
            }
        }
    }

    this->currentWidth = width;
    this->currentHeight = height;
    this->offscreenSurface.reset();
}

bool GraphicsBackend::Recreate()
{
    this->shutdown();

    const int maxRetries = 3;
    const int delayMilliseconds = 500;

    for (int i = 0; i < maxRetries; ++i)
    {
        try
        {
            if (i > 0)
            {
                int delay = delayMilliseconds * (i + 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }

            this->initialize(options);
            return true;
        }
        catch (const std::exception& e)
        {
            LogError("重建尝试 #{} 失败: {}", i + 1, e.what());
            if (i == maxRetries - 1)
            {
                this->shutdown();
                return false;
            }
            this->shutdown();
        }
    }

    return false;
}

void GraphicsBackend::SetEnableVSync(bool enable)
{
    if (enableVSync == enable)
    {
        return;
    }

    enableVSync = enable;
    if (nutContext)
    {
        nutContext->SetVSync(enable);
        nutContext->Resize(currentWidth, currentHeight);
    }
}

void GraphicsBackend::SetQualityLevel(QualityLevel level)
{
    if (options.qualityLevel == level)
    {
        return;
    }

    options.qualityLevel = level;

    if (!Recreate())
    {
        LogError("使用新质量设置重建图形后端失败");
    }
}

bool GraphicsBackend::BeginFrame()
{
    m_activeRenderTarget = nullptr;
    if (nutContext)
    {
        nutContext->SetActiveRenderTarget(nullptr);
    }

    if (isDeviceLost)
    {
        if (!Recreate())
        {
            LogError("BeginFrame: 重建失败");
            return false;
        }
    }

    if (!nutContext)
    {
        LogWarn("BeginFrame: nutContext为空");
        return false;
    }

    return true;
}

wgpu::TextureView GraphicsBackend::GetCurrentFrameView() const
{
    if (!nutContext)
    {
        LogWarn("GetCurrentFrameView: nutContext为空");
        return nullptr;
    }

    nutContext->SetActiveRenderTarget(nullptr);

    auto currentTexture = nutContext->GetCurrentTexture();
    if (!currentTexture || !currentTexture->GetTexture())
    {
        LogWarn("GetCurrentFrameView: 当前表面纹理为空");
        return nullptr;
    }
    return currentTexture->GetTexture().CreateView();
}

void GraphicsBackend::Submit()
{
    if (!graphiteRecorder || !graphiteContext)
    {
        LogError("Submit 失败: 记录器或上下文为空");
        return;
    }

    lastOffscreenImage.reset();

    std::unique_ptr<skgpu::graphite::Recording> recording = graphiteRecorder->snap();
    if (recording)
    {
        skgpu::graphite::InsertRecordingInfo info = {};
        info.fRecording = recording.get();

        if (!graphiteContext->insertRecording(info))
        {
            LogError("Submit: 插入记录失败");
            return;
        }

        if (!graphiteContext->submit())
        {
            LogError("Submit: 提交上下文失败");
            return;
        }
    }
    else
    {
        LogWarn("Submit: 记录器快照为空,跳过提交");
    }
}

void GraphicsBackend::PresentFrame()
{
    if (m_msaaSampleCount > 1)
    {
        ResolveMSAA();
    }

    Submit();

    m_currentSwapChainSurface.reset();

    if (nutContext)
    {
        nutContext->SetActiveRenderTarget(nullptr);

        nutContext->Present();
    }
    else if (offscreenSurface)
    {
        lastOffscreenImage = offscreenSurface->makeImageSnapshot();
    }

    m_activeRenderTarget = nullptr;
}

skgpu::graphite::Context* GraphicsBackend::GetGraphiteContext() const
{
    return graphiteContext.get();
}

skgpu::graphite::Recorder* GraphicsBackend::GetRecorder() const
{
    return graphiteRecorder.get();
}

wgpu::TextureFormat GraphicsBackend::GetSurfaceFormat() const
{
    return wgpu::TextureFormat::RGBA8Unorm;
}

sk_sp<SkImage> GraphicsBackend::DetachLastOffscreenImage()
{
    return std::move(lastOffscreenImage);
}

sk_sp<SkImage> GraphicsBackend::GPUToCPUImage(sk_sp<SkImage> src) const
{
    if (!src)
    {
        LogWarn("GPUToCPUImage: 源图像为空");
        return nullptr;
    }

    if (!src->isTextureBacked())
    {
        return src;
    }

    sk_sp<SkSurface> cpuSurface = SkSurfaces::Raster(src->imageInfo());
    if (!cpuSurface)
    {
        LogError("GPUToCPUImage: 创建 CPU 表面失败");
        return nullptr;
    }

    cpuSurface->getCanvas()->drawImage(src, 0.0f, 0.0f);
    sk_sp<SkImage> result = cpuSurface->makeImageSnapshot();

    if (!result)
    {
        LogError("GPUToCPUImage: 创建图像快照失败");
        return nullptr;
    }

    return result;
}

RuntimeWGSLMaterial* GraphicsBackend::CreateOrGetDefaultMaterial()
{
    static std::unique_ptr<RuntimeWGSLMaterial> defaultMaterial = nullptr;

    static uint32_t cachedSampleCount = 0;
    static std::weak_ptr<Nut::NutContext> cachedContext;

    uint32_t currentSampleCount = GetSampleCount();

    bool needsRebuild = !defaultMaterial;
    if (defaultMaterial)
    {
        if (cachedSampleCount != currentSampleCount) needsRebuild = true;
        if (cachedContext.lock() != nutContext) needsRebuild = true;
    }

    if (needsRebuild)
    {
        defaultMaterial = std::make_unique<RuntimeWGSLMaterial>();

        const std::string defaultShader = R"(
/// @file Common2D.wgsl
/// @brief 2D渲染通用着色器模板
/// @author Luma Engine
/// @version 1.0
/// @date 2025

import Std;

/// @brief 顶点着色器主函数
/// @details 处理顶点变换、UV变换和颜色传递，支持实例化渲染
/// @param input 顶点输入数据
/// @param instanceIdx 实例索引，用于访问实例数据数组
/// @return 处理后的顶点输出数据
@vertex
fn vs_main(input: VertexInput, @builtin(instance_index) instanceIdx: u32) -> VertexOutput {
    // 从实例数据数组中获取当前实例的数据
    let instance = instanceDatas[instanceIdx];

    // 将局部坐标按实例尺寸进行缩放
    let localPos = input.position * instance.size;

    // 将局部坐标变换到裁剪空间
    let clipPosition = TransformVertex(localPos, instance, engineData);

    // 对UV坐标进行变换，应用实例的UV矩形
    let transformedUV = TransformUV(input.uv, instance.uvRect);

    // 构建顶点输出结构
    var out: VertexOutput;
    out.clipPosition = clipPosition;    ///< 裁剪空间位置
    out.uv = transformedUV;             ///< 变换后的UV坐标
    out.color = instance.color;         ///< 实例颜色（包含透明度）

    return out;
}

/// @brief 片段着色器主函数
/// @details 采样纹理并与顶点颜色混合，输出最终像素颜色
/// @param in 顶点着色器传递过来的插值数据
/// @return 输出到颜色附件的RGBA颜色值
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    // 从主纹理采样颜色，使用主采样器
    let texColor = textureSample(mainTexture, mainSampler, in.uv);

    // 将纹理颜色与顶点颜色相乘（支持透明度混合）
    return texColor * in.color;
}
)";


        if (defaultMaterial->Initialize(nutContext, defaultShader, GetSurfaceFormat(), currentSampleCount))
        {
            cachedSampleCount = currentSampleCount;
            cachedContext = nutContext;
            LogInfo("Default WGSL Material initialized successfully.");
        }
        else
        {
            LogError("Failed to initialize default material.");
            defaultMaterial.reset();
            return nullptr;
        }
    }
    return defaultMaterial.get();
}

LightingRenderer& GraphicsBackend::GetLightingRenderer()
{
    auto& renderer = LightingRenderer::GetInstance();
    if (!renderer.IsInitialized() && nutContext)
    {
        renderer.Initialize(nutContext);
    }
    return renderer;
}

RuntimeWGSLMaterial* GraphicsBackend::CreateOrGetLitMaterial()
{
    static std::unique_ptr<RuntimeWGSLMaterial> litMaterial = nullptr;

    static uint32_t cachedSampleCount = 0;
    static std::weak_ptr<Nut::NutContext> cachedContext;

    uint32_t currentSampleCount = GetSampleCount();

    bool needsRebuild = !litMaterial;
    if (litMaterial)
    {
        if (cachedSampleCount != currentSampleCount) needsRebuild = true;
        if (cachedContext.lock() != nutContext) needsRebuild = true;
    }

    if (needsRebuild)
    {
        litMaterial = std::make_unique<RuntimeWGSLMaterial>();

        // 使用 SpriteLit shader - 带光照的精灵渲染
        // 注意：EmissionGlobalData 和 emissionGlobal 绑定已在 SpriteLitCore.wgsl 中定义
        const std::string litShader = R"(
// SpriteLit - 带光照的2D精灵渲染着色器
// Feature: 2d-lighting-enhancement (Emission support)
import Std;
import Lighting;
import SpriteLit;

// 注意：EmissionGlobalData 和 emissionGlobal 绑定已在 SpriteLitCore.wgsl 中定义

@vertex
fn vs_main(
    input: VertexInput,
    @builtin(instance_index) instanceIndex: u32
) -> SpriteLitVertexOutput {
    return TransformSpriteLitVertex(input, instanceIndex);
}

@fragment
fn fs_main(input: SpriteLitVertexOutput) -> @location(0) vec4<f32> {
    // 采样主纹理
    var texColor = textureSample(mainTexture, mainSampler, input.uv);
    
    // 应用实例颜色
    var baseColor = texColor * input.color;
    
    // 计算光照（带阴影）
    var totalLight = CalculateTotalLightingWithShadow(input.worldPos, input.lightLayer);
    
    // 应用光照到基础颜色
    var litColor = baseColor.rgb * totalLight;
    
    // 计算自发光贡献（独立于场景光照）
    // Feature: 2d-lighting-enhancement
    var emissionContribution = vec3<f32>(0.0);
    if (emissionGlobal.emissionEnabled != 0u) {
        // 自发光 = 自发光颜色 × 自发光强度 × 全局自发光缩放
        let emissionColor = input.emissionColor.rgb;
        let emissionIntensity = input.emissionIntensity * emissionGlobal.emissionScale;
        emissionContribution = emissionColor * emissionIntensity;
    }
    
    // 最终颜色 = 光照颜色 + 自发光（加法混合，不受光照影响）
    var finalColor = vec4<f32>(litColor + emissionContribution, baseColor.a);
    
    // 丢弃完全透明的像素
    if (finalColor.a < 0.001) {
        discard;
    }
    
    return finalColor;
}
)";

        if (litMaterial->Initialize(nutContext, litShader, GetSurfaceFormat(), currentSampleCount))
        {
            cachedSampleCount = currentSampleCount;
            cachedContext = nutContext;
            LogInfo("Lit WGSL Material initialized successfully.");
        }
        else
        {
            LogError("Failed to initialize lit material.");
            litMaterial.reset();
            return nullptr;
        }
    }
    return litMaterial.get();
}

void GraphicsBackend::shutdown()
{
    m_activeRenderTarget = nullptr;

    offscreenSurface.reset();
    lastOffscreenImage.reset();

    graphiteRecorder.reset();
    graphiteContext.reset();

    if (m_msaaTexture)
    {
        m_msaaTexture->GetTexture().Destroy();
        m_msaaTexture = nullptr;
    }

    nutContext.reset();


    Nut::ShaderCache::Shutdown();

    isDeviceLost = false;
}

std::unique_ptr<GraphicsBackend> GraphicsBackend::Create(const GraphicsBackendOptions& options)
{
    std::unique_ptr<GraphicsBackend> backend(new GraphicsBackend());
    try
    {
        backend->initialize(options);
    }
    catch (const std::exception& e)
    {
        LogError("初始化图形后端失败: {}", e.what());
        return nullptr;
    }
    m_instance = backend.get();
    return backend;
}

#include "GraphicsBackend.h"

#include "include/core/SkData.h"
#include "include/gpu/graphite/Image.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkSurface.h"
#include "../Utils/PCH.h"
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


Nut::TextureA GraphicsBackend::LoadTextureFromFile(const std::string& filename)
{
    if (!nutContext)
    {
        LogError("LoadTextureFromFile 失败: nutContext为空");
        return nullptr;
    }
    
    return nutContext->LoadTextureFromFile(filename);
}

Nut::TextureA GraphicsBackend::LoadTextureFromData(const unsigned char* data, size_t size)
{
    if (!nutContext)
    {
        LogError("LoadTextureFromData 失败: nutContext为空");
        return nullptr;
    }
    
    return nutContext->LoadTextureFromMemory(data, size);
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
        
        
        for (const auto& type : opts.backendTypePriority)
        {
            switch (type)
            {
            case BackendType::D3D12:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::D3D12);
                break;
            case BackendType::D3D11:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::D3D11);
                break;
            case BackendType::Vulkan:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::Vulkan);
                break;
            case BackendType::Metal:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::Metal);
                break;
            case BackendType::OpenGL:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::OpenGL);
                break;
            case BackendType::OpenGLES:
                nutDesc.backendTypePriority.push_back(Nut::BackendType::OpenGLES);
                break;
            }
        }
        
        
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
    if (m_msaaSampleCount <= 1 || !nutContext || !m_msaaTexture || m_activeRenderTarget)
    {
        return;
    }

    auto currentTexture = nutContext->GetCurrentTexture();
    if (!currentTexture.GetTexture())
    {
        LogError("ResolveMSAA: 获取当前纹理失败");
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

    auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(texture.GetTexture().Get());
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

        auto m_surface = SkSurfaces::WrapBackendTexture(graphiteRecorder.get(),
                                                        backendTex,
                                                        kBGRA_8888_SkColorType,
                                                        SkColorSpace::MakeSRGB(),
                                                        nullptr);
        if (!m_surface)
        {
            LogError("GetSurface: 包装活动渲染目标失败");
        }
        return m_surface;
    }

    if (nutContext)
    {
        if (m_msaaSampleCount > 1 && m_msaaTexture)
        {
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_msaaTexture.GetTexture().Get());
            if (!backendTex.isValid())
            {
                LogError("GetSurface: MSAA 纹理的后端纹理无效");
                return nullptr;
            }

            auto skSurface = SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                            kBGRA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
            if (!skSurface)
            {
                LogError("GetSurface: 包装 MSAA 纹理失败");
            }
            return skSurface;
        }
        else
        {
            auto currentTexture = nutContext->GetCurrentTexture();
            if (!currentTexture.GetTexture())
            {
                LogError("GetSurface: 获取当前纹理失败");
                return nullptr;
            }
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(currentTexture.GetTexture().Get());
            if (!backendTex.isValid())
            {
                LogError("GetSurface: 当前表面纹理的后端纹理无效");
                return nullptr;
            }

            auto skSurface = SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                            kBGRA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
            if (!skSurface)
            {
                LogError("GetSurface: 包装表面纹理失败");
            }
            return skSurface;
        }
    }
    else
    {
        if (!offscreenSurface || offscreenSurface->width() != currentWidth || offscreenSurface->height() !=
            currentHeight)
        {
            SkImageInfo imageInfo = SkImageInfo::Make(
                currentWidth, currentHeight,
                kBGRA_8888_SkColorType,
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
                m_msaaTexture.GetTexture().Destroy();
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
    // ImGui 等 UI 始终应渲染到交换链纹理，因此强制回到默认表面。
    nutContext->SetActiveRenderTarget(nullptr);

    auto currentTexture = nutContext->GetCurrentTexture();
    if (!currentTexture.GetTexture())
    {
        LogWarn("GetCurrentFrameView: 当前表面纹理为空");
        return nullptr;
    }
    return currentTexture.GetTexture().CreateView();
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
    if (nutContext)
    {
        m_activeRenderTarget = nullptr;
        // 确保最终呈现的目标是交换链而非自定义 RenderTarget。
        nutContext->SetActiveRenderTarget(nullptr);
        nutContext->Present();
    }
    else if (offscreenSurface)
    {
        lastOffscreenImage = offscreenSurface->makeImageSnapshot();
        if (!lastOffscreenImage)
        {
            LogWarn("PresentFrame: 创建离屏图像快照失败");
        }
    }
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
    
    return wgpu::TextureFormat::BGRA8Unorm;
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

void GraphicsBackend::shutdown()
{
    m_activeRenderTarget = nullptr;

    offscreenSurface.reset();
    lastOffscreenImage.reset();

    graphiteRecorder.reset();
    graphiteContext.reset();

    if (m_msaaTexture)
    {
        m_msaaTexture.GetTexture().Destroy();
        m_msaaTexture = nullptr;
    }

    nutContext.reset();

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
    return backend;
}

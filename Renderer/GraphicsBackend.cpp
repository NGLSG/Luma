#include "GraphicsBackend.h"

#include "include/core/SkData.h"
#include "include/gpu/graphite/Image.h"

#include <future>

#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkSurface.h"
#include "../Utils/PCH.h"
#include "../Utils/stb_image.h"
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

wgpu::Device GraphicsBackend::CreateDevice(const GraphicsBackendOptions& options,
                                           const std::unique_ptr<dawn::native::Instance>& instance)
{
    wgpu::RequestAdapterOptions adapterOptions;

    static const std::vector<const char*> kToggles = {
        "allow_unsafe_apis",
        "use_user_defined_labels_in_backend",
        "disable_robustness",
        "use_tint_ir",
        "dawn.validation",
    };
    wgpu::DawnTogglesDescriptor togglesDesc;
    togglesDesc.enabledToggleCount = kToggles.size();
    togglesDesc.enabledToggles = kToggles.data();
    adapterOptions.nextInChain = &togglesDesc;
    adapterOptions.featureLevel = wgpu::FeatureLevel::Core;
    switch (options.qualityLevel)
    {
    case QualityLevel::Low:
        adapterOptions.powerPreference = wgpu::PowerPreference::LowPower;
        break;
    case QualityLevel::Medium:
    case QualityLevel::High:
    case QualityLevel::Ultra:
        adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
        break;
    }
    for (const auto& type : options.backendTypePriority)
    {
        switch (type)
        {
        case BackendType::D3D12: adapterOptions.backendType = wgpu::BackendType::D3D12;
            break;
        case BackendType::D3D11: adapterOptions.backendType = wgpu::BackendType::D3D11;
            break;
        case BackendType::Vulkan: adapterOptions.backendType = wgpu::BackendType::Vulkan;
            break;
        case BackendType::Metal: adapterOptions.backendType = wgpu::BackendType::Metal;
            break;
        case BackendType::OpenGL: adapterOptions.backendType = wgpu::BackendType::OpenGL;
            break;
        case BackendType::OpenGLES: adapterOptions.backendType = wgpu::BackendType::OpenGLES;
            break;
        default: continue;
        }

        std::vector<dawn::native::Adapter> adapters = instance->EnumerateAdapters(&adapterOptions);
        if (adapters.empty())
        {
            continue;
        }

        wgpu::Adapter adapter = adapters[0].Get();
        std::vector<wgpu::FeatureName> features;
        if (adapter.HasFeature(wgpu::FeatureName::MSAARenderToSingleSampled))
        {
            features.push_back(wgpu::FeatureName::MSAARenderToSingleSampled);
        }
        if (adapter.HasFeature(wgpu::FeatureName::TransientAttachments))
        {
            features.push_back(wgpu::FeatureName::TransientAttachments);
        }
        if (adapter.HasFeature(wgpu::FeatureName::Unorm16TextureFormats))
        {
            features.push_back(wgpu::FeatureName::Unorm16TextureFormats);
        }
        if (adapter.HasFeature(wgpu::FeatureName::DualSourceBlending))
        {
            features.push_back(wgpu::FeatureName::DualSourceBlending);
        }
        if (adapter.HasFeature(wgpu::FeatureName::FramebufferFetch))
        {
            features.push_back(wgpu::FeatureName::FramebufferFetch);
        }
        if (adapter.HasFeature(wgpu::FeatureName::BufferMapExtendedUsages))
        {
            features.push_back(wgpu::FeatureName::BufferMapExtendedUsages);
        }
        if (adapter.HasFeature(wgpu::FeatureName::TextureCompressionETC2))
        {
            features.push_back(wgpu::FeatureName::TextureCompressionETC2);
        }
        if (adapter.HasFeature(wgpu::FeatureName::TextureCompressionBC))
        {
            features.push_back(wgpu::FeatureName::TextureCompressionBC);
        }
        if (adapter.HasFeature(wgpu::FeatureName::R8UnormStorage))
        {
            features.push_back(wgpu::FeatureName::R8UnormStorage);
        }
        if (adapter.HasFeature(wgpu::FeatureName::DawnLoadResolveTexture))
        {
            features.push_back(wgpu::FeatureName::DawnLoadResolveTexture);
        }
        if (adapter.HasFeature(wgpu::FeatureName::DawnPartialLoadResolveTexture))
        {
            features.push_back(wgpu::FeatureName::DawnPartialLoadResolveTexture);
        }

        wgpu::DeviceDescriptor deviceDescriptor;
        deviceDescriptor.requiredFeatures = features.data();
        deviceDescriptor.requiredFeatureCount = features.size();
        deviceDescriptor.nextInChain = &togglesDesc;
        deviceDescriptor.SetDeviceLostCallback(
            wgpu::CallbackMode::AllowSpontaneous,
            [this](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView msg)
            {
                LogError("Dawn device lost: {} - {}", static_cast<int>(reason), msg.data);
                this->isDeviceLost = true;
            });
        return adapter.CreateDevice(&deviceDescriptor);
    }
    return nullptr;
}

wgpu::Texture GraphicsBackend::LoadTextureFromFile(const std::string& filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        LogError("Failed to load texture from file: {}", filename);
        return nullptr;
    }


    channels = 4;


    wgpu::TextureDescriptor textureDesc;
    textureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = wgpu::TextureDimension::e2D;


    wgpu::Texture texture = device.CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("Failed to create WGPU texture for file: {}", filename);
        stbi_image_free(data);
        return nullptr;
    }


    wgpu::TexelCopyTextureInfo destination;
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};
    destination.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferLayout dataLayout;
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = static_cast<uint32_t>(width * channels);
    dataLayout.rowsPerImage = static_cast<uint32_t>(height);

    wgpu::Extent3D writeSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};


    device.GetQueue().WriteTexture(&destination, data,
                                   static_cast<size_t>(width * height * channels),
                                   &dataLayout, &writeSize);


    stbi_image_free(data);
    return texture;
}

wgpu::Texture GraphicsBackend::LoadTextureFromData(const unsigned char* data, size_t size)
{
    int width, height, channels;
    unsigned char* imgData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels,
                                                   STBI_rgb_alpha);

    if (!imgData)
    {
        LogError("Failed to load texture from data.");
        return nullptr;
    }

    channels = 4;

    wgpu::TextureDescriptor textureDesc;
    textureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = wgpu::TextureDimension::e2D;

    wgpu::Texture texture = device.CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("Failed to create WGPU texture from data.");
        stbi_image_free(imgData);
        return nullptr;
    }

    wgpu::TexelCopyTextureInfo destination;
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};
    destination.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferLayout dataLayout;
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = static_cast<uint32_t>(width * channels);
    dataLayout.rowsPerImage = static_cast<uint32_t>(height);

    wgpu::Extent3D writeSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

    device.GetQueue().WriteTexture(&destination, imgData,
                                   static_cast<size_t>(width * height * channels),
                                   &dataLayout, &writeSize);

    stbi_image_free(imgData);
    return texture;
}

wgpu::Surface GraphicsBackend::CreateSurface(const GraphicsBackendOptions& options,
                                             const std::unique_ptr<dawn::native::Instance>& dawnInstance)
{
    wgpu::SurfaceDescriptor surfaceDesc;

#if defined(_WIN32)
    wgpu::SurfaceDescriptorFromWindowsHWND surfaceChainedDesc;
    surfaceChainedDesc.hwnd = options.windowHandle.hWnd;
    surfaceChainedDesc.hinstance = options.windowHandle.hInst;
    surfaceDesc.nextInChain = &surfaceChainedDesc;
#elif defined(__APPLE__)
    wgpu::SurfaceDescriptorFromMetalLayer surfaceChainedDesc;
    surfaceChainedDesc.layer = options.windowHandle.metalLayer;
    surfaceDesc.nextInChain = &surfaceChainedDesc;
#elif defined(__linux__) && !defined(__ANDROID__)
    wgpu::SurfaceDescriptorFromXlibWindow surfaceChainedDesc;
    surfaceChainedDesc.display = options.windowHandle.x11Display;
    surfaceChainedDesc.window = options.windowHandle.x11Window;
    surfaceDesc.nextInChain = &surfaceChainedDesc;
#else
#error "Unsupported platform for surface creation"
    throw std::runtime_error("Unsupported platform for surface creation");
    return nullptr;
#endif
    return wgpu::Instance(dawnInstance->Get()).CreateSurface(&surfaceDesc);
}

void GraphicsBackend::configureSurface(uint16_t width, uint16_t height)
{
    if (!surface) return;

    wgpu::SurfaceConfiguration config;
    config.device = device;
    config.format = surfaceFormat;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.width = width;
    config.height = height;
    if (enableVSync)
        config.presentMode = wgpu::PresentMode::Fifo;
    else
    {
        config.presentMode = wgpu::PresentMode::Mailbox;
    }

    surface.Configure(&config);

    this->currentWidth = width;
    this->currentHeight = height;
    if (m_msaaSampleCount > 1)
    {
        if (m_msaaTexture)
        {
            m_msaaTexture.Destroy();
        }
        wgpu::TextureDescriptor msaaDesc;
        msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
        msaaDesc.size = {(uint32_t)width, (uint32_t)height, 1};
        msaaDesc.format = surfaceFormat;
        msaaDesc.sampleCount = m_msaaSampleCount;
        m_msaaTexture = device.CreateTexture(&msaaDesc);
    }
    else
    {
        if (m_msaaTexture)
        {
            m_msaaTexture.Destroy();
            m_msaaTexture = nullptr;
        }
    }
}

void GraphicsBackend::updateQualitySettings()
{
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
}

void GraphicsBackend::initialize(const GraphicsBackendOptions& options)
{
    this->options = options;
    isDeviceLost = false;
    enableVSync = options.enableVSync;
    updateQualitySettings();
    dawnProcSetProcs(&dawn::native::GetProcs());
    std::vector<wgpu::InstanceFeatureName> instanceFeatures = {wgpu::InstanceFeatureName::TimedWaitAny};
    wgpu::InstanceDescriptor instanceDesc;
    instanceDesc.requiredFeatureCount = instanceFeatures.size();
    instanceDesc.requiredFeatures = instanceFeatures.data();
    dawnInstance = std::make_unique<dawn::native::Instance>(&instanceDesc);
    device = CreateDevice(options, dawnInstance);
    if (!device)
    {
        throw std::runtime_error("Failed to create Dawn device.");
    }

    skgpu::graphite::DawnBackendContext backendContext;
    backendContext.fInstance = wgpu::Instance(dawnInstance->Get());
    backendContext.fDevice = device;
    backendContext.fQueue = device.GetQueue();

    graphiteContext = skgpu::graphite::ContextFactory::MakeDawn(backendContext, {});
    if (!graphiteContext)
    {
        throw std::runtime_error("Failed to create Skia Graphite context.");
    }
    graphiteRecorder = graphiteContext->makeRecorder();

    this->currentWidth = options.width;
    this->currentHeight = options.height;

    if (options.windowHandle.IsValid())
    {
        surface = CreateSurface(options, dawnInstance);
        if (!surface)
        {
            throw std::runtime_error("Failed to create WGPU surface.");
        }
        configureSurface(options.width, options.height);
    }
}

void GraphicsBackend::ResolveMSAA()
{
    if (m_msaaSampleCount <= 1 || !surface || !m_msaaTexture || !m_currentSurfaceTexture.texture)
    {
        return;
    }

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    wgpu::RenderPassColorAttachment colorAttachment;
    colorAttachment.view = m_msaaTexture.CreateView();
    colorAttachment.loadOp = wgpu::LoadOp::Load;
    colorAttachment.storeOp = wgpu::StoreOp::Store;


    colorAttachment.resolveTarget = m_currentSurfaceTexture.texture.CreateView();

    wgpu::RenderPassDescriptor passDesc;
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &colorAttachment;


    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

std::shared_ptr<RenderTarget> GraphicsBackend::CreateOrGetRenderTarget(const std::string& name, uint16_t width,
                                                                       uint16_t height)
{
    auto it = m_renderTargets.find(name);
    if (it != m_renderTargets.end())
    {
        auto& target = it->second;

        if (target->GetWidth() != width || target->GetHeight() != height)
        {
            target->GetTexture().Destroy();
            m_renderTargets.erase(it);
        }
        else
        {
            return target;
        }
    }


    wgpu::TextureDescriptor textureDesc;
    textureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    textureDesc.format = GetSurfaceFormat();
    textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;

    wgpu::Texture texture = device.CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("Failed to create RenderTarget texture: {}", name);
        return nullptr;
    }

    auto newTarget = std::make_shared<RenderTarget>(texture, width, height);
    m_renderTargets[name] = newTarget;
    return newTarget;
}

void GraphicsBackend::SetActiveRenderTarget(std::shared_ptr<RenderTarget> target)
{
    m_activeRenderTarget = std::move(target);
}

sk_sp<SkImage> GraphicsBackend::CreateImageFromCompressedData(const unsigned char* data, size_t size,
                                                              int width, int height, wgpu::TextureFormat format)
{
    wgpu::TextureDescriptor textureDesc;
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.size = {(uint32_t)width, (uint32_t)height, 1};
    textureDesc.format = format;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

    wgpu::Texture texture = device.CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("Failed to create WGPU texture for compressed data.");
        return nullptr;
    }


    wgpu::TexelCopyTextureInfo destination;
    destination.texture = texture;
    wgpu::TexelCopyBufferLayout dataLayout;
    dataLayout.bytesPerRow = (width / 4) * 16;
    dataLayout.rowsPerImage = height / 4;
    wgpu::Extent3D writeSize = {(uint32_t)width, (uint32_t)height, 1};
    device.GetQueue().WriteTexture(&destination, data, size, &dataLayout, &writeSize);


    auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(texture.Get());
    if (!backendTex.isValid())
    {
        LogError("Failed to create valid BackendTexture from WGPU texture.");
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
        LogError("Failed to wrap backend texture for compressed data.");
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

        return SkSurfaces::WrapBackendTexture(graphiteRecorder.get(),
                                              backendTex,
                                              kBGRA_8888_SkColorType,
                                              nullptr,
                                              nullptr);
    }

    if (surface)
    {
        if (m_msaaSampleCount > 1 && m_msaaTexture)
        {
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_msaaTexture.Get());
            return SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                  kBGRA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
        }
        else
        {
            if (!m_currentSurfaceTexture.texture)
            {
                if (!BeginFrame()) return nullptr;
            }
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_currentSurfaceTexture.texture.Get());
            return SkSurfaces::WrapBackendTexture(graphiteRecorder.get(), backendTex,
                                                  kBGRA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
        }
    }
    else
    {
        if (!offscreenSurface || offscreenSurface->width() != currentWidth || offscreenSurface->height() !=
            currentHeight)
        {
            offscreenSurface = SkSurfaces::RenderTarget(graphiteRecorder.get(),
                                                        SkImageInfo::Make(
                                                            currentWidth, currentHeight, kRGBA_8888_SkColorType,
                                                            kPremul_SkAlphaType));
        }
        return offscreenSurface;
    }

    return nullptr;
}

sk_sp<SkImage> GraphicsBackend::CreateSpriteImageFromFile(const char* filePath) const
{
    sk_sp<SkData> encodedData = SkData::MakeFromFileName(filePath);
    return CreateSpriteImageFromData(encodedData);
}

sk_sp<SkImage> GraphicsBackend::CreateSpriteImageFromData(const sk_sp<SkData>& data) const
{
    if (!data) return nullptr;
    sk_sp<SkImage> cpuImage = SkImages::DeferredFromEncodedData(data);
    if (!cpuImage) return nullptr;
    if (GetRecorder())
        return SkImages::TextureFromImage(GetRecorder(), cpuImage.get());
    return nullptr;
}


void GraphicsBackend::Resize(uint16_t width, uint16_t height)
{
    if (width == currentWidth && height == currentHeight) return;

    if (surface)
    {
        configureSurface(width, height);
    }
    else
    {
        this->currentWidth = width;
        this->currentHeight = height;
        this->offscreenSurface.reset();
    }
}


bool GraphicsBackend::Recreate()
{
    LogInfo("Attempting to recreate graphics backend...");

    this->shutdown();

    const int maxRetries = 3;
    const int delayMilliseconds = 500;

    for (int i = 0; i < maxRetries; ++i)
    {
        try
        {
            if (i > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds * (i + 1)));
            }

            LogInfo("Recreation attempt # {}", i + 1);
            this->initialize(options);
            LogInfo("Graphics backend recreated successfully on attempt # {}", i + 1);
            return true;
        }
        catch (const std::exception& e)
        {
            LogError("Recreation attempt #{} failed: {}", i + 1, e.what());
            if (i == maxRetries - 1)
            {
                LogError("Failed to recreate graphics backend after multiple attempts.");
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
    enableVSync = enable;
    configureSurface(currentWidth, currentHeight);
}

void GraphicsBackend::SetQualityLevel(QualityLevel level)
{
    options.qualityLevel = level;

    if (!Recreate())
    {
        LogError("Failed to recreate graphics backend with new quality settings.");
    }
}

bool GraphicsBackend::BeginFrame()
{
    if (isDeviceLost)
    {
        if (!Recreate())
        {
            return false;
        }
    }

    if (!surface) return false;


    if (m_currentSurfaceTexture.texture)
    {
        m_currentSurfaceTexture.texture.Destroy();
    }
    m_currentSurfaceTexture = {};


    surface.GetCurrentTexture(&m_currentSurfaceTexture);


    if (m_currentSurfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal)
    {
        m_currentSurfaceTexture = {};
        if (m_currentSurfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Lost)
        {
            configureSurface(this->currentWidth, this->currentHeight);
        }
        return false;
    }
    return true;
}

wgpu::TextureView GraphicsBackend::GetCurrentFrameView() const
{
    if (!m_currentSurfaceTexture.texture) return nullptr;
    return m_currentSurfaceTexture.texture.CreateView();
}

void GraphicsBackend::Submit()
{
    if (!graphiteRecorder || !graphiteContext) return;


    lastOffscreenImage.reset();

    std::unique_ptr<skgpu::graphite::Recording> recording = graphiteRecorder->snap();
    if (recording)
    {
        skgpu::graphite::InsertRecordingInfo info = {};
        info.fRecording = recording.get();
        graphiteContext->insertRecording(info);
        graphiteContext->submit();
    }
}

void GraphicsBackend::PresentFrame()
{
    if (surface)
    {
        surface.Present();
    }
    else if (offscreenSurface)
    {
        lastOffscreenImage = offscreenSurface->makeImageSnapshot();
    }


    if (m_currentSurfaceTexture.texture)
    {
        m_currentSurfaceTexture.texture.Destroy();
    }
    m_currentSurfaceTexture = {};

    device.Tick();
}

skgpu::graphite::Context* GraphicsBackend::GetGraphiteContext() const
{
    return graphiteContext.get();
}

skgpu::graphite::Recorder* GraphicsBackend::GetRecorder() const
{
    return graphiteRecorder.get();
}


sk_sp<SkImage> GraphicsBackend::DetachLastOffscreenImage()
{
    return std::move(lastOffscreenImage);
}


sk_sp<SkImage> GraphicsBackend::GPUToCPUImage(sk_sp<SkImage> src) const
{
    if (!src)
    {
        return nullptr;
    }

    if (!src->isTextureBacked())
    {
        return src;
    }


    sk_sp<SkSurface> cpuSurface = SkSurfaces::Raster(src->imageInfo());
    if (!cpuSurface)
    {
        return nullptr;
    }


    cpuSurface->getCanvas()->drawImage(src, 0.0f, 0.0f);


    return cpuSurface->makeImageSnapshot();
}

void GraphicsBackend::shutdown()
{
    m_renderTargets.clear();
    m_activeRenderTarget = nullptr;


    offscreenSurface.reset();
    lastOffscreenImage.reset();
    graphiteRecorder.reset();
    graphiteContext.reset();


    if (m_msaaTexture)
    {
        m_msaaTexture.Destroy();
        m_msaaTexture = nullptr;
    }


    if (m_currentSurfaceTexture.texture)
    {
        m_currentSurfaceTexture.texture.Destroy();
    }
    m_currentSurfaceTexture = {};


    if (surface)
    {
        surface.Unconfigure();
        surface = nullptr;
    }


    if (device)
    {
        device.Tick();
        device = nullptr;
    }


    dawnInstance.reset();

    isDeviceLost = false;
    LogInfo("GraphicsBackend resources have been successfully released.");
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
        LogError("Failed to initialize GraphicsBackend: {}", e.what());
        return nullptr;
    }
    return backend;
}

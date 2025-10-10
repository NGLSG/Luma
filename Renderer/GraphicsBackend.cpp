
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
    if (!instance)
    {
        LogError("CreateDevice 失败: Instance 为空指针");
        return nullptr;
    }

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
        const char* backendName = "未知";
        switch (type)
        {
        case BackendType::D3D12:
            adapterOptions.backendType = wgpu::BackendType::D3D12;
            backendName = "D3D12";
            break;
        case BackendType::D3D11:
            adapterOptions.backendType = wgpu::BackendType::D3D11;
            backendName = "D3D11";
            break;
        case BackendType::Vulkan:
            adapterOptions.backendType = wgpu::BackendType::Vulkan;
            backendName = "Vulkan";
            break;
        case BackendType::Metal:
            adapterOptions.backendType = wgpu::BackendType::Metal;
            backendName = "Metal";
            break;
        case BackendType::OpenGL:
            adapterOptions.backendType = wgpu::BackendType::OpenGL;
            backendName = "OpenGL";
            break;
        case BackendType::OpenGLES:
            adapterOptions.backendType = wgpu::BackendType::OpenGLES;
            backendName = "OpenGLES";
            break;
        default:
            LogWarn("跳过不支持的后端类型");
            continue;
        }

        std::vector<dawn::native::Adapter> adapters = instance->EnumerateAdapters(&adapterOptions);
        if (adapters.empty())
        {
            LogWarn("未找到 {} 后端适配器", backendName);
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
                LogError("Dawn 设备丢失 - 原因代码: {}, 消息: {}", static_cast<int>(reason), msg.data ? msg.data : "无消息");
                this->isDeviceLost = true;
            });

        wgpu::Device createdDevice = adapter.CreateDevice(&deviceDescriptor);
        if (createdDevice)
        {
            LogInfo("成功创建设备: {}", backendName);
            return createdDevice;
        }
        else
        {
            LogError("创建 {} 设备失败", backendName);
        }
    }

    LogError("所有后端类型都无法创建设备");
    return nullptr;
}

wgpu::Texture GraphicsBackend::LoadTextureFromFile(const std::string& filename)
{
    if (filename.empty())
    {
        LogError("LoadTextureFromFile 失败: 文件名为空");
        return nullptr;
    }

    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        LogError("从文件加载纹理失败: {} - stbi 错误: {}", filename, stbi_failure_reason());
        return nullptr;
    }

    if (width <= 0 || height <= 0)
    {
        LogError("从文件加载纹理失败: {} - 无效的图像尺寸 ({}x{})", filename, width, height);
        stbi_image_free(data);
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
        LogError("为文件创建 WGPU 纹理失败: {}", filename);
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
    if (!data || size == 0)
    {
        LogError("LoadTextureFromData 失败: 数据指针为空或大小为 0");
        return nullptr;
    }

    int width, height, channels;
    unsigned char* imgData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels,
                                                   STBI_rgb_alpha);

    if (!imgData)
    {
        LogError("从内存加载纹理失败 - stbi 错误: {}", stbi_failure_reason());
        return nullptr;
    }

    if (width <= 0 || height <= 0)
    {
        LogError("从内存加载纹理失败 - 无效的图像尺寸 ({}x{})", width, height);
        stbi_image_free(imgData);
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
        LogError("从内存数据创建 WGPU 纹理失败");
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
    if (!dawnInstance)
    {
        LogError("CreateSurface 失败: Dawn instance 为空");
        return nullptr;
    }

    if (!options.windowHandle.IsValid())
    {
        LogError("CreateSurface 失败: 窗口句柄无效");
        return nullptr;
    }

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
    LogError("不支持的平台: 无法创建表面");
    throw std::runtime_error("不支持的平台: 无法创建表面");
    return nullptr;
#endif

    wgpu::Surface surface = wgpu::Instance(dawnInstance->Get()).CreateSurface(&surfaceDesc);
    if (!surface)
    {
        LogError("创建 WGPU 表面失败");
        return nullptr;
    }

    LogInfo("成功创建 WGPU 表面");
    return surface;
}

void GraphicsBackend::configureSurface(uint16_t width, uint16_t height)
{
    if (!surface)
    {
        LogWarn("configureSurface: 表面为空,跳过配置");
        return;
    }

    if (width == 0 || height == 0)
    {
        LogError("configureSurface 失败: 无效的尺寸 ({}x{})", width, height);
        return;
    }

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

        if (!m_msaaTexture)
        {
            LogError("创建 MSAA 纹理失败 (样本数: {})", m_msaaSampleCount);
        }
        else
        {
            LogInfo("创建 MSAA 纹理 (样本数: {})", m_msaaSampleCount);
        }
    }
    else
    {
        if (m_msaaTexture)
        {
            m_msaaTexture.Destroy();
            m_msaaTexture = nullptr;
            LogInfo("已销毁 MSAA 纹理");
        }
    }
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

void GraphicsBackend::initialize(const GraphicsBackendOptions& options)
{
    this->options = options;
    isDeviceLost = false;
    enableVSync = options.enableVSync;
    updateQualitySettings();

    try
    {
        dawnProcSetProcs(&dawn::native::GetProcs());
        std::vector<wgpu::InstanceFeatureName> instanceFeatures = {wgpu::InstanceFeatureName::TimedWaitAny};
        wgpu::InstanceDescriptor instanceDesc;
        instanceDesc.requiredFeatureCount = instanceFeatures.size();
        instanceDesc.requiredFeatures = instanceFeatures.data();
        dawnInstance = std::make_unique<dawn::native::Instance>(&instanceDesc);

        if (!dawnInstance)
        {
            LogError("创建 Dawn instance 失败");
            throw std::runtime_error("创建 Dawn instance 失败");
        }

        device = CreateDevice(options, dawnInstance);
        if (!device)
        {
            LogError("创建 Dawn 设备失败");
            throw std::runtime_error("创建 Dawn 设备失败");
        }

        skgpu::graphite::DawnBackendContext backendContext;
        backendContext.fInstance = wgpu::Instance(dawnInstance->Get());
        backendContext.fDevice = device;
        backendContext.fQueue = device.GetQueue();

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

        if (options.windowHandle.IsValid())
        {
            surface = CreateSurface(options, dawnInstance);
            if (!surface)
            {
                LogError("创建 WGPU 表面失败");
                throw std::runtime_error("创建 WGPU 表面失败");
            }
            configureSurface(options.width, options.height);
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
    if (m_msaaSampleCount <= 1 || !surface || !m_msaaTexture || !m_currentSurfaceTexture.texture)
    {
        return;
    }

    try
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        if (!encoder)
        {
            LogError("ResolveMSAA: 创建命令编码器失败");
            return;
        }

        wgpu::RenderPassColorAttachment colorAttachment;
        colorAttachment.view = m_msaaTexture.CreateView();
        colorAttachment.loadOp = wgpu::LoadOp::Load;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.resolveTarget = m_currentSurfaceTexture.texture.CreateView();

        wgpu::RenderPassDescriptor passDesc;
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
        if (!pass)
        {
            LogError("ResolveMSAA: 开始渲染通道失败");
            return;
        }
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        if (!commands)
        {
            LogError("ResolveMSAA: 完成命令缓冲区失败");
            return;
        }

        device.GetQueue().Submit(1, &commands);
    }
    catch (const std::exception& e)
    {
        LogError("ResolveMSAA 过程中发生异常: {}", e.what());
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
        LogError("创建渲染目标纹理失败: {}", name);
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

    wgpu::TextureDescriptor textureDesc;
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.size = {(uint32_t)width, (uint32_t)height, 1};
    textureDesc.format = format;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

    wgpu::Texture texture = device.CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("为压缩数据创建 WGPU 纹理失败");
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
                                              kBGRA_8888_SkColorType,
                                              nullptr,
                                              nullptr);
        if (!surface)
        {
            LogError("GetSurface: 包装活动渲染目标失败");
        }
        return surface;
    }

    if (surface)
    {
        if (m_msaaSampleCount > 1 && m_msaaTexture)
        {
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_msaaTexture.Get());
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
            if (!m_currentSurfaceTexture.texture)
            {
                if (!BeginFrame())
                {
                    LogError("GetSurface: BeginFrame 失败");
                    return nullptr;
                }
            }
            auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(m_currentSurfaceTexture.texture.Get());
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
            offscreenSurface = SkSurfaces::RenderTarget(graphiteRecorder.get(),
                                                        SkImageInfo::Make(
                                                            currentWidth, currentHeight, kRGBA_8888_SkColorType,
                                                            kPremul_SkAlphaType));
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
    configureSurface(currentWidth, currentHeight);
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
    if (isDeviceLost)
    {
        if (!Recreate())
        {
            LogError("BeginFrame: 重建失败");
            return false;
        }
    }

    if (!surface)
    {
        LogWarn("BeginFrame: 表面为空");
        return false;
    }

    if (m_currentSurfaceTexture.texture)
    {
        m_currentSurfaceTexture.texture.Destroy();
    }
    m_currentSurfaceTexture = {};

    surface.GetCurrentTexture(&m_currentSurfaceTexture);

    if (m_currentSurfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal)
    {
        LogWarn("BeginFrame: 获取表面纹理失败,状态码: {}", static_cast<int>(m_currentSurfaceTexture.status));
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
    if (!m_currentSurfaceTexture.texture)
    {
        LogWarn("GetCurrentFrameView: 当前表面纹理为空");
        return nullptr;
    }
    return m_currentSurfaceTexture.texture.CreateView();
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
    if (surface)
    {
        surface.Present();
    }
    else if (offscreenSurface)
    {
        lastOffscreenImage = offscreenSurface->makeImageSnapshot();
        if (!lastOffscreenImage)
        {
            LogWarn("PresentFrame: 创建离屏图像快照失败");
        }
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
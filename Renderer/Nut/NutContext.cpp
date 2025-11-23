#include "NutContext.h"

#include <format>
#include <future>
#include <iostream>

#include "stb_image.h"
#include "stb_image_write.h"
#include "dawn/dawn_proc.h"

namespace Nut
{
    NutContext::NutContext() : m_size(0)
    {
    }

    bool NutContext::createInstance()
    {
        wgpu::InstanceDescriptor instanceDescriptor;
        std::vector<wgpu::InstanceFeatureName> features = {wgpu::InstanceFeatureName::TimedWaitAny};
        instanceDescriptor.requiredFeatureCount = features.size();
        instanceDescriptor.requiredFeatures = features.data();
        m_instance = std::make_unique<dawn::native::Instance>(&instanceDescriptor);
        return m_instance != nullptr;
    }

    static void onDeviceLog(const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message)
    {
        std::cerr << "=================================================" << std::endl;
        std::cerr << "WGPU Device Error (C++ Callback): " << std::endl;
        std::cerr << "  Type: ";
        switch (type)
        {
        case wgpu::ErrorType::Validation: std::cerr << "Validation";
            break;
        case wgpu::ErrorType::OutOfMemory: std::cerr << "OutOfMemory";
            break;
        case wgpu::ErrorType::Internal: std::cerr << "Internal";
            break;
        case wgpu::ErrorType::Unknown: std::cerr << "Unknown";
            break;
        default: std::cerr << "Unhandled Error";
            break;
        }
        std::cerr << std::endl;
        std::cerr << "  Message: " << message.data << std::endl;

        std::cerr << "=================================================" << std::endl;
    };

    bool NutContext::createDevice()
    {
        wgpu::RequestAdapterOptions adapterOptions;
        static const std::vector<const char*> kToggles = {
            "allow_unsafe_apis",
            "use_user_defined_labels_in_backend",
            "disable_robustness",
            "use_tint_ir",
            "dawn.validation",
            "enable_dawn_backend_validation",
            "enable_dawn_logging",
            "dump_shaders",

        };
        wgpu::DawnTogglesDescriptor togglesDesc;
        togglesDesc.enabledToggleCount = kToggles.size();
        togglesDesc.enabledToggles = kToggles.data();

        switch (m_descriptor.qualityLevel)
        {
        case QualityLevel::Low:
            adapterOptions.powerPreference = wgpu::PowerPreference::LowPower;
            break;
        case QualityLevel::Medium:
            adapterOptions.powerPreference = wgpu::PowerPreference::Undefined;
            break;
        case QualityLevel::High:
            adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
            break;
        default:
            throw std::runtime_error("Invalid quality level");
        }

        for (const auto& backend : m_descriptor.backendTypePriority)
        {
            switch (backend)
            {
            case BackendType::OpenGL:
                adapterOptions.backendType = wgpu::BackendType::OpenGL;
                break;
            case BackendType::Vulkan:
                adapterOptions.backendType = wgpu::BackendType::Vulkan;
                break;
            case BackendType::Metal:
                adapterOptions.backendType = wgpu::BackendType::Metal;
                break;
            case BackendType::OpenGLES:
                adapterOptions.backendType = wgpu::BackendType::OpenGLES;
                break;
            case BackendType::D3D12:
                adapterOptions.backendType = wgpu::BackendType::D3D12;
                break;
            case BackendType::D3D11:
                adapterOptions.backendType = wgpu::BackendType::D3D11;
                break;
            default:
                adapterOptions.backendType = wgpu::BackendType::Null;
                break;
            }
            adapterOptions.nextInChain = &togglesDesc;
            adapterOptions.featureLevel = wgpu::FeatureLevel::Core;
            auto adapters = m_instance->EnumerateAdapters(&adapterOptions);
            wgpu::Adapter adapter = adapters[0].Get();


            std::vector<wgpu::FeatureName> features;
            constexpr wgpu::FeatureName allPossibleFeatures[] = {
                wgpu::FeatureName::DepthClipControl,
                wgpu::FeatureName::Depth32FloatStencil8,
                wgpu::FeatureName::TextureCompressionBC,
                wgpu::FeatureName::TextureCompressionBCSliced3D,
                wgpu::FeatureName::TextureCompressionETC2,
                wgpu::FeatureName::TextureCompressionASTC,
                wgpu::FeatureName::TextureCompressionASTCSliced3D,
                wgpu::FeatureName::TimestampQuery,
                wgpu::FeatureName::IndirectFirstInstance,
                wgpu::FeatureName::ShaderF16,
                wgpu::FeatureName::RG11B10UfloatRenderable,
                wgpu::FeatureName::BGRA8UnormStorage,
                wgpu::FeatureName::Float32Filterable,
                wgpu::FeatureName::Float32Blendable,
                wgpu::FeatureName::ClipDistances,
                wgpu::FeatureName::DualSourceBlending,
                wgpu::FeatureName::Subgroups,
                wgpu::FeatureName::TextureFormatsTier1,
                wgpu::FeatureName::TextureFormatsTier2,
                wgpu::FeatureName::DawnInternalUsages,
                wgpu::FeatureName::DawnMultiPlanarFormats,
                wgpu::FeatureName::DawnNative,
                wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses,
                wgpu::FeatureName::ImplicitDeviceSynchronization,
                wgpu::FeatureName::TransientAttachments,
                wgpu::FeatureName::MSAARenderToSingleSampled,
                wgpu::FeatureName::D3D11MultithreadProtected,
                wgpu::FeatureName::ANGLETextureSharing,
                wgpu::FeatureName::PixelLocalStorageCoherent,
                wgpu::FeatureName::PixelLocalStorageNonCoherent,
                wgpu::FeatureName::Unorm16TextureFormats,
                wgpu::FeatureName::Snorm16TextureFormats,
                wgpu::FeatureName::MultiPlanarFormatExtendedUsages,
                wgpu::FeatureName::MultiPlanarFormatP010,
                wgpu::FeatureName::HostMappedPointer,
                wgpu::FeatureName::MultiPlanarRenderTargets,
                wgpu::FeatureName::MultiPlanarFormatNv12a,
                wgpu::FeatureName::FramebufferFetch,
                wgpu::FeatureName::BufferMapExtendedUsages,
                wgpu::FeatureName::AdapterPropertiesMemoryHeaps,
                wgpu::FeatureName::AdapterPropertiesD3D,
                wgpu::FeatureName::AdapterPropertiesVk,
                wgpu::FeatureName::R8UnormStorage,
                wgpu::FeatureName::DawnFormatCapabilities,
                wgpu::FeatureName::DawnDrmFormatCapabilities,
                wgpu::FeatureName::Norm16TextureFormats,
                wgpu::FeatureName::MultiPlanarFormatNv16,
                wgpu::FeatureName::MultiPlanarFormatNv24,
                wgpu::FeatureName::MultiPlanarFormatP210,
                wgpu::FeatureName::MultiPlanarFormatP410,
                wgpu::FeatureName::SharedTextureMemoryVkDedicatedAllocation,
                wgpu::FeatureName::SharedTextureMemoryAHardwareBuffer,
                wgpu::FeatureName::SharedTextureMemoryDmaBuf,
                wgpu::FeatureName::SharedTextureMemoryOpaqueFD,
                wgpu::FeatureName::SharedTextureMemoryZirconHandle,
                wgpu::FeatureName::SharedTextureMemoryDXGISharedHandle,
                wgpu::FeatureName::SharedTextureMemoryD3D11Texture2D,
                wgpu::FeatureName::SharedTextureMemoryIOSurface,
                wgpu::FeatureName::SharedTextureMemoryEGLImage,
                wgpu::FeatureName::SharedFenceVkSemaphoreOpaqueFD,
                wgpu::FeatureName::SharedFenceSyncFD,
                wgpu::FeatureName::SharedFenceVkSemaphoreZirconHandle,
                wgpu::FeatureName::SharedFenceDXGISharedHandle,
                wgpu::FeatureName::SharedFenceMTLSharedEvent,
                wgpu::FeatureName::SharedBufferMemoryD3D12Resource,
                wgpu::FeatureName::StaticSamplers,
                wgpu::FeatureName::YCbCrVulkanSamplers,
                wgpu::FeatureName::ShaderModuleCompilationOptions,
                wgpu::FeatureName::DawnLoadResolveTexture,
                wgpu::FeatureName::DawnPartialLoadResolveTexture,
                wgpu::FeatureName::MultiDrawIndirect,
                wgpu::FeatureName::DawnTexelCopyBufferRowAlignment,
                wgpu::FeatureName::FlexibleTextureViews,
                wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix,
                wgpu::FeatureName::SharedFenceEGLSync,
                wgpu::FeatureName::DawnDeviceAllocatorControl,
                wgpu::FeatureName::TextureComponentSwizzle,
                wgpu::FeatureName::ChromiumExperimentalPrimitiveId,
            };

            features.clear();

            for (const auto feature : allPossibleFeatures)
            {
                if (adapter.HasFeature(feature))
                {
                    features.push_back(feature);
                }
            }
            wgpu::DeviceDescriptor deviceDescriptor;
            deviceDescriptor.requiredFeatures = features.data();
            deviceDescriptor.requiredFeatureCount = features.size();
            deviceDescriptor.nextInChain = &togglesDesc;
            deviceDescriptor.SetDeviceLostCallback(
                wgpu::CallbackMode::AllowSpontaneous,
                [this](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView msg)
                {
                    if (reason == wgpu::DeviceLostReason::Destroyed)
                    {
                        return;
                    }
                    std::cerr << std::format("Dawn 设备丢失 - 原因代码: {}, 消息: {}", static_cast<int>(reason),
                                             msg.data ? msg.data : "无消息") << std::endl;
                    m_isDeviceLost = true;
                });
            deviceDescriptor.SetUncapturedErrorCallback(onDeviceLog);
            m_device = adapter.CreateDevice(&deviceDescriptor);
            m_device.SetLoggingCallback([](wgpu::LoggingType t, wgpu::StringView msg)
            {
                switch (t)
                {
                case wgpu::LoggingType::Info:

                    break;
                case wgpu::LoggingType::Warning:
                    std::cout << "[WGPU Warning] " << msg.data << std::endl;
                    break;
                case wgpu::LoggingType::Error:
                    std::cerr << "[WGPU Error] " << msg.data << std::endl;
                    break;
                case wgpu::LoggingType::Verbose:
                    std::cout << "[WGPU Verbose] " << msg.data << std::endl;
                    break;
                }
            });
            if (m_device)
            {
                return true;
            }
        }
        return false;
    }

    bool NutContext::createSurface()
    {
        if (!m_descriptor.windowHandle.IsValid())
        {
            return false;
        }
        wgpu::SurfaceDescriptor surfaceDescriptor;
#if defined(_WIN32)
        wgpu::SurfaceDescriptorFromWindowsHWND platDesc;
        platDesc.hwnd = m_descriptor.windowHandle.hWnd;
        platDesc.hinstance = m_descriptor.windowHandle.hInst;

#elif defined(__linux__) && !defined(__ANDROID__)
        wgpu::SurfaceDescriptorFromXlibWindow platDesc;
        platDesc.display = m_descriptor.windowHandle.x11Display;
        platDesc.window = m_descriptor.windowHandle.x11Window;
#elif defined(__ANDROID__)
        wgpu::SurfaceDescriptorFromAndroidNativeWindow platDesc;
        platDesc.window = m_descriptor.windowHandle.aNativeWindow;
#elif defined(__APPLE__)
        wgpu::SurfaceDescriptorFromMetalLayer platDesc;
        platDesc.layer = m_descriptor.windowHandle.metalLayer;
#else
        return false;
#endif
        surfaceDescriptor.nextInChain = reinterpret_cast<wgpu::ChainedStruct const*>(&platDesc);
        m_surface = wgpu::Instance(m_instance->Get()).CreateSurface(&surfaceDescriptor);
        return m_surface != nullptr;
    }

    void NutContext::configureSurface(uint32_t width, uint32_t height)
    {
        if (!m_surface)
        {
            std::cerr << "Surface is null" << std::endl;
            return;
        }
        if (width == 0 || height == 0)
        {
            std::cerr << "Invalid size" << std::endl;
            return;
        }
        wgpu::SurfaceConfiguration surfaceConfig;
        surfaceConfig.width = width;
        surfaceConfig.height = height;
        surfaceConfig.device = m_device;
        if (m_descriptor.enableVSync)
        {
            surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
        }
        else
        {
            surfaceConfig.presentMode = wgpu::PresentMode::Mailbox;
        }
        surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
        surfaceConfig.format = m_graphicsFormat;
        m_size.width = width;
        m_size.height = height;
        m_surface.Configure(&surfaceConfig);
    }

    std::shared_ptr<NutContext> NutContext::Create(const NutContextDescriptor& descriptor)
    {
        std::shared_ptr<NutContext> result = std::make_shared<NutContext>();
        result->Initialize(descriptor);
        return result;
    }

    GraphicsContextCreateStatus NutContext::Initialize(const NutContextDescriptor& desc)
    {
        static bool Initialized = false;
        if (Initialized)
        {
            return GraphicsContextCreateStatus::ERROR_ALREADY_CREATED;
        }
        dawnProcSetProcs(&dawn::native::GetProcs());
        m_descriptor = desc;
        if (!createInstance())
        {
            return GraphicsContextCreateStatus::ERROR_INSTANCE_CREATION;
        }
        if (!createDevice())
        {
            return GraphicsContextCreateStatus::ERROR_DEVICE_CREATION;
        }
        if (!createSurface())
        {
            return GraphicsContextCreateStatus::ERROR_SURFACE_CREATION;
        }
        Initialized = true;
        configureSurface(desc.width, desc.height);
        return SUCCESS;
    }

    std::shared_ptr<RenderTarget> NutContext::CreateOrGetRenderTarget(const std::string& name, uint16_t width,
                                                                      uint16_t height)
    {
        if (name.empty())
        {
            std::cerr << "Invalid name" << std::endl;
            return nullptr;
        }

        if (width == 0 || height == 0)
        {
            std::cerr << "Invalid size" << std::endl;
            return nullptr;
        }

        auto it = m_renderTargets.find(name);
        if (it != m_renderTargets.end())
        {
            auto& target = it->second;
            if (target->GetHeight() != height || target->GetWidth() != width)
            {
                target->GetTexture().Destroy();
                m_renderTargets.erase(name);
            }
            else
            {
                return target;
            }
        }

        wgpu::TextureDescriptor texDesc;
        texDesc.format = m_graphicsFormat;
        texDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        texDesc.usage =
            wgpu::TextureUsage::RenderAttachment
            | wgpu::TextureUsage::TextureBinding
            | wgpu::TextureUsage::CopySrc
            | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;
        wgpu::Texture tex = m_device.CreateTexture(&texDesc);
        if (!tex)
        {
            std::cerr << "Failed to create texture" << std::endl;
            return nullptr;
        }
        auto newTarget = std::make_shared<RenderTarget>(tex, width, height);
        m_renderTargets[name] = newTarget;
        return newTarget;
    }

    void NutContext::SetActiveRenderTarget(std::shared_ptr<RenderTarget> target)
    {
        m_currentRenderTarget = std::move(target);
    }

    void NutContext::Resize(uint32_t width, uint32_t height)
    {
        configureSurface(width, height);
    }

    void NutContext::SetVSync(bool enable)
    {
        m_descriptor.enableVSync = enable;
    }

    wgpu::Device& NutContext::GetWGPUDevice()
    {
        return m_device;
    }

    wgpu::Surface& NutContext::GetWGPUSurface()
    {
        return m_surface;
    }

    wgpu::Instance NutContext::GetWGPUInstance() const
    {
        return m_instance->Get();
    }

    Size NutContext::GetCurrentSwapChainSize() const
    {
        return m_size;
    }

    wgpu::CommandBuffer NutContext::EndRenderFrame(RenderPass& renderPass)
    {
        return renderPass.End();
    }

    void NutContext::Submit(const std::vector<wgpu::CommandBuffer>& cmds)

    {
        m_device.GetQueue().Submit(cmds.size(), cmds.data());
    }

    RenderPassBuilder NutContext::BeginRenderFrame()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_commandEncoders.emplace_back(m_device.CreateCommandEncoder());

        return RenderPassBuilder{m_commandEncoders.back()};
    }


    void NutContext::Present()
    {
        if (m_currentRenderTarget)
        {
            m_device.Tick();
            m_commandEncoders.clear();
            return;
        }

        if (!m_currentSurfaceTexture.texture)
        {
            m_device.Tick();
            m_commandEncoders.clear();
            return;
        }

        if (wgpu::Status::Success != m_surface.Present())
        {
            std::cerr << "Failed to present the surface." << std::endl;
        }

        if (m_currentSurfaceTexture.texture)
        {
            m_currentSurfaceTexture.texture = nullptr;
        }
        m_device.Tick();
        m_commandEncoders.clear();
    }

    ComputePassBuilder NutContext::BeginComputeFrame()
    {
        std::lock_guard<std::mutex> lock(m_cmutex);
        m_computeCommandEncoders.emplace_back(m_device.CreateCommandEncoder());

        return ComputePassBuilder{m_computeCommandEncoders.back()};
    }

    wgpu::CommandBuffer NutContext::EndComputeFrame(ComputePass& computePass)
    {
        return computePass.End();
    }

    void NutContext::ClearCommands()
    {
        for (auto& enc : m_computeCommandEncoders)
        {
            enc = nullptr;
        }
        m_computeCommandEncoders.clear();
        for (auto& enc : m_commandEncoders)
        {
            enc = nullptr;
        }
        m_commandEncoders.clear();
    }

    TextureAPtr NutContext::GetCurrentTexture()
    {
        
        if (m_currentRenderTarget)
        {
            return std::make_shared<TextureA>(m_currentRenderTarget->GetTexture(), shared_from_this());
        }

        
        if (m_currentSurfaceTexture.texture)
        {
            return std::make_shared<TextureA>(m_currentSurfaceTexture.texture, shared_from_this());
        }

        
        wgpu::SurfaceTexture surfaceTexture;
        m_surface.GetCurrentTexture(&surfaceTexture);

        if (surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal ||
            surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal)
        {
            m_currentSurfaceTexture = std::move(surfaceTexture);
            return std::make_shared<TextureA>(m_currentSurfaceTexture.texture, shared_from_this());
        }

        
        std::cerr << "获取 Surface Texture 失败，状态码: " << static_cast<int>(surfaceTexture.status) << std::endl;
        return nullptr;
    }

    wgpu::Sampler NutContext::CreateSampler(wgpu::SamplerDescriptor const* desc)
    {
        return m_device.CreateSampler(desc);
    }

    TextureAPtr NutContext::LoadTextureFromFile(const std::string& file)
    {
        return TextureBuilder()
               .LoadFromFile(file)
               .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
               .SetUsage(TextureUsageFlags::GetCommonTextureUsage())
               .Build(shared_from_this());
    }

    TextureAPtr NutContext::CreateTextureFromMemory(const void* data, size_t size)
    {
        return TextureBuilder()
               .LoadFromMemory(data, size)
               .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
               .SetUsage(TextureUsageFlags::GetCommonTextureUsage())
               .Build(shared_from_this());
    }

    TextureAPtr NutContext::CreateTexture(const TextureDescriptor& descriptor)
    {
        const auto& desc = descriptor.GetDescriptor();
        auto texture = m_device.CreateTexture(&desc);

        if (!texture)
        {
            std::cerr << "创建纹理失败" << std::endl;
            return nullptr;
        }

        return std::make_shared<TextureA>(texture, shared_from_this());
    }

    TextureAPtr NutContext::CreateTextureFromCompressedData(const unsigned char* data, size_t size, uint32_t width,
                                                            uint32_t height, wgpu::TextureFormat format,
                                                            uint32_t bytesPerRow, uint32_t rowsPerImage)
    {
        if (!data || size == 0 || width == 0 || height == 0)
        {
            std::cerr << "CreateTextureFromCompressedData: invalid input data/size." << std::endl;
            return nullptr;
        }

        wgpu::TextureDescriptor textureDesc{};
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = {width, height, 1};
        textureDesc.format = format;
        textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

        wgpu::Texture texture = m_device.CreateTexture(&textureDesc);
        if (!texture)
        {
            std::cerr << "CreateTextureFromCompressedData: failed to create texture." << std::endl;
            return nullptr;
        }

        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = texture;

        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = bytesPerRow != 0 ? bytesPerRow : (width / 4) * 16;
        dataLayout.rowsPerImage = rowsPerImage != 0 ? rowsPerImage : height / 4;
        wgpu::Extent3D writeSize = {width, height, 1};
        m_device.GetQueue().WriteTexture(&destination, data, size, &dataLayout, &writeSize);

        return std::make_shared<TextureA>(texture, shared_from_this());
    }

    bool NutContext::ResolveTexture(const TextureAPtr& source, const TextureAPtr& resolveTarget)
    {
        if (!source || !resolveTarget)
        {
            std::cerr << "ResolveTexture: source or destination texture is invalid." << std::endl;
            return false;
        }

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();
        if (!encoder)
        {
            std::cerr << "ResolveTexture: failed to create command encoder." << std::endl;
            return false;
        }

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = source->GetTexture().CreateView();
        colorAttachment.resolveTarget = resolveTarget->GetTexture().CreateView();
        colorAttachment.loadOp = wgpu::LoadOp::Load;
        colorAttachment.storeOp = wgpu::StoreOp::Store;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
        if (!pass)
        {
            std::cerr << "ResolveTexture: failed to begin render pass." << std::endl;
            return false;
        }
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        if (!commands)
        {
            std::cerr << "ResolveTexture: failed to finish command buffer." << std::endl;
            return false;
        }

        m_device.GetQueue().Submit(1, &commands);
        return true;
    }

    TextureAPtr NutContext::AcquireSwapChainTexture()
    {
        m_currentRenderTarget = nullptr;
        return GetCurrentTexture();
    }
}

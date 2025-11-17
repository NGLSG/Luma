#ifndef NOAI_GRAPHICSCONTEXT_H
#define NOAI_GRAPHICSCONTEXT_H
#include "RenderPass.h"
#include "RenderTarget.h"
#include "TextureA.h"
#include "dawn/native/DawnNative.h"

namespace Nut {

/**
 * @brief 定义图形后端类型。
 */
enum class BackendType
{
    D3D12, ///< Direct3D 12 后端。
    D3D11, ///< Direct3D 11 后端。
    Vulkan, ///< Vulkan 后端。
    Metal, ///< Metal 后端。
    OpenGL, ///< OpenGL 后端。
    OpenGLES, ///< OpenGL ES 后端。
};

/**
 * @brief 定义渲染质量等级。
 */
enum class QualityLevel
{
    Low, ///< 低质量。
    Medium, ///< 中等质量。
    High, ///< 高质量。
};

/**
 * @brief 封装原生窗口句柄信息。
 *
 * 根据不同的操作系统，包含不同的原生窗口句柄类型。
 */
struct NativeWindowHandle
{
#if defined(_WIN32)
    void* hWnd = nullptr; ///< Windows 窗口句柄。
    void* hInst = nullptr; ///< Windows 实例句柄。
#elif defined(__APPLE__)
    void* metalLayer = nullptr; ///< Metal 层对象。
#elif defined(__linux__) && !defined(__ANDROID__)
    void* x11Display = nullptr; ///< X11 显示器句柄。
    unsigned long x11Window = 0; ///< X11 窗口ID。
#elif defined(__ANDROID__)
    void* aNativeWindow = nullptr; ///< Android 原生窗口指针。
#else

    void* placeholder = nullptr; ///< 占位符，用于未知平台。
#endif

    /**
     * @brief 检查原生窗口句柄是否有效。
     * @return 如果句柄有效则返回 true，否则返回 false。
     */
    bool IsValid() const
    {
#if defined(_WIN32)
        return hWnd != nullptr && hInst != nullptr;
#elif defined(__APPLE__)
        return metalLayer != nullptr;
#elif defined(__linux__) && !defined(__ANDROID__)
        return x11Display != nullptr && x11Window != 0;
#elif defined(__ANDROID__)
        return aNativeWindow != nullptr;
#else
        return placeholder != nullptr;
#endif
    }
};

struct GraphicsContextDescriptor
{
    std::vector<BackendType> backendTypePriority = {
        BackendType::D3D12, BackendType::Vulkan, BackendType::Metal
    }; ///< 后端类型优先级列表。
    NativeWindowHandle windowHandle; ///< 原生窗口句柄。
    uint16_t width = 1; ///< 初始宽度。
    uint16_t height = 1; ///< 初始高度。
    bool enableVSync = true; ///< 是否启用垂直同步。
    QualityLevel qualityLevel = QualityLevel::High; ///< 初始质量等级。
};

enum GraphicsContextCreateStatus: uint32_t
{
    SUCCESS = 0,
    ERROR_DEVICE_CREATION = 1,
    ERROR_NONE_AVAILABLE_ADAPTER = 2,
    ERROR_SURFACE_CREATION = 3,
    ERROR_INSTANCE_CREATION = 4,
    ERROR_ALREADY_CREATED = 5,
};

class RenderPassBuilder;
class ComputePassBuilder;

class NutContext : public std::enable_shared_from_this<NutContext>
{
private:
    wgpu::SurfaceTexture m_currentSurfaceTexture;
    std::unique_ptr<dawn::native::Instance> m_instance;
    wgpu::Device m_device;
    wgpu::Surface m_surface;
    GraphicsContextDescriptor m_descriptor;
    std::unordered_map<std::string, std::shared_ptr<RenderTarget>> m_renderTargets;
    wgpu::TextureFormat m_graphicsFormat = wgpu::TextureFormat::BGRA8Unorm;
    Size m_size = {0, 0};
    std::shared_ptr<RenderTarget> m_currentRenderTarget;
    bool m_isDeviceLost = false;
    std::vector<wgpu::CommandEncoder> m_commandEncoders;
    std::mutex m_mutex;

    std::vector<wgpu::CommandEncoder> m_computeCommandEncoders;
    std::mutex m_cmutex;

public:
    NutContext(const NutContext&) = delete;
    NutContext& operator=(const NutContext&) = delete;

    NutContext();

private:
    bool createInstance();
    bool createDevice();
    bool createSurface();

    void configureSurface(uint32_t width, uint32_t height);

public:
    static std::shared_ptr<NutContext> Create(const GraphicsContextDescriptor& descriptor);
    GraphicsContextCreateStatus Initialize(const GraphicsContextDescriptor& desc);

    std::shared_ptr<RenderTarget> CreateOrGetRenderTarget(const std::string& name, uint16_t width, uint16_t height);

    void SetActiveRenderTarget(std::shared_ptr<RenderTarget> target);
    void Resize(uint32_t width, uint32_t height);
    void SetVSync(bool enable);

    wgpu::Device& GetWGPUDevice();

    [[nodiscard]] wgpu::Surface& GetWGPUSurface();
    wgpu::Instance GetWGPUInstance() const;

    Size GetCurrentSwapChainSize() const;

    RenderPassBuilder BeginRenderFrame();

    wgpu::CommandBuffer EndRenderFrame(RenderPass& renderPass);

    void Submit(const std::vector<wgpu::CommandBuffer>& cmds);
    void Present();

    ComputePassBuilder BeginComputeFrame();

    wgpu::CommandBuffer EndComputeFrame(ComputePass& computePass);

    void ClearCommands();

    TextureA GetCurrentTexture();

    wgpu::Sampler CreateSampler(wgpu::SamplerDescriptor const* desc);

    TextureA LoadTextureFromFile(const std::string& file);
    TextureA LoadTextureFromMemory(const void* data, size_t size);
    TextureA CreateTexture(const TextureDescriptor& descriptor);
};

} // namespace Nut

#endif //NOAI_GRAPHICSCONTEXT_H

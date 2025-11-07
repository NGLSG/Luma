#ifndef GRAPHICSBACKEND_H
#define GRAPHICSBACKEND_H
#include <iostream>
#include <memory>
#include <stdexcept>
#include "dawn/dawn_proc.h"
#include "RenderTarget.h"

#include <imgui.h>

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
    Ultra ///< 超高质量。
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

/**
 * @brief 图形后端初始化选项。
 *
 * 包含初始化图形后端所需的所有配置参数。
 */
struct GraphicsBackendOptions
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

/**
 * @brief 图形后端管理类。
 *
 * 负责初始化、管理和销毁图形渲染后端，提供高级渲染操作接口。
 */
class LUMA_API GraphicsBackend
{
private:
    std::unique_ptr<dawn::native::Instance> dawnInstance; ///< Dawn 原生实例。
    wgpu::Device device; ///< WebGPU 设备。
    std::unique_ptr<skgpu::graphite::Context> graphiteContext; ///< Skia Graphite 上下文。
    std::unique_ptr<skgpu::graphite::Recorder> graphiteRecorder; ///< Skia Graphite 记录器。
    GraphicsBackendOptions options; ///< 图形后端选项。
    wgpu::Texture m_msaaTexture; ///< 多重采样抗锯齿纹理。
    int m_msaaSampleCount = 1; ///< 多重采样抗锯齿样本计数。
    bool isDeviceLost = false; ///< 设备是否丢失的标志。

    wgpu::Surface surface; ///< WebGPU 表面。
    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::RGBA8Unorm; ///< 表面纹理格式。
    wgpu::SurfaceTexture m_currentSurfaceTexture; ///< 当前表面纹理。


    sk_sp<SkSurface> offscreenSurface; ///< 离屏渲染表面。
    sk_sp<SkImage> lastOffscreenImage; ///< 上一个离屏渲染图像。

    std::unordered_map<std::string, std::shared_ptr<RenderTarget>> m_renderTargets; ///< 渲染目标映射表。
    std::shared_ptr<RenderTarget> m_activeRenderTarget = nullptr; ///< 当前激活的渲染目标。

    uint16_t currentWidth = 0; ///< 当前宽度。
    uint16_t currentHeight = 0; ///< 当前高度。

    GraphicsBackend(); ///< 私有构造函数，通过 Create 静态方法创建实例。
    bool enableVSync = true; ///< 是否启用垂直同步。


    wgpu::Device CreateDevice(const GraphicsBackendOptions& options,
                              const std::unique_ptr<dawn::native::Instance>& instance);


    static wgpu::Surface CreateSurface(const GraphicsBackendOptions& options,
                                       const std::unique_ptr<dawn::native::Instance>& dawnInstance);


    void updateQualitySettings();


    void configureSurface(uint16_t width, uint16_t height);


    void initialize(const GraphicsBackendOptions& options);

public:
    GraphicsBackend(const GraphicsBackend&) = delete; ///< 禁用拷贝构造函数。
    GraphicsBackend& operator=(const GraphicsBackend&) = delete; ///< 禁用赋值操作符。
    ~GraphicsBackend();

    /**
     * @brief 解析多重采样抗锯齿纹理。
     */
    void ResolveMSAA();

    /**
     * @brief 创建或获取一个渲染目标。
     * @param name 渲染目标的名称。
     * @param width 渲染目标的宽度。
     * @param height 渲染目标的高度。
     * @return 指向渲染目标的共享指针。
     */
    std::shared_ptr<RenderTarget> CreateOrGetRenderTarget(const std::string& name, uint16_t width, uint16_t height);

    /**
     * @brief 设置当前激活的渲染目标。
     * @param target 要激活的渲染目标。
     */
    void SetActiveRenderTarget(std::shared_ptr<RenderTarget> target);

    /**
     * @brief 从压缩数据创建图像。
     * @param data 压缩图像数据。
     * @param size 数据大小。
     * @param width 图像宽度。
     * @param height 图像高度。
     * @param format 纹理格式。
     * @return Skia 图像智能指针。
     */
    sk_sp<SkImage> CreateImageFromCompressedData(const unsigned char* data, size_t size,
                                                 int width, int height, wgpu::TextureFormat format);

    /**
     * @brief 获取当前的 Skia 表面。
     * @return Skia 表面智能指针。
     */
    sk_sp<SkSurface> GetSurface();

    /**
     * @brief 从文件路径创建精灵图像。
     * @param filePath 图像文件路径。
     * @return Skia 图像智能指针。
     */
    sk_sp<SkImage> CreateSpriteImageFromFile(const char* filePath) const;
    /**
     * @brief 从 Skia 数据对象创建精灵图像。
     * @param data 包含图像数据的 Skia 数据对象。
     * @return Skia 图像智能指针。
     */
    sk_sp<SkImage> CreateSpriteImageFromData(const sk_sp<SkData>& data) const;

    /**
     * @brief 调整图形后端的大小。
     * @param width 新的宽度。
     * @param height 新的高度。
     */
    void Resize(uint16_t width, uint16_t height);

    /**
     * @brief 重新创建图形后端资源。
     * @return 如果重新创建成功则返回 true，否则返回 false。
     */
    bool Recreate();

    /**
     * @brief 检查设备是否丢失。
     * @return 如果设备丢失则返回 true，否则返回 false。
     */
    bool IsDeviceLost() const { return isDeviceLost; }

    /**
     * @brief 设置是否启用垂直同步。
     * @param enable 是否启用垂直同步。
     */
    void SetEnableVSync(bool enable);

    /**
     * @brief 设置渲染质量等级。
     * @param level 要设置的质量等级。
     */
    void SetQualityLevel(QualityLevel level);

    /**
     * @brief 开始一个新帧的渲染。
     * @return 如果成功开始帧则返回 true，否则返回 false。
     */
    bool BeginFrame();

    /**
     * @brief 提交渲染命令。
     */
    void Submit();

    /**
     * @brief 呈现当前帧到屏幕。
     */
    void PresentFrame();

    /**
     * @brief 获取当前帧的纹理视图。
     * @return 当前帧的 WebGPU 纹理视图。
     */
    wgpu::TextureView GetCurrentFrameView() const;

    /**
     * @brief 分离并获取上一个离屏图像。
     * @return 上一个离屏图像的 Skia 智能指针。
     */
    sk_sp<SkImage> DetachLastOffscreenImage();
    /**
     * @brief 将 GPU 图像转换为 CPU 可访问的图像。
     * @param src 源 GPU 图像。
     * @return 转换后的 CPU 图像的 Skia 智能指针。
     */
    sk_sp<SkImage> GPUToCPUImage(sk_sp<SkImage> src) const;

    /**
     * @brief 获取 Skia Graphite 上下文。
     * @return Skia Graphite 上下文指针。
     */
    skgpu::graphite::Context* GetGraphiteContext() const;
    /**
     * @brief 获取 Skia Graphite 记录器。
     * @return Skia Graphite 记录器指针。
     */
    skgpu::graphite::Recorder* GetRecorder() const;

    /**
     * @brief 获取 WebGPU 设备。
     * @return WebGPU 设备对象。
     */
    wgpu::Device GetDevice() const { return device; }

    /**
     * @brief 获取表面纹理格式。
     * @return 表面纹理格式。
     */
    wgpu::TextureFormat GetSurfaceFormat() const { return surfaceFormat; }

    /**
     * @brief 关闭图形后端并释放资源。
     */
    void shutdown();

    /**
     * @brief 创建一个 GraphicsBackend 实例。
     * @param options 初始化选项。
     * @return 指向 GraphicsBackend 实例的唯一指针。
     */
    static std::unique_ptr<GraphicsBackend> Create(const GraphicsBackendOptions& options);

    /**
     * @brief 从文件加载纹理。
     * @param filename 纹理文件路径。
     * @return 加载的 WebGPU 纹理。
     */
    wgpu::Texture LoadTextureFromFile(const std::string& filename);

    wgpu::Texture LoadTextureFromData(const unsigned char* data, size_t size);
};

#endif

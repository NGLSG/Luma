#ifndef GRAPHICSBACKEND_H
#define GRAPHICSBACKEND_H
#include <iostream>
#include <memory>
#include <stdexcept>
#include "RenderTarget.h"
#include "Nut/NutContext.h"
#include "Nut/TextureA.h"

#include <imgui.h>
#include "dawn/native/DawnNative.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImage.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/Recorder.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/dawn/DawnBackendContext.h"
#include "include/gpu/graphite/dawn/DawnGraphiteTypes.h"
#include "include/gpu/graphite/dawn/DawnUtils.h"
#include "Nut/TextureA.h"
class RuntimeWGSLMaterial;
/**
 * @brief 定义图形后端类型。
 */
using BackendType = Nut::BackendType;
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
using NativeWindowHandle = Nut::NativeWindowHandle;
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
    std::shared_ptr<Nut::NutContext> nutContext; ///< Nut图形上下文，管理wgpu资源。
    std::unique_ptr<skgpu::graphite::Context> graphiteContext; ///< Skia Graphite 上下文。
    std::unique_ptr<skgpu::graphite::Recorder> graphiteRecorder; ///< Skia Graphite 记录器。
    GraphicsBackendOptions options; ///< 图形后端选项。
    Nut::TextureAPtr m_msaaTexture; ///< 多重采样抗锯齿纹理。
    int m_msaaSampleCount = 1; ///< 多重采样抗锯齿样本计数。
    bool isDeviceLost = false; ///< 设备是否丢失的标志。
    sk_sp<SkSurface> m_currentSwapChainSurface;

    sk_sp<SkSurface> offscreenSurface; ///< 离屏渲染表面。
    sk_sp<SkImage> lastOffscreenImage; ///< 上一个离屏渲染图像。

    std::shared_ptr<RenderTarget> m_activeRenderTarget = nullptr; ///< 当前激活的渲染目标。

    uint16_t currentWidth = 0; ///< 当前宽度。
    uint16_t currentHeight = 0; ///< 当前高度。

    GraphicsBackend(); ///< 私有构造函数，通过 Create 静态方法创建实例。
    bool enableVSync = true; ///< 是否启用垂直同步。

    void updateQualitySettings();


    void initialize(const GraphicsBackendOptions& options);
    inline static GraphicsBackend* m_instance;
    /**
     * @brief 解析多重采样抗锯齿纹理。
     */
    void ResolveMSAA();

public:
    GraphicsBackend(const GraphicsBackend&) = delete; ///< 禁用拷贝构造函数。
    GraphicsBackend& operator=(const GraphicsBackend&) = delete; ///< 禁用赋值操作符。
    ~GraphicsBackend();

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
    wgpu::Device GetDevice() const { return nutContext ? nutContext->GetWGPUDevice() : wgpu::Device(); }

    /**
     * @brief 获取表面纹理格式。
     * @return 表面纹理格式。
     */
    wgpu::TextureFormat GetSurfaceFormat() const;

    /**
     * @brief 获取 NutContext。
     * @return NutContext 智能指针。
     */
    std::shared_ptr<Nut::NutContext> GetNutContext() const { return nutContext; }

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
     * @return 加载的纹理。
     */
    Nut::TextureAPtr LoadTextureFromFile(const std::string& filename);

    Nut::TextureAPtr LoadTextureFromData(const unsigned char* data, size_t size);

    Nut::TextureAPtr GetMSAATexture() const { return m_msaaTexture; }

    uint32_t GetSampleCount() const { return m_msaaSampleCount; }

    RuntimeWGSLMaterial* CreateOrGetDefaultMaterial();

    static GraphicsBackend* GetInstance()
    {
        return m_instance;
    }
};

#endif

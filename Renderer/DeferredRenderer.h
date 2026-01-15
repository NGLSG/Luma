/**
 * @file DeferredRenderer.h
 * @brief 延迟渲染器 - 管理 G-Buffer 和延迟光照渲染
 * 
 * 该类负责：
 * - 管理 G-Buffer（位置、法线、颜色、材质缓冲区）
 * - 实现延迟光照计算
 * - 支持延迟/前向混合渲染
 * - 自动渲染模式切换
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 8.1, 8.2, 8.3, 8.4, 8.5
 */

#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "../Components/LightingTypes.h"
#include "Nut/Buffer.h"
#include "Nut/Pipeline.h"
#include "Nut/RenderTarget.h"
#include "Nut/NutContext.h"
#include <memory>
#include <array>

namespace Systems
{
    class LightingSystem;
}

/**
 * @brief 渲染模式枚举
 * 
 * Requirements: 8.5
 */
enum class RenderMode : uint8_t
{
    Forward,    ///< 前向渲染模式
    Deferred,   ///< 延迟渲染模式
    Auto        ///< 自动模式（根据光源数量切换）
};

/**
 * @brief G-Buffer 类型枚举
 * 
 * Requirements: 8.2
 */
enum class GBufferType : uint8_t
{
    Position = 0,   ///< 位置缓冲区 (RGBA16Float)
    Normal = 1,     ///< 法线缓冲区 (RGBA8Snorm)
    Albedo = 2,     ///< 颜色缓冲区 (RGBA8Unorm)
    Material = 3,   ///< 材质缓冲区 (RGBA8Unorm)
    Count = 4       ///< 缓冲区数量
};

/**
 * @brief G-Buffer 数据结构（GPU 传输用）
 * 
 * 该结构体用于将 G-Buffer 全局设置传输到 GPU，需要对齐到 16 字节边界。
 * 总大小: 32 字节
 * 
 * Requirements: 8.2
 */
struct alignas(16) GBufferGlobalData
{
    uint32_t bufferWidth;          ///< G-Buffer 宽度（4 字节）
    uint32_t bufferHeight;         ///< G-Buffer 高度（4 字节）
    uint32_t renderMode;           ///< 渲染模式（4 字节）
    uint32_t enableDeferred;       ///< 是否启用延迟渲染（4 字节）
    // -- 16 字节边界 --
    float nearPlane;               ///< 近裁剪面（4 字节）
    float farPlane;                ///< 远裁剪面（4 字节）
    float padding1;                ///< 对齐填充（4 字节）
    float padding2;                ///< 对齐填充（4 字节）
    // -- 32 字节边界 --

    /**
     * @brief 默认构造函数
     */
    GBufferGlobalData()
        : bufferWidth(0)
        , bufferHeight(0)
        , renderMode(static_cast<uint32_t>(RenderMode::Forward))
        , enableDeferred(0)
        , nearPlane(0.1f)
        , farPlane(1000.0f)
        , padding1(0.0f)
        , padding2(0.0f)
    {
    }
};

// 静态断言确保 GBufferGlobalData 大小和对齐正确
static_assert(sizeof(GBufferGlobalData) == 32, "GBufferGlobalData must be 32 bytes for GPU alignment");
static_assert(alignof(GBufferGlobalData) == 16, "GBufferGlobalData must be aligned to 16 bytes");

/**
 * @brief 延迟光照参数结构（GPU 传输用）
 * 
 * 该结构体用于延迟光照 Pass 的参数传输。
 * 总大小: 32 字节
 * 
 * Requirements: 8.3
 */
struct alignas(16) DeferredLightingParams
{
    uint32_t lightCount;           ///< 光源数量（4 字节）
    uint32_t maxLightsPerPixel;    ///< 每像素最大光源数（4 字节）
    uint32_t enableShadows;        ///< 是否启用阴影（4 字节）
    uint32_t debugMode;            ///< 调试模式（4 字节）
    // -- 16 字节边界 --
    float ambientIntensity;        ///< 环境光强度（4 字节）
    float padding1;                ///< 对齐填充（4 字节）
    float padding2;                ///< 对齐填充（4 字节）
    float padding3;                ///< 对齐填充（4 字节）
    // -- 32 字节边界 --

    /**
     * @brief 默认构造函数
     */
    DeferredLightingParams()
        : lightCount(0)
        , maxLightsPerPixel(8)
        , enableShadows(1)
        , debugMode(0)
        , ambientIntensity(1.0f)
        , padding1(0.0f)
        , padding2(0.0f)
        , padding3(0.0f)
    {
    }
};

// 静态断言确保 DeferredLightingParams 大小和对齐正确
static_assert(sizeof(DeferredLightingParams) == 32, "DeferredLightingParams must be 32 bytes for GPU alignment");
static_assert(alignof(DeferredLightingParams) == 16, "DeferredLightingParams must be aligned to 16 bytes");

/**
 * @brief 合成参数结构（GPU 传输用）
 * 
 * 该结构体用于延迟/前向混合合成的参数传输。
 * 总大小: 32 字节
 * 
 * Requirements: 8.4
 */
struct alignas(16) CompositeParams
{
    uint32_t enableDeferred;       ///< 是否启用延迟渲染（4 字节）
    uint32_t enableForward;        ///< 是否启用前向渲染（4 字节）
    uint32_t blendMode;            ///< 混合模式（4 字节）
    uint32_t debugMode;            ///< 调试模式（4 字节）
    // -- 16 字节边界 --
    float deferredWeight;          ///< 延迟渲染权重（4 字节）
    float forwardWeight;           ///< 前向渲染权重（4 字节）
    float padding1;                ///< 对齐填充（4 字节）
    float padding2;                ///< 对齐填充（4 字节）
    // -- 32 字节边界 --

    /**
     * @brief 默认构造函数
     */
    CompositeParams()
        : enableDeferred(1)
        , enableForward(1)
        , blendMode(0)
        , debugMode(0)
        , deferredWeight(1.0f)
        , forwardWeight(1.0f)
        , padding1(0.0f)
        , padding2(0.0f)
    {
    }
};

// 静态断言确保 CompositeParams 大小和对齐正确
static_assert(sizeof(CompositeParams) == 32, "CompositeParams must be 32 bytes for GPU alignment");
static_assert(alignof(CompositeParams) == 16, "CompositeParams must be aligned to 16 bytes");

/**
 * @brief 延迟渲染器
 * 
 * 负责：
 * - 管理 G-Buffer 的创建和销毁
 * - 实现 G-Buffer 写入 Pass
 * - 实现延迟光照计算 Pass
 * - 支持延迟/前向混合渲染
 * - 自动渲染模式切换
 * 
 * Requirements: 8.1, 8.2, 8.3, 8.4, 8.5
 */
class LUMA_API DeferredRenderer
{
public:
    /// 自动切换到延迟渲染的光源数量阈值
    static constexpr uint32_t AUTO_DEFERRED_LIGHT_THRESHOLD = 16;

    DeferredRenderer();
    ~DeferredRenderer();

    // 禁用拷贝
    DeferredRenderer(const DeferredRenderer&) = delete;
    DeferredRenderer& operator=(const DeferredRenderer&) = delete;

    /**
     * @brief 初始化延迟渲染器
     * @param context NutContext
     * @return 是否成功
     */
    bool Initialize(const std::shared_ptr<Nut::NutContext>& context);

    /**
     * @brief 关闭延迟渲染器
     */
    void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return m_initialized; }

    // ============ G-Buffer 管理 ============
    // Requirements: 8.2

    /**
     * @brief 创建或重新创建 G-Buffer
     * @param width 缓冲区宽度
     * @param height 缓冲区高度
     * @return 是否成功
     */
    bool CreateGBuffer(uint32_t width, uint32_t height);

    /**
     * @brief 销毁 G-Buffer
     */
    void DestroyGBuffer();

    /**
     * @brief 检查 G-Buffer 是否有效
     * @return 是否有效
     */
    bool IsGBufferValid() const;

    /**
     * @brief 获取 G-Buffer 渲染目标
     * @param type G-Buffer 类型
     * @return 渲染目标指针
     */
    std::shared_ptr<Nut::RenderTarget> GetGBuffer(GBufferType type) const;

    /**
     * @brief 获取 G-Buffer 纹理视图
     * @param type G-Buffer 类型
     * @return 纹理视图
     */
    wgpu::TextureView GetGBufferView(GBufferType type) const;

    /**
     * @brief 获取 G-Buffer 宽度
     */
    uint32_t GetGBufferWidth() const { return m_gBufferWidth; }

    /**
     * @brief 获取 G-Buffer 高度
     */
    uint32_t GetGBufferHeight() const { return m_gBufferHeight; }

    // ============ 渲染模式控制 ============
    // Requirements: 8.5

    /**
     * @brief 设置渲染模式
     * @param mode 渲染模式
     */
    void SetRenderMode(RenderMode mode);

    /**
     * @brief 获取当前渲染模式
     * @return 当前渲染模式
     */
    RenderMode GetRenderMode() const { return m_renderMode; }

    /**
     * @brief 获取实际使用的渲染模式（考虑自动切换）
     * @return 实际渲染模式
     */
    RenderMode GetEffectiveRenderMode() const { return m_effectiveRenderMode; }

    /**
     * @brief 检查是否使用延迟渲染
     * @return 是否使用延迟渲染
     */
    bool IsUsingDeferredRendering() const { return m_effectiveRenderMode == RenderMode::Deferred; }

    /**
     * @brief 设置自动切换阈值
     * @param threshold 光源数量阈值
     */
    void SetAutoSwitchThreshold(uint32_t threshold) { m_autoSwitchThreshold = threshold; }

    /**
     * @brief 获取自动切换阈值
     * @return 光源数量阈值
     */
    uint32_t GetAutoSwitchThreshold() const { return m_autoSwitchThreshold; }

    /**
     * @brief 更新渲染模式（根据光源数量自动切换）
     * @param lightCount 当前光源数量
     * Requirements: 8.5
     */
    void UpdateRenderMode(uint32_t lightCount);

    // ============ 延迟光照 Pass ============
    // Requirements: 8.1, 8.3

    /**
     * @brief 设置光照系统引用
     * @param lightingSystem 光照系统指针
     */
    void SetLightingSystem(Systems::LightingSystem* lightingSystem);

    /**
     * @brief 获取延迟光照参数缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetDeferredLightingParamsBuffer() const;

    /**
     * @brief 获取 G-Buffer 全局数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetGBufferGlobalBuffer() const;

    /**
     * @brief 更新延迟光照参数
     * @param lightCount 光源数量
     * @param maxLightsPerPixel 每像素最大光源数
     * @param enableShadows 是否启用阴影
     * @param ambientIntensity 环境光强度
     */
    void UpdateDeferredLightingParams(
        uint32_t lightCount,
        uint32_t maxLightsPerPixel,
        bool enableShadows,
        float ambientIntensity
    );

    /**
     * @brief 将 G-Buffer 数据绑定到管线
     * @param pipeline 渲染管线
     * @param groupIndex 绑定组索引
     */
    void BindGBufferData(Nut::RenderPipeline* pipeline, uint32_t groupIndex);

    // ============ 延迟/前向混合 ============
    // Requirements: 8.4

    /**
     * @brief 检查对象是否应该使用前向渲染
     * @param isTransparent 是否透明
     * @return 是否使用前向渲染
     */
    bool ShouldUseForwardRendering(bool isTransparent) const;

    /**
     * @brief 获取最终合成渲染目标
     * @return 渲染目标指针
     */
    std::shared_ptr<Nut::RenderTarget> GetCompositeTarget() const;

    /**
     * @brief 获取前向渲染目标（用于透明物体）
     * @return 渲染目标指针
     */
    std::shared_ptr<Nut::RenderTarget> GetForwardTarget() const;

    /**
     * @brief 创建前向渲染目标
     * @param width 宽度
     * @param height 高度
     * @return 是否成功
     */
    bool CreateForwardTarget(uint32_t width, uint32_t height);

    // ============ 合成参数 ============
    // Requirements: 8.4

    /**
     * @brief 混合模式枚举
     */
    enum class BlendMode : uint32_t
    {
        Alpha = 0,      ///< Alpha 混合
        Additive = 1,   ///< 加法混合
        Multiply = 2    ///< 乘法混合
    };

    /**
     * @brief 设置混合模式
     * @param mode 混合模式
     */
    void SetBlendMode(BlendMode mode) { m_blendMode = mode; }

    /**
     * @brief 获取混合模式
     */
    BlendMode GetBlendMode() const { return m_blendMode; }

    /**
     * @brief 设置延迟渲染权重
     * @param weight 权重 [0, 1]
     */
    void SetDeferredWeight(float weight) { m_deferredWeight = weight; }

    /**
     * @brief 获取延迟渲染权重
     */
    float GetDeferredWeight() const { return m_deferredWeight; }

    /**
     * @brief 设置前向渲染权重
     * @param weight 权重 [0, 1]
     */
    void SetForwardWeight(float weight) { m_forwardWeight = weight; }

    /**
     * @brief 获取前向渲染权重
     */
    float GetForwardWeight() const { return m_forwardWeight; }

    /**
     * @brief 获取合成参数缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetCompositeParamsBuffer() const;

    /**
     * @brief 更新合成参数
     */
    void UpdateCompositeParams();

    // ============ 调试功能 ============

    /**
     * @brief 设置调试模式
     * @param mode 调试模式（0=关闭，1=位置，2=法线，3=颜色，4=材质）
     */
    void SetDebugMode(uint32_t mode) { m_debugMode = mode; }

    /**
     * @brief 获取调试模式
     * @return 调试模式
     */
    uint32_t GetDebugMode() const { return m_debugMode; }

    /**
     * @brief 获取单例实例
     */
    static DeferredRenderer& GetInstance();

private:
    /**
     * @brief 创建 G-Buffer 全局数据缓冲区
     */
    void CreateGBufferGlobalBuffer();

    /**
     * @brief 创建延迟光照参数缓冲区
     */
    void CreateDeferredLightingParamsBuffer();

    /**
     * @brief 更新 G-Buffer 全局数据
     */
    void UpdateGBufferGlobalData();

    /**
     * @brief 获取 G-Buffer 纹理格式
     * @param type G-Buffer 类型
     * @return 纹理格式
     */
    static wgpu::TextureFormat GetGBufferFormat(GBufferType type);

private:
    std::shared_ptr<Nut::NutContext> m_context;
    Systems::LightingSystem* m_lightingSystem = nullptr;
    bool m_initialized = false;

    // G-Buffer
    std::array<std::shared_ptr<Nut::RenderTarget>, static_cast<size_t>(GBufferType::Count)> m_gBuffers;
    uint32_t m_gBufferWidth = 0;
    uint32_t m_gBufferHeight = 0;

    // 渲染模式
    RenderMode m_renderMode = RenderMode::Forward;
    RenderMode m_effectiveRenderMode = RenderMode::Forward;
    uint32_t m_autoSwitchThreshold = AUTO_DEFERRED_LIGHT_THRESHOLD;

    // GPU 缓冲区
    std::shared_ptr<Nut::Buffer> m_gBufferGlobalBuffer;
    std::shared_ptr<Nut::Buffer> m_deferredLightingParamsBuffer;
    GBufferGlobalData m_gBufferGlobalData;
    DeferredLightingParams m_deferredLightingParams;

    // 合成渲染目标
    std::shared_ptr<Nut::RenderTarget> m_compositeTarget;
    std::shared_ptr<Nut::RenderTarget> m_forwardTarget;

    // 合成参数
    std::shared_ptr<Nut::Buffer> m_compositeParamsBuffer;
    CompositeParams m_compositeParams;
    BlendMode m_blendMode = BlendMode::Alpha;
    float m_deferredWeight = 1.0f;
    float m_forwardWeight = 1.0f;

    // 调试
    uint32_t m_debugMode = 0;

    static DeferredRenderer* s_instance;
};

#endif // DEFERRED_RENDERER_H

/**
 * @file RenderBufferManager.h
 * @brief 渲染缓冲区管理器
 * 
 * 统一管理所有光照相关的渲染缓冲区，包括 Light Buffer、Shadow Buffer、
 * Emission Buffer、Normal Buffer 等。支持动态分辨率、窗口大小变化响应
 * 和缓冲区复用。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 12.1, 12.2, 12.3, 12.4
 */

#ifndef RENDER_BUFFER_MANAGER_H
#define RENDER_BUFFER_MANAGER_H

#include "Nut/RenderTarget.h"
#include "Nut/NutContext.h"
#include "../Components/LightingTypes.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

/**
 * @brief 缓冲区类型枚举
 * 
 * 定义所有光照相关的渲染缓冲区类型。
 * Requirements: 12.1
 */
enum class RenderBufferType : uint8_t
{
    Light = 0,          ///< 光照缓冲区
    Shadow,             ///< 阴影缓冲区
    Emission,           ///< 自发光缓冲区
    Normal,             ///< 法线缓冲区
    Bloom,              ///< Bloom 缓冲区
    BloomTemp,          ///< Bloom 临时缓冲区（用于 ping-pong）
    LightShaft,         ///< 光束效果缓冲区
    Fog,                ///< 雾效缓冲区
    ToneMapping,        ///< 色调映射缓冲区
    Composite,          ///< 合成缓冲区
    GBufferPosition,    ///< G-Buffer 位置
    GBufferNormal,      ///< G-Buffer 法线
    GBufferAlbedo,      ///< G-Buffer 颜色
    GBufferMaterial,    ///< G-Buffer 材质
    Forward,            ///< 前向渲染缓冲区
    Count               ///< 缓冲区类型数量
};

/**
 * @brief 缓冲区配置结构
 * 
 * 定义单个缓冲区的配置参数。
 * Requirements: 12.1
 */
struct BufferConfig
{
    wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm;  ///< 纹理格式
    float scale = 1.0f;                                             ///< 分辨率缩放因子
    bool enabled = true;                                            ///< 是否启用
    bool persistent = true;                                         ///< 是否持久（不参与临时池）
    std::string debugName;                                          ///< 调试名称

    BufferConfig() = default;
    
    BufferConfig(wgpu::TextureFormat fmt, float scl, bool en = true, bool pers = true, const std::string& name = "")
        : format(fmt), scale(scl), enabled(en), persistent(pers), debugName(name) {}
};

/**
 * @brief 缓冲区信息结构
 * 
 * 存储缓冲区的运行时信息。
 */
struct BufferInfo
{
    std::shared_ptr<Nut::RenderTarget> target;  ///< 渲染目标
    BufferConfig config;                         ///< 配置
    uint32_t width = 0;                          ///< 实际宽度
    uint32_t height = 0;                         ///< 实际高度
    uint64_t lastUsedFrame = 0;                  ///< 上次使用的帧号
    bool isDirty = true;                         ///< 是否需要重新创建
};

/**
 * @brief 临时缓冲区请求结构
 * 
 * 用于从临时缓冲区池请求缓冲区。
 * Requirements: 12.4
 */
struct TempBufferRequest
{
    uint32_t width;                              ///< 请求宽度
    uint32_t height;                             ///< 请求高度
    wgpu::TextureFormat format;                  ///< 请求格式
    std::string debugName;                       ///< 调试名称（可选）
};

/**
 * @brief 缓冲区大小变化回调类型
 */
using BufferResizeCallback = std::function<void(uint32_t newWidth, uint32_t newHeight)>;

/**
 * @brief 渲染缓冲区管理器
 * 
 * 该类负责：
 * - 统一管理所有光照相关缓冲区的创建和销毁
 * - 支持动态分辨率，根据质量等级调整缓冲区大小
 * - 响应窗口大小变化，自动重新分配缓冲区
 * - 实现缓冲区复用，减少内存占用
 * - 提供缓冲区调试可视化功能
 * 
 * Requirements: 12.1, 12.2, 12.3, 12.4, 12.5
 */
class LUMA_API RenderBufferManager
{
public:
    /// 最大临时缓冲区数量
    static constexpr size_t MAX_TEMP_BUFFERS = 16;
    
    /// 临时缓冲区过期帧数（超过此帧数未使用则释放）
    static constexpr uint64_t TEMP_BUFFER_EXPIRE_FRAMES = 60;

    /**
     * @brief 获取单例实例
     * @return RenderBufferManager 单例引用
     */
    static RenderBufferManager& GetInstance();

    /**
     * @brief 销毁单例实例
     */
    static void DestroyInstance();

    // ==================== 初始化和清理 ====================

    /**
     * @brief 初始化缓冲区管理器
     * @param context NutContext
     * @param baseWidth 基础宽度
     * @param baseHeight 基础高度
     * @return 是否成功
     */
    bool Initialize(
        const std::shared_ptr<Nut::NutContext>& context,
        uint32_t baseWidth,
        uint32_t baseHeight);

    /**
     * @brief 关闭缓冲区管理器
     */
    void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return m_initialized; }

    // ==================== 缓冲区配置 ====================
    // Requirements: 12.1

    /**
     * @brief 设置缓冲区配置
     * @param type 缓冲区类型
     * @param config 配置
     */
    void SetBufferConfig(RenderBufferType type, const BufferConfig& config);

    /**
     * @brief 获取缓冲区配置
     * @param type 缓冲区类型
     * @return 配置的常量引用
     */
    const BufferConfig& GetBufferConfig(RenderBufferType type) const;

    /**
     * @brief 应用默认配置
     * 
     * 根据设计文档设置所有缓冲区的默认配置。
     */
    void ApplyDefaultConfigs();

    // ==================== 缓冲区访问 ====================
    // Requirements: 12.1

    /**
     * @brief 获取缓冲区
     * @param type 缓冲区类型
     * @return 渲染目标指针，如果不存在或未启用则返回 nullptr
     */
    std::shared_ptr<Nut::RenderTarget> GetBuffer(RenderBufferType type);

    /**
     * @brief 获取缓冲区纹理视图
     * @param type 缓冲区类型
     * @return 纹理视图，如果不存在则返回无效视图
     */
    wgpu::TextureView GetBufferView(RenderBufferType type);

    /**
     * @brief 检查缓冲区是否有效
     * @param type 缓冲区类型
     * @return 是否有效
     */
    bool IsBufferValid(RenderBufferType type) const;

    /**
     * @brief 获取缓冲区实际宽度
     * @param type 缓冲区类型
     * @return 宽度
     */
    uint32_t GetBufferWidth(RenderBufferType type) const;

    /**
     * @brief 获取缓冲区实际高度
     * @param type 缓冲区类型
     * @return 高度
     */
    uint32_t GetBufferHeight(RenderBufferType type) const;

    // ==================== 动态分辨率 ====================
    // Requirements: 12.2

    /**
     * @brief 设置全局渲染缩放
     * @param scale 缩放因子 [0.25, 2.0]
     * 
     * 此缩放会应用到所有缓冲区的基础分辨率上。
     */
    void SetGlobalRenderScale(float scale);

    /**
     * @brief 获取全局渲染缩放
     * @return 当前缩放因子
     */
    float GetGlobalRenderScale() const { return m_globalRenderScale; }

    /**
     * @brief 设置特定缓冲区的缩放
     * @param type 缓冲区类型
     * @param scale 缩放因子
     */
    void SetBufferScale(RenderBufferType type, float scale);

    /**
     * @brief 获取特定缓冲区的缩放
     * @param type 缓冲区类型
     * @return 缩放因子
     */
    float GetBufferScale(RenderBufferType type) const;

    // ==================== 窗口大小变化响应 ====================
    // Requirements: 12.3

    /**
     * @brief 处理窗口大小变化
     * @param newWidth 新宽度
     * @param newHeight 新高度
     * 
     * 当窗口大小变化时调用此方法，会重新分配所有缓冲区。
     */
    void OnWindowResize(uint32_t newWidth, uint32_t newHeight);

    /**
     * @brief 获取基础宽度
     */
    uint32_t GetBaseWidth() const { return m_baseWidth; }

    /**
     * @brief 获取基础高度
     */
    uint32_t GetBaseHeight() const { return m_baseHeight; }

    /**
     * @brief 注册窗口大小变化回调
     * @param callback 回调函数
     * @return 回调 ID
     */
    int RegisterResizeCallback(BufferResizeCallback callback);

    /**
     * @brief 取消注册窗口大小变化回调
     * @param callbackId 回调 ID
     */
    void UnregisterResizeCallback(int callbackId);

    // ==================== 缓冲区复用 ====================
    // Requirements: 12.4

    /**
     * @brief 从临时缓冲区池获取缓冲区
     * @param request 请求参数
     * @return 渲染目标指针
     * 
     * 如果池中有匹配的缓冲区则复用，否则创建新的。
     */
    std::shared_ptr<Nut::RenderTarget> AcquireTempBuffer(const TempBufferRequest& request);

    /**
     * @brief 释放临时缓冲区回池
     * @param buffer 要释放的缓冲区
     * 
     * 缓冲区不会立即销毁，而是放回池中等待复用。
     */
    void ReleaseTempBuffer(std::shared_ptr<Nut::RenderTarget> buffer);

    /**
     * @brief 清理过期的临时缓冲区
     * 
     * 应在每帧结束时调用。
     */
    void CleanupExpiredTempBuffers();

    /**
     * @brief 获取临时缓冲区池大小
     * @return 池中缓冲区数量
     */
    size_t GetTempBufferPoolSize() const { return m_tempBufferPool.size(); }

    /**
     * @brief 清空临时缓冲区池
     */
    void ClearTempBufferPool();

    // ==================== 帧管理 ====================

    /**
     * @brief 开始新帧
     * 
     * 更新帧计数器，应在每帧开始时调用。
     */
    void BeginFrame();

    /**
     * @brief 结束帧
     * 
     * 清理过期资源，应在每帧结束时调用。
     */
    void EndFrame();

    /**
     * @brief 获取当前帧号
     */
    uint64_t GetCurrentFrame() const { return m_currentFrame; }

    // ==================== 调试功能 ====================
    // Requirements: 12.5

    /**
     * @brief 启用/禁用调试模式
     * @param enable 是否启用
     */
    void SetDebugMode(bool enable) { m_debugMode = enable; }

    /**
     * @brief 获取调试模式状态
     */
    bool IsDebugMode() const { return m_debugMode; }

    /**
     * @brief 获取缓冲区调试信息
     * @param type 缓冲区类型
     * @return 调试信息字符串
     */
    std::string GetBufferDebugInfo(RenderBufferType type) const;

    /**
     * @brief 获取所有缓冲区的内存使用统计
     * @return 总内存使用量（字节）
     */
    size_t GetTotalMemoryUsage() const;

    /**
     * @brief 获取缓冲区类型名称
     * @param type 缓冲区类型
     * @return 类型名称字符串
     */
    static const char* GetBufferTypeName(RenderBufferType type);

    // ==================== 质量等级集成 ====================

    /**
     * @brief 应用质量等级设置
     * @param renderScale 渲染缩放
     * @param enableBloom 是否启用 Bloom
     * @param enableLightShafts 是否启用光束
     * @param enableFog 是否启用雾效
     */
    void ApplyQualitySettings(
        float renderScale,
        bool enableBloom,
        bool enableLightShafts,
        bool enableFog);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    RenderBufferManager();

    /**
     * @brief 私有析构函数
     */
    ~RenderBufferManager();

    // 禁用拷贝和移动
    RenderBufferManager(const RenderBufferManager&) = delete;
    RenderBufferManager& operator=(const RenderBufferManager&) = delete;
    RenderBufferManager(RenderBufferManager&&) = delete;
    RenderBufferManager& operator=(RenderBufferManager&&) = delete;

    /**
     * @brief 创建单个缓冲区
     * @param type 缓冲区类型
     * @return 是否成功
     */
    bool CreateBuffer(RenderBufferType type);

    /**
     * @brief 销毁单个缓冲区
     * @param type 缓冲区类型
     */
    void DestroyBuffer(RenderBufferType type);

    /**
     * @brief 重新创建所有缓冲区
     */
    void RecreateAllBuffers();

    /**
     * @brief 计算缓冲区实际尺寸
     * @param type 缓冲区类型
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     */
    void CalculateBufferSize(RenderBufferType type, uint32_t& outWidth, uint32_t& outHeight) const;

    /**
     * @brief 创建渲染目标
     * @param width 宽度
     * @param height 高度
     * @param format 纹理格式
     * @param debugName 调试名称
     * @return 渲染目标指针
     */
    std::shared_ptr<Nut::RenderTarget> CreateRenderTarget(
        uint32_t width,
        uint32_t height,
        wgpu::TextureFormat format,
        const std::string& debugName);

    /**
     * @brief 计算纹理内存大小
     * @param width 宽度
     * @param height 高度
     * @param format 纹理格式
     * @return 内存大小（字节）
     */
    static size_t CalculateTextureMemorySize(
        uint32_t width,
        uint32_t height,
        wgpu::TextureFormat format);

    /**
     * @brief 通知大小变化回调
     */
    void NotifyResizeCallbacks();

private:
    static RenderBufferManager* s_instance;

    std::shared_ptr<Nut::NutContext> m_context;
    bool m_initialized = false;
    bool m_debugMode = false;

    // 基础尺寸
    uint32_t m_baseWidth = 0;
    uint32_t m_baseHeight = 0;

    // 全局渲染缩放
    float m_globalRenderScale = 1.0f;

    // 缓冲区存储
    std::array<BufferInfo, static_cast<size_t>(RenderBufferType::Count)> m_buffers;

    // 临时缓冲区池
    struct TempBufferEntry
    {
        std::shared_ptr<Nut::RenderTarget> target;
        uint32_t width;
        uint32_t height;
        wgpu::TextureFormat format;
        uint64_t lastUsedFrame;
        bool inUse;
    };
    std::vector<TempBufferEntry> m_tempBufferPool;

    // 帧计数器
    uint64_t m_currentFrame = 0;

    // 回调管理
    std::vector<std::pair<int, BufferResizeCallback>> m_resizeCallbacks;
    int m_nextCallbackId = 0;
};

#endif // RENDER_BUFFER_MANAGER_H

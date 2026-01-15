#ifndef LIGHTING_RENDERER_H
#define LIGHTING_RENDERER_H

/**
 * @file LightingRenderer.h
 * @brief 光照渲染器 - 负责将光照数据绑定到渲染管线
 * 
 * 该类作为 LightingSystem 和渲染管线之间的桥梁，
 * 管理光照相关的 GPU 缓冲区绑定。
 * 
 * Feature: 2d-lighting-system
 * Feature: 2d-lighting-enhancement (Emission Buffer)
 */

#include "../Components/LightingTypes.h"
#include "Nut/Buffer.h"
#include "Nut/Pipeline.h"
#include "Nut/RenderTarget.h"
#include <memory>

namespace Systems
{
    class LightingSystem;
    class ShadowRenderer;
    class AmbientZoneSystem;
}

namespace Nut
{
    class NutContext;
    class RenderPass;
}

/**
 * @brief 自发光数据结构（GPU 传输用）
 *
 * 该结构体用于将自发光全局设置传输到 GPU，需要对齐到 16 字节边界。
 * 总大小: 16 字节
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 4.3
 */
struct alignas(16) EmissionGlobalData
{
    uint32_t emissionEnabled;      ///< 是否启用自发光（4 字节）
    float emissionScale;           ///< 自发光全局缩放（4 字节）
    float padding1;                ///< 对齐填充（4 字节）
    float padding2;                ///< 对齐填充（4 字节）
    // -- 16 字节边界 --

    /**
     * @brief 默认构造函数
     */
    EmissionGlobalData()
        : emissionEnabled(1)
        , emissionScale(1.0f)
        , padding1(0.0f)
        , padding2(0.0f)
    {
    }
};

// 静态断言确保 EmissionGlobalData 大小和对齐正确
static_assert(sizeof(EmissionGlobalData) == 16, "EmissionGlobalData must be 16 bytes for GPU alignment");
static_assert(alignof(EmissionGlobalData) == 16, "EmissionGlobalData must be aligned to 16 bytes");

/**
 * @brief 光照渲染器
 * 
 * 负责：
 * - 从 LightingSystem 获取光照数据
 * - 将光照数据绑定到渲染管线的 Group 1
 * - 将阴影数据绑定到渲染管线的 Group 2
 * - 管理默认光照缓冲区（当没有光照系统时使用）
 * - 管理 Emission Buffer（自发光缓冲区）
 */
class LUMA_API LightingRenderer
{
public:
    LightingRenderer();
    ~LightingRenderer();

    /**
     * @brief 初始化光照渲染器
     * @param context NutContext
     * @return 是否成功
     */
    bool Initialize(const std::shared_ptr<Nut::NutContext>& context);

    /**
     * @brief 关闭光照渲染器
     */
    void Shutdown();

    /**
     * @brief 设置光照系统引用
     * @param lightingSystem 光照系统指针（可为空）
     */
    void SetLightingSystem(Systems::LightingSystem* lightingSystem);

    /**
     * @brief 设置环境光区域系统引用
     * @param ambientZoneSystem 环境光区域系统指针（可为空）
     * Feature: 2d-lighting-enhancement
     */
    void SetAmbientZoneSystem(Systems::AmbientZoneSystem* ambientZoneSystem);

    /**
     * @brief 将光照数据绑定到管线
     * @param pipeline 渲染管线
     * @param groupIndex 绑定组索引（默认为1）
     */
    void BindLightingData(Nut::RenderPipeline* pipeline, uint32_t groupIndex = 1);

    /**
     * @brief 将阴影数据绑定到管线
     * @param pipeline 渲染管线
     * @param groupIndex 绑定组索引（默认为2）
     */
    void BindShadowData(Nut::RenderPipeline* pipeline, uint32_t groupIndex = 2);

    /**
     * @brief 将光照和阴影数据都绑定到管线
     * @param pipeline 渲染管线
     * @param lightingGroupIndex 光照绑定组索引（默认为1）
     * @param shadowGroupIndex 阴影绑定组索引（默认为2）
     */
    void BindAllLightingData(Nut::RenderPipeline* pipeline, 
                              uint32_t lightingGroupIndex = 1,
                              uint32_t shadowGroupIndex = 2);

    /**
     * @brief 获取光照全局数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetGlobalBuffer() const;

    /**
     * @brief 获取光源数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetLightBuffer() const;

    /**
     * @brief 获取阴影参数缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetShadowParamsBuffer() const;

    /**
     * @brief 获取阴影边缘缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetShadowEdgeBuffer() const;

    /**
     * @brief 获取间接光照全局数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetIndirectGlobalBuffer() const;

    /**
     * @brief 获取反射体数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetReflectorBuffer() const;

    /**
     * @brief 获取面光源全局数据缓冲区
     * @return 缓冲区指针
     * Feature: 2d-lighting-enhancement
     */
    std::shared_ptr<Nut::Buffer> GetAreaLightGlobalBuffer() const;

    /**
     * @brief 获取面光源数据缓冲区
     * @return 缓冲区指针
     * Feature: 2d-lighting-enhancement
     */
    std::shared_ptr<Nut::Buffer> GetAreaLightBuffer() const;

    /**
     * @brief 获取环境光区域全局数据缓冲区
     * @return 缓冲区指针
     * Feature: 2d-lighting-enhancement
     */
    std::shared_ptr<Nut::Buffer> GetAmbientZoneGlobalBuffer() const;

    /**
     * @brief 获取环境光区域数据缓冲区
     * @return 缓冲区指针
     * Feature: 2d-lighting-enhancement
     */
    std::shared_ptr<Nut::Buffer> GetAmbientZoneBuffer() const;

    /**
     * @brief 将间接光照数据绑定到管线
     * @param pipeline 渲染管线
     * @param groupIndex 绑定组索引（默认为3）
     */
    void BindIndirectLightingData(Nut::RenderPipeline* pipeline, uint32_t groupIndex = 3);

    /**
     * @brief 将所有光照数据（直接光照、阴影、间接光照）绑定到管线
     * @param pipeline 渲染管线
     * @param lightingGroupIndex 光照绑定组索引（默认为1）
     * @param shadowGroupIndex 阴影绑定组索引（默认为2）
     * @param indirectGroupIndex 间接光照绑定组索引（默认为3）
     */
    void BindAllLightingDataWithIndirect(Nut::RenderPipeline* pipeline, 
                                          uint32_t lightingGroupIndex = 1,
                                          uint32_t shadowGroupIndex = 2,
                                          uint32_t indirectGroupIndex = 3);

    /**
     * @brief 将自发光数据绑定到管线
     * @param pipeline 渲染管线
     * @param groupIndex 绑定组索引（默认为3，与间接光照同组）
     * @param bindingIndex 绑定索引（默认为2）
     * Feature: 2d-lighting-enhancement
     * Requirements: 4.3
     */
    void BindEmissionData(Nut::RenderPipeline* pipeline, uint32_t groupIndex = 3, uint32_t bindingIndex = 2);

    /**
     * @brief 将所有光照数据（直接光照、阴影、间接光照、自发光）绑定到管线
     * @param pipeline 渲染管线
     * @param lightingGroupIndex 光照绑定组索引（默认为1）
     * @param shadowGroupIndex 阴影绑定组索引（默认为2）
     * @param indirectGroupIndex 间接光照和自发光绑定组索引（默认为3）
     * Feature: 2d-lighting-enhancement
     * Requirements: 4.3
     */
    void BindAllLightingDataWithEmission(Nut::RenderPipeline* pipeline, 
                                          uint32_t lightingGroupIndex = 1,
                                          uint32_t shadowGroupIndex = 2,
                                          uint32_t indirectGroupIndex = 3);

    /**
     * @brief 检查间接光照是否启用
     */
    bool IsIndirectLightingEnabled() const;

    /**
     * @brief 获取反射体数量
     */
    uint32_t GetReflectorCount() const;

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief 获取当前光源数量
     */
    uint32_t GetLightCount() const;

    /**
     * @brief 获取当前阴影边缘数量
     */
    uint32_t GetShadowEdgeCount() const;

    /**
     * @brief 检查阴影是否启用
     */
    bool IsShadowEnabled() const;

    /**
     * @brief 获取单例实例
     */
    static LightingRenderer& GetInstance();

    // ============ Emission Buffer 管理 ============
    // Feature: 2d-lighting-enhancement
    // Requirements: 4.3

    /**
     * @brief 创建或重新创建 Emission Buffer
     * @param width 缓冲区宽度
     * @param height 缓冲区高度
     * @return 是否成功
     */
    bool CreateEmissionBuffer(uint32_t width, uint32_t height);

    /**
     * @brief 获取 Emission Buffer 渲染目标
     * @return Emission Buffer 渲染目标指针
     */
    std::shared_ptr<Nut::RenderTarget> GetEmissionBuffer() const;

    /**
     * @brief 获取 Emission Buffer 纹理视图
     * @return 纹理视图
     */
    wgpu::TextureView GetEmissionBufferView() const;

    /**
     * @brief 获取自发光全局数据缓冲区
     * @return 缓冲区指针
     */
    std::shared_ptr<Nut::Buffer> GetEmissionGlobalBuffer() const;

    /**
     * @brief 更新自发光全局数据
     * @param enabled 是否启用自发光
     * @param scale 自发光全局缩放
     */
    void UpdateEmissionGlobalData(bool enabled, float scale = 1.0f);

    /**
     * @brief 清除 Emission Buffer
     */
    void ClearEmissionBuffer();

    /**
     * @brief 检查 Emission Buffer 是否有效
     * @return 是否有效
     */
    bool IsEmissionBufferValid() const;

    /**
     * @brief 获取 Emission Buffer 宽度
     */
    uint32_t GetEmissionBufferWidth() const;

    /**
     * @brief 获取 Emission Buffer 高度
     */
    uint32_t GetEmissionBufferHeight() const;

private:
    /**
     * @brief 创建默认光照缓冲区
     */
    void CreateDefaultBuffers();

    /**
     * @brief 创建默认阴影缓冲区
     */
    void CreateDefaultShadowBuffers();

    /**
     * @brief 创建默认间接光照缓冲区
     */
    void CreateDefaultIndirectBuffers();

    /**
     * @brief 创建默认面光源缓冲区
     * Feature: 2d-lighting-enhancement
     */
    void CreateDefaultAreaLightBuffers();

    /**
     * @brief 创建默认环境光区域缓冲区
     * Feature: 2d-lighting-enhancement
     */
    void CreateDefaultAmbientZoneBuffers();

    /**
     * @brief 创建自发光全局数据缓冲区
     * Feature: 2d-lighting-enhancement
     * Requirements: 4.3
     */
    void CreateEmissionGlobalBuffer();

    /**
     * @brief 更新默认光照数据
     */
    void UpdateDefaultLightingData();

private:
    std::shared_ptr<Nut::NutContext> m_context;
    Systems::LightingSystem* m_lightingSystem = nullptr;
    Systems::AmbientZoneSystem* m_ambientZoneSystem = nullptr;
    bool m_initialized = false;

    // 默认光照缓冲区（当没有 LightingSystem 时使用）
    std::shared_ptr<Nut::Buffer> m_defaultGlobalBuffer;
    std::shared_ptr<Nut::Buffer> m_defaultLightBuffer;
    ECS::LightingGlobalData m_defaultGlobalData;

    // 默认阴影缓冲区（当没有 ShadowRenderer 时使用）
    std::shared_ptr<Nut::Buffer> m_defaultShadowParamsBuffer;
    std::shared_ptr<Nut::Buffer> m_defaultShadowEdgeBuffer;

    // 默认间接光照缓冲区（当没有 IndirectLightingSystem 时使用）
    std::shared_ptr<Nut::Buffer> m_defaultIndirectGlobalBuffer;
    std::shared_ptr<Nut::Buffer> m_defaultReflectorBuffer;

    // 默认面光源缓冲区（当没有 AreaLightSystem 时使用）
    // Feature: 2d-lighting-enhancement
    std::shared_ptr<Nut::Buffer> m_defaultAreaLightGlobalBuffer;
    std::shared_ptr<Nut::Buffer> m_defaultAreaLightBuffer;

    // 默认环境光区域缓冲区（当没有 AmbientZoneSystem 时使用）
    // Feature: 2d-lighting-enhancement
    std::shared_ptr<Nut::Buffer> m_defaultAmbientZoneGlobalBuffer;
    std::shared_ptr<Nut::Buffer> m_defaultAmbientZoneBuffer;

    // Emission Buffer（自发光缓冲区）
    // Feature: 2d-lighting-enhancement
    // Requirements: 4.3
    std::shared_ptr<Nut::RenderTarget> m_emissionBuffer;
    std::shared_ptr<Nut::Buffer> m_emissionGlobalBuffer;
    EmissionGlobalData m_emissionGlobalData;
    uint32_t m_emissionBufferWidth = 0;
    uint32_t m_emissionBufferHeight = 0;

    static LightingRenderer* s_instance;
};

#endif // LIGHTING_RENDERER_H

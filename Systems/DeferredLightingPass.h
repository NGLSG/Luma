/**
 * @file DeferredLightingPass.h
 * @brief 延迟光照 Pass - 执行延迟光照计算
 * 
 * 该类负责：
 * - 从 G-Buffer 读取几何信息
 * - 计算所有光源的光照贡献
 * - 输出最终光照结果
 * - 支持光源体积渲染优化
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 8.1, 8.3
 */

#ifndef DEFERRED_LIGHTING_PASS_H
#define DEFERRED_LIGHTING_PASS_H

#include "ISystem.h"
#include "../Renderer/DeferredRenderer.h"
#include "../Renderer/Nut/Pipeline.h"
#include "../Renderer/Nut/Buffer.h"
#include "../Renderer/Nut/RenderTarget.h"
#include <memory>

namespace Nut
{
    class NutContext;
    class ShaderModule;
}

namespace Systems
{
    class LightingSystem;

    /**
     * @brief 光源体积类型
     * 
     * 用于延迟渲染中的光源体积渲染优化
     * Requirements: 8.3
     */
    enum class LightVolumeType : uint8_t
    {
        Fullscreen,     ///< 全屏四边形（用于方向光和环境光）
        Sphere,         ///< 球体（用于点光源）
        Cone,           ///< 圆锥体（用于聚光灯）
        Rectangle       ///< 矩形（用于面光源）
    };

    /**
     * @brief 光源体积数据
     * 
     * 用于光源体积渲染的数据结构
     */
    struct LightVolumeData
    {
        LightVolumeType type;
        glm::vec2 position;
        float radius;
        float innerAngle;
        float outerAngle;
        glm::vec2 direction;
        uint32_t lightIndex;
    };

    /**
     * @brief 延迟光照 Pass
     * 
     * 负责执行延迟光照计算，从 G-Buffer 读取几何信息，
     * 计算所有光源的光照贡献，输出最终光照结果。
     * 
     * Requirements: 8.1, 8.3
     */
    class LUMA_API DeferredLightingPass
    {
    public:
        DeferredLightingPass();
        ~DeferredLightingPass();

        // 禁用拷贝
        DeferredLightingPass(const DeferredLightingPass&) = delete;
        DeferredLightingPass& operator=(const DeferredLightingPass&) = delete;

        /**
         * @brief 初始化延迟光照 Pass
         * @param context NutContext
         * @return 是否成功
         */
        bool Initialize(const std::shared_ptr<Nut::NutContext>& context);

        /**
         * @brief 关闭延迟光照 Pass
         */
        void Shutdown();

        /**
         * @brief 检查是否已初始化
         */
        bool IsInitialized() const { return m_initialized; }

        /**
         * @brief 设置光照系统引用
         * @param lightingSystem 光照系统指针
         */
        void SetLightingSystem(LightingSystem* lightingSystem);

        /**
         * @brief 设置延迟渲染器引用
         * @param deferredRenderer 延迟渲染器指针
         */
        void SetDeferredRenderer(DeferredRenderer* deferredRenderer);

        // ============ 渲染执行 ============

        /**
         * @brief 执行延迟光照 Pass
         * @param outputTarget 输出渲染目标
         * 
         * 从 G-Buffer 读取几何信息，计算光照，输出到目标
         * Requirements: 8.1, 8.3
         */
        void Execute(Nut::RenderTarget* outputTarget);

        /**
         * @brief 执行全屏光照计算
         * 
         * 使用全屏四边形计算所有光源的光照贡献
         */
        void ExecuteFullscreenLighting();

        /**
         * @brief 执行光源体积渲染
         * 
         * 使用光源体积优化光照计算
         * Requirements: 8.3
         */
        void ExecuteLightVolumeRendering();

        // ============ 光源体积管理 ============

        /**
         * @brief 生成光源体积数据
         * @param lights 光源数据列表
         * @return 光源体积数据列表
         */
        std::vector<LightVolumeData> GenerateLightVolumes(
            const std::vector<ECS::LightData>& lights);

        /**
         * @brief 检查是否应该使用光源体积渲染
         * @param lightCount 光源数量
         * @return 是否使用光源体积渲染
         */
        bool ShouldUseLightVolumeRendering(uint32_t lightCount) const;

        /**
         * @brief 设置光源体积渲染阈值
         * @param threshold 光源数量阈值
         */
        void SetLightVolumeThreshold(uint32_t threshold) { m_lightVolumeThreshold = threshold; }

        /**
         * @brief 获取光源体积渲染阈值
         */
        uint32_t GetLightVolumeThreshold() const { return m_lightVolumeThreshold; }

        // ============ 调试功能 ============

        /**
         * @brief 设置调试模式
         * @param mode 调试模式
         */
        void SetDebugMode(uint32_t mode) { m_debugMode = mode; }

        /**
         * @brief 获取调试模式
         */
        uint32_t GetDebugMode() const { return m_debugMode; }

        /**
         * @brief 获取单例实例
         */
        static DeferredLightingPass& GetInstance();

    private:
        /**
         * @brief 创建渲染管线
         */
        bool CreatePipelines();

        /**
         * @brief 创建全屏四边形顶点缓冲区
         */
        void CreateFullscreenQuadBuffer();

        /**
         * @brief 绑定 G-Buffer 纹理
         */
        void BindGBufferTextures();

        /**
         * @brief 绑定光照数据
         */
        void BindLightingData();

    private:
        std::shared_ptr<Nut::NutContext> m_context;
        LightingSystem* m_lightingSystem = nullptr;
        DeferredRenderer* m_deferredRenderer = nullptr;
        bool m_initialized = false;

        // 渲染管线
        std::unique_ptr<Nut::RenderPipeline> m_fullscreenLightingPipeline;
        std::unique_ptr<Nut::RenderPipeline> m_lightVolumePipeline;

        // 顶点缓冲区
        std::shared_ptr<Nut::Buffer> m_fullscreenQuadBuffer;

        // 光源体积
        uint32_t m_lightVolumeThreshold = 32;  // 超过此数量使用光源体积渲染
        std::vector<LightVolumeData> m_lightVolumes;

        // 调试
        uint32_t m_debugMode = 0;

        static DeferredLightingPass* s_instance;
    };
}

#endif // DEFERRED_LIGHTING_PASS_H

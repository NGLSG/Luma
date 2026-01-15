#ifndef POST_PROCESS_SYSTEM_H
#define POST_PROCESS_SYSTEM_H

/**
 * @file PostProcessSystem.h
 * @brief 后处理系统
 * 
 * 负责管理和执行后处理效果管线，包括 Bloom、光束、雾效、
 * 色调映射和颜色分级等效果。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 5.1, 12.1
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Components/PostProcessSettingsComponent.h"
#include "../Renderer/Nut/Buffer.h"
#include "../Renderer/Nut/RenderTarget.h"
#include "../Renderer/Nut/Pipeline.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

// Forward declarations
namespace Nut
{
    class NutContext;
    class Pipeline;
    class BindGroup;
}

namespace Systems
{
    /**
     * @brief 后处理 Pass 类型枚举
     */
    enum class PostProcessPassType : uint8_t
    {
        BloomExtract,      ///< Bloom 提取 Pass
        BloomBlurH,        ///< Bloom 水平模糊 Pass
        BloomBlurV,        ///< Bloom 垂直模糊 Pass
        BloomComposite,    ///< Bloom 合成 Pass
        LightShaft,        ///< 光束 Pass
        Fog,               ///< 雾效 Pass
        ToneMapping,       ///< 色调映射 Pass
        ColorGrading,      ///< 颜色分级 Pass
        Final              ///< 最终合成 Pass
    };

    /**
     * @brief 渲染缓冲区配置
     * 
     * 定义后处理系统使用的渲染缓冲区格式和缩放。
     * Requirements: 12.1
     */
    struct RenderBufferConfig
    {
        wgpu::TextureFormat lightBufferFormat = wgpu::TextureFormat::RGBA16Float;
        wgpu::TextureFormat emissionFormat = wgpu::TextureFormat::RGBA16Float;
        wgpu::TextureFormat bloomFormat = wgpu::TextureFormat::RGBA16Float;
        
        float lightBufferScale = 1.0f;    ///< 光照缓冲区分辨率缩放
        float bloomBufferScale = 0.5f;    ///< Bloom 缓冲区分辨率缩放（降采样）
        
        /**
         * @brief 默认构造函数
         */
        RenderBufferConfig() = default;
    };

    /**
     * @brief 后处理系统
     * 
     * 该系统继承自 ISystem 接口，负责：
     * - 管理后处理效果管线
     * - 管理中间渲染缓冲区
     * - 执行各种后处理效果
     * - 与质量管理系统集成
     * 
     * Requirements: 5.1, 12.1
     */
    class PostProcessSystem : public ISystem
    {
    public:
        /// 最大 Bloom 迭代次数
        static constexpr int MAX_BLOOM_ITERATIONS = 16;
        
        /// 最大 Bloom 降采样级别
        static constexpr int MAX_BLOOM_MIP_LEVELS = 8;

        /**
         * @brief 默认构造函数
         */
        PostProcessSystem();

        /**
         * @brief 析构函数
         */
        ~PostProcessSystem() override = default;

        /**
         * @brief 系统创建时调用
         * 
         * 初始化后处理系统所需的资源，包括 GPU 缓冲区和着色器。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * 
         * 从场景获取后处理设置并更新 GPU 缓冲区。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * 
         * 清理后处理系统占用的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 后处理管线执行 ====================

        /**
         * @brief 执行完整的后处理管线
         * 
         * @param settings 后处理设置
         * @param input 输入渲染目标
         * @param output 输出渲染目标
         */
        void Execute(
            const ECS::PostProcessSettingsComponent& settings,
            std::shared_ptr<Nut::RenderTarget> input,
            std::shared_ptr<Nut::RenderTarget> output
        );

        /**
         * @brief 单独执行 Bloom 效果
         * 
         * 执行完整的 Bloom 管线：提取 -> 降采样模糊 -> 升采样合成
         * 
         * @param settings 后处理设置
         * @param sceneInput 场景颜色输入
         * @param emissionInput 自发光缓冲区输入（可选）
         * @param output 输出渲染目标
         */
        void ExecuteBloom(
            const ECS::PostProcessSettingsComponent& settings,
            std::shared_ptr<Nut::RenderTarget> sceneInput,
            std::shared_ptr<Nut::RenderTarget> emissionInput,
            std::shared_ptr<Nut::RenderTarget> output
        );

        /**
         * @brief 单独执行 Bloom 效果（简化版本）
         * 
         * @param settings 后处理设置
         */
        void ExecuteBloom(const ECS::PostProcessSettingsComponent& settings);

        /**
         * @brief 单独执行光束效果
         * 
         * @param settings 后处理设置
         */
        void ExecuteLightShafts(const ECS::PostProcessSettingsComponent& settings);

        /**
         * @brief 执行光束效果（带光源信息）
         * 
         * @param settings 后处理设置
         * @param lightWorldPos 光源世界位置
         * @param lightColor 光源颜色
         * @param lightIntensity 光源强度
         * @param sceneInput 场景颜色输入
         * @param shadowInput 阴影缓冲区输入（可选，用于遮挡）
         * @param output 输出渲染目标
         */
        void ExecuteLightShafts(
            const ECS::PostProcessSettingsComponent& settings,
            const glm::vec2& lightWorldPos,
            const ECS::Color& lightColor,
            float lightIntensity,
            std::shared_ptr<Nut::RenderTarget> sceneInput,
            std::shared_ptr<Nut::RenderTarget> shadowInput,
            std::shared_ptr<Nut::RenderTarget> output
        );

        /**
         * @brief 将世界坐标转换为屏幕空间 UV 坐标
         * 
         * @param worldPos 世界坐标位置
         * @return 屏幕空间 UV 坐标 [0, 1]
         */
        glm::vec2 WorldToScreenUV(const glm::vec2& worldPos) const;

        /**
         * @brief 设置阴影缓冲区（用于光束遮挡）
         * 
         * @param shadowBuffer 阴影缓冲区
         */
        void SetShadowBuffer(std::shared_ptr<Nut::RenderTarget> shadowBuffer);

        /**
         * @brief 获取阴影缓冲区
         * @return 阴影缓冲区指针
         */
        std::shared_ptr<Nut::RenderTarget> GetShadowBuffer() const { return m_shadowBuffer; }

        /**
         * @brief 获取光束效果缓冲区（用于调试）
         * @return 光束效果缓冲区指针
         */
        std::shared_ptr<Nut::RenderTarget> GetLightShaftBuffer() const { return m_lightShaftBuffer; }

        /**
         * @brief 获取雾效缓冲区（用于调试）
         * @return 雾效缓冲区指针
         */
        std::shared_ptr<Nut::RenderTarget> GetFogBuffer() const { return m_fogBuffer; }

        /**
         * @brief 单独执行雾效
         * 
         * @param settings 后处理设置
         */
        void ExecuteFog(const ECS::PostProcessSettingsComponent& settings);

        /**
         * @brief 设置雾效光源穿透数据
         * 
         * 设置影响雾效的光源列表，用于光源穿透效果。
         * 
         * @param lights 光源穿透数据列表
         * @param maxPenetration 最大穿透量 [0, 1]
         * Requirements: 11.4
         */
        void SetFogLights(const std::vector<ECS::FogLightData>& lights, float maxPenetration = 0.8f);

        /**
         * @brief 清除雾效光源穿透数据
         * 
         * 禁用光源穿透效果。
         */
        void ClearFogLights();

        /**
         * @brief 单独执行色调映射
         * 
         * @param settings 后处理设置
         */
        void ExecuteToneMapping(const ECS::PostProcessSettingsComponent& settings);

        /**
         * @brief 单独执行颜色分级
         * 
         * @param settings 后处理设置
         */
        void ExecuteColorGrading(const ECS::PostProcessSettingsComponent& settings);

        // ==================== 缓冲区访问 ====================

        /**
         * @brief 获取 Bloom 缓冲区（用于调试）
         * @return Bloom 缓冲区指针
         */
        std::shared_ptr<Nut::RenderTarget> GetBloomBuffer() const { return m_bloomBuffer; }

        /**
         * @brief 获取 Emission 缓冲区
         * @return Emission 缓冲区指针
         */
        std::shared_ptr<Nut::RenderTarget> GetEmissionBuffer() const { return m_emissionBuffer; }

        /**
         * @brief 获取后处理全局数据缓冲区
         * @return 后处理全局数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetGlobalBuffer() const { return m_globalBuffer; }

        /**
         * @brief 获取当前后处理设置
         * @return 后处理设置的常量引用
         */
        const ECS::PostProcessSettingsComponent& GetSettings() const { return m_settings; }

        // ==================== 缓冲区管理 ====================

        /**
         * @brief 设置渲染缓冲区配置
         * @param config 缓冲区配置
         */
        void SetBufferConfig(const RenderBufferConfig& config);

        /**
         * @brief 获取渲染缓冲区配置
         * @return 缓冲区配置的常量引用
         */
        const RenderBufferConfig& GetBufferConfig() const { return m_bufferConfig; }

        /**
         * @brief 调整缓冲区大小
         * 
         * 当窗口大小变化时调用。
         * 
         * @param width 新的宽度
         * @param height 新的高度
         */
        void ResizeBuffers(uint16_t width, uint16_t height);

        /**
         * @brief 检查系统是否已初始化
         * @return true 如果已初始化
         */
        bool IsInitialized() const { return m_initialized; }

        // ==================== 调试 ====================

        /**
         * @brief 设置调试模式
         * @param enable 是否启用调试模式
         */
        void SetDebugMode(bool enable) { m_debugMode = enable; }

        /**
         * @brief 获取调试模式状态
         * @return 是否处于调试模式
         */
        bool IsDebugMode() const { return m_debugMode; }

        /**
         * @brief 获取指定类型的中间缓冲区（用于调试可视化）
         * @param passType Pass 类型
         * @return 对应的渲染目标指针，如果不存在则返回 nullptr
         */
        std::shared_ptr<Nut::RenderTarget> GetIntermediateBuffer(PostProcessPassType passType) const;

    private:
        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateBuffers(EngineContext& engineCtx);

        /**
         * @brief 创建渲染目标
         * @param width 宽度
         * @param height 高度
         * @param format 纹理格式
         * @return 渲染目标的共享指针
         */
        std::shared_ptr<Nut::RenderTarget> CreateRenderTarget(
            uint16_t width,
            uint16_t height,
            wgpu::TextureFormat format
        );

        /**
         * @brief 创建 Bloom 降采样链
         * @param baseWidth 基础宽度
         * @param baseHeight 基础高度
         */
        void CreateBloomMipChain(uint16_t baseWidth, uint16_t baseHeight);

        /**
         * @brief 从场景获取后处理设置
         * @param scene 运行时场景
         */
        void UpdateSettingsFromScene(RuntimeScene* scene);

        /**
         * @brief 更新 GPU 全局数据缓冲区
         */
        void UpdateGlobalBuffer();

        /**
         * @brief 验证并钳制设置参数
         * @param settings 后处理设置（会被修改）
         */
        static void ValidateSettings(ECS::PostProcessSettingsComponent& settings);

        // ==================== Bloom 相关私有方法 ====================

        /**
         * @brief 创建 Bloom 渲染管线
         */
        void CreateBloomPipelines();

        /**
         * @brief 执行 Bloom 提取 Pass
         * 
         * 从场景颜色和 Emission Buffer 提取高亮区域
         * 
         * @param sceneInput 场景颜色输入
         * @param emissionInput 自发光缓冲区输入（可选）
         * @param output 输出到 Bloom 缓冲区
         * @param settings 后处理设置
         */
        void ExecuteBloomExtract(
            std::shared_ptr<Nut::RenderTarget> sceneInput,
            std::shared_ptr<Nut::RenderTarget> emissionInput,
            std::shared_ptr<Nut::RenderTarget> output,
            const ECS::PostProcessSettingsComponent& settings
        );

        /**
         * @brief 执行 Bloom 降采样模糊 Pass
         * 
         * 对 Bloom 缓冲区进行多级降采样模糊
         * 
         * @param iterations 模糊迭代次数
         */
        void ExecuteBloomDownsample(int iterations);

        /**
         * @brief 执行 Bloom 升采样合并 Pass
         * 
         * 将各级模糊结果升采样并合并
         * 
         * @param iterations 升采样迭代次数
         */
        void ExecuteBloomUpsample(int iterations);

        /**
         * @brief 执行 Bloom 合成 Pass
         * 
         * 将 Bloom 效果叠加到场景颜色
         * 
         * @param sceneInput 场景颜色输入
         * @param bloomInput Bloom 缓冲区输入
         * @param output 最终输出
         * @param settings 后处理设置
         */
        void ExecuteBloomComposite(
            std::shared_ptr<Nut::RenderTarget> sceneInput,
            std::shared_ptr<Nut::RenderTarget> bloomInput,
            std::shared_ptr<Nut::RenderTarget> output,
            const ECS::PostProcessSettingsComponent& settings
        );

        /**
         * @brief 执行单次高斯模糊 Pass
         * 
         * @param input 输入渲染目标
         * @param output 输出渲染目标
         * @param horizontal 是否为水平方向
         */
        void ExecuteGaussianBlurPass(
            std::shared_ptr<Nut::RenderTarget> input,
            std::shared_ptr<Nut::RenderTarget> output,
            bool horizontal
        );

    private:
        // ==================== 设置和状态 ====================
        
        ECS::PostProcessSettingsComponent m_settings;  ///< 当前后处理设置
        RenderBufferConfig m_bufferConfig;             ///< 缓冲区配置
        bool m_initialized = false;                    ///< 是否已初始化
        bool m_debugMode = false;                      ///< 调试模式标志
        bool m_settingsDirty = true;                   ///< 设置是否需要更新

        // ==================== GPU 资源 ====================
        
        std::shared_ptr<Nut::NutContext> m_nutContext; ///< Nut 图形上下文
        std::shared_ptr<Nut::Buffer> m_globalBuffer;   ///< 后处理全局数据缓冲区

        // ==================== 渲染缓冲区 ====================
        
        std::shared_ptr<Nut::RenderTarget> m_emissionBuffer;  ///< 自发光缓冲区
        std::shared_ptr<Nut::RenderTarget> m_bloomBuffer;     ///< Bloom 缓冲区
        std::shared_ptr<Nut::RenderTarget> m_tempBuffer;      ///< 临时缓冲区（用于 ping-pong）
        
        /// Bloom 降采样链（用于多级模糊）
        std::vector<std::shared_ptr<Nut::RenderTarget>> m_bloomMipChain;

        /// Bloom 升采样链（用于合并）
        std::vector<std::shared_ptr<Nut::RenderTarget>> m_bloomUpsampleChain;

        // ==================== Bloom 渲染管线 ====================
        
        std::shared_ptr<Nut::RenderPipeline> m_bloomExtractPipeline;     ///< Bloom 提取管线
        std::shared_ptr<Nut::RenderPipeline> m_bloomDownsamplePipeline;  ///< Bloom 降采样管线
        std::shared_ptr<Nut::RenderPipeline> m_bloomUpsamplePipeline;    ///< Bloom 升采样管线
        std::shared_ptr<Nut::RenderPipeline> m_bloomCompositePipeline;   ///< Bloom 合成管线
        std::shared_ptr<Nut::RenderPipeline> m_bloomBlurHPipeline;       ///< Bloom 水平模糊管线
        std::shared_ptr<Nut::RenderPipeline> m_bloomBlurVPipeline;       ///< Bloom 垂直模糊管线

        // ==================== Bloom 相关缓冲区 ====================
        
        std::shared_ptr<Nut::Buffer> m_bloomBlurParamsBuffer;  ///< Bloom 模糊参数缓冲区
        
        /// Bloom 模糊参数结构（与 WGSL 对应）
        struct BloomBlurParams
        {
            float texelSizeX;
            float texelSizeY;
            float directionX;
            float directionY;
        };

        // ==================== 光束效果相关 ====================
        
        std::shared_ptr<Nut::Buffer> m_lightShaftParamsBuffer;  ///< 光束参数缓冲区
        std::shared_ptr<Nut::RenderTarget> m_lightShaftBuffer;  ///< 光束效果缓冲区
        std::shared_ptr<Nut::RenderTarget> m_shadowBuffer;      ///< 阴影缓冲区（用于光束遮挡）
        std::shared_ptr<Nut::RenderPipeline> m_lightShaftPipeline;  ///< 光束渲染管线
        std::shared_ptr<Nut::RenderPipeline> m_lightShaftOccludedPipeline;  ///< 带遮挡的光束渲染管线
        std::shared_ptr<Nut::RenderPipeline> m_lightShaftCompositePipeline;  ///< 光束合成管线

        // ==================== 雾效相关 ====================
        
        std::shared_ptr<Nut::Buffer> m_fogParamsBuffer;  ///< 雾效参数缓冲区
        std::shared_ptr<Nut::RenderTarget> m_fogBuffer;  ///< 雾效缓冲区
        std::shared_ptr<Nut::RenderPipeline> m_fogPipeline;  ///< 雾效渲染管线
        std::shared_ptr<Nut::RenderPipeline> m_heightFogPipeline;  ///< 高度雾渲染管线
        std::shared_ptr<Nut::Buffer> m_fogLightParamsBuffer;  ///< 光源穿透参数缓冲区
        std::shared_ptr<Nut::Buffer> m_fogLightsBuffer;  ///< 光源穿透数据缓冲区
        std::shared_ptr<Nut::RenderPipeline> m_fogWithLightPipeline;  ///< 带光源穿透的雾效管线

        // ==================== 色调映射相关 ====================
        
        std::shared_ptr<Nut::Buffer> m_toneMappingParamsBuffer;  ///< 色调映射参数缓冲区
        std::shared_ptr<Nut::RenderTarget> m_toneMappingBuffer;  ///< 色调映射缓冲区
        std::shared_ptr<Nut::RenderPipeline> m_toneMappingPipeline;  ///< 色调映射渲染管线
        std::shared_ptr<Nut::RenderPipeline> m_colorAdjustmentsPipeline;  ///< 颜色调整渲染管线
        std::shared_ptr<Nut::RenderPipeline> m_lutPipeline;  ///< LUT 颜色分级渲染管线
        Nut::TextureAPtr m_lutTexture;  ///< LUT 纹理
        std::string m_currentLutPath;  ///< 当前加载的 LUT 路径

        // ==================== 采样器 ====================
        
        std::shared_ptr<Nut::Sampler> m_linearSampler;  ///< 线性采样器（用于模糊）

        // ==================== 缓冲区尺寸 ====================
        
        uint16_t m_currentWidth = 0;   ///< 当前缓冲区宽度
        uint16_t m_currentHeight = 0;  ///< 当前缓冲区高度

        // ==================== 中间缓冲区映射（用于调试） ====================
        
        std::unordered_map<PostProcessPassType, std::shared_ptr<Nut::RenderTarget>> m_intermediateBuffers;
    };
}

#endif // POST_PROCESS_SYSTEM_H

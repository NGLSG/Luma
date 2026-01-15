#ifndef QUALITY_MANAGER_H
#define QUALITY_MANAGER_H

/**
 * @file QualityManager.h
 * @brief 质量管理系统
 * 
 * 负责管理渲染质量等级、动态质量调整和各系统的质量参数同步。
 * 实现单例模式，提供全局质量控制接口。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 9.1, 9.2, 9.3, 9.5
 */

#include "../Components/QualitySettingsComponent.h"
#include "../Components/LightingTypes.h"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

// Forward declarations
class RuntimeScene;
namespace Systems
{
    class LightingSystem;
    class ShadowRenderer;
    class PostProcessSystem;
}

namespace Systems
{
    /**
     * @brief 帧率采样数据
     * 
     * 用于自动质量调整的帧率监控
     */
    struct FrameRateSample
    {
        float frameRate;                                    ///< 帧率值
        std::chrono::steady_clock::time_point timestamp;    ///< 采样时间戳
    };

    /**
     * @brief 质量变化回调类型
     */
    using QualityChangeCallback = std::function<void(ECS::QualityLevel oldLevel, ECS::QualityLevel newLevel)>;

    /**
     * @brief 质量管理器
     * 
     * 该类负责：
     * - 管理全局质量等级
     * - 提供质量等级预设配置
     * - 实现自动质量调整（基于帧率）
     * - 同步质量设置到各渲染系统
     * 
     * Requirements: 9.1, 9.2, 9.3, 9.5
     */
    class QualityManager
    {
    public:
        /// 帧率采样窗口大小（用于平滑帧率计算）
        static constexpr int FRAME_RATE_SAMPLE_COUNT = 30;
        
        /// 质量调整冷却时间（秒）
        static constexpr float QUALITY_ADJUST_COOLDOWN = 2.0f;
        
        /// 最小帧率采样间隔（秒）
        static constexpr float MIN_SAMPLE_INTERVAL = 0.033f;

        /**
         * @brief 获取单例实例
         * @return QualityManager 单例引用
         */
        static QualityManager& GetInstance();

        /**
         * @brief 销毁单例实例
         * 
         * 在程序退出时调用以清理资源
         */
        static void DestroyInstance();

        // ==================== 质量等级管理 ====================

        /**
         * @brief 设置质量等级
         * 
         * 设置新的质量等级并应用预设配置。
         * 如果等级发生变化，会触发质量变化回调。
         * 
         * @param level 新的质量等级
         * Requirements: 9.1, 9.3
         */
        void SetQualityLevel(ECS::QualityLevel level);

        /**
         * @brief 获取当前质量等级
         * @return 当前质量等级
         */
        ECS::QualityLevel GetQualityLevel() const { return m_settings.level; }

        /**
         * @brief 获取当前质量设置
         * @return 质量设置的常量引用
         */
        const ECS::QualitySettingsComponent& GetSettings() const { return m_settings; }

        /**
         * @brief 应用自定义设置
         * 
         * 应用自定义质量设置，会将质量等级设置为 Custom。
         * 
         * @param settings 自定义质量设置
         * Requirements: 9.4
         */
        void ApplyCustomSettings(const ECS::QualitySettingsComponent& settings);

        /**
         * @brief 获取指定质量等级的预设配置
         * 
         * @param level 质量等级
         * @return 预设的质量设置
         * Requirements: 9.2
         */
        static ECS::QualitySettingsComponent GetPreset(ECS::QualityLevel level);

        // ==================== 自动质量调整 ====================

        /**
         * @brief 启用/禁用自动质量调整
         * 
         * @param enable 是否启用
         * Requirements: 9.5
         */
        void SetAutoQualityEnabled(bool enable);

        /**
         * @brief 获取自动质量调整是否启用
         * @return 是否启用
         */
        bool IsAutoQualityEnabled() const { return m_settings.enableAutoQuality; }

        /**
         * @brief 设置目标帧率
         * 
         * @param targetFps 目标帧率
         * Requirements: 9.5
         */
        void SetTargetFrameRate(float targetFps);

        /**
         * @brief 获取目标帧率
         * @return 目标帧率
         */
        float GetTargetFrameRate() const { return m_settings.targetFrameRate; }

        /**
         * @brief 设置质量调整阈值
         * 
         * 当帧率偏离目标值超过此阈值时触发质量调整。
         * 
         * @param threshold 帧率偏差阈值
         * Requirements: 9.5
         */
        void SetQualityAdjustThreshold(float threshold);

        /**
         * @brief 获取质量调整阈值
         * @return 质量调整阈值
         */
        float GetQualityAdjustThreshold() const { return m_settings.qualityAdjustThreshold; }

        /**
         * @brief 更新自动质量调整
         * 
         * 根据当前帧率自动调整质量等级。
         * 应在每帧调用此方法。
         * 
         * @param currentFrameRate 当前帧率
         * Requirements: 9.5
         */
        void UpdateAutoQuality(float currentFrameRate);

        /**
         * @brief 获取平均帧率
         * 
         * 返回最近采样窗口内的平均帧率。
         * 
         * @return 平均帧率
         */
        float GetAverageFrameRate() const;

        /**
         * @brief 获取帧率稳定性
         * 
         * 返回帧率的标准差，用于判断帧率是否稳定。
         * 
         * @return 帧率标准差
         */
        float GetFrameRateStability() const;

        // ==================== 系统集成 ====================

        /**
         * @brief 设置 LightingSystem 引用
         * 
         * @param lightingSystem LightingSystem 指针
         * Requirements: 9.2, 9.3
         */
        void SetLightingSystem(LightingSystem* lightingSystem);

        /**
         * @brief 设置 ShadowRenderer 引用
         * 
         * @param shadowRenderer ShadowRenderer 指针
         * Requirements: 9.2, 9.3
         */
        void SetShadowRenderer(ShadowRenderer* shadowRenderer);

        /**
         * @brief 设置 PostProcessSystem 引用
         * 
         * @param postProcessSystem PostProcessSystem 指针
         * Requirements: 9.2, 9.3
         */
        void SetPostProcessSystem(PostProcessSystem* postProcessSystem);

        /**
         * @brief 应用质量设置到所有已连接的系统
         * 
         * 将当前质量设置同步到 LightingSystem、ShadowRenderer 和 PostProcessSystem。
         * 
         * Requirements: 9.2, 9.3
         */
        void ApplySettingsToSystems();

        // ==================== 回调管理 ====================

        /**
         * @brief 注册质量变化回调
         * 
         * @param callback 回调函数
         * @return 回调 ID，用于取消注册
         */
        int RegisterQualityChangeCallback(QualityChangeCallback callback);

        /**
         * @brief 取消注册质量变化回调
         * 
         * @param callbackId 回调 ID
         */
        void UnregisterQualityChangeCallback(int callbackId);

        // ==================== 调试 ====================

        /**
         * @brief 获取上次质量调整的时间
         * @return 上次调整距今的秒数
         */
        float GetTimeSinceLastAdjustment() const;

        /**
         * @brief 获取质量调整次数
         * @return 调整次数
         */
        int GetAdjustmentCount() const { return m_adjustmentCount; }

        /**
         * @brief 重置统计数据
         */
        void ResetStatistics();

    private:
        /**
         * @brief 私有构造函数（单例模式）
         */
        QualityManager();

        /**
         * @brief 私有析构函数
         */
        ~QualityManager() = default;

        // 禁用拷贝和移动
        QualityManager(const QualityManager&) = delete;
        QualityManager& operator=(const QualityManager&) = delete;
        QualityManager(QualityManager&&) = delete;
        QualityManager& operator=(QualityManager&&) = delete;

        /**
         * @brief 添加帧率采样
         * 
         * @param frameRate 帧率值
         */
        void AddFrameRateSample(float frameRate);

        /**
         * @brief 检查是否应该降低质量
         * 
         * @param avgFrameRate 平均帧率
         * @return true 如果应该降低质量
         */
        bool ShouldDecreaseQuality(float avgFrameRate) const;

        /**
         * @brief 检查是否应该提高质量
         * 
         * @param avgFrameRate 平均帧率
         * @return true 如果应该提高质量
         */
        bool ShouldIncreaseQuality(float avgFrameRate) const;

        /**
         * @brief 降低质量等级
         * 
         * @return true 如果成功降低
         */
        bool DecreaseQualityLevel();

        /**
         * @brief 提高质量等级
         * 
         * @return true 如果成功提高
         */
        bool IncreaseQualityLevel();

        /**
         * @brief 触发质量变化回调
         * 
         * @param oldLevel 旧质量等级
         * @param newLevel 新质量等级
         */
        void NotifyQualityChange(ECS::QualityLevel oldLevel, ECS::QualityLevel newLevel);

        /**
         * @brief 应用设置到 LightingSystem
         */
        void ApplyToLightingSystem();

        /**
         * @brief 应用设置到 ShadowRenderer
         */
        void ApplyToShadowRenderer();

        /**
         * @brief 应用设置到 PostProcessSystem
         */
        void ApplyToPostProcessSystem();

    private:
        static QualityManager* s_instance;                      ///< 单例实例

        ECS::QualitySettingsComponent m_settings;               ///< 当前质量设置

        // 系统引用
        LightingSystem* m_lightingSystem = nullptr;             ///< LightingSystem 引用
        ShadowRenderer* m_shadowRenderer = nullptr;             ///< ShadowRenderer 引用
        PostProcessSystem* m_postProcessSystem = nullptr;       ///< PostProcessSystem 引用

        // 帧率监控
        std::vector<FrameRateSample> m_frameRateSamples;        ///< 帧率采样数据
        std::chrono::steady_clock::time_point m_lastSampleTime; ///< 上次采样时间
        std::chrono::steady_clock::time_point m_lastAdjustTime; ///< 上次质量调整时间

        // 回调管理
        std::vector<std::pair<int, QualityChangeCallback>> m_callbacks;  ///< 质量变化回调列表
        int m_nextCallbackId = 0;                               ///< 下一个回调 ID

        // 统计数据
        int m_adjustmentCount = 0;                              ///< 质量调整次数
    };
}

#endif // QUALITY_MANAGER_H

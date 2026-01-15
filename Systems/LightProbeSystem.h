#ifndef LIGHT_PROBE_SYSTEM_H
#define LIGHT_PROBE_SYSTEM_H

/**
 * @file LightProbeSystem.h
 * @brief 光照探针系统
 * 
 * 负责管理光照探针、计算间接光照采样和插值。
 * 支持探针网格生成、烘焙模式和实时更新模式。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Components/LightProbeComponent.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>

namespace Systems
{
    /**
     * @brief 光照探针信息结构（用于管理和查询）
     */
    struct LightProbeInfo
    {
        ECS::LightProbeData data;       ///< 光照探针数据
        glm::vec2 position;             ///< 世界坐标位置
        float influenceRadius;          ///< 影响半径
        bool isBaked;                   ///< 是否已烘焙
        bool needsUpdate;               ///< 是否需要更新
    };

    /**
     * @brief 光照探针系统
     * 
     * 该系统继承自 ISystem 接口，负责：
     * - 收集场景中的所有光照探针
     * - 生成探针网格
     * - 烘焙静态光照探针数据
     * - 实时更新动态光照探针
     * - 在指定位置插值间接光照
     */
    class LightProbeSystem : public ISystem
    {
    public:
        /// 每帧最大光照探针数量
        static constexpr uint32_t MAX_LIGHT_PROBES = 256;

        /// 默认更新频率（秒）
        static constexpr float DEFAULT_UPDATE_FREQUENCY = 0.1f;

        /**
         * @brief 默认构造函数
         */
        LightProbeSystem();

        /**
         * @brief 析构函数
         */
        ~LightProbeSystem() override = default;

        /**
         * @brief 系统创建时调用
         * 
         * 初始化光照探针系统所需的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * 
         * 收集光照探针、执行实时更新。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * 
         * 清理光照探针系统占用的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 探针网格生成 ====================

        /**
         * @brief 生成探针网格
         * 
         * 根据配置自动生成光照探针网格。
         * 
         * @param scene 运行时场景
         * @param config 探针网格配置
         */
        void GenerateProbeGrid(RuntimeScene* scene, const ECS::LightProbeGridConfig& config);

        /**
         * @brief 清除生成的探针网格
         */
        void ClearProbeGrid();

        // ==================== 探针烘焙 ====================

        /**
         * @brief 烘焙所有探针
         * 
         * 预计算所有光照探针的间接光照数据。
         * 
         * @param scene 运行时场景
         */
        void BakeAllProbes(RuntimeScene* scene);

        /**
         * @brief 烘焙单个探针
         * 
         * @param scene 运行时场景
         * @param probeIndex 探针索引
         */
        void BakeProbe(RuntimeScene* scene, uint32_t probeIndex);

        /**
         * @brief 检查是否所有探针都已烘焙
         * @return true 如果所有探针都已烘焙
         */
        bool AreAllProbesBaked() const;

        // ==================== 间接光照插值 ====================

        /**
         * @brief 在指定位置插值间接光照
         * 
         * 根据周围探针的采样值计算指定位置的间接光照。
         * 使用距离加权插值算法。
         * 
         * @param position 世界坐标位置
         * @return 间接光照颜色 (RGB)
         */
        glm::vec3 InterpolateIndirectLightAt(const glm::vec2& position) const;

        /**
         * @brief 在指定位置插值间接光照（带强度）
         * 
         * @param position 世界坐标位置
         * @param outIntensity 输出的间接光照强度
         * @return 间接光照颜色 (RGB)
         */
        glm::vec3 InterpolateIndirectLightAt(const glm::vec2& position, float& outIntensity) const;

        /**
         * @brief 获取影响指定位置的探针列表
         * 
         * @param position 世界坐标位置
         * @return 影响该位置的探针索引列表
         */
        std::vector<uint32_t> GetProbesAffecting(const glm::vec2& position) const;

        // ==================== 探针数据访问 ====================

        /**
         * @brief 获取所有探针数据列表
         * @return 探针数据的常量引用
         */
        const std::vector<ECS::LightProbeData>& GetAllProbeData() const { return m_probeData; }

        /**
         * @brief 获取当前探针数量
         * @return 探针数量
         */
        uint32_t GetProbeCount() const { return static_cast<uint32_t>(m_probeData.size()); }

        /**
         * @brief 获取探针数据缓冲区
         * @return 探针数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetProbeBuffer() const { return m_probeBuffer; }

        /**
         * @brief 获取单例实例
         */
        static LightProbeSystem* GetInstance() { return s_instance; }

        // ==================== 实时更新控制 ====================

        /**
         * @brief 设置实时更新频率
         * @param frequency 更新频率（秒）
         */
        void SetUpdateFrequency(float frequency) { m_updateFrequency = frequency; }

        /**
         * @brief 获取实时更新频率
         * @return 更新频率（秒）
         */
        float GetUpdateFrequency() const { return m_updateFrequency; }

        /**
         * @brief 设置是否启用实时更新
         * @param enable 是否启用
         */
        void SetRealtimeUpdateEnabled(bool enable) { m_realtimeUpdateEnabled = enable; }

        /**
         * @brief 获取是否启用实时更新
         * @return 是否启用实时更新
         */
        bool IsRealtimeUpdateEnabled() const { return m_realtimeUpdateEnabled; }

        /**
         * @brief 标记所有探针需要更新
         */
        void MarkAllProbesDirty();

        // ==================== 静态工具函数（用于测试） ====================

        /**
         * @brief 计算两点之间的距离
         * @param a 点 A
         * @param b 点 B
         * @return 距离
         */
        static float CalculateDistance(const glm::vec2& a, const glm::vec2& b);

        /**
         * @brief 计算距离权重
         * 
         * 使用平方反比衰减计算权重。
         * 
         * @param distance 距离
         * @param influenceRadius 影响半径
         * @return 权重值 [0, 1]
         */
        static float CalculateDistanceWeight(float distance, float influenceRadius);

        /**
         * @brief 双线性插值
         * 
         * 在四个探针之间进行双线性插值。
         * 
         * @param topLeft 左上探针值
         * @param topRight 右上探针值
         * @param bottomLeft 左下探针值
         * @param bottomRight 右下探针值
         * @param tx X 方向插值因子 [0, 1]
         * @param ty Y 方向插值因子 [0, 1]
         * @return 插值结果
         */
        static glm::vec3 BilinearInterpolate(
            const glm::vec3& topLeft,
            const glm::vec3& topRight,
            const glm::vec3& bottomLeft,
            const glm::vec3& bottomRight,
            float tx,
            float ty);

        /**
         * @brief 三角形重心插值
         * 
         * 在三个探针之间进行重心插值。
         * 
         * @param v0 探针 0 值
         * @param v1 探针 1 值
         * @param v2 探针 2 值
         * @param barycentricCoords 重心坐标 (u, v, w)
         * @return 插值结果
         */
        static glm::vec3 BarycentricInterpolate(
            const glm::vec3& v0,
            const glm::vec3& v1,
            const glm::vec3& v2,
            const glm::vec3& barycentricCoords);

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

    private:
        /**
         * @brief 收集场景中的所有光照探针
         * @param scene 运行时场景
         */
        void CollectLightProbes(RuntimeScene* scene);

        /**
         * @brief 更新实时探针
         * @param scene 运行时场景
         * @param deltaTime 时间间隔
         */
        void UpdateRealtimeProbes(RuntimeScene* scene, float deltaTime);

        /**
         * @brief 采样探针位置的间接光照
         * @param scene 运行时场景
         * @param position 探针位置
         * @param outColor 输出颜色
         * @param outIntensity 输出强度
         */
        void SampleIndirectLightAtProbe(
            RuntimeScene* scene,
            const glm::vec2& position,
            glm::vec3& outColor,
            float& outIntensity);

        /**
         * @brief 更新 GPU 探针缓冲区
         */
        void UpdateProbeBuffer();

        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateBuffers(EngineContext& engineCtx);

    private:
        std::vector<LightProbeInfo> m_probes;                 ///< 所有收集到的探针信息
        std::vector<ECS::LightProbeData> m_probeData;         ///< 探针数据（用于 GPU）

        std::shared_ptr<Nut::Buffer> m_probeBuffer;           ///< GPU 探针数据缓冲区

        ECS::LightProbeGridConfig m_gridConfig;               ///< 当前网格配置
        bool m_hasGeneratedGrid = false;                      ///< 是否已生成网格

        float m_updateFrequency = DEFAULT_UPDATE_FREQUENCY;   ///< 实时更新频率
        float m_timeSinceLastUpdate = 0.0f;                   ///< 距上次更新的时间
        bool m_realtimeUpdateEnabled = true;                  ///< 是否启用实时更新

        bool m_debugMode = false;                             ///< 调试模式标志
        bool m_buffersCreated = false;                        ///< 缓冲区是否已创建

        EngineContext* m_engineCtx = nullptr;                 ///< 引擎上下文缓存

        static LightProbeSystem* s_instance;                  ///< 单例实例
    };
}

#endif // LIGHT_PROBE_SYSTEM_H

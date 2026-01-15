#ifndef AREA_LIGHT_SYSTEM_H
#define AREA_LIGHT_SYSTEM_H

/**
 * @file AreaLightSystem.h
 * @brief 面光源系统
 * 
 * 负责管理面光源、计算面光源光照贡献。
 * 与现有的 LightingSystem 集成。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 1.1, 1.2, 1.4, 1.6
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Components/AreaLightComponent.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>

namespace Systems
{
    /**
     * @brief 面光源信息结构（用于排序和剔除）
     */
    struct AreaLightInfo
    {
        ECS::AreaLightData data;       ///< 面光源数据
        int priority;                   ///< 优先级
        float distanceToCamera;         ///< 到相机的距离
    };

    /**
     * @brief 面光源边界框
     * 
     * 用于视锥剔除的 AABB 边界框
     */
    struct AreaLightBounds
    {
        float minX;
        float minY;
        float maxX;
        float maxY;

        /**
         * @brief 检查两个边界框是否相交
         */
        bool Intersects(const AreaLightBounds& other) const
        {
            return !(maxX < other.minX || minX > other.maxX ||
                     maxY < other.minY || minY > other.maxY);
        }
    };

    /**
     * @brief 面光源系统
     * 
     * 该系统继承自 ISystem 接口，负责：
     * - 收集场景中的所有面光源
     * - 执行视锥剔除
     * - 按优先级排序面光源
     * - 计算面光源光照贡献
     * - 将面光源转换为等效点光源（用于近似渲染）
     */
    class AreaLightSystem : public ISystem
    {
    public:
        /// 每帧最大面光源数量
        static constexpr uint32_t MAX_AREA_LIGHTS_PER_FRAME = 64;

        /// 每个面光源的最大采样点数量（用于转换为点光源）
        static constexpr uint32_t MAX_SAMPLES_PER_AREA_LIGHT = 16;

        /**
         * @brief 默认构造函数
         */
        AreaLightSystem();

        /**
         * @brief 析构函数
         */
        ~AreaLightSystem() override = default;

        /**
         * @brief 系统创建时调用
         * 
         * 初始化面光源系统所需的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * 
         * 收集面光源、执行剔除、排序并更新数据。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * 
         * 清理面光源系统占用的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 面光源数据访问 ====================

        /**
         * @brief 获取可见面光源列表
         * @return 可见面光源数据的常量引用
         */
        const std::vector<ECS::AreaLightData>& GetVisibleAreaLights() const { return m_visibleAreaLights; }

        /**
         * @brief 获取当前面光源数量
         * @return 可见面光源数量
         */
        uint32_t GetAreaLightCount() const { return static_cast<uint32_t>(m_visibleAreaLights.size()); }

        /**
         * @brief 获取面光源数据缓冲区
         * @return 面光源数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetAreaLightBuffer() const { return m_areaLightBuffer; }

        // ==================== 面光源转换 ====================

        /**
         * @brief 将面光源转换为等效的多点光源（用于近似渲染）
         * 
         * 通过在面光源表面采样多个点，将面光源近似为多个点光源。
         * 这种方法可以在不支持面光源的渲染管线中实现近似效果。
         * 
         * @param areaLight 面光源数据
         * @param sampleCount 采样点数量（建议 4-16）
         * @return 等效的点光源列表
         */
        static std::vector<ECS::LightData> ConvertToPointLights(
            const ECS::AreaLightData& areaLight, 
            int sampleCount);

        /**
         * @brief 获取所有面光源转换后的点光源列表
         * @param sampleCount 每个面光源的采样点数量
         * @return 所有等效点光源列表
         */
        std::vector<ECS::LightData> GetConvertedPointLights(int sampleCount = 4) const;

        // ==================== 光照计算 ====================

        /**
         * @brief 计算矩形面光源在指定点的光照贡献
         * 
         * @param areaLight 面光源数据
         * @param targetPosition 目标点位置
         * @return 光照贡献值 [0, ∞)
         */
        static float CalculateRectangleLightContribution(
            const ECS::AreaLightData& areaLight,
            const glm::vec2& targetPosition);

        /**
         * @brief 计算圆形面光源在指定点的光照贡献
         * 
         * @param areaLight 面光源数据
         * @param targetPosition 目标点位置
         * @return 光照贡献值 [0, ∞)
         */
        static float CalculateCircleLightContribution(
            const ECS::AreaLightData& areaLight,
            const glm::vec2& targetPosition);

        /**
         * @brief 计算面光源在指定点的光照贡献（自动选择形状）
         * 
         * @param areaLight 面光源数据
         * @param targetPosition 目标点位置
         * @return 光照贡献值 [0, ∞)
         */
        static float CalculateAreaLightContribution(
            const ECS::AreaLightData& areaLight,
            const glm::vec2& targetPosition);

        /**
         * @brief 计算面光源在指定点的光照颜色贡献
         * 
         * @param areaLight 面光源数据
         * @param targetPosition 目标点位置
         * @param spriteLayerMask 精灵的光照层掩码
         * @return 光照颜色贡献 (RGB)
         */
        static glm::vec3 CalculateAreaLightColorContribution(
            const ECS::AreaLightData& areaLight,
            const glm::vec2& targetPosition,
            uint32_t spriteLayerMask);

        // ==================== 静态工具函数（用于测试） ====================

        /**
         * @brief 计算面光源的边界框
         * @param areaLight 面光源数据
         * @return 面光源边界框
         */
        static AreaLightBounds CalculateAreaLightBounds(const ECS::AreaLightData& areaLight);

        /**
         * @brief 检查面光源是否在视锥内
         * @param lightBounds 面光源边界框
         * @param viewBounds 视锥边界框
         * @return true 如果面光源在视锥内
         */
        static bool IsAreaLightInView(const AreaLightBounds& lightBounds, const AreaLightBounds& viewBounds);

        /**
         * @brief 按优先级和距离排序面光源
         * @param lights 面光源信息列表
         */
        static void SortAreaLightsByPriority(std::vector<AreaLightInfo>& lights);

        /**
         * @brief 限制面光源数量
         * @param lights 面光源信息列表
         * @param maxCount 最大数量
         */
        static void LimitAreaLightCount(std::vector<AreaLightInfo>& lights, uint32_t maxCount);

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
         * @brief 收集场景中的所有面光源
         * @param scene 运行时场景
         */
        void CollectAreaLights(RuntimeScene* scene);

        /**
         * @brief 执行视锥剔除
         * @param cameraPosition 相机位置
         * @param viewWidth 视图宽度
         * @param viewHeight 视图高度
         */
        void CullAreaLights(const glm::vec2& cameraPosition, float viewWidth, float viewHeight);

        /**
         * @brief 更新 GPU 面光源缓冲区
         */
        void UpdateAreaLightBuffer();

        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateBuffers(EngineContext& engineCtx);

    private:
        std::vector<AreaLightInfo> m_allAreaLights;              ///< 所有收集到的面光源
        std::vector<ECS::AreaLightData> m_visibleAreaLights;     ///< 可见面光源数据

        std::shared_ptr<Nut::Buffer> m_areaLightBuffer;          ///< GPU 面光源数据缓冲区

        bool m_debugMode = false;                                ///< 调试模式标志
        bool m_buffersCreated = false;                           ///< 缓冲区是否已创建
        bool m_isDirty = true;                                   ///< 数据是否需要更新
        bool m_connectedToLightingSystem = false;                ///< 是否已连接到 LightingSystem
    };
}

#endif // AREA_LIGHT_SYSTEM_H

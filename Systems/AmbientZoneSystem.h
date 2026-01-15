#ifndef AMBIENT_ZONE_SYSTEM_H
#define AMBIENT_ZONE_SYSTEM_H

/**
 * @file AmbientZoneSystem.h
 * @brief 环境光区域系统
 * 
 * 负责管理环境光区域、计算区域内的环境光颜色。
 * 支持区域形状检测、边缘柔和度过渡、渐变颜色插值和多区域优先级混合。
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 2.1, 2.3, 2.4, 2.5, 2.6
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Components/AmbientZoneComponent.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>

namespace Systems
{
    /**
     * @brief 环境光区域信息结构（用于排序和查询）
     */
    struct AmbientZoneInfo
    {
        ECS::AmbientZoneData data;     ///< 环境光区域数据
        int priority;                   ///< 优先级
        float distanceToCamera;         ///< 到相机的距离
    };

    /**
     * @brief 环境光区域边界框
     * 
     * 用于空间查询的 AABB 边界框
     */
    struct AmbientZoneBounds
    {
        float minX;
        float minY;
        float maxX;
        float maxY;

        /**
         * @brief 检查两个边界框是否相交
         */
        bool Intersects(const AmbientZoneBounds& other) const
        {
            return !(maxX < other.minX || minX > other.maxX ||
                     maxY < other.minY || minY > other.maxY);
        }

        /**
         * @brief 检查点是否在边界框内
         */
        bool Contains(const glm::vec2& point) const
        {
            return point.x >= minX && point.x <= maxX &&
                   point.y >= minY && point.y <= maxY;
        }
    };

    /**
     * @brief 环境光区域系统
     * 
     * 该系统继承自 ISystem 接口，负责：
     * - 收集场景中的所有环境光区域
     * - 执行空间查询，找出影响指定位置的区域
     * - 计算点在区域内的检测
     * - 实现边缘柔和度过渡
     * - 实现渐变颜色插值
     * - 实现多区域优先级混合
     */
    class AmbientZoneSystem : public ISystem
    {
    public:
        /// 每帧最大环境光区域数量
        static constexpr uint32_t MAX_AMBIENT_ZONES_PER_FRAME = 32;

        /**
         * @brief 默认构造函数
         */
        AmbientZoneSystem();

        /**
         * @brief 析构函数
         */
        ~AmbientZoneSystem() override = default;

        /**
         * @brief 系统创建时调用
         * 
         * 初始化环境光区域系统所需的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * 
         * 收集环境光区域、更新数据。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * 
         * 清理环境光区域系统占用的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 环境光计算 ====================

        /**
         * @brief 计算指定位置的环境光颜色
         * 
         * 根据位置找出所有影响该点的环境光区域，
         * 按优先级混合计算最终的环境光颜色。
         * 
         * @param position 世界坐标位置
         * @return 环境光颜色 (RGBA)
         */
        ECS::Color CalculateAmbientAt(const glm::vec2& position) const;

        /**
         * @brief 获取影响指定位置的区域列表
         * 
         * @param position 世界坐标位置
         * @return 影响该位置的环境光区域数据列表
         */
        std::vector<const ECS::AmbientZoneData*> GetZonesAt(const glm::vec2& position) const;

        // ==================== 区域数据访问 ====================

        /**
         * @brief 获取所有环境光区域列表
         * @return 环境光区域数据的常量引用
         */
        const std::vector<ECS::AmbientZoneData>& GetAllZones() const { return m_allZoneData; }

        /**
         * @brief 获取当前环境光区域数量
         * @return 环境光区域数量
         */
        uint32_t GetZoneCount() const { return static_cast<uint32_t>(m_allZoneData.size()); }

        /**
         * @brief 获取环境光区域数据缓冲区
         * @return 环境光区域数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetAmbientZoneBuffer() const { return m_ambientZoneBuffer; }

        /**
         * @brief 获取环境光区域全局数据缓冲区
         * @return 环境光区域全局数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetAmbientZoneGlobalBuffer() const { return m_ambientZoneGlobalBuffer; }

        // ==================== 静态工具函数 ====================

        /**
         * @brief 检查点是否在矩形区域内
         * 
         * @param position 目标点位置
         * @param zonePosition 区域中心位置
         * @param width 区域宽度
         * @param height 区域高度
         * @return true 如果点在区域内
         */
        static bool IsPointInRectangle(
            const glm::vec2& position,
            const glm::vec2& zonePosition,
            float width,
            float height);

        /**
         * @brief 检查点是否在圆形区域内
         * 
         * @param position 目标点位置
         * @param zonePosition 区域中心位置
         * @param radius 区域半径（使用 width/2）
         * @return true 如果点在区域内
         */
        static bool IsPointInCircle(
            const glm::vec2& position,
            const glm::vec2& zonePosition,
            float radius);

        /**
         * @brief 检查点是否在区域内（自动选择形状）
         * 
         * @param position 目标点位置
         * @param zone 环境光区域数据
         * @return true 如果点在区域内
         */
        static bool IsPointInZone(
            const glm::vec2& position,
            const ECS::AmbientZoneData& zone);

        /**
         * @brief 计算点到矩形区域边缘的距离因子
         * 
         * 返回值范围 [0, 1]，0 表示在边缘，1 表示在中心
         * 
         * @param position 目标点位置
         * @param zonePosition 区域中心位置
         * @param width 区域宽度
         * @param height 区域高度
         * @param edgeSoftness 边缘柔和度
         * @return 距离因子 [0, 1]
         */
        static float CalculateRectangleEdgeFactor(
            const glm::vec2& position,
            const glm::vec2& zonePosition,
            float width,
            float height,
            float edgeSoftness);

        /**
         * @brief 计算点到圆形区域边缘的距离因子
         * 
         * 返回值范围 [0, 1]，0 表示在边缘，1 表示在中心
         * 
         * @param position 目标点位置
         * @param zonePosition 区域中心位置
         * @param radius 区域半径
         * @param edgeSoftness 边缘柔和度
         * @return 距离因子 [0, 1]
         */
        static float CalculateCircleEdgeFactor(
            const glm::vec2& position,
            const glm::vec2& zonePosition,
            float radius,
            float edgeSoftness);

        /**
         * @brief 计算点在区域内的边缘因子（自动选择形状）
         * 
         * @param position 目标点位置
         * @param zone 环境光区域数据
         * @return 边缘因子 [0, 1]
         */
        static float CalculateEdgeFactor(
            const glm::vec2& position,
            const ECS::AmbientZoneData& zone);

        /**
         * @brief 计算区域内的渐变颜色
         * 
         * 根据渐变模式和位置计算颜色插值。
         * 
         * @param zone 环境光区域数据
         * @param localPosition 相对于区域中心的局部坐标
         * @return 渐变后的颜色
         */
        static ECS::Color CalculateGradientColor(
            const ECS::AmbientZoneData& zone,
            const glm::vec2& localPosition);

        /**
         * @brief 计算环境光区域的边界框
         * @param zone 环境光区域数据
         * @return 环境光区域边界框
         */
        static AmbientZoneBounds CalculateZoneBounds(const ECS::AmbientZoneData& zone);

        /**
         * @brief 按优先级排序环境光区域
         * @param zones 环境光区域信息列表
         */
        static void SortZonesByPriority(std::vector<AmbientZoneInfo>& zones);

        /**
         * @brief 混合多个环境光区域的颜色
         * 
         * 按优先级和权重混合多个区域的环境光颜色。
         * 
         * @param zones 影响该点的区域列表
         * @param position 目标点位置
         * @return 混合后的环境光颜色
         */
        static ECS::Color BlendZoneColors(
            const std::vector<const ECS::AmbientZoneData*>& zones,
            const glm::vec2& position);

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
         * @brief 收集场景中的所有环境光区域
         * @param scene 运行时场景
         */
        void CollectAmbientZones(RuntimeScene* scene);

        /**
         * @brief 更新 GPU 环境光区域缓冲区
         */
        void UpdateAmbientZoneBuffer();

        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateBuffers(EngineContext& engineCtx);

    private:
        std::vector<AmbientZoneInfo> m_allZones;              ///< 所有收集到的环境光区域
        std::vector<ECS::AmbientZoneData> m_allZoneData;      ///< 所有环境光区域数据

        std::shared_ptr<Nut::Buffer> m_ambientZoneBuffer;     ///< GPU 环境光区域数据缓冲区
        std::shared_ptr<Nut::Buffer> m_ambientZoneGlobalBuffer; ///< GPU 环境光区域全局数据缓冲区

        bool m_debugMode = false;                             ///< 调试模式标志
        bool m_buffersCreated = false;                        ///< 缓冲区是否已创建
    };
}

#endif // AMBIENT_ZONE_SYSTEM_H

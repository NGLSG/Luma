#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include "ISystem.h"
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include "../Data/RaycastResult.h"
#include <memory>

#include "Transform.h"
#ifndef B2_NULL_INDEX
#define B2_NULL_INDEX ( -1 )
#endif
namespace Systems
{
    /**
     * @brief 表示一对实体，通常用于物理碰撞或触发事件。
     */
    struct EntityPair
    {
        entt::entity entityA; ///< 第一个实体。
        entt::entity entityB; ///< 第二个实体。

        /**
         * @brief 比较两个 EntityPair 是否相等，不考虑实体顺序。
         * @param other 另一个 EntityPair 对象。
         * @return 如果两个 EntityPair 包含相同的实体（无论顺序），则返回 true；否则返回 false。
         */
        bool operator==(const EntityPair& other) const
        {
            return (entityA == other.entityA && entityB == other.entityB) ||
                (entityA == other.entityB && entityB == other.entityA);
        }
    };

    /**
     * @brief 为 EntityPair 提供哈希函数，以便在哈希容器中使用。
     */
    struct EntityPairHash
    {
        /**
         * @brief 计算给定 EntityPair 的哈希值。
         * @param p 要计算哈希值的 EntityPair 对象。
         * @return EntityPair 的哈希值。
         */
        std::size_t operator()(const EntityPair& p) const
        {
            auto hashA = std::hash<entt::entity>{}(p.entityA);
            auto hashB = std::hash<entt::entity>{}(p.entityB);
            return hashA ^ (hashB << 1);
        }
    };

    /// @brief 前向声明 TaskSystem 类。
    class TaskSystem;

    /**
     * @brief 物理系统，负责管理游戏世界的物理模拟。
     * @details 这是一个最终类，不能被继承。
     */
    class PhysicsSystem final : public ISystem
    {
    public:
        // 禁用拷贝构造函数和拷贝赋值运算符，确保 PhysicsSystem 不可拷贝。
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;

        /**
         * @brief 移动构造函数。
         * @param other 要移动的 PhysicsSystem 对象。
         */
        PhysicsSystem(PhysicsSystem&& other) noexcept;

        /**
         * @brief 移动赋值运算符。
         * @param other 要移动的 PhysicsSystem 对象。
         * @return 对当前 PhysicsSystem 对象的引用。
         */
        PhysicsSystem& operator=(PhysicsSystem&& other) noexcept;

        /**
         * @brief 构造一个新的 PhysicsSystem 对象。
         */
        PhysicsSystem();

        /**
         * @brief 析构 PhysicsSystem 对象。
         */
        ~PhysicsSystem() override;

        /**
         * @brief 在系统创建时调用，用于初始化物理世界和相关资源。
         * @param scene 当前运行时场景。
         * @param engineCtx 引擎上下文。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 在系统更新时调用，用于执行物理模拟步进。
         * @param scene 当前运行时场景。
         * @param deltaTime 自上次更新以来的时间增量。
         * @param engineCtx 引擎上下文。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 在系统销毁时调用，用于清理物理世界和相关资源。
         * @param scene 当前运行时场景。
         */
        void OnDestroy(RuntimeScene* scene) override;

        /**
         * @brief 获取当前物理世界的 ID。
         * @return 物理世界的 b2WorldId。
         */
        b2WorldId GetWorld() const;

        /**
         * @brief 执行从起点到终点的射线投射。
         * @param startPoint 射线投射的起始点。
         * @param endPoint 射线投射的结束点。
         * @return 如果射线击中任何物体，则返回包含 RayCastResult 的 std::optional；否则返回 std::nullopt。
         */
        std::optional<RayCastResult> RayCast(const ECS::Vector2f& startPoint, const ECS::Vector2f& endPoint) const;

    private:
        void CreateShapesForEntity(entt::entity entity, entt::registry& registry, const ECS::Transform& transform);
        void RecreateAllShapesForEntity(entt::entity entity, entt::registry& registry);

        bool Destroyed = false; ///< 指示物理系统是否已被销毁。

    private:
        b2WorldId m_world; ///< Box2D 物理世界的 ID。
        std::unique_ptr<TaskSystem> m_taskSystem; ///< 任务系统，可能用于异步物理操作。

        float m_accumulator = 0.0f; ///< 物理步进累加器。
        std::unordered_set<EntityPair, EntityPairHash> m_currentContacts; ///< 当前正在接触的实体对集合。
        std::unordered_set<EntityPair, EntityPairHash> m_currentTriggers; ///< 当前正在触发的实体对集合。
    };
}

#endif
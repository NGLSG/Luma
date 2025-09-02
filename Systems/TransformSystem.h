#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include "ISystem.h"
#include <entt/entt.hpp>

/**
 * @brief 包含所有系统相关的命名空间。
 */
namespace Systems
{
    /**
     * @brief 负责处理实体变换（位置、旋转、缩放）的系统。
     *
     * 该系统管理场景中实体的变换组件，并可能更新它们的全局变换。
     */
    class TransformSystem final : public ISystem
    {
    public:
        /**
         * @brief 在系统创建时调用，用于初始化变换系统。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param context 引擎上下文，提供对核心服务的访问。
         * @return 无
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 每帧更新时调用，用于处理实体的变换逻辑。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 自上一帧以来的时间间隔（秒）。
         * @param context 引擎上下文，提供对核心服务的访问。
         * @return 无
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        // 私有方法，根据要求不添加Doxygen注释。
        void UpdateWorldTransform(entt::entity entity, entt::registry& registry);
    };
}

#endif
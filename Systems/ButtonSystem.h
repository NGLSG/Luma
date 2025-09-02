#ifndef BUTTONSYSTEM_H
#define BUTTONSYSTEM_H

#include "ISystem.h"
#include "../../Data/EngineContext.h"
#include <entt/entt.hpp>

namespace ECS
{
    /**
     * @brief 按钮组件的前向声明。
     */
    struct ButtonComponent;
    /**
     * @brief 可序列化事件目标的前向声明。
     */
    struct SerializableEventTarget;
}

namespace Systems
{
    /**
     * @brief 按钮系统，负责处理游戏中的按钮交互逻辑。
     *        继承自ISystem，提供创建和更新功能。
     */
    class ButtonSystem : public ISystem
    {
    public:
        /**
         * @brief 系统创建时调用的初始化方法。
         * @param scene 指向当前运行时场景的指针。
         * @param context 引擎上下文，包含引擎的全局数据和资源。
         * @return 无。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 系统每帧更新时调用的方法。
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 自上一帧以来的时间间隔（秒）。
         * @param context 引擎上下文，包含引擎的全局数据和资源。
         * @return 无。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        void handleButtonInteraction(RuntimeScene* scene, entt::entity entity,
                                     ECS::ButtonComponent& button, const EngineContext& context);
        void invokeButtonEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets);
    };
}

#endif
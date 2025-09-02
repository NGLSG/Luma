#ifndef SCROLLVIEWSYSTEM_H
#define SCROLLVIEWSYSTEM_H

#include "ISystem.h"
#include "../../Data/EngineContext.h"
#include <entt/entt.hpp>

namespace ECS
{
    /**
     * @brief 可序列化的事件目标结构体。
     */
    struct SerializableEventTarget;
    /**
     * @brief 变换组件结构体，通常包含位置、旋转和缩放信息。
     */
    struct Transform;
    /**
     * @brief 滚动视图组件结构体。
     */
    struct ScrollViewComponent;
}

namespace Systems
{
    /**
     * @brief 滚动视图系统，负责处理滚动视图的逻辑和交互。
     *
     * 该系统继承自ISystem，提供滚动视图的创建和更新功能。
     */
    class ScrollViewSystem : public ISystem
    {
    public:
        /**
         * @brief 在系统创建时调用，用于初始化滚动视图系统。
         * @param scene 运行时场景指针。
         * @param context 引擎上下文引用。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 每帧更新时调用，用于处理滚动视图的逻辑。
         * @param scene 运行时场景指针。
         * @param deltaTime 距离上一帧的时间间隔。
         * @param context 引擎上下文引用。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        void handleScrollInteraction(RuntimeScene* scene, entt::entity entity,
                                     ECS::ScrollViewComponent& scrollView, const EngineContext& context);
        void invokeScrollChangedEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets,
                                      const ECS::Vector2f& scrollPosition);
        bool isPointInViewport(const ECS::Vector2f& point, const ECS::Transform& transform,
                               const ECS::Vector2f& viewportSize);
        ECS::Vector2f clampScrollPosition(const ECS::Vector2f& scrollPos, const ECS::Vector2f& contentSize,
                                          const ECS::Vector2f& viewportSize);
    };
}

#endif
#ifndef INTERACTIONSYSTEM_H
#define INTERACTIONSYSTEM_H

#include "ISystem.h"
#include "Data/EngineContext.h"
#include "Renderer/Camera.h"
#include <entt/entt.hpp>

namespace Systems
{
    /**
     * @brief 交互系统。
     *
     * 负责处理用户与场景中实体之间的交互，例如鼠标悬停、点击等。
     */
    class InteractionSystem : public ISystem
    {
    public:
        /**
         * @brief 在系统创建时调用。
         *
         * 用于初始化交互系统所需的资源和状态。
         *
         * @param scene 运行时场景指针。
         * @param context 引擎上下文。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 在系统更新时调用。
         *
         * 每帧更新交互状态，处理输入并触发相应的交互事件。
         *
         * @param scene 运行时场景指针。
         * @param deltaTime 帧时间间隔。
         * @param context 引擎上下文。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        /**
         * @brief 执行几何拾取操作。
         *
         * 根据世界坐标下的鼠标位置，检测哪个实体被选中。
         *
         * @param registry ECS 注册表。
         * @param worldMousePos 世界坐标下的鼠标位置。
         * @return 被拾取的实体，如果没有则返回 `entt::null`。
         */
        entt::entity performGeometricPicking(entt::registry& registry, const ECS::Vector2f& worldMousePos);

        /**
         * @brief 处理悬停事件。
         *
         * 根据当前悬停的实体，触发悬停进入、悬停离开或悬停保持事件。
         *
         * @param registry ECS 注册表。
         * @param currentHoveredEntity 当前悬停的实体。
         */
        void handleHoverEvents(entt::registry& registry, entt::entity currentHoveredEntity);

        /**
         * @brief 处理点击事件。
         *
         * 根据输入状态，检测鼠标点击并触发相应的点击事件。
         *
         * @param registry ECS 注册表。
         * @param inputState 输入状态。
         */
        void handleClickEvents(entt::registry& registry, const InputState& inputState);

        /**
         * @brief 将屏幕坐标转换为世界坐标。
         *
         * @param globalScreenPos 全局屏幕坐标。
         * @param cameraProps 摄像机属性。
         * @param viewport 视口矩形。
         * @return 转换后的世界坐标。
         */
        ECS::Vector2f screenToWorld(const ECS::Vector2f& globalScreenPos, const Camera::CamProperties& cameraProps,
                                    const ECS::RectF& viewport);

    private:
        entt::entity m_hoveredEntity = entt::null; ///< 当前悬停的实体。
        entt::entity m_pressedEntity = entt::null; ///< 当前按下的实体。
        bool m_wasLeftMouseDownLastFrame = false;   ///< 上一帧鼠标左键是否按下。
    };
}

#endif
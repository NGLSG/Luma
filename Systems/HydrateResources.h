#ifndef HYDRATERESOURCES_H
#define HYDRATERESOURCES_H
#include "ISystem.h"
#include "entt/entt.hpp"

namespace Systems
{
    /**
     * @brief 资源水合与更新处理系统。
     *
     * 该系统负责在资源（如精灵、脚本、文本等）更新时进行相应的处理。
     */
    class HydrateResources final : public ISystem
    {
    public:
        /**
         * @brief 系统创建时调用。
         * @param scene 运行时场景指针。
         * @param context 引擎上下文。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 系统每帧更新时调用。
         * @param scene 运行时场景指针。
         * @param deltaTime 帧之间的时间间隔。
         * @param context 引擎上下文。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

        /**
         * @brief 系统销毁时调用。
         * @param scene 运行时场景指针。
         */
        void OnDestroy(RuntimeScene* scene) override;

    private:
        /**
         * @brief 当精灵组件更新时调用。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnSpriteUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当脚本组件更新时调用。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnScriptUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当文本组件更新时调用。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnTextUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当输入文本组件更新时调用。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnInputTextUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当瓦片地图组件更新时调用。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnTilemapUpdated(entt::registry& registry, entt::entity entity);

        EngineContext* m_context = nullptr; ///< 引擎上下文指针。

        std::vector<ListenerHandle> m_listeners; ///< 事件监听器句柄列表。
    };
}

#endif
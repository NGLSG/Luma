#ifndef HYDRATERESOURCES_H
#define HYDRATERESOURCES_H
#include "ISystem.h"
#include "entt/entt.hpp"

namespace Systems
{
    /**
     * @brief 资源水合与更新处理系统。
     *
     * 该系统负责在组件附加或其资源句柄改变时，加载实际的资源数据（如纹理、字体、脚本元数据等）。
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
         * @brief 当精灵组件更新时调用，加载纹理和材质。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnSpriteUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当脚本组件更新时调用，加载C#脚本元数据。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnScriptUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当文本组件更新时调用，加载字体。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnTextUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当输入文本组件更新时调用，加载字体和背景图像。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnInputTextUpdated(entt::registry& registry, entt::entity entity);

        void OnToggleButtonUpdated(entt::registry& registry, entt::entity entity);
        void OnRadioButtonUpdated(entt::registry& registry, entt::entity entity);
        void OnCheckBoxUpdated(entt::registry& registry, entt::entity entity);
        void OnSliderUpdated(entt::registry& registry, entt::entity entity);
        void OnComboBoxUpdated(entt::registry& registry, entt::entity entity);
        void OnExpanderUpdated(entt::registry& registry, entt::entity entity);
        void OnProgressBarUpdated(entt::registry& registry, entt::entity entity);
        void OnTabControlUpdated(entt::registry& registry, entt::entity entity);
        void OnListBoxUpdated(entt::registry& registry, entt::entity entity);
        /**
         * @brief 当按钮组件更新时调用，加载背景图像。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnButtonUpdated(entt::registry& registry, entt::entity entity);

        /**
         * @brief 当瓦片地图组件更新时调用，解析瓦片、生成碰撞体等。
         * @param registry ECS注册表。
         * @param entity 发生更新的实体。
         */
        void OnTilemapUpdated(entt::registry& registry, entt::entity entity);

        EngineContext* m_context = nullptr; ///< 引擎上下文指针。

        std::vector<ListenerHandle> m_listeners; ///< 事件监听器句柄列表。
    };
}

#endif // HYDRATERESOURCES_H

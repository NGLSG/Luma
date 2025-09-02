#ifndef LUMAENGINE_ACTIVITYCOMPONENT_H
#define LUMAENGINE_ACTIVITYCOMPONENT_H
#include "ComponentRegistry.h"
#include "IComponent.h"

namespace ECS
{
    /**
     * @brief 活动组件。
     *
     * 该组件用于标记实体是否处于活动状态。
     */
    struct ActivityComponent : IComponent
    {
        bool isActive = false; ///< 指示组件是否处于活动状态。

        /**
         * @brief 构造一个新的活动组件实例。
         * @param isActive 初始活动状态。
         */
        ActivityComponent(bool isActive) : isActive(isActive)
        {
        }

        /**
         * @brief 构造一个新的活动组件实例，默认设置为活动状态。
         */
        ActivityComponent() : isActive(true)
        {
        };
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于序列化和反序列化 ECS::ActivityComponent。
     */
    template <>
    struct convert<ECS::ActivityComponent>
    {
        /**
         * @brief 将 ActivityComponent 编码为 YAML 节点。
         * @param ac 要编码的 ActivityComponent 实例。
         * @return 表示 ActivityComponent 的 YAML 节点。
         */
        static Node encode(const ECS::ActivityComponent& ac)
        {
            Node node;
            node["isActive"] = ac.isActive;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ActivityComponent。
         * @param node 包含 ActivityComponent 数据的 YAML 节点。
         * @param ac 要填充的 ActivityComponent 实例。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::ActivityComponent& ac)
        {
            if (!node.IsMap())
                return false;

            if (node["isActive"])
                ac.isActive = node["isActive"].as<bool>();
            return true;
        }
    };
}

/**
 * @brief 注册 ECS::ActivityComponent 到组件注册表。
 *
 * 此宏块负责将 ActivityComponent 及其属性注册到引擎的组件系统中，
 * 以便进行反射、序列化和编辑器集成。
 */
REGISTRY
{
    Registry_<ECS::ActivityComponent>("ActivityComponent")
        .SetHidden() ///< 将此组件设置为隐藏，可能不在编辑器中直接显示。
        .SetNonRemovable() ///< 将此组件设置为不可移除。
        .property("isActive", &ECS::ActivityComponent::isActive); ///< 注册 isActive 属性。
}
#endif
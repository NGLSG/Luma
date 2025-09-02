#ifndef RELATIONSHIPCOMPONENT_H
#define RELATIONSHIPCOMPONENT_H
#pragma once

#include <entt/entt.hpp>
#include <vector>
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 父组件，表示实体的父实体。
     *
     * 该组件用于存储一个实体（父实体）的ID，从而建立父子关系。
     */
    struct ParentComponent : public IComponent
    {
        entt::entity parent = entt::null; ///< 父实体

        ParentComponent() = default;

        /**
         * @brief 构造函数，初始化父实体。
         * @param p 父实体ID。
         */
        explicit ParentComponent(entt::entity p) : parent(p)
        {
        }
    };


    /**
     * @brief 子组件，表示实体的子实体列表。
     *
     * 该组件用于存储一个实体所拥有的所有子实体的ID列表。
     */
    struct ChildrenComponent : public IComponent
    {
        std::vector<entt::entity> children; ///< 子实体列表
    };
}

namespace YAML
{
    template <>
    struct convert<entt::entity>
    {
        /**
         * @brief 将 entt::entity 编码为 YAML 节点。
         * @param entity 要编码的实体。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const entt::entity& entity)
        {
            Node node;
            node = static_cast<std::uint32_t>(entity);
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 entt::entity。
         * @param node 要解码的 YAML 节点。
         * @param entity 解码后的实体。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, entt::entity& entity)
        {
            if (!node.IsScalar())
                return false;

            entity = static_cast<entt::entity>(node.as<std::uint32_t>());
            return true;
        }
    };

    template <>
    struct convert<ECS::ParentComponent>
    {
        /**
         * @brief 将 ECS::ParentComponent 编码为 YAML 节点。
         * @param component 要编码的父组件。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ECS::ParentComponent& component)
        {
            Node node;
            node["parent"] = component.parent;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 ECS::ParentComponent。
         * @param node 要解码的 YAML 节点。
         * @param component 解码后的父组件。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, ECS::ParentComponent& component)
        {
            if (!node.IsMap() || !node["parent"].IsDefined())
                return false;

            component.parent = node["parent"].as<entt::entity>();
            return true;
        }
    };

    template <>
    struct convert<ECS::ChildrenComponent>
    {
        /**
         * @brief 将 ECS::ChildrenComponent 编码为 YAML 节点。
         * @param component 要编码的子组件。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ECS::ChildrenComponent& component)
        {
            Node node;
            node["children"] = component.children;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 ECS::ChildrenComponent。
         * @param node 要解码的 YAML 节点。
         * @param component 解码后的子组件。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, ECS::ChildrenComponent& component)
        {
            if (!node.IsMap() || !node["children"].IsDefined())
                return false;

            component.children = node["children"].as<std::vector<entt::entity>>();
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::ParentComponent>("ParentComponent")
        .SetHidden()
        .property("parent", &ECS::ParentComponent::parent);

    Registry_<ECS::ChildrenComponent>("ChildrenComponent")
        .SetHidden()
        .property("children", &ECS::ChildrenComponent::children);
}
#endif
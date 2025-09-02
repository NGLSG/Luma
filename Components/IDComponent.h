#ifndef IDCOMPONENT_H
#define IDCOMPONENT_H

#include "../Utils/Guid.h"
#include <string>

#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief ID组件结构体
     *
     * 包含一个组件的名称和全局唯一标识符。
     */
    struct IDComponent : IComponent
    {
        ///< 组件的名称
        std::string name;
        ///< 组件的全局唯一标识符
        Guid guid;

        /**
         * @brief IDComponent 构造函数
         *
         * 使用指定的名称和全局唯一标识符构造一个 IDComponent 对象。
         *
         * @param n 组件的名称
         * @param g 组件的全局唯一标识符
         */
        IDComponent(const std::string& n, const Guid& g)
            : name(n), guid(g)
        {
        }


        /**
         * @brief IDComponent 默认构造函数
         *
         * 创建一个默认的 IDComponent 对象。
         */
        IDComponent() = default;
    };
}

namespace YAML
{
    template <>
    struct convert<ECS::IDComponent>
    {
        /**
         * @brief 将 IDComponent 对象编码为 YAML 节点
         *
         * 将给定的 IDComponent 对象的名称和 GUID 转换为 YAML 节点。
         *
         * @param rhs 要编码的 IDComponent 对象
         * @return 表示 IDComponent 对象的 YAML 节点
         */
        static Node encode(const ECS::IDComponent& rhs)
        {
            Node node;
            node["name"] = rhs.name;
            node["guid"] = rhs.guid.ToString();
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 IDComponent 对象
         *
         * 尝试将给定的 YAML 节点解码并填充到 IDComponent 对象中。
         *
         * @param node 包含 IDComponent 数据的 YAML 节点
         * @param rhs 用于存储解码结果的 IDComponent 对象
         * @return 如果解码成功则返回 true，否则返回 false
         */
        static bool decode(const Node& node, ECS::IDComponent& rhs)
        {
            if (!node.IsMap() || !node["name"] || !node["guid"])
                return false;

            rhs.name = node["name"].as<std::string>();
            rhs.guid = Guid::FromString(node["guid"].as<std::string>());
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::IDComponent>("IDComponent")
        .SetHidden()
        .property("name", &ECS::IDComponent::name)
        .property("guid", &ECS::IDComponent::guid);
}

#endif
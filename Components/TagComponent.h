#ifndef TAGCOMPONENT_H
#define TAGCOMPONENT_H
#include <string>

#include "IComponent.h"
#include "yaml-cpp/node/node.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 表示一个标签组件，用于标识实体。
     *
     * 标签组件通常包含一个字符串名称，用于对实体进行分类或查找。
     */
    struct TagComponent : IComponent
    {
        /**
         * @brief 默认构造函数。
         */
        TagComponent() = default;

        /// 标签的名称。
        std::string name;

        /**
         * @brief 构造函数，使用给定的标签名称初始化。
         * @param tagName 标签的名称。
         */
        TagComponent(const std::string& tagName) : name(tagName)
        {
        }

        /**
         * @brief 移动构造函数，使用给定的标签名称初始化。
         * @param tagName 标签的名称（右值引用）。
         */
        TagComponent(std::string&& tagName) : name(std::move(tagName))
        {
        }
    };
}

namespace YAML
{
    /**
     * @brief YAML 序列化和反序列化 `ECS::TagComponent` 的转换器。
     *
     * 允许 `ECS::TagComponent` 对象在 YAML 格式和 C++ 对象之间进行转换。
     */
    template <>
    struct convert<ECS::TagComponent>
    {
        /**
         * @brief 将 `TagComponent` 对象编码为 YAML 节点。
         * @param tag 要编码的 TagComponent 对象。
         * @return 表示 TagComponent 的 YAML 节点。
         */
        static Node encode(const ECS::TagComponent& tag)
        {
            Node node;
            node["name"] = tag.name;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 `TagComponent` 对象。
         * @param node 包含 TagComponent 数据的 YAML 节点。
         * @param tag 用于存储解码结果的 TagComponent 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::TagComponent& tag)
        {
            if (!node.IsMap() || !node["name"])
                return false;

            tag.name = node["name"].as<std::string>();
            return true;
        }
    };
}

/**
 * @brief 注册 `TagComponent` 到组件注册系统，并声明其可序列化的属性。
 *
 * 此宏用于将 `ECS::TagComponent` 类型及其 `name` 属性注册到全局组件注册表中，
 * 以便在运行时可以通过字符串名称创建和访问组件及其属性。
 */
REGISTRY
{
    Registry_<ECS::TagComponent>("TagComponent")
        .property("name", &ECS::TagComponent::name);
}
#endif
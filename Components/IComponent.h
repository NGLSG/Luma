#ifndef ICOMPONENT_H
#define ICOMPONENT_H
#include "../Utils/InspectorUI.h"

namespace ECS
{
    /**
     * @brief 组件基类接口。
     */
    struct IComponent
    {
        ///< 是否启用
        bool Enable = true;
    };

    /**
     * @brief 可序列化的事件目标结构体。
     */
    struct SerializableEventTarget
    {
        ///< 目标实体GUID
        Guid targetEntityGuid;
        ///< 目标组件名称
        std::string targetComponentName;
        ///< 目标方法名称
        std::string targetMethodName;
    };
}

namespace YAML
{
    template <>
    struct convert<ECS::IComponent>
    {
        /**
         * @brief 将ECS::IComponent编码成YAML节点。
         * @param rhs 要编码的ECS::IComponent对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const ECS::IComponent& rhs)
        {
            Node node;
            node["Enable"] = rhs.Enable;
            return node;
        }

        /**
         * @brief 将YAML节点解码成ECS::IComponent。
         * @param node 要解码的YAML节点。
         * @param rhs 解码后的ECS::IComponent对象。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, ECS::IComponent& rhs)
        {
            if (!node.IsMap() || !node["Enable"])
                return false;

            rhs.Enable = node["Enable"].as<bool>();
            return true;
        }
    };

    template <>
    struct convert<ECS::SerializableEventTarget>
    {
        /**
         * @brief 将ECS::SerializableEventTarget编码成YAML节点。
         * @param rhs 要编码的ECS::SerializableEventTarget对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const ECS::SerializableEventTarget& rhs)
        {
            Node node;
            node["targetEntityGuid"] = rhs.targetEntityGuid.ToString();
            node["targetComponentName"] = rhs.targetComponentName;
            node["targetMethodName"] = rhs.targetMethodName;
            return node;
        }

        /**
         * @brief 将YAML节点解码成ECS::SerializableEventTarget。
         * @param node 要解码的YAML节点。
         * @param rhs 解码后的ECS::SerializableEventTarget对象。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, ECS::SerializableEventTarget& rhs)
        {
            if (!node.IsMap() || !node["targetEntityGuid"] || !node["targetComponentName"] || !node["targetMethodName"])
                return false;

            rhs.targetEntityGuid = Guid::FromString(node["targetEntityGuid"].as<std::string>());
            rhs.targetComponentName = node["targetComponentName"].as<std::string>();
            rhs.targetMethodName = node["targetMethodName"].as<std::string>();
            return true;
        }
    };
}

namespace CustomDrawing
{
    template <>
    struct WidgetDrawer<ECS::SerializableEventTarget>
    {
        /**
         * @brief 绘制ECS::SerializableEventTarget的UI小部件。
         * @param label  标签文本。
         * @param target  要绘制的ECS::SerializableEventTarget对象。
         * @param drawData 绘制数据。
         * @return 绘制是否成功。
         */
        static bool Draw(const std::string& label, ECS::SerializableEventTarget& target, const UIDrawData& drawData);
    };
}


#endif
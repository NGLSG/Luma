#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H
#include "IComponent.h"
#include "AssetHandle.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "ComponentRegistry.h"
#include "ScriptMetadata.h"
#include "../Utils/Guid.h"

namespace ECS
{
    /**
     * @brief 脚本组件，用于将脚本逻辑附加到实体。
     *
     * 包含脚本资产句柄、属性覆盖、事件链接以及与托管脚本相关的元数据。
     */
    struct ScriptComponent : public IComponent
    {
        AssetHandle scriptAsset = AssetHandle(AssetType::CSharpScript); ///< 当前脚本资产的句柄。
        AssetHandle lastScriptAsset = AssetHandle(AssetType::CSharpScript); ///< 上一个脚本资产的句柄。

        YAML::Node propertyOverrides; ///< 脚本属性的 YAML 覆盖节点。
        std::unordered_map<std::string, std::vector<SerializableEventTarget>> eventLinks; ///< 事件名称到可序列化事件目标的映射。

        intptr_t* managedGCHandle = nullptr; ///< 托管垃圾回收句柄的指针（通常用于 C# 脚本）。
        const ScriptClassMetadata* metadata = nullptr; ///< 脚本类元数据的指针。

        /**
         * @brief 构造一个新的 ScriptComponent 实例。
         * @param scriptAsset 脚本资产的句柄。
         */
        ScriptComponent(AssetHandle scriptAsset = AssetHandle(AssetType::CSharpScript)) :
            scriptAsset(scriptAsset), lastScriptAsset(scriptAsset)
        {
            propertyOverrides = YAML::Node();
        }
    };

    /**
     * @brief 脚本组件集合，用于附加多个脚本到单个实体。
     * 并非完全数据,而是容器和逻辑
     */
    struct ScriptsComponent : public IComponent
    {
        std::vector<ScriptComponent> scripts; ///< 脚本组件列表。

        /**
         * @brief 构造一个新的 ScriptsComponent 实例。
         */
        ScriptsComponent() = default;


        ScriptComponent& AddScript(const AssetHandle& scriptAsset, entt::entity entity);
        ScriptComponent& GetScriptByName(const std::string& scriptName);
        ScriptComponent& GetScriptByAsset(const AssetHandle& scriptAsset);
        std::vector<AssetHandle> GetAllScriptsByName(const std::string& scriptName);
        void RemoveScriptByAsset(const AssetHandle& scriptAsset);
        void RemoveScriptByName(const std::string& scriptName);
    };
}


namespace YAML
{
    /**
     * @brief 为 ECS::ScriptComponent 提供 YAML 序列化和反序列化转换。
     */
    template <>
    struct convert<ECS::ScriptComponent>
    {
        /**
         * @brief 将 ScriptComponent 对象编码为 YAML 节点。
         * @param rhs 要编码的 ScriptComponent 对象。
         * @return 表示 ScriptComponent 的 YAML 节点。
         */
        static Node encode(const ECS::ScriptComponent& rhs)
        {
            Node node;
            node["Enable"] = rhs.Enable;
            node["scriptAsset"] = rhs.scriptAsset;
            node["propertyOverrides"] = rhs.propertyOverrides;


            Node eventLinksNode;
            for (const auto& [eventName, targets] : rhs.eventLinks)
            {
                Node targetList;
                for (const auto& target : targets)
                {
                    targetList.push_back(target);
                }
                eventLinksNode[eventName] = targetList;
            }
            node["eventLinks"] = eventLinksNode;

            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 ScriptComponent 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 要填充的 ScriptComponent 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::ScriptComponent& rhs)
        {
            if (!node.IsMap() || !node["scriptAsset"])
                return false;
            rhs.Enable = node["Enable"].as<bool>(true);
            rhs.scriptAsset = node["scriptAsset"].as<AssetHandle>();

            if (node["propertyOverrides"])
            {
                rhs.propertyOverrides = node["propertyOverrides"];
            }
            else
            {
                rhs.propertyOverrides = YAML::Node();
            }


            if (node["eventLinks"])
            {
                const Node& eventLinksNode = node["eventLinks"];
                if (eventLinksNode.IsMap())
                {
                    rhs.eventLinks.clear();
                    for (const auto& it : eventLinksNode)
                    {
                        std::string eventName = it.first.as<std::string>();
                        const Node& targetsNode = it.second;

                        if (targetsNode.IsSequence())
                        {
                            std::vector<ECS::SerializableEventTarget> targets;
                            for (const auto& targetNode : targetsNode)
                            {
                                targets.push_back(targetNode.as<ECS::SerializableEventTarget>());
                            }
                            rhs.eventLinks[eventName] = std::move(targets);
                        }
                    }
                }
            }

            return true;
        }
    };

    template <>
    struct convert<ECS::ScriptsComponent>
    {
        static Node encode(const ECS::ScriptsComponent& rhs)
        {
            Node node;
            node["Enable"] = rhs.Enable;
            Node scriptsNode;
            for (const auto& script : rhs.scripts)
            {
                scriptsNode.push_back(script);
            }
            node["scripts"] = scriptsNode;
            return node;
        }

        static bool decode(const Node& node, ECS::ScriptsComponent& rhs)
        {
            if (!node.IsMap() || !node["scripts"])
                return false;
            rhs.Enable = node["Enable"].as<bool>(true);
            const Node& scriptsNode = node["scripts"];
            if (scriptsNode.IsSequence())
            {
                rhs.scripts.clear();
                for (const auto& scriptNode : scriptsNode)
                {
                    rhs.scripts.push_back(scriptNode.as<ECS::ScriptComponent>());
                }
            }
            return true;
        }
    };
}

class RuntimeGameObject;

namespace CustomDrawing
{
    class ScriptMetadataHelper;
    class PropertyValueFactory;
    class PropertyDrawer;
    class EventDrawer;

    /**
     * @brief 属性值工厂，用于创建默认属性值。
     */
    class PropertyValueFactory
    {
    public:
        /**
         * @brief 根据类型名称创建默认的 YAML 节点值。
         * @param typeName 属性的类型名称。
         * @param defaultValue 可选的默认值字符串。
         * @return 包含默认值的 YAML 节点。
         */
        static YAML::Node CreateDefaultValue(const std::string& typeName, const std::string& defaultValue = "");

        /**
         * @brief 检查给定的类型名称是否是 Luma 事件类型。
         * @param typeName 要检查的类型名称。
         * @return 如果是 Luma 事件类型则返回 true，否则返回 false。
         */
        static bool IsLumaEventType(const std::string& typeName);
    };

    /**
     * @brief 属性绘制器，用于在 UI 中绘制属性。
     */
    class PropertyDrawer
    {
    public:
        /**
         * @brief 在 UI 中绘制一个属性。
         * @param label 属性的显示标签。
         * @param typeName 属性的类型名称。
         * @param valueNode 包含属性值的 YAML 节点（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果属性被修改则返回 true，否则返回 false。
         */
        static bool DrawProperty(const std::string& label, const std::string& typeName,
                                 YAML::Node& valueNode, const UIDrawData& drawData);
    };

    /**
     * @brief 事件绘制器，用于在 UI 中绘制事件。
     */
    class EventDrawer
    {
    public:
        /**
         * @brief 在 UI 中绘制一个事件。
         * @param eventName 事件的名称。
         * @param eventSignature 事件的签名。
         * @param targets 事件的可序列化目标列表（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果事件被修改则返回 true，否则返回 false。
         */
        static bool DrawEvent(const std::string& eventName, const std::string& eventSignature,
                              std::vector<ECS::SerializableEventTarget>& targets, const UIDrawData& drawData);
    };

    /**
     * @brief 脚本元数据辅助类，提供与脚本元数据相关的实用函数。
     */
    class ScriptMetadataHelper
    {
    public:
        /**
         * @brief 根据脚本元数据初始化属性覆盖。
         * @param propertyOverrides 属性覆盖的 YAML 节点（输入/输出）。
         * @param metadata 脚本类元数据。
         */
        static void InitializePropertyOverrides(YAML::Node& propertyOverrides, const ScriptClassMetadata* metadata);

        /**
         * @brief 根据脚本元数据初始化事件链接。
         * @param eventLinks 事件链接的映射（输入/输出）。
         * @param metadata 脚本类元数据。
         */
        static void InitializeEventLinks(
            std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>& eventLinks,
            const ScriptClassMetadata* metadata);

        /**
         * @brief 获取与给定事件签名匹配的方法列表。
         * @param metadata 脚本类元数据。
         * @param eventSignature 事件签名字符串。
         * @return 匹配的方法名称列表。
         */
        static std::vector<std::string> GetMatchingMethods(const ScriptClassMetadata* metadata,
                                                           const std::string& eventSignature);

        /**
         * @brief 获取目标实体的脚本类元数据。
         * @param targetEntityGuid 目标实体的全局唯一标识符。
         * @return 指向 ScriptClassMetadata 的指针，如果未找到则为 nullptr。
         */
        static const ScriptClassMetadata* GetTargetScriptMetadata(const Guid& targetEntityGuid);

        /**
         * @brief 根据全局唯一标识符获取运行时游戏对象。
         * @param targetEntityGuid 目标实体的全局唯一标识符。
         * @return 对应的 RuntimeGameObject 对象。
         */
        static RuntimeGameObject GetGameObjectByGuid(const Guid& targetEntityGuid);

        /**
         * @brief 获取目标实体可用的方法列表。
         * @param targetEntityGuid 目标实体的全局唯一标识符。
         * @param eventSignature 可选的事件签名，用于过滤方法。
         * @return 包含方法名称和签名的对列表。
         */
        static std::vector<std::pair<std::string, std::string>> GetAvailableMethods(
            const Guid& targetEntityGuid, const std::string& eventSignature = "");
    };

    /**
     * @brief YAML::Node 的 WidgetDrawer 特化，用于在 UI 中绘制 YAML 节点。
     */
    template <>
    struct WidgetDrawer<YAML::Node>
    {
        /**
         * @brief 在 UI 中绘制一个 YAML 节点。
         * @param label 节点的显示标签。
         * @param value 要绘制的 YAML 节点（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果节点被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, YAML::Node& value, const UIDrawData& drawData);

    private:
        /**
         * @brief 递归绘制 YAML 节点。
         * @param key 节点的键。
         * @param node 要绘制的 YAML 节点。
         */
        static void drawYamlNodeRecursive(const std::string& key, YAML::Node node);
    };

    /**
     * @brief std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>> 的 WidgetDrawer 特化，用于在 UI 中绘制事件链接映射。
     */
    template <>
    struct WidgetDrawer<std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>>
    {
        /**
         * @brief 在 UI 中绘制事件链接映射。
         * @param label 映射的显示标签。
         * @param eventLinks 要绘制的事件链接映射（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果映射被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label,
                         std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>& eventLinks,
                         const UIDrawData& drawData);
    };

    /**
     * @brief ScriptPropertyMetadata 的 WidgetDrawer 特化，用于在 UI 中绘制脚本属性元数据。
     */
    template <>
    struct WidgetDrawer<ScriptPropertyMetadata>
    {
        /**
         * @brief 在 UI 中绘制脚本属性元数据。
         * @param label 属性的显示标签。
         * @param property 要绘制的脚本属性元数据（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果属性被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, ScriptPropertyMetadata& property, const UIDrawData& drawData);
    };

    /**
     * @brief ScriptMethodMetadata 的 WidgetDrawer 特化，用于在 UI 中绘制脚本方法元数据。
     */
    template <>
    struct WidgetDrawer<ScriptMethodMetadata>
    {
        /**
         * @brief 在 UI 中绘制脚本方法元数据。
         * @param label 方法的显示标签。
         * @param method 要绘制的脚本方法元数据（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果方法被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, ScriptMethodMetadata& method, const UIDrawData& drawData);
    };

    /**
     * @brief ScriptClassMetadata 的 WidgetDrawer 特化，用于在 UI 中绘制脚本类元数据。
     */
    template <>
    struct WidgetDrawer<ScriptClassMetadata>
    {
        /**
         * @brief 在 UI 中绘制脚本类元数据。
         * @param label 类的显示标签。
         * @param metadata 要绘制的脚本类元数据（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果元数据被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, ScriptClassMetadata& metadata, const UIDrawData& drawData);
    };

    /**
     * @brief const ScriptClassMetadata* 的 WidgetDrawer 特化，用于在 UI 中绘制脚本类元数据指针。
     */
    template <>
    struct WidgetDrawer<const ScriptClassMetadata*>
    {
        /**
         * @brief 在 UI 中绘制脚本类元数据指针。
         * @param label 指针的显示标签。
         * @param metadataPtr 要绘制的脚本类元数据指针（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果指针指向的元数据被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, const ScriptClassMetadata*& metadataPtr, const UIDrawData& drawData);
    };

    template <>
    struct WidgetDrawer<ECS::ScriptComponent>
    {
        /**
         * @brief 在 UI 中绘制 ScriptComponent 组件。
         * @param label 组件的显示标签。
         * @param component 要绘制的 ScriptComponent 组件（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果组件被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, ECS::ScriptComponent& component, const UIDrawData& drawData);
    };

    template <>
    struct WidgetDrawer<ECS::ScriptsComponent>
    {
        /**
         * @brief 在 UI 中绘制 ScriptComponent 组件。
         * @param label 组件的显示标签。
         * @param component 要绘制的 ScriptComponent 组件（输入/输出）。
         * @param drawData UI 绘制所需的数据。
         * @return 如果组件被修改则返回 true，否则返回 false。
         */
        static bool Draw(const std::string& label, ECS::ScriptsComponent& component, const UIDrawData& drawData);
    };
}

REGISTRY
{
    Registry_<ECS::SerializableEventTarget>("SerializableEventTarget")
        .property("targetEntityGuid", &ECS::SerializableEventTarget::targetEntityGuid)
        .property("targetComponentName", &ECS::SerializableEventTarget::targetComponentName)
        .property("targetMethodName", &ECS::SerializableEventTarget::targetMethodName);

    Registry_<ECS::ScriptComponent>("ScriptComponent")
        .property("scriptAsset", &ECS::ScriptComponent::scriptAsset)
        .property("propertyOverrides", &ECS::ScriptComponent::propertyOverrides)
        .property("eventLinks", &ECS::ScriptComponent::eventLinks)
        .property("metadata", &ECS::ScriptComponent::metadata);
    Registry_<ECS::ScriptsComponent>("ScriptsComponent")
        .property("scripts", &ECS::ScriptsComponent::scripts);
}
#endif

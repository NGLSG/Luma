#ifndef SCRIPTMETADATA_H
#define SCRIPTMETADATA_H
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>

/**
 * @brief 脚本属性元数据结构体。
 *
 * 描述了脚本中一个属性（Property）的元信息，例如名称、类型、默认值等。
 */
struct ScriptPropertyMetadata
{
    std::string name; ///< 属性的名称。
    std::string type; ///< 属性的类型。
    std::string defaultValue; ///< 属性的默认值。
    bool canGet; ///< 指示属性是否可读（是否有getter）。
    bool canSet; ///< 指示属性是否可写（是否有setter）。
    bool isPublic; ///< 指示属性是否为公共的。
    std::string eventSignature; ///< 如果属性是事件，则为事件的签名。
};

/**
 * @brief 脚本方法元数据结构体。
 *
 * 描述了脚本中一个方法（Method）的元信息，例如名称、返回类型和签名。
 */
struct ScriptMethodMetadata
{
    std::string name; ///< 方法的名称。
    std::string returnType; ///< 方法的返回类型。
    std::string signature; ///< 方法的参数类型签名。
};

/**
 * @brief 脚本类元数据结构体。
 *
 * 描述了脚本中一个类（Class）的元信息，包括其名称、命名空间、导出的属性和公共方法。
 */
struct ScriptClassMetadata
{
    std::string name; ///< 类的短名称。
    std::string fullName; ///< 类的完整名称（包含命名空间）。
    std::string nspace; ///< 类所在的命名空间。
    std::vector<ScriptPropertyMetadata> exportedProperties; ///< 类导出的属性列表。
    std::vector<ScriptMethodMetadata> publicMethods; ///< 类的公共方法列表。
    std::vector<ScriptMethodMetadata> publicStaticMethods; ///< 类的公共静态方法列表。

    /**
     * @brief 检查脚本类元数据是否有效。
     *
     * 当类的名称和完整名称都不为空时，认为元数据是有效的。
     *
     * @return 如果元数据有效则返回 true，否则返回 false。
     */
    bool Valid() const { return !name.empty() && !fullName.empty(); }
};

/**
 * @brief 顶层脚本元数据结构体。
 *
 * 包含所有脚本类的元数据以及项目中所有可用的类型列表，代表整个YAML文件的内容。
 */
struct ScriptMetadata
{
    std::vector<ScriptClassMetadata> scripts; ///< 所有脚本类的元数据列表。
    std::vector<std::string> availableTypes; ///< 项目中所有可用类型的列表。
};


namespace YAML
{
    template <>
    struct convert<ScriptMetadata>
    {
        /**
         * @brief 从 YAML 节点解码 ScriptMetadata 对象。
         *
         * @param node 包含整个脚本元数据的根 YAML 节点。
         * @param rhs 将解码的数据存储到的 ScriptMetadata 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ScriptMetadata& rhs)
        {
            if (!node.IsMap()) return false;

            if (node["Scripts"])
            {
                rhs.scripts = node["Scripts"].as<std::vector<ScriptClassMetadata>>();
            }

            if (node["AvailableTypes"])
            {
                rhs.availableTypes = node["AvailableTypes"].as<std::vector<std::string>>();
            }

            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 ScriptPropertyMetadata 类型。
     *
     * 允许将 ScriptPropertyMetadata 对象编码为 YAML 节点或从 YAML 节点解码为 ScriptPropertyMetadata 对象。
     */
    template <>
    struct convert<ScriptPropertyMetadata>
    {
        /**
         * @brief 将 ScriptPropertyMetadata 对象编码为 YAML 节点。
         *
         * @param rhs 要编码的 ScriptPropertyMetadata 对象。
         * @return 表示 ScriptPropertyMetadata 对象的 YAML 节点。
         */
        static Node encode(const ScriptPropertyMetadata& rhs)
        {
            Node node;
            node["Name"] = rhs.name;
            node["Type"] = rhs.type;
            if (!rhs.defaultValue.empty())
            {
                node["DefaultValue"] = rhs.defaultValue;
            }
            node["CanGet"] = rhs.canGet;
            node["CanSet"] = rhs.canSet;
            node["IsPublic"] = rhs.isPublic;
            if (!rhs.eventSignature.empty())
            {
                node["EventSignature"] = rhs.eventSignature;
            }
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ScriptPropertyMetadata 对象。
         *
         * @param node 包含 ScriptPropertyMetadata 数据的 YAML 节点。
         * @param rhs 将解码的数据存储到的 ScriptPropertyMetadata 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ScriptPropertyMetadata& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.name = node["Name"].as<std::string>();
            rhs.type = node["Type"].as<std::string>();

            if (node["DefaultValue"])
            {
                // DefaultValue 可能是数字或字符串，统一作为字符串处理
                rhs.defaultValue = node["DefaultValue"].as<std::string>();
            }
            else
            {
                rhs.defaultValue = "";
            }

            rhs.canGet = node["CanGet"].as<bool>();
            rhs.canSet = node["CanSet"].as<bool>();
            rhs.isPublic = node["IsPublic"].as<bool>();

            if (node["EventSignature"])
            {
                rhs.eventSignature = node["EventSignature"].as<std::string>();
            }
            else
            {
                rhs.eventSignature = "";
            }

            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 ScriptMethodMetadata 类型。
     *
     * 允许将 ScriptMethodMetadata 对象编码为 YAML 节点或从 YAML 节点解码为 ScriptMethodMetadata 对象。
     */
    template <>
    struct convert<ScriptMethodMetadata>
    {
        /**
         * @brief 将 ScriptMethodMetadata 对象编码为 YAML 节点。
         *
         * @param rhs 要编码的 ScriptMethodMetadata 对象。
         * @return 表示 ScriptMethodMetadata 对象的 YAML 节点。
         */
        static Node encode(const ScriptMethodMetadata& rhs)
        {
            Node node;
            node["Name"] = rhs.name;
            node["ReturnType"] = rhs.returnType;
            node["Signature"] = rhs.signature;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ScriptMethodMetadata 对象。
         *
         * @param node 包含 ScriptMethodMetadata 数据的 YAML 节点。
         * @param rhs 将解码的数据存储到的 ScriptMethodMetadata 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ScriptMethodMetadata& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.name = node["Name"].as<std::string>();
            rhs.returnType = node["ReturnType"].as<std::string>();
            rhs.signature = node["Signature"].as<std::string>();

            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 ScriptClassMetadata 类型。
     *
     * 允许将 ScriptClassMetadata 对象编码为 YAML 节点或从 YAML 节点解码为 ScriptClassMetadata 对象。
     */
    template <>
    struct convert<ScriptClassMetadata>
    {
        /**
         * @brief 将 ScriptClassMetadata 对象编码为 YAML 节点。
         *
         * @param rhs 要编码的 ScriptClassMetadata 对象。
         * @return 表示 ScriptClassMetadata 对象的 YAML 节点。
         */
        static Node encode(const ScriptClassMetadata& rhs)
        {
            Node node;
            node["Name"] = rhs.name;
            node["FullName"] = rhs.fullName;
            node["Namespace"] = rhs.nspace;

            if (!rhs.exportedProperties.empty())
            {
                node["ExportedProperties"] = rhs.exportedProperties;
            }

            if (!rhs.publicMethods.empty())
            {
                node["PublicMethods"] = rhs.publicMethods;
            }

            if (!rhs.publicStaticMethods.empty())
            {
                node["PublicStaticMethods"] = rhs.publicStaticMethods;
            }

            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ScriptClassMetadata 对象。
         *
         * @param node 包含 ScriptClassMetadata 数据的 YAML 节点。
         * @param rhs 将解码的数据存储到的 ScriptClassMetadata 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ScriptClassMetadata& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.name = node["Name"].as<std::string>();
            rhs.fullName = node["FullName"].as<std::string>();
            rhs.nspace = node["Namespace"].as<std::string>();

            if (node["ExportedProperties"])
            {
                rhs.exportedProperties = node["ExportedProperties"].as<std::vector<ScriptPropertyMetadata>>();
            }

            if (node["PublicMethods"])
            {
                rhs.publicMethods = node["PublicMethods"].as<std::vector<ScriptMethodMetadata>>();
            }

            if (node["PublicStaticMethods"])
            {
                rhs.publicStaticMethods = node["PublicStaticMethods"].as<std::vector<ScriptMethodMetadata>>();
            }
            return true;
        }
    };
}

#endif //SCRIPTMETADATA_H
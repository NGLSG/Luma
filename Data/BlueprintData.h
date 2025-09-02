#ifndef LUMAENGINE_BLUEPRINTDATA_H
#define LUMAENGINE_BLUEUMAENGINE_BLUEPRINTDATA_H

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>


/**
 * @brief 表示蓝图中的一个函数节点。
 *
 * 包含函数所属的类名、函数名以及其输入和输出参数。
 */
struct FunctionNode
{
    std::string ClassName; ///< 函数所属的类名。

    std::string FunctionName; ///< 函数的名称。

    std::unordered_map<std::string, std::string> Inputs; ///< 函数的输入参数，键为参数名，值为参数类型或值。

    std::unordered_map<std::string, std::string> Outputs; ///< 函数的输出参数，键为参数名，值为参数类型或值。
};


/**
 * @brief 表示一个完整的蓝图。
 *
 * 包含蓝图的名称和其中包含的所有函数节点。
 */
struct Blueprint
{
    std::string Name; ///< 蓝图的名称。

    std::vector<FunctionNode> Nodes; ///< 蓝图中包含的所有函数节点。
};


namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于将 FunctionNode 对象与 YAML 节点之间进行转换。
     */
    template <>
    struct convert<FunctionNode>
    {
        /**
         * @brief 将 FunctionNode 对象编码为 YAML 节点。
         * @param rhs 要编码的 FunctionNode 对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const FunctionNode& rhs)
        {
            Node node;
            node["ClassName"] = rhs.ClassName;
            node["FunctionName"] = rhs.FunctionName;

            if (!rhs.Inputs.empty())
            {
                node["Inputs"] = rhs.Inputs;
            }
            if (!rhs.Outputs.empty())
            {
                node["Outputs"] = rhs.Outputs;
            }
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 FunctionNode 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 FunctionNode 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, FunctionNode& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }


            rhs.ClassName = node["ClassName"].as<std::string>();
            rhs.FunctionName = node["FunctionName"].as<std::string>();


            if (node["Inputs"])
            {
                rhs.Inputs = node["Inputs"].as<std::unordered_map<std::string, std::string>>();
            }
            if (node["Outputs"])
            {
                rhs.Outputs = node["Outputs"].as<std::unordered_map<std::string, std::string>>();
            }
            return true;
        }
    };


    /**
     * @brief YAML 转换器特化，用于将 Blueprint 对象与 YAML 节点之间进行转换。
     */
    template <>
    struct convert<Blueprint>
    {
        /**
         * @brief 将 Blueprint 对象编码为 YAML 节点。
         * @param rhs 要编码的 Blueprint 对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const Blueprint& rhs)
        {
            Node node;
            node["Name"] = rhs.Name;
            node["Nodes"] = rhs.Nodes;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Blueprint 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 Blueprint 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Blueprint& rhs)
        {
            if (!node.IsMap() || !node["Name"] || !node["Nodes"])
            {
                return false;
            }

            rhs.Name = node["Name"].as<std::string>();
            rhs.Nodes = node["Nodes"].as<std::vector<FunctionNode>>();
            return true;
        }
    };
}

#endif
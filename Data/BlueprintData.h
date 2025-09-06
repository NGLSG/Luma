#ifndef LUMAENGINE_BLUEPRINTDATA_H
#define LUMAENGINE_BLUEPRINTDATA_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <yaml-cpp/yaml.h>

/**
 * @brief 定义蓝图节点的类型。
 */
enum class BlueprintNodeType
{
    Event, ///< 事件节点，作为执行的起点 (例如 OnUpdate, OnCollision)。
    FunctionCall, ///< 调用一个成员函数或静态函数。
    VariableGet, ///< 获取一个蓝图变量的值。
    VariableSet, ///< 设置一个蓝图变量的值。
    FlowControl, ///< 流程控制节点 (例如 If, ForLoop)。
    FunctionEntry, ///< 函数入口节点，定义函数的开始和参数输出。
    Declaration, /// < 声明节点，用于声明局部变量等。

};

/**
 * @brief 蓝图中定义的变量。
 */
struct BlueprintVariable
{
    std::string Name; ///< 变量名。
    std::string Type; ///< 变量的C#类型 (例如 "System.Single", "UnityEngine.GameObject")。
    std::string DefaultValue; ///< 变量的默认值（以字符串形式存储）。
};

/**
 * @brief 表示蓝图中的一个节点。
 *
 * 节点是蓝图的基本组成单元，可以是事件、函数调用等。
 */
struct BlueprintNode
{
    uint32_t ID; ///< 在蓝图内唯一的节点ID。
    BlueprintNodeType Type; ///< 节点的类型。
    std::string Comment; ///< 节点上的注释。

    // 根据节点类型选择性使用的字段
    std::string TargetClassFullName; ///< [FunctionCall] 目标函数的完整类名。
    std::string TargetMemberName; ///< [FunctionCall, Event] 目标函数、事件或属性的名称。
    std::string VariableName; ///< [VariableGet, VariableSet] 目标变量的名称。
    bool IsStatic = false; ///< [FunctionCall] 指示目标函数是否为静态。
    // 用于存储未连接的输入引脚的默认值
    std::unordered_map<std::string, std::string> InputDefaults;

    // 用于可视化编辑器的位置信息
    struct
    {
        float x = 0.0f;
        float y = 0.0f;
    } Position;
};


/**
 * @brief 表示蓝图中两个节点引脚之间的连接（连线）。
 */
struct BlueprintLink
{
    uint32_t FromNodeID; ///< 输出节点ID。
    std::string FromPinName; ///< 输出引脚名称 (例如 "Then", "ReturnValue")。
    uint32_t ToNodeID; ///< 输入节点ID。
    std::string ToPinName; ///< 输入引脚名称 (例如 "Execute", "Value")。
};

struct BlueprintParameter
{
    std::string Name;
    std::string Type;
};

struct BlueprintFunction
{
    uint32_t ID;
    std::string Name;
    std::vector<BlueprintParameter> Parameters;
    std::string ReturnType = "void";
    std::string Visibility = "Public";
    bool IsStatic = false;
};

struct BlueprintCommentRegion
{
    uint32_t ID;
    std::string Title;
    uint32_t FunctionID = 0;

    struct
    {
        float x = 0.0f, y = 0.0f;
    } Position;

    struct
    {
        float w = 100.0f, h = 100.0f;
    } Size;
};

/**
 * @brief 表示一个完整的蓝图，它本身可以被视为一个类。
 */
struct Blueprint
{
    std::string Name; ///< 蓝图的名称，将作为生成的C#类名。
    std::string ParentClass = "Script"; ///< 蓝图继承的基类 (例如 "Luma.SDK.Script")。

    std::vector<BlueprintVariable> Variables; ///< 蓝图中定义的所有变量。
    std::vector<BlueprintFunction> Functions; ///< 蓝图中定义的所有函数。
    std::vector<BlueprintCommentRegion> CommentRegions; ///< 蓝图中的注释区域。
    std::vector<BlueprintNode> Nodes; ///< 蓝图中所有的节点。
    std::vector<BlueprintLink> Links; ///< 蓝图中所有的连接。
};


// 辅助函数，用于枚举和字符串的转换
namespace
{
    inline std::string BlueprintNodeTypeToString(BlueprintNodeType type)
    {
        switch (type)
        {
        case BlueprintNodeType::Event: return "Event";
        case BlueprintNodeType::FunctionCall: return "FunctionCall";
        case BlueprintNodeType::VariableGet: return "VariableGet";
        case BlueprintNodeType::VariableSet: return "VariableSet";
        case BlueprintNodeType::FlowControl: return "FlowControl";
        case BlueprintNodeType::FunctionEntry: return "FunctionEntry";
        case BlueprintNodeType::Declaration: return "Declaration";
        default: return "Unknown";
        }
    }

    inline BlueprintNodeType StringToBlueprintNodeType(const std::string& str)
    {
        if (str == "Event") return BlueprintNodeType::Event;
        if (str == "FunctionCall") return BlueprintNodeType::FunctionCall;
        if (str == "VariableGet") return BlueprintNodeType::VariableGet;
        if (str == "VariableSet") return BlueprintNodeType::VariableSet;
        if (str == "FlowControl") return BlueprintNodeType::FlowControl;
        if (str == "FunctionEntry") return BlueprintNodeType::FunctionEntry;
        if (str == "Declaration") return BlueprintNodeType::Declaration;
        return BlueprintNodeType::FunctionCall; // 默认
    }
}


namespace YAML
{
    template <>
    struct convert<BlueprintParameter>
    {
        static Node encode(const BlueprintParameter& rhs)
        {
            Node node;
            node["Name"] = rhs.Name;
            node["Type"] = rhs.Type;
            return node;
        }

        static bool decode(const Node& node, BlueprintParameter& rhs)
        {
            if (!node.IsMap() || !node["Name"] || !node["Type"]) return false;
            rhs.Name = node["Name"].as<std::string>();
            rhs.Type = node["Type"].as<std::string>();
            return true;
        }
    };

    template <>
    struct convert<BlueprintFunction>
    {
        static Node encode(const BlueprintFunction& rhs)
        {
            Node node;
            node["ID"] = rhs.ID;
            node["Name"] = rhs.Name;
            node["ReturnType"] = rhs.ReturnType;
            node["Visibility"] = rhs.Visibility;
            node["IsStatic"] = rhs.IsStatic;
            if (!rhs.Parameters.empty()) node["Parameters"] = rhs.Parameters;
            return node;
        }

        static bool decode(const Node& node, BlueprintFunction& rhs)
        {
            if (!node.IsMap() || !node["ID"] || !node["Name"]) return false;
            rhs.ID = node["ID"].as<uint32_t>();
            rhs.Name = node["Name"].as<std::string>();
            rhs.ReturnType = node["ReturnType"] ? node["ReturnType"].as<std::string>() : "void";
            rhs.Visibility = node["Visibility"] ? node["Visibility"].as<std::string>() : "Public";
            rhs.IsStatic = node["IsStatic"] ? node["IsStatic"].as<bool>() : false;
            if (node["Parameters"]) rhs.Parameters = node["Parameters"].as<std::vector<BlueprintParameter>>();
            return true;
        }
    };

    template <>
    struct convert<BlueprintCommentRegion>
    {
        static Node encode(const BlueprintCommentRegion& rhs)
        {
            Node node;
            node["ID"] = rhs.ID;
            node["Title"] = rhs.Title;
            node["FunctionID"] = rhs.FunctionID;
            node["Position"]["x"] = rhs.Position.x;
            node["Position"]["y"] = rhs.Position.y;
            node["Size"]["w"] = rhs.Size.w;
            node["Size"]["h"] = rhs.Size.h;
            return node;
        }

        static bool decode(const Node& node, BlueprintCommentRegion& rhs)
        {
            if (!node.IsMap() || !node["ID"] || !node["Title"]) return false;
            rhs.ID = node["ID"].as<uint32_t>();
            rhs.Title = node["Title"].as<std::string>();
            rhs.FunctionID = node["FunctionID"] ? node["FunctionID"].as<uint32_t>() : 0;
            if (node["Position"])
            {
                rhs.Position.x = node["Position"]["x"].as<float>();
                rhs.Position.y = node["Position"]["y"].as<float>();
            }
            if (node["Size"])
            {
                rhs.Size.w = node["Size"]["w"].as<float>();
                rhs.Size.h = node["Size"]["h"].as<float>();
            }
            return true;
        }
    };

    template <>
    struct convert<BlueprintVariable>
    {
        static Node encode(const BlueprintVariable& rhs)
        {
            Node node;
            node["Name"] = rhs.Name;
            node["Type"] = rhs.Type;
            if (!rhs.DefaultValue.empty())
            {
                node["DefaultValue"] = rhs.DefaultValue;
            }
            return node;
        }

        static bool decode(const Node& node, BlueprintVariable& rhs)
        {
            if (!node.IsMap() || !node["Name"] || !node["Type"]) return false;
            rhs.Name = node["Name"].as<std::string>();
            rhs.Type = node["Type"].as<std::string>();
            rhs.DefaultValue = node["DefaultValue"] ? node["DefaultValue"].as<std::string>() : "";
            return true;
        }
    };

    template <>
    struct convert<BlueprintNode>
    {
        static Node encode(const BlueprintNode& rhs)
        {
            Node node;
            node["ID"] = rhs.ID;
            node["Type"] = BlueprintNodeTypeToString(rhs.Type);
            if (!rhs.Comment.empty()) node["Comment"] = rhs.Comment;

            if (!rhs.TargetClassFullName.empty()) node["TargetClassFullName"] = rhs.TargetClassFullName;
            if (!rhs.TargetMemberName.empty()) node["TargetMemberName"] = rhs.TargetMemberName;
            if (!rhs.VariableName.empty()) node["VariableName"] = rhs.VariableName;
            if (!rhs.InputDefaults.empty()) node["InputDefaults"] = rhs.InputDefaults;
            node["IsStatic"] = rhs.IsStatic;

            node["Position"]["x"] = rhs.Position.x;
            node["Position"]["y"] = rhs.Position.y;

            return node;
        }

        static bool decode(const Node& node, BlueprintNode& rhs)
        {
            if (!node.IsMap() || !node["ID"] || !node["Type"]) return false;

            rhs.ID = node["ID"].as<uint32_t>();
            rhs.Type = StringToBlueprintNodeType(node["Type"].as<std::string>());
            rhs.Comment = node["Comment"] ? node["Comment"].as<std::string>() : "";

            rhs.TargetClassFullName = node["TargetClassFullName"] ? node["TargetClassFullName"].as<std::string>() : "";
            rhs.TargetMemberName = node["TargetMemberName"] ? node["TargetMemberName"].as<std::string>() : "";
            rhs.VariableName = node["VariableName"] ? node["VariableName"].as<std::string>() : "";
            if (node["IsStatic"])
            {
                rhs.IsStatic = node["IsStatic"].as<bool>();
            }

            if (node["InputDefaults"])
            {
                rhs.InputDefaults = node["InputDefaults"].as<std::unordered_map<std::string, std::string>>();
            }
            if (node["Position"])
            {
                rhs.Position.x = node["Position"]["x"].as<float>();
                rhs.Position.y = node["Position"]["y"].as<float>();
            }

            return true;
        }
    };

    template <>
    struct convert<BlueprintLink>
    {
        static Node encode(const BlueprintLink& rhs)
        {
            Node node;
            node["FromNodeID"] = rhs.FromNodeID;
            node["FromPinName"] = rhs.FromPinName;
            node["ToNodeID"] = rhs.ToNodeID;
            node["ToPinName"] = rhs.ToPinName;
            return node;
        }

        static bool decode(const Node& node, BlueprintLink& rhs)
        {
            if (!node.IsMap() || !node["FromNodeID"] || !node["ToNodeID"]) return false;
            rhs.FromNodeID = node["FromNodeID"].as<uint32_t>();
            rhs.FromPinName = node["FromPinName"].as<std::string>();
            rhs.ToNodeID = node["ToNodeID"].as<uint32_t>();
            rhs.ToPinName = node["ToPinName"].as<std::string>();
            return true;
        }
    };

    template <>
    struct convert<Blueprint>
    {
        static Node encode(const Blueprint& rhs)
        {
            Node node;
            node["Name"] = rhs.Name;
            node["ParentClass"] = rhs.ParentClass;
            if (!rhs.Variables.empty()) node["Variables"] = rhs.Variables;
            if (!rhs.Nodes.empty()) node["Nodes"] = rhs.Nodes;
            if (!rhs.Links.empty()) node["Links"] = rhs.Links;
            if (!rhs.Functions.empty()) node["Functions"] = rhs.Functions;
            if (!rhs.CommentRegions.empty()) node["CommentRegions"] = rhs.CommentRegions;
            return node;
        }

        static bool decode(const Node& node, Blueprint& rhs)
        {
            if (!node.IsMap() || !node["Name"] || !node["ParentClass"]) return false;

            rhs.Name = node["Name"].as<std::string>();
            rhs.ParentClass = node["ParentClass"].as<std::string>();

            if (node["Variables"]) rhs.Variables = node["Variables"].as<std::vector<BlueprintVariable>>();
            if (node["Nodes"]) rhs.Nodes = node["Nodes"].as<std::vector<BlueprintNode>>();
            if (node["Links"]) rhs.Links = node["Links"].as<std::vector<BlueprintLink>>();
            if (node["Functions"]) rhs.Functions = node["Functions"].as<std::vector<BlueprintFunction>>();
            if (node["CommentRegions"])
                rhs.CommentRegions = node["CommentRegions"].as<std::vector<
                    BlueprintCommentRegion>>();

            return true;
        }
    };
}
#endif //LUMAENGINE_BLUEPRINTDATA_H

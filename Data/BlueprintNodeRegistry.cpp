#include "BlueprintNodeRegistry.h"
#include "ScriptMetadataRegistry.h"
#include "Utils/Logger.h"
#include <imgui_node_editor.h>
#include "BlueprintData.h"
#include <sstream> // ✨ 新增: 用于字符串流处理

namespace ed = ax::NodeEditor;

namespace
{
    /**
     * @brief 去除字符串两端的空白字符。
     * @param str 输入字符串。
     * @return 去除空白后的新字符串。
     */
    std::string Trim(const std::string& str)
    {
        const auto strBegin = str.find_first_not_of(" \t\n\r");
        if (strBegin == std::string::npos)
            return "";

        const auto strEnd = str.find_last_not_of(" \t\n\r");
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    /**
     * @brief 按指定分隔符分割字符串。
     * @param s 输入字符串。
     * @param delimiter 分隔符。
     * @return 分割后的字符串向量。
     */
    std::vector<std::string> Split(const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }
}


BlueprintNodeRegistry::BlueprintNodeRegistry()
{
    RegisterAll();
}

void BlueprintNodeRegistry::RegisterAll()
{
    m_definitions.clear();
    m_categorizedDefinitions.clear();

    RegisterCoreNodes();
    RegisterScriptNodes();

    //测试输出所有注册的节点
    for (const auto& [name, def] : m_definitions)
    {
        LogInfo("Registered Blueprint Node: {} (Category: {})", name, def.Category);
    }
}

void BlueprintNodeRegistry::RegisterNode(BlueprintNodeDefinition definition)
{
    std::string fullName = definition.FullName;
    if (m_definitions.count(fullName))
    {
        LogWarn("Blueprint Node Definition '{}' already exists and will be overwritten.", fullName);
    }

    m_definitions[fullName] = std::move(definition);

    m_categorizedDefinitions.clear();
    for (const auto& [name, def] : m_definitions)
    {
        m_categorizedDefinitions[def.Category].push_back(&m_definitions.at(name));
    }
}

const BlueprintNodeDefinition* BlueprintNodeRegistry::GetDefinition(const std::string& fullName) const
{
    auto it = m_definitions.find(fullName);
    if (it != m_definitions.end())
    {
        return &it->second;
    }
    return nullptr;
}

const std::map<std::string, std::vector<const BlueprintNodeDefinition*>>&
BlueprintNodeRegistry::GetCategorizedDefinitions() const
{
    return m_categorizedDefinitions;
}

void BlueprintNodeRegistry::RegisterScriptNodes()
{
    const auto& allMetadata = ScriptMetadataRegistry::GetInstance().GetAllMetadata();

    for (const auto& [className, classMeta] : allMetadata)
    {
        std::string category = "脚本函数|" + (classMeta.nspace == "<global namespace>" ? "全局" : classMeta.nspace);

        for (const auto& methodMeta : classMeta.publicStaticMethods)
        {
            BlueprintNodeDefinition def;
            def.FullName = classMeta.fullName + "." + methodMeta.name;
            def.DisplayName = classMeta.name + "::" + methodMeta.name;
            def.Category = category;
            def.NodeType = BlueprintNodeType::FunctionCall;

            def.InputPins.push_back({"", "Exec", ed::PinKind::Input});

            auto paramTypes = Split(methodMeta.signature, ',');
            if (!(paramTypes.size() == 1 && (paramTypes[0] == "void" || paramTypes[0].empty())))
            {
                for (size_t i = 0; i < paramTypes.size(); ++i)
                {
                    def.InputPins.push_back({"参数" + std::to_string(i + 1), Trim(paramTypes[i]), ed::PinKind::Input});
                }
            }

            def.OutputPins.push_back({"然后", "Exec", ed::PinKind::Output});
            if (methodMeta.returnType != "void")
            {
                def.OutputPins.push_back({"返回值", methodMeta.returnType, ed::PinKind::Output});
            }
            RegisterNode(def);
        }

        for (const auto& methodMeta : classMeta.publicMethods)
        {
            BlueprintNodeDefinition def;
            def.FullName = classMeta.fullName + "." + methodMeta.name;
            def.DisplayName = classMeta.name + "." + methodMeta.name;
            def.Category = category;
            def.NodeType = BlueprintNodeType::FunctionCall;

            def.InputPins.push_back({"目标", classMeta.fullName, ed::PinKind::Input});
            def.InputPins.push_back({"", "Exec", ed::PinKind::Input});

            auto paramTypes = Split(methodMeta.signature, ',');
            if (!(paramTypes.size() == 1 && (paramTypes[0] == "void" || paramTypes[0].empty())))
            {
                for (size_t i = 0; i < paramTypes.size(); ++i)
                {
                    def.InputPins.push_back({"参数" + std::to_string(i + 1), Trim(paramTypes[i]), ed::PinKind::Input});
                }
            }

            def.OutputPins.push_back({"然后", "Exec", ed::PinKind::Output});
            if (methodMeta.returnType != "void")
            {
                def.OutputPins.push_back({"返回值", methodMeta.returnType, ed::PinKind::Output});
            }
            RegisterNode(def);
        }
    }
}

void BlueprintNodeRegistry::RegisterCoreNodes()
{
    const std::string entityType = "Luma.SDK.Entity";
    RegisterNode({
        .FullName = "Variable.Declare",
        .DisplayName = "声明变量",
        .Category = "变量",
        .Description = "在当前作用域内声明一个新的局部变量。",
        .NodeType = BlueprintNodeType::Declaration,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"变量名", "NodeInputText", ed::PinKind::Input},
            {"变量类型", "SelectType", ed::PinKind::Input},
            {"初始值", "NodeInputText", ed::PinKind::Input},
        },
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"输出变量", "System.Object", ed::PinKind::Output} // 类型是占位符, 会动态更新
        }
    });

    RegisterNode({
        .FullName = "Utility.GetSelf",
        .DisplayName = "获取自身引用",
        .Category = "通用",
        .Description = "获取当前蓝图脚本实例的引用 (this)。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {},
        .OutputPins = {
            {"自身", "self", ed::PinKind::Output}
        }
    });
    RegisterNode({
        .FullName = "FlowControl.Return",
        .DisplayName = "返回值",
        .Category = "流程控制",
        .Description = "从当前函数返回，可选择性地附带一个返回值。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"返回类型", "SelectType", ed::PinKind::Input},
            {"输入值", "System.Object", ed::PinKind::Input}
        },
        .OutputPins = {}
    });

    RegisterNode({
        .FullName = "FlowControl.If",
        .DisplayName = "如果分支",
        .Category = "流程控制",
        .Description = "根据条件的真假执行不同的分支，之后合并执行。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"条件", "InputString", ed::PinKind::Input}
        },
        .OutputPins = {
            {"为真", "Exec", ed::PinKind::Output},
            {"为假", "Exec", ed::PinKind::Output},
            {"然后", "Exec", ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "FlowControl.ForLoop",
        .DisplayName = "循环",
        .Category = "流程控制",
        .Description = "在指定的索引范围内重复执行一段逻辑。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"起始索引", "System.Int32", ed::PinKind::Input},
            {"结束索引", "System.Int32", ed::PinKind::Input}
        },
        .OutputPins = {
            {"循环体", "Exec", ed::PinKind::Output},
            {"当前索引", "System.Int32", ed::PinKind::Output},
            {"然后", "Exec", ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnCreate",
        .DisplayName = "当创建时",
        .Category = "事件|生命周期",
        .Description = "当脚本实例首次被创建时执行一次。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnUpdate",
        .DisplayName = "当每帧更新时",
        .Category = "事件|生命周期",
        .Description = "在每一帧执行一次。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"帧间隔", "System.Single", ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnDestroy",
        .DisplayName = "当销毁时",
        .Category = "事件|生命周期",
        .Description = "当实体或组件被销毁时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {{"然后", "Exec", ed::PinKind::Output}}
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnEnable",
        .DisplayName = "当启用时",
        .Category = "事件|生命周期",
        .Description = "当组件或实体被启用时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {{"然后", "Exec", ed::PinKind::Output}}
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnDisable",
        .DisplayName = "当禁用时",
        .Category = "事件|生命周期",
        .Description = "当组件或实体被禁用时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {{"然后", "Exec", ed::PinKind::Output}}
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnCollisionEnter",
        .DisplayName = "当碰撞开始时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体进入该碰撞体时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnCollisionStay",
        .DisplayName = "当碰撞持续时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体停留在该碰撞体内时，每帧执行一次。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnCollisionExit",
        .DisplayName = "当碰撞结束时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体离开该碰撞体时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnTriggerEnter",
        .DisplayName = "当触发器进入时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体进入该触发器区域时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnTriggerStay",
        .DisplayName = "当触发器停留时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体停留在该触发器内时，每帧执行一次。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Script.OnTriggerExit",
        .DisplayName = "当触发器退出时",
        .Category = "事件|物理",
        .Description = "当另一个碰撞体离开该触发器区域时执行。",
        .NodeType = BlueprintNodeType::Event,
        .InputPins = {},
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"另一物体", entityType, ed::PinKind::Output}
        }
    });
}

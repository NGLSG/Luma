#include "BlueprintNodeRegistry.h"
#include "ScriptMetadataRegistry.h"
#include "Utils/Logger.h"
#include <imgui_node_editor.h>
#include "BlueprintData.h"
#include <sstream>

namespace ed = ax::NodeEditor;
const std::string entityType = "Luma.SDK.Entity";
namespace
{
    std::string Trim(const std::string& str)
    {
        const auto strBegin = str.find_first_not_of(" \t\n\r");
        if (strBegin == std::string::npos)
            return "";

        const auto strEnd = str.find_last_not_of(" \t\n\r");
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }


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
    RegisterSDKNodes();
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
            {"输出变量", "System.Object", ed::PinKind::Output}
        }
    });
    RegisterNode({
        .FullName = "Utility.Input",
        .DisplayName = "输入",
        .Category = "通用",
        .Description = "提供一个无需声明变量的即时输入值。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {
            {"值", "NodeInputText", ed::PinKind::Input},
            {"类型", "SelectType", ed::PinKind::Input},
        },
        .OutputPins = {
            {"输出", "System.Object", ed::PinKind::Output}
        }
    });
    RegisterNode({
        .FullName = "Variable.Set",
        .DisplayName = "设置变量",
        .Category = "变量",
        .Description = "修改一个已存在变量的值。",
        .NodeType = BlueprintNodeType::VariableSet,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"变量名", "NodeInputText", ed::PinKind::Input},
            {"值", "NodeInputText", ed::PinKind::Input},
        },
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output}
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
        .FullName = "Utility.GetEntity",
        .DisplayName = "获取实体",
        .Category = "通用",
        .Description = "获取当前蓝图脚本实例的实体。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {},
        .OutputPins = {
            {"实体", entityType, ed::PinKind::Output}
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
            {"条件", "System.Boolean", ed::PinKind::Input}
        },
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"为真", "Exec", ed::PinKind::Output},
            {"为假", "Exec", ed::PinKind::Output},

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
            {"然后", "Exec", ed::PinKind::Output},
            {"循环体", "Exec", ed::PinKind::Output},
            {"当前索引", "System.Int32", ed::PinKind::Output},

        }
    });

    RegisterNode({
        .FullName = "Utility.MakeArray",
        .DisplayName = "创建数组",
        .Category = "通用",
        .Description = "创建一个指定类型的数组。",
        .NodeType = BlueprintNodeType::FlowControl,
        .InputPins = {
            {"", "Exec", ed::PinKind::Input},
            {"元素类型", "SelectType", ed::PinKind::Input},
            {"参数列表", "Args", ed::PinKind::Input},
        },
        .OutputPins = {
            {"然后", "Exec", ed::PinKind::Output},
            {"数组", "System.Array", ed::PinKind::Output},

        },
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

void BlueprintNodeRegistry::RegisterSDKNodes()
{
    RegisterNode({
        .FullName = "Luma.SDK.Debug.Log", .DisplayName = "日志::普通输出", .Category = "SDK|调试",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"message", "System.Object"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Debug.LogWarning", .DisplayName = "日志::输出警告", .Category = "SDK|调试",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"message", "System.Object"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Debug.LogError", .DisplayName = "日志::输出错误", .Category = "SDK|调试",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"message", "System.Object"}},
        .OutputPins = {{"然后", "Exec"}}
    });


    RegisterNode({
        .FullName = "Luma.SDK.Input.GetCursorPosition", .DisplayName = "鼠标::获取坐标", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "Luma.SDK.Vector2Int"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.GetCursorDelta", .DisplayName = "鼠标::获取移动增量", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "Luma.SDK.Vector2Int"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.GetScrollDelta", .DisplayName = "鼠标::获取滚动增量", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "Luma.SDK.Vector2"}}
    });


    RegisterNode({
        .FullName = "Luma.SDK.Input.IsLeftMouseButtonPressed", .DisplayName = "鼠标::左键按下瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsLeftMouseButtonDown", .DisplayName = "鼠标::左键持续按下", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsLeftMouseButtonUp", .DisplayName = "鼠标::左键抬起瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });


    RegisterNode({
        .FullName = "Luma.SDK.Input.IsRightMouseButtonPressed", .DisplayName = "鼠标::右键按下瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsRightMouseButtonDown", .DisplayName = "鼠标::右键持续按下", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsRightMouseButtonUp", .DisplayName = "鼠标::右键抬起瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });


    RegisterNode({
        .FullName = "Luma.SDK.Input.IsMiddleMouseButtonPressed", .DisplayName = "鼠标::中键按下瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsMiddleMouseButtonDown", .DisplayName = "鼠标::中键持续按下", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsMiddleMouseButtonUp", .DisplayName = "鼠标::中键抬起瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });

    RegisterNode({
        .FullName = "Luma.SDK.Input.IsKeyPressed", .DisplayName = "键盘::按键按下瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"scancode", "Luma.SDK.Scancode"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsKeyUp", .DisplayName = "键盘::按键抬起瞬间", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"scancode", "Luma.SDK.Scancode"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Input.IsKeyDown", .DisplayName = "键盘::按键持续按下", .Category = "SDK|输入",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"", "Exec"}, {"scancode", "Luma.SDK.Scancode"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.EventManager.Subscribe", .DisplayName = "事件系统::订阅事件", .Category = "SDK|事件",
        .Description = "订阅一个类型的事件，当该类型事件被触发时，调用指定的回调函数。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"事件类型", "TemplateType"},
            {"回调函数", "FunctionSelection"}
        },
        .OutputPins = {
            {"然后", "Exec"},
            {"返回值", "Luma.SDK.EventHandle"}
        }
    });
    RegisterNode({
        .FullName = "Luma.SDK.EventManager.Unsubscribe", .DisplayName = "事件系统::取消订阅", .Category = "SDK|事件",
        .Description = "取消订阅之前订阅的事件，停止接收该事件的通知。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"订阅句柄", "Luma.SDK.EventHandle"}
        },
        .OutputPins = {
            {"然后", "Exec"}
        }
    });
    RegisterNode({
        .FullName = "Luma.SDK.EventManager.Publish", .DisplayName = "事件系统::触发事件", .Category = "SDK|事件",
        .Description = "触发一个事件，通知所有订阅该事件类型的监听器。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"事件实例", "object"}
        },
        .OutputPins = {
            {"然后", "Exec"}
        }
    });


    RegisterNode({
        .FullName = "Luma.SDK.Entity.SetActive", .DisplayName = "实体.设置激活状态", .Category = "SDK|实体",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", entityType}, {"", "Exec"}, {"active", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.Entity.HasComponent", .DisplayName = "实体.拥有组件", .Category = "SDK|实体|组件",
        .Description = "检查实体是否拥有指定类型的组件。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"目标", entityType},
            {"组件类型", "TemplateType"}
        },
        .OutputPins = {
            {"然后", "Exec"},
            {"返回值", "System.Boolean"}
        }
    });
    RegisterNode({
        .FullName = "Luma.SDK.Entity.SendMessage", .DisplayName = "实体.触发实体方法", .Category = "SDK|实体",
        .Description = "向实体发送一个消息，触发对应的消息处理函数",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"目标", entityType},
            {"消息名", "System.String"},
            {"参数列表", "System.Object[]"}
        },
        .OutputPins = {
            {"然后", "Exec"}
        },
    });


    RegisterNode({
        .FullName = "Luma.SDK.Entity.GetComponent", .DisplayName = "实体.获取组件", .Category = "SDK|实体|组件",
        .Description = "获取实体上指定类型的组件实例。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"目标", entityType},
            {"组件类型", "TemplateType"}
        },
        .OutputPins = {
            {"然后", "Exec"},
            {"返回值", "System.Object"}
        }
    });

    RegisterNode({
        .FullName = "Luma.SDK.Entity.AddComponent", .DisplayName = "实体.添加组件", .Category = "SDK|实体|组件",
        .Description = "为实体添加一个指定类型的新组件。",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {
            {"", "Exec"},
            {"目标", entityType},
            {"组件类型", "TemplateType"}
        },
        .OutputPins = {
            {"然后", "Exec"},
            {"返回值", "System.Object"}
        }
    });

    


    const std::string animControllerType = "Luma.SDK.Components.AnimationController";

    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.Play", .DisplayName = "动画器.播放动画",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"animationName", "System.String"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.Stop", .DisplayName = "动画器.停止动画",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.IsPlaying", .DisplayName = "动画器.是否正在播放指定动画",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"animationName", "System.String"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Boolean"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.SetFrameRate", .DisplayName = "动画器.设置动画帧率",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"frameRate", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.SetFloat", .DisplayName = "动画器.设置浮点参数",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"name", "System.String"}, {"value", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.SetBool", .DisplayName = "动画器.设置开关参数",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"name", "System.String"}, {"value", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.SetTrigger", .DisplayName = "动画器.设置触发器参数",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"name", "System.String"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AnimationController.SetInt", .DisplayName = "动画器.设置整数参数",
        .Category = "SDK|动画",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", animControllerType}, {"", "Exec"}, {"name", "System.String"}, {"value", "System.Int32"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // ==================== 光照组件 ====================
    const std::string directionalLightType = "Luma.SDK.Components.DirectionalLight";
    const std::string pointLightType = "Luma.SDK.Components.PointLight";
    const std::string spotLightType = "Luma.SDK.Components.SpotLight";
    const std::string areaLightType = "Luma.SDK.Components.AreaLight";
    const std::string lightProbeType = "Luma.SDK.Components.LightProbe";

    // 方向光组件
    RegisterNode({
        .FullName = "Luma.SDK.DirectionalLight.SetColor", .DisplayName = "方向光.设置颜色",
        .Category = "SDK|光照|方向光",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", directionalLightType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.DirectionalLight.SetIntensity", .DisplayName = "方向光.设置强度",
        .Category = "SDK|光照|方向光",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", directionalLightType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.DirectionalLight.SetDirection", .DisplayName = "方向光.设置方向",
        .Category = "SDK|光照|方向光",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", directionalLightType}, {"", "Exec"}, {"direction", "Luma.SDK.Vector2"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.DirectionalLight.SetCastShadows", .DisplayName = "方向光.设置投射阴影",
        .Category = "SDK|光照|方向光",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", directionalLightType}, {"", "Exec"}, {"castShadows", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 点光源组件
    RegisterNode({
        .FullName = "Luma.SDK.PointLight.SetColor", .DisplayName = "点光源.设置颜色",
        .Category = "SDK|光照|点光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", pointLightType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PointLight.SetIntensity", .DisplayName = "点光源.设置强度",
        .Category = "SDK|光照|点光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", pointLightType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PointLight.SetRadius", .DisplayName = "点光源.设置半径",
        .Category = "SDK|光照|点光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", pointLightType}, {"", "Exec"}, {"radius", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PointLight.SetCastShadows", .DisplayName = "点光源.设置投射阴影",
        .Category = "SDK|光照|点光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", pointLightType}, {"", "Exec"}, {"castShadows", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 聚光灯组件
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetColor", .DisplayName = "聚光灯.设置颜色",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetIntensity", .DisplayName = "聚光灯.设置强度",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetRadius", .DisplayName = "聚光灯.设置半径",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"radius", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetInnerAngle", .DisplayName = "聚光灯.设置内角",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"innerAngle", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetOuterAngle", .DisplayName = "聚光灯.设置外角",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"outerAngle", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.SpotLight.SetCastShadows", .DisplayName = "聚光灯.设置投射阴影",
        .Category = "SDK|光照|聚光灯",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", spotLightType}, {"", "Exec"}, {"castShadows", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 面光源组件
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetColor", .DisplayName = "面光源.设置颜色",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetIntensity", .DisplayName = "面光源.设置强度",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetSize", .DisplayName = "面光源.设置尺寸",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"width", "System.Single"}, {"height", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetRadius", .DisplayName = "面光源.设置半径",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"radius", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetCastShadows", .DisplayName = "面光源.设置投射阴影",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"castShadows", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AreaLight.SetShadowSoftness", .DisplayName = "面光源.设置阴影柔和度",
        .Category = "SDK|光照|面光源",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", areaLightType}, {"", "Exec"}, {"shadowSoftness", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 光照探针组件
    RegisterNode({
        .FullName = "Luma.SDK.LightProbe.SetInfluenceRadius", .DisplayName = "光照探针.设置影响半径",
        .Category = "SDK|光照|光照探针",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightProbeType}, {"", "Exec"}, {"influenceRadius", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightProbe.GetSampledColor", .DisplayName = "光照探针.获取采样颜色",
        .Category = "SDK|光照|光照探针",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightProbeType}, {"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "Luma.SDK.Color"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightProbe.GetSampledIntensity", .DisplayName = "光照探针.获取采样强度",
        .Category = "SDK|光照|光照探针",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightProbeType}, {"", "Exec"}},
        .OutputPins = {{"然后", "Exec"}, {"返回值", "System.Single"}}
    });

    // ==================== 后处理组件 ====================
    const std::string postProcessType = "Luma.SDK.Components.PostProcessSettings";
    const std::string qualitySettingsType = "Luma.SDK.Components.QualitySettings";
    const std::string lightingSettingsType = "Luma.SDK.Components.LightingSettings";
    const std::string ambientZoneType = "Luma.SDK.Components.AmbientZone";

    // 后处理设置
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetEnableBloom", .DisplayName = "后处理.启用Bloom",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"enable", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetBloomIntensity", .DisplayName = "后处理.设置Bloom强度",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetEnableFog", .DisplayName = "后处理.启用雾效",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"enable", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetFogColor", .DisplayName = "后处理.设置雾效颜色",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetFogDensity", .DisplayName = "后处理.设置雾效密度",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"density", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetExposure", .DisplayName = "后处理.设置曝光",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"exposure", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetContrast", .DisplayName = "后处理.设置对比度",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"contrast", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.PostProcessSettings.SetSaturation", .DisplayName = "后处理.设置饱和度",
        .Category = "SDK|后处理",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", postProcessType}, {"", "Exec"}, {"saturation", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 质量设置
    RegisterNode({
        .FullName = "Luma.SDK.QualitySettings.SetRenderScale", .DisplayName = "质量设置.设置渲染缩放",
        .Category = "SDK|质量设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", qualitySettingsType}, {"", "Exec"}, {"scale", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.QualitySettings.SetShadowMapResolution", .DisplayName = "质量设置.设置阴影贴图分辨率",
        .Category = "SDK|质量设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", qualitySettingsType}, {"", "Exec"}, {"resolution", "System.Int32"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.QualitySettings.SetMaxLightsPerFrame", .DisplayName = "质量设置.设置每帧最大光源数",
        .Category = "SDK|质量设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", qualitySettingsType}, {"", "Exec"}, {"maxLights", "System.Int32"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 光照设置
    RegisterNode({
        .FullName = "Luma.SDK.LightingSettings.SetAmbientColor", .DisplayName = "光照设置.设置环境光颜色",
        .Category = "SDK|光照设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightingSettingsType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightingSettings.SetAmbientIntensity", .DisplayName = "光照设置.设置环境光强度",
        .Category = "SDK|光照设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightingSettingsType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightingSettings.SetEnableShadows", .DisplayName = "光照设置.启用阴影",
        .Category = "SDK|光照设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightingSettingsType}, {"", "Exec"}, {"enable", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightingSettings.SetShadowSoftness", .DisplayName = "光照设置.设置阴影柔和度",
        .Category = "SDK|光照设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightingSettingsType}, {"", "Exec"}, {"softness", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.LightingSettings.SetEnableIndirectLighting", .DisplayName = "光照设置.启用间接光照",
        .Category = "SDK|光照设置",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", lightingSettingsType}, {"", "Exec"}, {"enable", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // 环境区域
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetPrimaryColor", .DisplayName = "环境区域.设置主颜色",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetSecondaryColor", .DisplayName = "环境区域.设置次颜色",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"color", "Luma.SDK.Color"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetIntensity", .DisplayName = "环境区域.设置强度",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"intensity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetSize", .DisplayName = "环境区域.设置尺寸",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"width", "System.Single"}, {"height", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetEdgeSoftness", .DisplayName = "环境区域.设置边缘柔和度",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"softness", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.AmbientZone.SetPriority", .DisplayName = "环境区域.设置优先级",
        .Category = "SDK|环境区域",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", ambientZoneType}, {"", "Exec"}, {"priority", "System.Int32"}},
        .OutputPins = {{"然后", "Exec"}}
    });

    // ==================== 阴影投射器组件 ====================
    const std::string shadowCasterType = "Luma.SDK.Components.ShadowCaster";

    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetOpacity", .DisplayName = "阴影投射器.设置不透明度",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"opacity", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetSelfShadow", .DisplayName = "阴影投射器.设置自阴影",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"selfShadow", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetCircleRadius", .DisplayName = "阴影投射器.设置圆形半径",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"radius", "System.Single"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetRectangleSize", .DisplayName = "阴影投射器.设置矩形尺寸",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"size", "Luma.SDK.Vector2"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetOffset", .DisplayName = "阴影投射器.设置偏移",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"offset", "Luma.SDK.Vector2"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetEnableSDF", .DisplayName = "阴影投射器.启用SDF阴影",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"enable", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
    RegisterNode({
        .FullName = "Luma.SDK.ShadowCaster.SetIsStatic", .DisplayName = "阴影投射器.设置静态",
        .Category = "SDK|阴影",
        .NodeType = BlueprintNodeType::FunctionCall,
        .InputPins = {{"目标", shadowCasterType}, {"", "Exec"}, {"isStatic", "System.Boolean"}},
        .OutputPins = {{"然后", "Exec"}}
    });
}

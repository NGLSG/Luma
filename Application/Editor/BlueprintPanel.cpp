#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "BlueprintPanel.h"
#include "BlueprintNodeRegistry.h"
#include "ScriptMetadataRegistry.h"
#include "Loaders/BlueprintLoader.h"
#include "Resources/AssetManager.h"
#include "Utils/Path.h"
#include "Utils/Logger.h"
#include <imgui.h>
#include <algorithm>
#include <string_view>
#include <implot.h>
#include "imgui_internal.h"
#include "Input/Cursor.h"
#include "Input/Keyboards.h"

struct PairHash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

BlueprintPanel::~BlueprintPanel()
{
    ImPlot::DestroyContext();
    if (m_nodeEditorContext)
    {
        ed::DestroyEditor(m_nodeEditorContext);
        m_nodeEditorContext = nullptr;
    }
}

void BlueprintPanel::Initialize(EditorContext* context)
{
    m_context = context;
    ed::Config config;
    config.SettingsFile = nullptr;
    m_nodeEditorContext = ed::CreateEditor(&config);
    ImPlot::CreateContext();
    m_scriptCompiledListener = EventBus::GetInstance().Subscribe<CSharpScriptCompiledEvent>(
        [this](const CSharpScriptCompiledEvent& e)
        {
            BlueprintNodeRegistry::GetInstance().RegisterAll();
            if (m_currentBlueprint)
            {
                OpenBlueprint(m_currentBlueprintGuid);
            }
        });
}

void BlueprintPanel::Update(float deltaTime)
{
    if (m_context->currentEditingBlueprintGuid.Valid() && m_context->currentEditingBlueprintGuid !=
        m_currentBlueprintGuid)
    {
        OpenBlueprint(m_context->currentEditingBlueprintGuid);
    }
    else if (!m_context->currentEditingBlueprintGuid.Valid() && m_currentBlueprintGuid.Valid())
    {
        CloseCurrentBlueprint();
    }
}

void BlueprintPanel::Shutdown()
{
    CloseCurrentBlueprint();
    if (m_nodeEditorContext)
    {
        ed::DestroyEditor(m_nodeEditorContext);
        m_nodeEditorContext = nullptr;
    }
}

void BlueprintPanel::OpenBlueprint(const Guid& blueprintGuid)
{
    if (m_currentBlueprintGuid == blueprintGuid && m_currentBlueprint)
        return;

    CloseCurrentBlueprint();

    auto loader = BlueprintLoader();
    m_currentBlueprint = loader.LoadAsset(blueprintGuid);

    if (!m_currentBlueprint)
    {
        LogError("无法加载蓝图，GUID: {}", blueprintGuid.ToString());
        return;
    }

    m_currentBlueprintGuid = blueprintGuid;
    m_currentBlueprintName = m_currentBlueprint->GetBlueprintData().Name;
    strncpy(m_blueprintNameBuffer, m_currentBlueprintName.c_str(), sizeof(m_blueprintNameBuffer));
    m_blueprintNameBuffer[sizeof(m_blueprintNameBuffer) - 1] = '\0';
    initializeFromBlueprintData();

    const auto& allTypes = ScriptMetadataRegistry::GetInstance().GetAvailableTypes();
    m_sortedTypeNames = allTypes;
    std::sort(m_sortedTypeNames.begin(), m_sortedTypeNames.end(), [](const std::string& a, const std::string& b)
    {
        if (a.length() != b.length())
        {
            return a.length() < b.length();
        }
        return a < b;
    });

    SetVisible(true);
    m_context->currentEditingBlueprintGuid = blueprintGuid;
    LogInfo("打开蓝图进行编辑: {}", m_currentBlueprintName);
}

void BlueprintPanel::ClearEditorState()
{
    m_nodes.clear();
    m_links.clear();
    m_regions.clear();
    m_nodeMap.clear();
    m_pinMap.clear();
    m_inputStringWindows.clear();
    m_nextNodeId = 1;
    m_nextPinId = 1;
    m_nextLinkId = 1;
    m_nextFunctionId = 1;
    m_nextRegionId = 1;
}

void BlueprintPanel::CloseCurrentBlueprint()
{
    if (!m_currentBlueprint) return;
    LogInfo("关闭蓝图: {}", m_currentBlueprintName);

    ClearEditorState();

    m_currentBlueprint = nullptr;
    m_currentBlueprintGuid = Guid();
    m_currentBlueprintName.clear();

    if (m_context)
    {
        m_context->currentEditingBlueprintGuid = Guid();
    }
}

void BlueprintPanel::Focus()
{
    m_requestFocus = true;
}

void BlueprintPanel::Draw()
{
    if (!m_isVisible) return;
    if (m_requestFocus)
    {
        ImGui::SetNextWindowFocus();
        m_requestFocus = false;
    }

    std::string panelName = m_currentBlueprint
                                ? (m_currentBlueprintName + "###BlueprintEditor")
                                : "蓝图编辑器###BlueprintEditor";

    if (ImGui::Begin(panelName.c_str(), &m_isVisible, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse))
    {
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        drawMenuBar();

        if (!m_currentBlueprint)
        {
            ImVec2 center = ImGui::GetContentRegionAvail();
            const char* text = "请双击蓝图资源以开始编辑";
            ImVec2 textSize = ImGui::CalcTextSize(text);
            ImGui::SetCursorPos(ImVec2((center.x - textSize.x) * 0.5f, (center.y - textSize.y) * 0.5f));
            ImGui::TextUnformatted(text);
        }
        else
        {
            if (ImGui::BeginChild("MainContent", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
            {
                static float splitterWidth = 350.0f;
                ImVec2 contentSize = ImGui::GetContentRegionAvail();

                if (ImGui::BeginChild("NodeEditorWrapper", ImVec2(contentSize.x - splitterWidth - 10, 0), true,
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove))
                {
                    drawNodeEditor();
                }
                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::Button("##splitter", ImVec2(10, -1));
                if (ImGui::IsItemActive())
                {
                    splitterWidth -= ImGui::GetIO().MouseDelta.x;
                    splitterWidth = std::clamp(splitterWidth, 250.0f, contentSize.x - 250.0f);
                }
                ImGui::SameLine();

                if (m_variablesPanelOpen)
                {
                    if (ImGui::BeginChild("SidePanel", ImVec2(splitterWidth, 0), true))
                    {
                        ImGui::Text("蓝图名称:");
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::InputText("##BlueprintName", m_blueprintNameBuffer, sizeof(m_blueprintNameBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            m_currentBlueprintName = m_blueprintNameBuffer;
                            m_currentBlueprint->GetBlueprintData().Name = m_currentBlueprintName;
                            updateSelfNodePinTypes();
                        }
                        ImGui::Separator();
                        if (ImGui::BeginTabBar("SidePanelTabs"))
                        {
                            if (ImGui::BeginTabItem("节点列表"))
                            {
                                drawNodeListPanel();
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("变量"))
                            {
                                drawVariablesPanel();
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("函数"))
                            {
                                drawFunctionsPanel();
                                ImGui::EndTabItem();
                            }
                            ImGui::EndTabBar();
                        }
                    }
                    ImGui::EndChild();
                }
            }
            ImGui::EndChild();
            updateInputStringWindows();
        }
    }
    ImGui::End();
    handleShortcutInput();

    if (m_currentBlueprint)
    {
        drawInputStringWindows();
        drawSelectTypeWindows();
        drawCreateFunctionPopup();
        drawSelectFunctionWindows();
        drawCreateRegionPopup();
    }
}

void BlueprintPanel::updateSelfNodePinTypes()
{
    if (!m_currentBlueprint) return;
    std::string selfType = "GameScripts." + m_currentBlueprint->GetBlueprintData().Name;
    for (auto& node : m_nodes)
    {
        BlueprintNode* sourceData = findSourceDataById(node.sourceDataID);
        if (sourceData && sourceData->TargetMemberName == "GetSelf")
        {
            for (auto& pin : node.outputPins)
            {
                if (pin.name == "自身")
                {
                    pin.type = selfType;
                }
            }
        }
    }
}

void BlueprintPanel::drawSelectTypeWindows()
{
    std::erase_if(m_selectTypeWindows, [](const auto& window) { return !window.isOpen; });

    for (auto& window : m_selectTypeWindows)
    {
        if (!window.isOpen) continue;

        ImGui::SetNextWindowSize(ImVec2(250, 350), ImGuiCond_FirstUseEver);
        if (window.needsFocus)
        {
            ImGui::SetNextWindowFocus();
            window.needsFocus = false;
        }

        if (ImGui::Begin(window.windowId.c_str(), &window.isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
        {
            BlueprintNode* sourceData = findSourceDataById(window.nodeId);
            if (sourceData)
            {
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere(0);
                ImGui::InputText("搜索", window.searchBuffer, sizeof(window.searchBuffer));
                ImGui::Separator();

                if (ImGui::BeginChild("##TypeScrollingRegion"))
                {
                    std::string& selectedType = sourceData->InputDefaults[window.pinName];
                    std::string_view search_sv(window.searchBuffer);

                    if (std::string_view("void").find(search_sv) != std::string_view::npos)
                    {
                        if (ImGui::Selectable("void", selectedType == "void"))
                        {
                            selectedType = "void";
                            window.isOpen = false;
                        }
                    }

                    for (const auto& typeName : m_sortedTypeNames)
                    {
                        if (typeName == "void")
                        {
                            continue;
                        }

                        if (search_sv.empty() || std::string_view(typeName).find(search_sv) != std::string_view::npos)
                        {
                            if (ImGui::Selectable(typeName.c_str(), selectedType == typeName))
                            {
                                selectedType = typeName;
                                window.isOpen = false;
                            }
                        }
                    }
                    ImGui::EndChild();
                }
            }
            else
            {
                ImGui::Text("错误: 找不到源节点数据。");
                if (ImGui::Button("关闭"))
                {
                    window.isOpen = false;
                }
            }
        }
        ImGui::End();
    }
}

BlueprintPanel::SelectTypeWindow* BlueprintPanel::findSelectTypeWindow(uint32_t nodeId, const std::string& pinName)
{
    for (auto& window : m_selectTypeWindows)
    {
        if (window.nodeId == nodeId && window.pinName == pinName)
        {
            return &window;
        }
    }
    return nullptr;
}

void BlueprintPanel::drawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("文件"))
        {
            if (ImGui::MenuItem("保存", "Ctrl+S", false, m_currentBlueprint != nullptr))
            {
                saveToBlueprintData();
            }
            if (ImGui::MenuItem("关闭", "Ctrl+W", false, m_currentBlueprint != nullptr))
            {
                CloseCurrentBlueprint();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("创建"))
        {
            if (ImGui::MenuItem("创建函数..."))
            {
                m_isEditingFunction = false;
                m_functionEditorBuffer = {};
                m_functionEditorBuffer.Name = "NewFunction";
                m_functionEditorBuffer.ReturnType = "void";
                m_functionEditorBuffer.Visibility = "public";
                m_showCreateFunctionPopup = true;
            }
            if (ImGui::MenuItem("创建逻辑区域..."))
            {
                m_showCreateRegionPopup = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("视图"))
        {
            ImGui::MenuItem("侧边栏", nullptr, &m_variablesPanelOpen);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void BlueprintPanel::drawNodeEditor()
{
    ed::SetCurrentEditor(m_nodeEditorContext);
    rebuildPinConnections();

    ed::Begin("BlueprintEditor");

    handleRegionInteraction();
    drawRegions();

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BLUEPRINT_NODE_DEFINITION"))
        {
            const char* nodeFullName = (const char*)payload->Data;
            const auto* definition = BlueprintNodeRegistry::GetInstance().GetDefinition(nodeFullName);
            if (definition)
            {
                ImVec2 nodePosition = ed::ScreenToCanvas({static_cast<float>(LumaCursor::GetPosition().x), static_cast<float>(LumaCursor::GetPosition().y)});
                createNodeFromDefinition(definition, nodePosition);
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BLUEPRINT_FUNCTION_CALL"))
        {
            const char* funcName = (const char*)payload->Data;
            const auto& funcs = m_currentBlueprint->GetBlueprintData().Functions;
            auto it = std::find_if(funcs.begin(), funcs.end(), [&](const BlueprintFunction& f)
            {
                return f.Name == funcName;
            });
            if (it != funcs.end())
            {
                
                ImVec2 nodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
                createFunctionCallNode(*it, nodePosition);
            }
        }
    }
    ImGui::EndDragDropTarget();

    for (auto& node : m_nodes)
    {
        BlueprintNode* sourceData = findSourceDataById(node.sourceDataID);
        if (!sourceData) continue;


        if (sourceData->TargetMemberName == "MakeArray")
        {
            std::string elementType = "System.Object";
            if (sourceData->InputDefaults.count("元素类型") && !sourceData->InputDefaults.at("元素类型").empty())
            {
                elementType = sourceData->InputDefaults.at("元素类型");
            }


            for (auto& pin : node.outputPins)
            {
                if (pin.name == "数组")
                {
                    pin.type = elementType + "[]";
                }
            }


            for (auto& pin : node.inputPins)
            {
                if (pin.name.rfind("_dyn_element_", 0) == 0)
                {
                    pin.type = elementType;
                }
            }
        }


        ed::BeginNode(node.id);
        ImGui::TextUnformatted(node.name.c_str());
        ImGui::Spacing();

        ImGui::BeginGroup();


        bool requestAddParameter = false;
        ed::PinId pinIdToDelete = 0;

        for (auto& pin : node.inputPins)
        {
            if (sourceData->Type == BlueprintNodeType::VariableSet && pin.name == "值")
            {
                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::Text("-> %s", pin.name.c_str());
                ed::EndPin();

                if (!pin.isConnected)
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    std::string& value = sourceData->InputDefaults[pin.name];
                    char buffer[256];
                    strncpy(buffer, value.c_str(), sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0';
                    if (ImGui::InputText(("##" + pin.name + std::to_string(pin.id.Get())).c_str(), buffer,
                                         sizeof(buffer)))
                    {
                        value = buffer;
                    }
                }
            }
            else if (pin.type == "SelectType" || pin.type == "TemplateType")
            {
                ImGui::TextUnformatted(pin.name.c_str());
                std::string& selectedType = sourceData->InputDefaults[pin.name];
                if (selectedType.empty()) { selectedType = "void"; }

                std::string buttonText = selectedType;
                if (ImGui::Button((buttonText + "##" + std::to_string(pin.id.Get())).c_str(), ImVec2(150, 0)))
                {
                    SelectTypeWindow* window = findSelectTypeWindow(node.sourceDataID, pin.name);
                    if (!window)
                    {
                        SelectTypeWindow newWindow;
                        newWindow.nodeId = node.sourceDataID;
                        newWindow.pinName = pin.name;
                        newWindow.isOpen = true;
                        newWindow.needsFocus = true;
                        newWindow.windowId = "选择类型##" + std::to_string(node.sourceDataID) + "_" + pin.name;
                        m_selectTypeWindows.push_back(newWindow);
                    }
                    else
                    {
                        window->isOpen = true;
                        window->needsFocus = true;
                    }
                }
            }
            else if (sourceData->TargetMemberName == "Return" && pin.name == "输入值")
            {
                std::string returnType = sourceData->InputDefaults["返回类型"];
                if (returnType == "void") continue;
                pin.type = returnType;

                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::Text("-> %s (%s)", pin.name.c_str(), pin.type.c_str());
                ed::EndPin();
            }
            else if (pin.type == "NodeInputText")
            {
                ImGui::TextUnformatted(pin.name.c_str());
                ImGui::SetNextItemWidth(150);
                std::string& value = sourceData->InputDefaults[pin.name];

                char buffer[256];
                strncpy(buffer, value.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText(("##" + pin.name + std::to_string(pin.id.Get())).c_str(), buffer, sizeof(buffer)))
                {
                    value = buffer;
                }
            }
            else if (sourceData->TargetClassFullName == "Luma.SDK.Debug" && pin.name == "message")
            {
                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::Text("-> %s", pin.name.c_str());
                ed::EndPin();

                if (!pin.isConnected)
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);

                    std::string& value = sourceData->InputDefaults[pin.name];
                    char buffer[256];
                    strncpy(buffer, value.c_str(), sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0';

                    std::string inputId = "##" + pin.name + std::to_string(pin.id.Get());
                    if (ImGui::InputText(inputId.c_str(), buffer, sizeof(buffer)))
                    {
                        value = buffer;
                    }
                }
            }
            else if (sourceData->TargetMemberName == "If" && pin.name == "条件")
            {
                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::Text("-> %s", pin.name.c_str());
                ed::EndPin();

                if (!pin.isConnected)
                {
                    ImGui::SameLine();
                    std::string& value = sourceData->InputDefaults[pin.name];
                    std::string buttonText = value.empty()
                                                 ? "编辑条件"
                                                 : (value.length() > 15 ? value.substr(0, 12) + "..." : value);

                    if (ImGui::Button((buttonText + "##" + std::to_string(pin.id.Get())).c_str(), ImVec2(100, 0)))
                    {
                        InputStringWindow* window = findInputStringWindow(node.sourceDataID, pin.name);
                        if (!window)
                        {
                            InputStringWindow newWindow;
                            newWindow.nodeId = node.sourceDataID;
                            newWindow.pinName = pin.name;
                            newWindow.isOpen = true;
                            newWindow.needsFocus = true;
                            newWindow.size = ImVec2(300, 200);
                            newWindow.windowId = "InputString##" + std::to_string(node.sourceDataID) + "_" + pin.name;
                            m_inputStringWindows.push_back(newWindow);
                        }
                        else
                        {
                            window->isOpen = true;
                            window->needsFocus = true;
                        }
                    }
                }
            }
            else if (pin.type == "FunctionSelection")
            {
                ImGui::TextUnformatted(pin.name.c_str());
                ImGui::SameLine();


                std::string& selectedFunction = sourceData->InputDefaults[pin.name];
                const char* buttonText = selectedFunction.empty() ? "(选择回调函数)" : selectedFunction.c_str();


                if (ImGui::Button((std::string(buttonText) + "##" + std::to_string(pin.id.Get())).c_str(),
                                  ImVec2(180, 0)))
                {
                    SelectFunctionWindow* window = findSelectFunctionWindow(node.sourceDataID, pin.name);
                    if (!window)
                    {
                        SelectFunctionWindow newWindow;
                        newWindow.nodeId = node.sourceDataID;
                        newWindow.pinName = pin.name;
                        newWindow.isOpen = true;
                        newWindow.needsFocus = true;

                        newWindow.windowId = "选择函数##" + std::to_string(node.sourceDataID) + "_" + pin.name;
                        m_selectFunctionWindows.push_back(newWindow);
                    }
                    else
                    {
                        window->isOpen = true;
                        window->needsFocus = true;
                    }
                }
            }
            else if (pin.name == "参数列表" && pin.type == "Args")
            {
                ImGui::PushID(pin.id.Get());

                if (ImGui::Button("添加元素"))
                {
                    requestAddParameter = true;
                }

                ImGui::PopID();
            }

            else if (pin.name.rfind("_dyn_element_", 0) == 0)
            {
                ed::BeginPin(pin.id, ed::PinKind::Input);

                std::string displayName = pin.name.substr(13);
                ImGui::Text("-> %s (%s)", displayName.c_str(), pin.type.c_str());
                ed::EndPin();

                ImGui::SameLine();
                ImGui::PushID(pin.id.Get());
                if (ImGui::Button("X"))
                {
                    pinIdToDelete = pin.id;
                }
                ImGui::PopID();
            }
            else
            {
                ed::BeginPin(pin.id, ed::PinKind::Input);
                ImGui::Text("-> %s", pin.name.c_str());
                ed::EndPin();
            }
        }

        if (requestAddParameter)
        {
            size_t insertPos = 0;
            size_t dynamicCount = 0;
            for (size_t i = 0; i < node.inputPins.size(); ++i)
            {
                if (node.inputPins[i].type == "Args")
                {
                    insertPos = i;
                }
                else if (node.inputPins[i].name.rfind("_dyn_element_", 0) == 0)
                {
                    dynamicCount++;
                }
            }


            std::string elementType = "System.Object";
            if (sourceData->InputDefaults.count("元素类型") && !sourceData->InputDefaults.at("元素类型").empty())
            {
                elementType = sourceData->InputDefaults.at("元素类型");
            }

            BPin newPin;
            newPin.id = getNextPinId();
            newPin.nodeId = node.id;

            newPin.name = "_dyn_element_" + std::to_string(dynamicCount);
            newPin.type = elementType;
            newPin.kind = ed::PinKind::Input;

            node.inputPins.insert(node.inputPins.begin() + insertPos, newPin);
        }

        if (pinIdToDelete.Get() != 0)
        {
            std::vector<ed::LinkId> linksToDelete;
            for (const auto& link : m_links)
            {
                if (link.endPinId == pinIdToDelete)
                {
                    linksToDelete.push_back(link.id);
                }
            }
            for (auto linkId : linksToDelete)
            {
                deleteLink(linkId);
            }
            std::erase_if(node.inputPins, [pinIdToDelete](const BPin& pin)
            {
                return pin.id == pinIdToDelete;
            });
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        for (auto& pin : node.outputPins)
        {
            if (sourceData->TargetMemberName == "Declare" && pin.name == "输出变量")
            {
                std::string varType = sourceData->InputDefaults["变量类型"];
                if (varType.empty()) varType = "System.Object";
                pin.type = varType;
            }
            else if (sourceData->TargetClassFullName == "Utility" && sourceData->TargetMemberName == "Input" && pin.name
                == "输出")
            {
                std::string selectedType = sourceData->InputDefaults["类型"];
                if (selectedType.empty()) selectedType = "System.Object";
                pin.type = selectedType;
            }
            else if ((sourceData->TargetMemberName == "GetComponent" || sourceData->TargetMemberName == "AddComponent")
                && pin.name == "返回值")
            {
                std::string componentType = sourceData->InputDefaults["组件类型"];
                if (componentType.empty() || componentType == "选择类型")
                {
                    pin.type = "System.Object";
                }
                else
                {
                    pin.type = componentType;
                }
            }
            ed::BeginPin(pin.id, ed::PinKind::Output);
            if (pin.type == "Exec")
            {
                ImGui::Text("%s ->", pin.name.c_str());
            }
            else
            {
                ImGui::Text("%s (%s) ->", pin.name.c_str(), pin.type.c_str());
            }
            ed::EndPin();
        }
        ImGui::EndGroup();

        ed::EndNode();
        node.position = ed::GetNodePosition(node.id);
    }


    for (const auto& link : m_links)
    {
        ed::Link(link.id, link.startPinId, link.endPinId);
    }

    if (ed::BeginCreate())
    {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId))
        {
            BPin* startPin = findPinById(startPinId);
            BPin* endPin = findPinById(endPinId);
            if (startPin && endPin && startPin->nodeId != endPin->nodeId)
            {
                if (canCreateLink(startPin, endPin))
                {
                    if (ed::AcceptNewItem())
                    {
                        BLink newLink{getNextLinkId(), startPin->id, endPin->id};
                        if (startPin->kind == ed::PinKind::Input)
                        {
                            std::swap(newLink.startPinId, newLink.endPinId);
                        }
                        m_links.push_back(newLink);
                    }
                }
                else
                {
                    ed::RejectNewItem(ImVec4(1, 0, 0, 1), 2.0f);
                }
            }
        }
    }
    ed::EndCreate();

    if (ed::BeginDelete())
    {
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId))
        {
            if (ed::AcceptDeletedItem()) { deleteLink(deletedLinkId); }
        }
        ed::NodeId deletedNodeId;
        while (ed::QueryDeletedNode(&deletedNodeId))
        {
            if (ed::AcceptDeletedItem()) { deleteNode(deletedNodeId); }
        }
    }
    ed::EndDelete();

    ed::Suspend();
    ed::NodeId contextNodeId;
    ed::LinkId contextLinkId;
    if (ed::ShowNodeContextMenu(&contextNodeId))
    {
        m_contextNodeId = contextNodeId;
        ImGui::OpenPopup("NodeContextMenu");
    }
    else if (ed::ShowLinkContextMenu(&contextLinkId))
    {
        m_contextLinkId = contextLinkId;
        ImGui::OpenPopup("LinkContextMenu");
    }
    else if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("CreateNodeMenu");
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ed::IsActive() && !ed::GetHoveredNode() && !
        ed::GetHoveredLink())
    {
        bool onRegion = false;
        for (auto it = m_regions.rbegin(); it != m_regions.rend(); ++it)
        {
            const auto& region = *it;
            ImVec2 canvas_br = ImVec2(region.position.x + region.size.x, region.position.y + region.size.y);
            ImRect regionRect(ed::CanvasToScreen(region.position), ed::CanvasToScreen(canvas_br));
            if (regionRect.Contains(ImGui::GetMousePos()))
            {
                m_contextRegionId = region.id;
                onRegion = true;
                break;
            }
        }
        if (onRegion)
        {
            ImGui::OpenPopup("RegionContextMenu");
        }
    }


    drawNodeContextMenu();
    drawLinkContextMenu();
    drawRegionContextMenu();
    drawBackgroundContextMenu();
    ed::Resume();

    ed::End();
}

void BlueprintPanel::drawNodeListPanel()
{
    static char searchBuffer[128] = "";
    ImGui::InputText("搜索", searchBuffer, sizeof(searchBuffer));
    ImGui::Separator();

    if (ImGui::BeginChild("NodeListScroll"))
    {
        const auto& categorizedNodes = BlueprintNodeRegistry::GetInstance().GetCategorizedDefinitions();
        for (const auto& [category, definitions] : categorizedNodes)
        {
            if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto* def : definitions)
                {
                    if (searchBuffer[0] != '\0' && std::string_view(def->DisplayName).find(searchBuffer) ==
                        std::string_view::npos)
                    {
                        continue;
                    }

                    bool eventExists = (def->NodeType == BlueprintNodeType::Event && doesEventNodeExist(def->FullName));

                    if (eventExists)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                        ImGui::Selectable(def->DisplayName.c_str(), false, ImGuiSelectableFlags_Disabled);
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::Selectable(def->DisplayName.c_str());
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload("BLUEPRINT_NODE_DEFINITION", def->FullName.c_str(),
                                                      def->FullName.length() + 1);
                            ImGui::Text("创建 %s", def->DisplayName.c_str());
                            ImGui::EndDragDropSource();
                        }
                    }
                }
            }
        }
    }
    ImGui::EndChild();
}

void BlueprintPanel::drawVariablesPanel()
{
    ImGui::TextUnformatted("蓝图变量");
    ImGui::SameLine(
        ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("添加变量").x -
        ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::Button("添加变量"))
    {
        BlueprintVariable newVar;
        newVar.Name = "NewVar" + std::to_string(m_currentBlueprint->GetBlueprintData().Variables.size());
        newVar.Type = "System.Single";
        m_currentBlueprint->GetBlueprintData().Variables.push_back(newVar);
    }
    ImGui::Separator();

    if (ImGui::BeginChild("VariableList"))
    {
        auto& variables = m_currentBlueprint->GetBlueprintData().Variables;
        for (size_t i = 0; i < variables.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            auto& var = variables[i];

            char nameBuffer[256];
            strncpy(nameBuffer, var.Name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputText("##VarName", nameBuffer, sizeof(nameBuffer)))
            {
                var.Name = nameBuffer;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            if (ImGui::BeginCombo("##VarType", var.Type.c_str()))
            {
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere(0);
                ImGui::InputText("搜索", m_variableTypeSearchBuffer, sizeof(m_variableTypeSearchBuffer));
                ImGui::Separator();

                for (const auto& typeName : m_sortedTypeNames)
                {
                    std::string_view search_sv(m_variableTypeSearchBuffer);
                    if (search_sv.empty() || std::string_view(typeName).find(search_sv) != std::string_view::npos)
                    {
                        if (ImGui::Selectable(typeName.c_str(), var.Type == typeName))
                        {
                            var.Type = typeName;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                variables.erase(variables.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void BlueprintPanel::drawFunctionsPanel()
{
    if (!m_currentBlueprint) return;

    if (ImGui::Button("创建函数"))
    {
        m_isEditingFunction = false;
        m_functionEditorBuffer = {};
        m_functionEditorBuffer.Name = "NewFunction" + std::to_string(
            m_currentBlueprint->GetBlueprintData().Functions.size());
        m_functionEditorBuffer.ReturnType = "void";
        m_functionEditorBuffer.Visibility = "public";
        m_functionEditorBuffer.IsStatic = false;

        strncpy(m_functionNameBuffer, m_functionEditorBuffer.Name.c_str(), sizeof(m_functionNameBuffer));
        m_functionNameBuffer[sizeof(m_functionNameBuffer) - 1] = '\0';
        m_functionTypeSearchBuffer[0] = '\0';

        m_showCreateFunctionPopup = true;
    }
    ImGui::Separator();

    if (ImGui::BeginChild("FunctionsList"))
    {
        auto& functions = m_currentBlueprint->GetBlueprintData().Functions;
        for (size_t i = 0; i < functions.size();)
        {
            ImGui::PushID(static_cast<int>(i));
            auto& func = functions[i];

            std::string signature = func.Name + "()";

            const float buttons_width = 100.0f;

            float selectableWidth = ImGui::GetContentRegionAvail().x - buttons_width;
            if (selectableWidth < 1.0f) selectableWidth = 1.0f;

            ImGui::Selectable(signature.c_str(), false, 0, ImVec2(selectableWidth, 0));

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("BLUEPRINT_FUNCTION_CALL", func.Name.c_str(), func.Name.length() + 1);
                ImGui::Text("调用函数 %s", func.Name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::SameLine();
            if (ImGui::Button(("编辑##" + std::to_string(func.ID)).c_str()))
            {
                m_contextFunctionName = func.Name;
                m_isEditingFunction = true;
                m_functionEditorBuffer = func;
                strncpy(m_functionNameBuffer, m_functionEditorBuffer.Name.c_str(), sizeof(m_functionNameBuffer));
                m_functionNameBuffer[sizeof(m_functionNameBuffer) - 1] = '\0';
                m_functionTypeSearchBuffer[0] = '\0';
                m_showCreateFunctionPopup = true;
            }

            ImGui::SameLine();
            if (ImGui::Button(("X##" + std::to_string(func.ID)).c_str()))
            {
                deleteFunction(func.Name);
                ImGui::PopID();
                continue;
            }
            ImGui::PopID();
            i++;
        }
    }
    ImGui::EndChild();
}

void BlueprintPanel::deleteFunction(const std::string& functionName)
{
    LogInfo("删除函数: {}", functionName);
    auto& functions = m_currentBlueprint->GetBlueprintData().Functions;
    uint32_t funcIdToDelete = 0;
    auto funcIt = std::find_if(functions.begin(), functions.end(),
                               [&](const BlueprintFunction& f) { return f.Name == functionName; });
    if (funcIt != functions.end())
    {
        funcIdToDelete = funcIt->ID;
    }

    std::vector<ed::NodeId> nodesToDelete;
    for (const auto& n : m_nodes)
    {
        auto* sourceData = findSourceDataById(n.sourceDataID);
        if (sourceData &&
            (sourceData->Type == BlueprintNodeType::FunctionEntry || sourceData->Type ==
                BlueprintNodeType::FunctionCall) &&
            sourceData->TargetMemberName == functionName)
        {
            nodesToDelete.push_back(n.id);
        }
    }

    for (auto nodeId : nodesToDelete)
    {
        deleteNode(nodeId);
    }

    if (funcIdToDelete != 0)
    {
        std::erase_if(m_regions, [&](const BRegion& region) { return region.functionId == funcIdToDelete; });
        std::erase_if(m_currentBlueprint->GetBlueprintData().CommentRegions,
                      [&](const BlueprintCommentRegion& region) { return region.FunctionID == funcIdToDelete; });
    }

    std::erase_if(functions, [&](const BlueprintFunction& f) { return f.Name == functionName; });
}

void BlueprintPanel::rebuildFunctionNodePins(const std::string& oldName, const BlueprintFunction& updatedFunc)
{
    if (oldName != updatedFunc.Name)
    {
        for (auto& region : m_regions)
        {
            if (region.functionId == updatedFunc.ID)
            {
                region.title = updatedFunc.Name;
                break;
            }
        }
        for (auto& region_data : m_currentBlueprint->GetBlueprintData().CommentRegions)
        {
            if (region_data.FunctionID == updatedFunc.ID)
            {
                region_data.Title = updatedFunc.Name;
                break;
            }
        }
    }

    for (auto& node : m_nodes)
    {
        auto* sourceData = findSourceDataById(node.sourceDataID);
        if (!sourceData || (sourceData->TargetMemberName != oldName && sourceData->TargetMemberName != updatedFunc.
            Name))
            continue;

        if (sourceData->Type != BlueprintNodeType::FunctionEntry && sourceData->Type != BlueprintNodeType::FunctionCall)
            continue;

        sourceData->TargetMemberName = updatedFunc.Name;
        node.name = updatedFunc.Name;

        std::vector<ed::LinkId> linksToDelete;
        for (const auto& pin : node.inputPins)
        {
            for (const auto& link : m_links) if (link.endPinId == pin.id) linksToDelete.push_back(link.id);
        }
        for (const auto& pin : node.outputPins)
        {
            for (const auto& link : m_links) if (link.startPinId == pin.id) linksToDelete.push_back(link.id);
        }
        for (auto linkId : linksToDelete) deleteLink(linkId);

        node.inputPins.clear();
        node.outputPins.clear();

        if (sourceData->Type == BlueprintNodeType::FunctionEntry)
        {
            node.outputPins.push_back({getNextPinId(), node.id, "然后", "Exec", ed::PinKind::Output});
            for (const auto& param : updatedFunc.Parameters)
            {
                node.outputPins.push_back({getNextPinId(), node.id, param.Name, param.Type, ed::PinKind::Output});
            }
        }
        else
        {
            node.inputPins.push_back({getNextPinId(), node.id, "", "Exec", ed::PinKind::Input});
            for (const auto& param : updatedFunc.Parameters)
            {
                node.inputPins.push_back({getNextPinId(), node.id, param.Name, param.Type, ed::PinKind::Input});
            }
            node.outputPins.push_back({getNextPinId(), node.id, "然后", "Exec", ed::PinKind::Output});
            if (updatedFunc.ReturnType != "void")
            {
                node.outputPins.push_back({
                    getNextPinId(), node.id, "返回值", updatedFunc.ReturnType, ed::PinKind::Output
                });
            }
        }
    }
    rebuildPinConnections();
}

void BlueprintPanel::drawCreateFunctionPopup()
{
    if (!m_showCreateFunctionPopup) return;

    const char* popupTitle = m_isEditingFunction ? "修改函数签名" : "创建新函数";
    ImGui::OpenPopup(popupTitle);
    if (ImGui::BeginPopupModal(popupTitle, &m_showCreateFunctionPopup, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::InputText("函数名", m_functionNameBuffer, sizeof(m_functionNameBuffer)))
        {
            m_functionEditorBuffer.Name = m_functionNameBuffer;
        }

        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("返回类型", m_functionEditorBuffer.ReturnType.c_str()))
        {
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere(0);
            ImGui::InputText("搜索", m_functionTypeSearchBuffer, sizeof(m_functionTypeSearchBuffer));
            ImGui::Separator();

            std::string_view search_sv(m_functionTypeSearchBuffer);

            if (std::string_view("void").find(search_sv) != std::string_view::npos)
            {
                if (ImGui::Selectable("void", m_functionEditorBuffer.ReturnType == "void"))
                {
                    m_functionEditorBuffer.ReturnType = "void";
                }
            }

            for (const auto& typeName : m_sortedTypeNames)
            {
                if (typeName == "void")
                {
                    continue;
                }

                if (search_sv.empty() || std::string_view(typeName).find(search_sv) != std::string_view::npos)
                {
                    if (ImGui::Selectable(typeName.c_str(), m_functionEditorBuffer.ReturnType == typeName))
                    {
                        m_functionEditorBuffer.ReturnType = typeName;
                    }
                }
            }
            ImGui::EndCombo();
        }

        const char* visibilities[] = {"公开", "私有", "受保护"};
        const char* visibilities_en[] = {"public", "private", "protected"};
        int currentVisibility = 0;
        if (m_functionEditorBuffer.Visibility == "private") currentVisibility = 1;
        if (m_functionEditorBuffer.Visibility == "protected") currentVisibility = 2;

        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("可见性", &currentVisibility, visibilities, IM_ARRAYSIZE(visibilities)))
        {
            m_functionEditorBuffer.Visibility = visibilities_en[currentVisibility];
        }

        ImGui::SeparatorText("参数列表");

        for (size_t i = 0; i < m_functionEditorBuffer.Parameters.size();)
        {
            ImGui::PushID(static_cast<int>(i));
            auto& param = m_functionEditorBuffer.Parameters[i];

            char paramNameBuffer[128];
            strncpy(paramNameBuffer, param.Name.c_str(), sizeof(paramNameBuffer) - 1);
            paramNameBuffer[sizeof(paramNameBuffer) - 1] = '\0';
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputText("##ParamName", paramNameBuffer, sizeof(paramNameBuffer)))
            {
                param.Name = paramNameBuffer;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            if (ImGui::BeginCombo("##ParamType", param.Type.c_str()))
            {
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere(0);
                ImGui::InputText("搜索", m_functionTypeSearchBuffer, sizeof(m_functionTypeSearchBuffer));
                ImGui::Separator();

                for (const auto& typeName : m_sortedTypeNames)
                {
                    std::string_view search_sv_param(m_functionTypeSearchBuffer);
                    if (search_sv_param.empty() || std::string_view(typeName).find(search_sv_param) !=
                        std::string_view::npos)
                    {
                        if (ImGui::Selectable(typeName.c_str(), param.Type == typeName))
                        {
                            param.Type = typeName;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("移除"))
            {
                m_functionEditorBuffer.Parameters.erase(m_functionEditorBuffer.Parameters.begin() + i);
                ImGui::PopID();
                continue;
            }

            ImGui::PopID();
            i++;
        }

        if (ImGui::Button("添加参数"))
        {
            m_functionEditorBuffer.Parameters.push_back({
                "newParam" + std::to_string(m_functionEditorBuffer.Parameters.size()), "System.Int32"
            });
        }

        ImGui::Separator();

        const char* buttonText = m_isEditingFunction ? "应用修改" : "创建";
        if (ImGui::Button(buttonText, ImVec2(120, 0)))
        {
            if (m_isEditingFunction)
            {
                auto& funcs = m_currentBlueprint->GetBlueprintData().Functions;
                auto it = std::find_if(funcs.begin(), funcs.end(),
                                       [&](const BlueprintFunction& f) { return f.Name == m_contextFunctionName; });
                if (it != funcs.end())
                {
                    std::string oldName = it->Name;
                    *it = m_functionEditorBuffer;
                    rebuildFunctionNodePins(oldName, *it);
                }
            }
            else
            {
                m_functionEditorBuffer.ID = getNextFunctionId();
                m_functionEditorBuffer.IsStatic = false;
                m_currentBlueprint->GetBlueprintData().Functions.push_back(m_functionEditorBuffer);

                BlueprintCommentRegion regionData;
                regionData.ID = getNextRegionId();
                regionData.Title = m_functionEditorBuffer.Name;
                regionData.FunctionID = m_functionEditorBuffer.ID;
                regionData.Position = {100.0f, 100.0f};
                regionData.Size = {600.0f, 400.0f};
                m_currentBlueprint->GetBlueprintData().CommentRegions.push_back(regionData);

                BRegion region;
                region.id = regionData.ID;
                region.title = regionData.Title;
                region.position = ImVec2(regionData.Position.x, regionData.Position.y);
                region.size = ImVec2(regionData.Size.w, regionData.Size.h);
                region.functionId = regionData.FunctionID;

                ImGuiID hash = ImHashStr(region.title.c_str(), 0, 0);
                region.color = ImPlot::GetColormapColor(((hash & 0xFF)) % ImPlot::GetColormapSize(ImPlotColormap_Deep),
                                                        ImPlotColormap_Deep);
                region.color.w = 0.4f;
                m_regions.push_back(region);

                BlueprintNode bpNode;
                bpNode.ID = getNextNodeId();
                bpNode.Type = BlueprintNodeType::FunctionEntry;
                bpNode.TargetMemberName = m_functionEditorBuffer.Name;
                ImVec2 entryNodePos = {regionData.Position.x + 20, regionData.Position.y + 40};
                bpNode.Position = {entryNodePos.x, entryNodePos.y};
                m_currentBlueprint->GetBlueprintData().Nodes.push_back(bpNode);

                BNode editorNode;
                editorNode.id = ed::NodeId(bpNode.ID);
                editorNode.sourceDataID = bpNode.ID;
                editorNode.name = bpNode.TargetMemberName;
                editorNode.position = {bpNode.Position.x, bpNode.Position.y};

                editorNode.outputPins.push_back({getNextPinId(), editorNode.id, "然后", "Exec", ed::PinKind::Output});
                for (const auto& param : m_functionEditorBuffer.Parameters)
                {
                    editorNode.outputPins.push_back({
                        getNextPinId(), editorNode.id, param.Name, param.Type, ed::PinKind::Output
                    });
                }
                m_nodes.push_back(editorNode);

                if (m_functionEditorBuffer.ReturnType != "void")
                {
                    const auto* returnDef = BlueprintNodeRegistry::GetInstance().GetDefinition("FlowControl.Return");
                    if (returnDef)
                    {
                        BlueprintNode bpReturnNode;
                        bpReturnNode.ID = getNextNodeId();
                        bpReturnNode.Type = returnDef->NodeType;
                        bpReturnNode.Position = {entryNodePos.x + 400, entryNodePos.y};

                        std::string_view fullName(returnDef->FullName);
                        size_t lastDot = fullName.find_last_of('.');
                        if (lastDot != std::string_view::npos)
                        {
                            bpReturnNode.TargetClassFullName = std::string(fullName.substr(0, lastDot));
                            bpReturnNode.TargetMemberName = std::string(fullName.substr(lastDot + 1));
                        }

                        bpReturnNode.InputDefaults["返回类型"] = m_functionEditorBuffer.ReturnType;
                        m_currentBlueprint->GetBlueprintData().Nodes.push_back(bpReturnNode);

                        BNode editorReturnNode;
                        editorReturnNode.id = ed::NodeId(bpReturnNode.ID);
                        editorReturnNode.sourceDataID = bpReturnNode.ID;
                        editorReturnNode.name = returnDef->DisplayName;
                        editorReturnNode.position = {bpReturnNode.Position.x, bpReturnNode.Position.y};

                        for (const auto& pinDef : returnDef->InputPins)
                        {
                            BPin pin = {
                                getNextPinId(), editorReturnNode.id, pinDef.Name, pinDef.Type, ed::PinKind::Input
                            };
                            if (pin.name == "输入值")
                            {
                                pin.type = m_functionEditorBuffer.ReturnType;
                            }
                            editorReturnNode.inputPins.push_back(pin);
                        }

                        m_nodes.push_back(editorReturnNode);
                    }
                }
            }

            m_showCreateFunctionPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0)))
        {
            m_showCreateFunctionPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void BlueprintPanel::drawCreateRegionPopup()
{
    if (!m_showCreateRegionPopup) return;

    ImGui::OpenPopup("创建逻辑区域");
    if (ImGui::BeginPopupModal("创建逻辑区域", &m_showCreateRegionPopup, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("标题", m_newRegionTitleBuffer, sizeof(m_newRegionTitleBuffer));
        ImGui::ColorEdit3("颜色", m_newRegionColorBuffer);
        ImGui::DragFloat2("大小", m_newRegionSizeBuffer, 1.0f, 50.0f, 2000.0f);

        ImGui::Separator();
        if (ImGui::Button("创建", ImVec2(120, 0)))
        {
            BlueprintCommentRegion regionData;
            regionData.ID = getNextRegionId();
            regionData.Title = m_newRegionTitleBuffer;
            regionData.FunctionID = 0;
            ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
            regionData.Position.x = canvasPos.x;
            regionData.Position.y = canvasPos.y;
            regionData.Size = {m_newRegionSizeBuffer[0], m_newRegionSizeBuffer[1]};
            m_currentBlueprint->GetBlueprintData().CommentRegions.push_back(regionData);

            BRegion region;
            region.id = regionData.ID;
            region.title = regionData.Title;
            region.position = ImVec2(regionData.Position.x, regionData.Position.y);
            region.size = ImVec2(regionData.Size.w, regionData.Size.h);
            region.functionId = regionData.FunctionID;
            region.color = ImVec4(m_newRegionColorBuffer[0], m_newRegionColorBuffer[1], m_newRegionColorBuffer[2],
                                  0.4f);
            m_regions.push_back(region);

            m_showCreateRegionPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0)))
        {
            m_showCreateRegionPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}


BlueprintPanel::SelectFunctionWindow* BlueprintPanel::findSelectFunctionWindow(
    uint32_t nodeId, const std::string& pinName)
{
    for (auto& window : m_selectFunctionWindows)
    {
        if (window.nodeId == nodeId && window.pinName == pinName)
        {
            return &window;
        }
    }
    return nullptr;
}

void BlueprintPanel::handleShortcutInput()
{
    if (!m_isFocused) return;
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::S.IsPressed())
    {
        saveToBlueprintData();
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::W.IsPressed())
    {
        CloseCurrentBlueprint();
    }
    if (Keyboard::Delete.IsPressed())
    {
        if (m_contextNodeId.Get() != 0)
        {
            deleteNode(m_contextNodeId);
            m_contextNodeId = ed::NodeId(0);
        }
        if (m_contextLinkId.Get() != 0)
        {
            deleteLink(m_contextLinkId);
            m_contextLinkId = ed::LinkId(0);
        }
    }
}

void BlueprintPanel::drawSelectFunctionWindows()
{
    std::erase_if(m_selectFunctionWindows, [](const auto& window) { return !window.isOpen; });

    for (auto& window : m_selectFunctionWindows)
    {
        if (!window.isOpen) continue;

        ImGui::SetNextWindowSize(ImVec2(250, 350), ImGuiCond_FirstUseEver);
        if (window.needsFocus)
        {
            ImGui::SetNextWindowFocus();
            window.needsFocus = false;
        }

        if (ImGui::Begin(window.windowId.c_str(), &window.isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
        {
            BlueprintNode* sourceData = findSourceDataById(window.nodeId);
            if (sourceData)
            {
                if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere(0);
                ImGui::InputText("搜索", window.searchBuffer, sizeof(window.searchBuffer));
                ImGui::Separator();

                if (ImGui::BeginChild("##FunctionScrollingRegion"))
                {
                    std::string& selectedFunction = sourceData->InputDefaults[window.pinName];
                    std::string_view search_sv(window.searchBuffer);

                    if (ImGui::Selectable("(无)", selectedFunction.empty()))
                    {
                        selectedFunction.clear();
                        window.isOpen = false;
                    }

                    const auto& functions = m_currentBlueprint->GetBlueprintData().Functions;
                    for (const auto& func : functions)
                    {
                        if (search_sv.empty() || std::string_view(func.Name).find(search_sv) != std::string_view::npos)
                        {
                            if (ImGui::Selectable(func.Name.c_str(), selectedFunction == func.Name))
                            {
                                selectedFunction = func.Name;
                                window.isOpen = false;
                            }
                        }
                    }
                    ImGui::EndChild();
                }
            }
            else
            {
                ImGui::Text("错误: 找不到源节点数据。");
                if (ImGui::Button("关闭"))
                {
                    window.isOpen = false;
                }
            }
        }
        ImGui::End();
    }
}

void BlueprintPanel::drawRegions()
{
    ed::Suspend();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float headerHeight = 30.0f;

    for (const auto& region : m_regions)
    {
        ImVec2 canvas_br = ImVec2(region.position.x + region.size.x, region.position.y + region.size.y);
        ImVec2 screen_tl = ed::CanvasToScreen(region.position);
        ImVec2 screen_br = ed::CanvasToScreen(canvas_br);
        ImVec2 screenSize = ImVec2(screen_br.x - screen_tl.x, screen_br.y - screen_tl.y);

        ImU32 headerColor = ImGui::GetColorU32(ImVec4(region.color.x, region.color.y, region.color.z,
                                                      region.color.w + 0.3f));
        ImU32 bodyColor = ImGui::GetColorU32(region.color);

        drawList->AddRectFilled(screen_tl, screen_br, bodyColor, 8.0f);
        drawList->AddRectFilled(screen_tl, ImVec2(screen_tl.x + screenSize.x, screen_tl.y + headerHeight), headerColor,
                                8.0f, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);

        ImVec2 textSize = ImGui::CalcTextSize(region.title.c_str());
        drawList->AddText(ImVec2(screen_tl.x + (screenSize.x - textSize.x) * 0.5f,
                                 screen_tl.y + (headerHeight - textSize.y) * 0.5f),
                          IM_COL32_WHITE, region.title.c_str());

        ImVec2 resizeHandlePos = ImVec2(screen_br.x - 15, screen_br.y - 15);
        drawList->AddTriangleFilled(resizeHandlePos, ImVec2(resizeHandlePos.x + 15, resizeHandlePos.y),
                                    ImVec2(resizeHandlePos.x + 15, resizeHandlePos.y + 15),
                                    IM_COL32(255, 255, 255, 128));
    }
    ed::Resume();
}

void BlueprintPanel::handleRegionInteraction()
{
    ed::Suspend();

    const float headerHeight = 30.0f;
    const float resizeHandleSize = 15.0f;
    const ImVec2 mousePos = ImGui::GetMousePos();
    const ImVec2 canvasMousePos = ed::ScreenToCanvas(mousePos);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_regionInteraction.type == ERegionInteractionType::None)
    {
        for (auto it = m_regions.rbegin(); it != m_regions.rend(); ++it)
        {
            BRegion& region = *it;
            ImVec2 canvas_br = ImVec2(region.position.x + region.size.x, region.position.y + region.size.y);
            ImVec2 screen_tl = ed::CanvasToScreen(region.position);
            ImVec2 screen_br = ed::CanvasToScreen(canvas_br);

            ImRect headerRect(screen_tl, ImVec2(screen_br.x, screen_tl.y + headerHeight));
            ImRect resizeRect(ImVec2(screen_br.x - resizeHandleSize, screen_br.y - resizeHandleSize), screen_br);

            if (resizeRect.Contains(mousePos))
            {
                m_regionInteraction.type = ERegionInteractionType::Resizing;
                m_regionInteraction.activeRegion = &region;
                m_regionInteraction.startMousePos = canvasMousePos;
                break;
            }
            if (headerRect.Contains(mousePos))
            {
                m_regionInteraction.type = ERegionInteractionType::Dragging;
                m_regionInteraction.activeRegion = &region;
                m_regionInteraction.startMousePos = canvasMousePos;

                m_regionInteraction.nodesToDrag.clear();
                ImRect regionCanvasRect(region.position, canvas_br);
                for (auto& node : m_nodes)
                {
                    if (regionCanvasRect.Contains(node.position))
                    {
                        m_regionInteraction.nodesToDrag.push_back(&node);
                    }
                }
                break;
            }
        }
    }
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_regionInteraction.activeRegion)
    {
        ImVec2 delta = ImVec2(canvasMousePos.x - m_regionInteraction.startMousePos.x,
                              canvasMousePos.y - m_regionInteraction.startMousePos.y);

        if (m_regionInteraction.type == ERegionInteractionType::Dragging)
        {
            m_regionInteraction.activeRegion->position.x += delta.x;
            m_regionInteraction.activeRegion->position.y += delta.y;
            for (auto* node : m_regionInteraction.nodesToDrag)
            {
                node->position.x += delta.x;
                node->position.y += delta.y;
                ed::SetNodePosition(node->id, node->position);
            }
        }
        else if (m_regionInteraction.type == ERegionInteractionType::Resizing)
        {
            m_regionInteraction.activeRegion->size.x += delta.x;
            m_regionInteraction.activeRegion->size.y += delta.y;
            m_regionInteraction.activeRegion->size.x = std::max(100.0f, m_regionInteraction.activeRegion->size.x);
            m_regionInteraction.activeRegion->size.y = std::max(100.0f, m_regionInteraction.activeRegion->size.y);
        }

        m_regionInteraction.startMousePos = canvasMousePos;
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_regionInteraction.type = ERegionInteractionType::None;
        m_regionInteraction.activeRegion = nullptr;
        m_regionInteraction.nodesToDrag.clear();
    }

    ed::Resume();
}

void BlueprintPanel::drawBackgroundContextMenu()
{
    ImVec2 open_position = ImGui::GetMousePosOnOpeningCurrentPopup();
    if (ImGui::BeginPopup("CreateNodeMenu"))
    {
        
        ImVec2 open_position_canvas = ed::ScreenToCanvas(open_position);

        if (m_currentBlueprint && !m_currentBlueprint->GetBlueprintData().Functions.empty())
        {
            if (ImGui::BeginMenu("函数调用"))
            {
                for (const auto& func : m_currentBlueprint->GetBlueprintData().Functions)
                {
                    if (ImGui::MenuItem(func.Name.c_str()))
                    {
                        createFunctionCallNode(func, open_position_canvas);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
        }


        if (ImGui::BeginMenu("变量"))
        {
            if (m_currentBlueprint)
            {
                auto& variables = m_currentBlueprint->GetBlueprintData().Variables;
                if (variables.empty())
                {
                    ImGui::MenuItem("(无可用变量)", nullptr, false, false);
                }
                else
                {
                    if (ImGui::BeginMenu("获取"))
                    {
                        for (const auto& var : variables)
                        {
                            if (ImGui::MenuItem(var.Name.c_str()))
                            {
                                createVariableNode(var, BlueprintNodeType::VariableGet,
                                                   ed::ScreenToCanvas(open_position));
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("设置"))
                    {
                        for (const auto& var : variables)
                        {
                            if (ImGui::MenuItem(var.Name.c_str()))
                            {
                                createVariableNode(var, BlueprintNodeType::VariableSet,
                                                   ed::ScreenToCanvas(open_position));
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        const auto& categorizedNodes = BlueprintNodeRegistry::GetInstance().GetCategorizedDefinitions();

        static char searchBuffer[128] = "";
        ImGui::InputText("搜索", searchBuffer, sizeof(searchBuffer));
        ImGui::Separator();

        for (const auto& [category, definitions] : categorizedNodes)
        {
            if (ImGui::BeginMenu(category.c_str()))
            {
                for (const auto* def : definitions)
                {
                    if (searchBuffer[0] == '\0' ||
                        std::string_view(def->DisplayName).find(searchBuffer) != std::string_view::npos)
                    {
                        bool eventExists = (def->NodeType == BlueprintNodeType::Event && doesEventNodeExist(
                            def->FullName));
                        if (ImGui::MenuItem(def->DisplayName.c_str(), nullptr, false, !eventExists))
                        {
                            createNodeFromDefinition(def, ed::ScreenToCanvas(open_position));
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
}

void BlueprintPanel::drawNodeContextMenu()
{
    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        if (m_contextNodeId)
        {
            if (ImGui::MenuItem("删除节点"))
            {
                deleteNode(m_contextNodeId);
            }
        }
        ImGui::EndPopup();
    }
}

void BlueprintPanel::drawLinkContextMenu()
{
    if (ImGui::BeginPopup("LinkContextMenu"))
    {
        if (m_contextLinkId)
        {
            if (ImGui::MenuItem("删除连接"))
            {
                deleteLink(m_contextLinkId);
            }
        }
        ImGui::EndPopup();
    }
}

void BlueprintPanel::drawRegionContextMenu()
{
    if (ImGui::BeginPopup("RegionContextMenu"))
    {
        if (ImGui::MenuItem("删除区域"))
        {
            std::erase_if(m_regions, [&](const BRegion& region) { return region.id == m_contextRegionId; });
            std::erase_if(m_currentBlueprint->GetBlueprintData().CommentRegions,
                          [&](const BlueprintCommentRegion& region)
                          {
                              return region.ID == m_contextRegionId;
                          });
        }
        ImGui::EndPopup();
    }
}

void BlueprintPanel::updateInputStringWindows()
{
    for (auto& window : m_inputStringWindows)
    {
        if (!window.isOpen) continue;

        BNode* node = nullptr;
        for (auto& n : m_nodes)
        {
            if (n.sourceDataID == window.nodeId)
            {
                node = &n;
                break;
            }
        }

        if (node)
        {
            ed::SetCurrentEditor(m_nodeEditorContext);
            ImVec2 nodeScreenPos = ed::CanvasToScreen(node->position);
            window.position = ImVec2(nodeScreenPos.x + 200, nodeScreenPos.y);
        }
        else
        {
            window.isOpen = false;
        }
    }

    m_inputStringWindows.erase(
        std::remove_if(m_inputStringWindows.begin(), m_inputStringWindows.end(),
                       [](const InputStringWindow& w) { return !w.isOpen; }),
        m_inputStringWindows.end());
}

void BlueprintPanel::drawInputStringWindows()
{
    for (auto& window : m_inputStringWindows)
    {
        if (!window.isOpen) continue;

        ImGui::SetNextWindowPos(window.position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(window.size, ImGuiCond_FirstUseEver);

        if (window.needsFocus)
        {
            ImGui::SetNextWindowFocus();
            window.needsFocus = false;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin(window.windowId.c_str(), &window.isOpen, flags))
        {
            BlueprintNode* sourceData = findSourceDataById(window.nodeId);
            if (sourceData)
            {
                std::string& value = sourceData->InputDefaults[window.pinName];

                static char buffer[4096];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::Text("编辑 %s:", window.pinName.c_str());
                ImGui::Separator();

                if (ImGui::InputTextMultiline("##input", buffer, sizeof(buffer),
                                              ImVec2(280, 150),
                                              ImGuiInputTextFlags_AllowTabInput))
                {
                    value = buffer;
                }

                ImGui::Separator();

                if (ImGui::Button("完成"))
                {
                    window.isOpen = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("清空"))
                {
                    value.clear();
                    buffer[0] = '\0';
                }
            }
            else
            {
                ImGui::Text("错误: 找不到对应的节点数据");
                if (ImGui::Button("关闭"))
                {
                    window.isOpen = false;
                }
            }
        }
        ImGui::End();
    }
}

BlueprintPanel::InputStringWindow* BlueprintPanel::findInputStringWindow(uint32_t nodeId, const std::string& pinName)
{
    for (auto& window : m_inputStringWindows)
    {
        if (window.nodeId == nodeId && window.pinName == pinName)
        {
            return &window;
        }
    }
    return nullptr;
}

void BlueprintPanel::initializeFromBlueprintData()
{
    ClearEditorState();

    auto& blueprintData = m_currentBlueprint->GetBlueprintData();

    std::unordered_map<uint32_t, ed::NodeId> bpNodeIdToEditorNodeId;
    std::unordered_map<std::pair<uint32_t, std::string>, ed::PinId, PairHash> bpPinToEditorPinId;

    for (auto& bpNodeData : blueprintData.Nodes)
    {
        BNode node;
        node.id = ed::NodeId(bpNodeData.ID);
        m_nextNodeId = std::max(m_nextNodeId, bpNodeData.ID + 1);

        node.sourceDataID = bpNodeData.ID;
        node.position = {bpNodeData.Position.x, bpNodeData.Position.y};

        const auto* definition = BlueprintNodeRegistry::GetInstance().GetDefinition(
            bpNodeData.TargetClassFullName + "." + bpNodeData.TargetMemberName);

        if (!definition)
        {
            if (bpNodeData.Type == BlueprintNodeType::VariableGet)
            {
                node.name = "获取 " + bpNodeData.VariableName;
                std::string varType = "System.Object";

                const auto& allVars = blueprintData.Variables;
                auto it = std::find_if(allVars.begin(), allVars.end(),
                                       [&](const BlueprintVariable& var)
                                       {
                                           return var.Name == bpNodeData.VariableName;
                                       });

                if (it != allVars.end())
                {
                    varType = it->Type;
                }

                BPin pin = {getNextPinId(), node.id, "值", varType, ed::PinKind::Output};
                node.outputPins.push_back(pin);
                bpPinToEditorPinId[{bpNodeData.ID, pin.name}] = pin.id;
            }
            else if (bpNodeData.Type == BlueprintNodeType::VariableSet)
            {
                node.name = "设置 " + bpNodeData.VariableName;
                std::string varType = "System.Object";

                const auto& allVars = blueprintData.Variables;
                auto it = std::find_if(allVars.begin(), allVars.end(),
                                       [&](const BlueprintVariable& var)
                                       {
                                           return var.Name == bpNodeData.VariableName;
                                       });

                if (it != allVars.end())
                {
                    varType = it->Type;
                }

                BPin execInPin = {getNextPinId(), node.id, "", "Exec", ed::PinKind::Input};
                BPin valueInPin = {getNextPinId(), node.id, "值", varType, ed::PinKind::Input};
                BPin thenOutPin = {getNextPinId(), node.id, "然后", "Exec", ed::PinKind::Output};

                node.inputPins.push_back(execInPin);
                node.inputPins.push_back(valueInPin);
                node.outputPins.push_back(thenOutPin);

                bpPinToEditorPinId[{bpNodeData.ID, execInPin.name}] = execInPin.id;
                bpPinToEditorPinId[{bpNodeData.ID, valueInPin.name}] = valueInPin.id;
                bpPinToEditorPinId[{bpNodeData.ID, thenOutPin.name}] = thenOutPin.id;
            }
            else if (bpNodeData.Type == BlueprintNodeType::FunctionEntry || bpNodeData.Type ==
                BlueprintNodeType::FunctionCall)
            {
                const auto& funcs = blueprintData.Functions;
                auto it = std::find_if(funcs.begin(), funcs.end(), [&](const BlueprintFunction& func)
                {
                    return func.Name == bpNodeData.TargetMemberName;
                });
                if (it != funcs.end())
                {
                    const auto& func = *it;
                    node.name = func.Name;
                    if (bpNodeData.Type == BlueprintNodeType::FunctionEntry)
                    {
                        BPin thenPin = {getNextPinId(), node.id, "然后", "Exec", ed::PinKind::Output};
                        node.outputPins.push_back(thenPin);
                        bpPinToEditorPinId[{bpNodeData.ID, thenPin.name}] = thenPin.id;
                        for (const auto& param : func.Parameters)
                        {
                            BPin paramPin = {getNextPinId(), node.id, param.Name, param.Type, ed::PinKind::Output};
                            node.outputPins.push_back(paramPin);
                            bpPinToEditorPinId[{bpNodeData.ID, paramPin.name}] = paramPin.id;
                        }
                    }
                    else
                    {
                        BPin execInPin = {getNextPinId(), node.id, "", "Exec", ed::PinKind::Input};
                        node.inputPins.push_back(execInPin);
                        bpPinToEditorPinId[{bpNodeData.ID, execInPin.name}] = execInPin.id;

                        for (const auto& param : func.Parameters)
                        {
                            BPin paramPin = {getNextPinId(), node.id, param.Name, param.Type, ed::PinKind::Input};
                            node.inputPins.push_back(paramPin);
                            bpPinToEditorPinId[{bpNodeData.ID, paramPin.name}] = paramPin.id;
                        }

                        BPin thenPin = {getNextPinId(), node.id, "然后", "Exec", ed::PinKind::Output};
                        node.outputPins.push_back(thenPin);
                        bpPinToEditorPinId[{bpNodeData.ID, thenPin.name}] = thenPin.id;

                        if (func.ReturnType != "void")
                        {
                            BPin returnPin = {
                                getNextPinId(), node.id, "返回值", func.ReturnType, ed::PinKind::Output
                            };
                            node.outputPins.push_back(returnPin);
                            bpPinToEditorPinId[{bpNodeData.ID, returnPin.name}] = returnPin.id;
                        }
                    }
                }
                else
                {
                    node.name = "未知函数: " + bpNodeData.TargetMemberName;
                }
            }
            else continue;
        }
        else
        {
            node.name = definition->DisplayName;
            for (const auto& pinDef : definition->InputPins)
            {
                BPin pin = {getNextPinId(), node.id, pinDef.Name, pinDef.Type, ed::PinKind::Input};
                node.inputPins.push_back(pin);
                bpPinToEditorPinId[{bpNodeData.ID, pin.name}] = pin.id;
            }
            for (const auto& pinDef : definition->OutputPins)
            {
                BPin pin = {getNextPinId(), node.id, pinDef.Name, pinDef.Type, ed::PinKind::Output};
                node.outputPins.push_back(pin);
                bpPinToEditorPinId[{bpNodeData.ID, pin.name}] = pin.id;
            }

            if (definition->FullName == "Utility.GetSelf")
            {
                std::string selfType = "GameScripts." + m_currentBlueprint->GetBlueprintData().Name;
                for (auto& pin : node.outputPins)
                {
                    if (pin.name == "自身")
                    {
                        pin.type = selfType;
                    }
                }
            }
        }


        auto insert_it = std::find_if(node.inputPins.begin(), node.inputPins.end(), [](const BPin& pin)
        {
            return pin.type == "Args";
        });


        if (insert_it != node.inputPins.end())
        {
            if (bpNodeData.InputDefaults.count("_DynamicArgsCount"))
            {
                try
                {
                    int count = std::stoi(bpNodeData.InputDefaults.at("_DynamicArgsCount"));
                    std::vector<BPin> dynamic_pins_to_add;
                    for (int i = 0; i < count; ++i)
                    {
                        BPin dynamicPin;
                        dynamicPin.id = getNextPinId();
                        dynamicPin.nodeId = node.id;
                        dynamicPin.name = bpNodeData.InputDefaults.at("_DynamicArg_" + std::to_string(i) + "_Name");
                        dynamicPin.type = bpNodeData.InputDefaults.at("_DynamicArg_" + std::to_string(i) + "_Type");
                        dynamicPin.kind = ed::PinKind::Input;
                        dynamic_pins_to_add.push_back(dynamicPin);
                    }

                    if (!dynamic_pins_to_add.empty())
                    {
                        node.inputPins.insert(insert_it, dynamic_pins_to_add.begin(), dynamic_pins_to_add.end());
                    }
                }
                catch (const std::exception& e)
                {
                    LogWarn("为节点 {} 加载动态参数失败: {}", bpNodeData.ID, e.what());
                }
            }
        }


        m_nodes.push_back(node);
        bpNodeIdToEditorNodeId[bpNodeData.ID] = node.id;
    }

    for (const auto& bpLink : blueprintData.Links)
    {
        auto startIt = bpPinToEditorPinId.find({bpLink.FromNodeID, bpLink.FromPinName});
        auto endIt = bpPinToEditorPinId.find({bpLink.ToNodeID, bpLink.ToPinName});

        if (startIt != bpPinToEditorPinId.end() && endIt != bpPinToEditorPinId.end())
        {
            m_links.push_back({getNextLinkId(), startIt->second, endIt->second});
        }
    }

    rebuildPinConnections();

    ed::SetCurrentEditor(m_nodeEditorContext);
    for (const auto& node : m_nodes)
    {
        ed::SetNodePosition(node.id, node.position);
    }

    m_nextFunctionId = 1;
    for (const auto& func : blueprintData.Functions)
    {
        m_nextFunctionId = std::max(m_nextFunctionId, func.ID + 1);
    }

    m_nextRegionId = 1;
    for (const auto& regionData : blueprintData.CommentRegions)
    {
        BRegion region;
        region.id = regionData.ID;
        region.title = regionData.Title;
        region.position = {regionData.Position.x, regionData.Position.y};
        region.size = {regionData.Size.w, regionData.Size.h};
        region.functionId = regionData.FunctionID;

        ImGuiID hash = ImHashStr(region.title.c_str(), 0, 0);
        region.color = ImPlot::GetColormapColor(((hash & 0xFF)) % ImPlot::GetColormapSize(ImPlotColormap_Deep),
                                                ImPlotColormap_Deep);
        region.color.w = 0.4f;

        m_regions.push_back(region);
        m_nextRegionId = std::max(m_nextRegionId, region.id + 1);
    }
}

void BlueprintPanel::saveToBlueprintData()
{
    if (!m_currentBlueprint) return;
    auto& blueprintData = m_currentBlueprint->GetBlueprintData();

    for (const auto& node : m_nodes)
    {
        BlueprintNode* sourceData = findSourceDataById(node.sourceDataID);
        if (sourceData)
        {
            sourceData->Position.x = node.position.x;
            sourceData->Position.y = node.position.y;


            std::vector<std::string> keysToRemove;
            for (const auto& pair : sourceData->InputDefaults)
            {
                if (pair.first.rfind("_dyn_element_", 0) == 0 || pair.first == "_DynamicArgsCount")
                {
                    keysToRemove.push_back(pair.first);
                }
            }
            for (const auto& key : keysToRemove)
            {
                sourceData->InputDefaults.erase(key);
            }

            int dynamicArgCount = 0;
            for (const auto& pin : node.inputPins)
            {
                if (pin.name.rfind("_dyn_element_", 0) == 0)
                {
                    std::string baseKey = "_DynamicArg_" + std::to_string(dynamicArgCount);
                    sourceData->InputDefaults[baseKey + "_Name"] = pin.name;
                    sourceData->InputDefaults[baseKey + "_Type"] = pin.type;
                    dynamicArgCount++;
                }
            }

            if (dynamicArgCount > 0)
            {
                sourceData->InputDefaults["_DynamicArgsCount"] = std::to_string(dynamicArgCount);
            }
        }
    }

    blueprintData.Links.clear();
    std::unordered_map<ed::PinId, std::pair<uint32_t, std::string>> editorPinToBpPin;
    for (const auto& node : m_nodes)
    {
        BlueprintNode* sourceData = findSourceDataById(node.sourceDataID);
        if (!sourceData) continue;
        for (const auto& pin : node.inputPins) { editorPinToBpPin[pin.id] = {sourceData->ID, pin.name}; }
        for (const auto& pin : node.outputPins) { editorPinToBpPin[pin.id] = {sourceData->ID, pin.name}; }
    }


    for (const auto& link : m_links)
    {
        auto startIt = editorPinToBpPin.find(link.startPinId);
        auto endIt = editorPinToBpPin.find(link.endPinId);
        if (startIt != editorPinToBpPin.end() && endIt != editorPinToBpPin.end())
        {
            blueprintData.Links.push_back({
                startIt->second.first, startIt->second.second,
                endIt->second.first, endIt->second.second
            });
        }
    }

    blueprintData.CommentRegions.clear();
    for (const auto& region : m_regions)
    {
        BlueprintCommentRegion regionData;
        regionData.ID = region.id;
        regionData.Title = region.title;
        regionData.FunctionID = region.functionId;
        regionData.Position = {region.position.x, region.position.y};
        regionData.Size = {region.size.x, region.size.y};
        blueprintData.CommentRegions.push_back(regionData);
    }

    auto meta = AssetManager::GetInstance().GetMetadata(m_currentBlueprintGuid);
    auto filePath = AssetManager::GetInstance().GetAssetsRootPath() / meta->assetPath;
    std::string content = YAML::Dump(YAML::convert<Blueprint>::encode(blueprintData));
    Path::WriteFile(filePath.string(), content);

    LogInfo("蓝图数据已保存: {}", filePath.string());
}

void BlueprintPanel::createVariableNode(const BlueprintVariable& variable, BlueprintNodeType type, ImVec2 position)
{
    if (!m_currentBlueprint) return;

    BlueprintNode bpNode;
    bpNode.ID = getNextNodeId();
    bpNode.Type = type;
    bpNode.VariableName = variable.Name;
    bpNode.Position = {position.x, position.y};

    m_currentBlueprint->GetBlueprintData().Nodes.push_back(bpNode);

    BNode editorNode;
    editorNode.id = ed::NodeId(bpNode.ID);
    editorNode.sourceDataID = bpNode.ID;
    editorNode.position = position;

    if (type == BlueprintNodeType::VariableGet)
    {
        editorNode.name = "获取 " + variable.Name;
        editorNode.outputPins.push_back({getNextPinId(), editorNode.id, "值", variable.Type, ed::PinKind::Output});
    }
    else if (type == BlueprintNodeType::VariableSet)
    {
        editorNode.name = "设置 " + variable.Name;
        editorNode.inputPins.push_back({getNextPinId(), editorNode.id, "", "Exec", ed::PinKind::Input});
        editorNode.inputPins.push_back({getNextPinId(), editorNode.id, "值", variable.Type, ed::PinKind::Input});
        editorNode.outputPins.push_back({getNextPinId(), editorNode.id, "然后", "Exec", ed::PinKind::Output});
    }

    m_nodes.push_back(editorNode);
    ed::SetNodePosition(editorNode.id, position);
}


void BlueprintPanel::createNodeFromDefinition(const BlueprintNodeDefinition* definition, ImVec2 position)
{
    if (!m_currentBlueprint) return;
    if (definition->NodeType == BlueprintNodeType::Event)
    {
        if (doesEventNodeExist(definition->FullName))
        {
            LogWarn("无法创建事件节点 '{}'，因为它已存在于蓝图中。", definition->DisplayName);
            return;
        }
    }
    BlueprintNode bpNode;
    bpNode.ID = getNextNodeId();
    bpNode.Type = definition->NodeType;
    bpNode.Position = {position.x, position.y};
    std::string_view fullName(definition->FullName);
    size_t lastDot = fullName.find_last_of('.');
    if (lastDot != std::string_view::npos)
    {
        bpNode.TargetClassFullName = std::string(fullName.substr(0, lastDot));
        bpNode.TargetMemberName = std::string(fullName.substr(lastDot + 1));
    }

    m_currentBlueprint->GetBlueprintData().Nodes.push_back(bpNode);

    BNode editorNode;
    editorNode.id = ed::NodeId(bpNode.ID);
    editorNode.sourceDataID = bpNode.ID;
    editorNode.name = definition->DisplayName;
    editorNode.position = position;

    for (const auto& pinDef : definition->InputPins)
    {
        editorNode.inputPins.push_back({getNextPinId(), editorNode.id, pinDef.Name, pinDef.Type, ed::PinKind::Input});
    }
    for (const auto& pinDef : definition->OutputPins)
    {
        editorNode.outputPins.push_back({getNextPinId(), editorNode.id, pinDef.Name, pinDef.Type, ed::PinKind::Output});
    }
    if (definition->FullName == "Utility.GetSelf")
    {
        std::string selfType = "GameScripts." + m_currentBlueprint->GetBlueprintData().Name;
        for (auto& pin : editorNode.outputPins)
        {
            if (pin.name == "自身")
            {
                pin.type = selfType;
            }
        }
    }
    m_nodes.push_back(editorNode);
    ed::SetNodePosition(editorNode.id, position);
}

void BlueprintPanel::createFunctionCallNode(const BlueprintFunction& func, ImVec2 position)
{
    if (!m_currentBlueprint) return;

    BlueprintNode bpNode;
    bpNode.ID = getNextNodeId();
    bpNode.Type = BlueprintNodeType::FunctionCall;
    bpNode.TargetMemberName = func.Name;
    bpNode.Position = {position.x, position.y};
    m_currentBlueprint->GetBlueprintData().Nodes.push_back(bpNode);

    BNode editorNode;
    editorNode.id = ed::NodeId(bpNode.ID);
    editorNode.sourceDataID = bpNode.ID;
    editorNode.name = func.Name;
    editorNode.position = position;

    editorNode.inputPins.push_back({getNextPinId(), editorNode.id, "", "Exec", ed::PinKind::Input});
    for (const auto& param : func.Parameters)
    {
        editorNode.inputPins.push_back({getNextPinId(), editorNode.id, param.Name, param.Type, ed::PinKind::Input});
    }

    editorNode.outputPins.push_back({getNextPinId(), editorNode.id, "然后", "Exec", ed::PinKind::Output});
    if (func.ReturnType != "void")
    {
        editorNode.outputPins.push_back({
            getNextPinId(), editorNode.id, "返回值", func.ReturnType, ed::PinKind::Output
        });
    }

    m_nodes.push_back(editorNode);
    ed::SetNodePosition(editorNode.id, position);
}


void BlueprintPanel::deleteNode(ed::NodeId nodeId)
{
    BNode* nodeToDelete = findNodeById(nodeId);
    if (!nodeToDelete) return;

    std::vector<ed::LinkId> linksToDelete;
    for (const auto& link : m_links)
    {
        bool isConnected = false;
        for (const auto& pin : nodeToDelete->inputPins)
        {
            if (link.endPinId == pin.id)
            {
                isConnected = true;
                break;
            }
        }
        if (isConnected)
        {
            linksToDelete.push_back(link.id);
            continue;
        }
        for (const auto& pin : nodeToDelete->outputPins)
        {
            if (link.startPinId == pin.id)
            {
                linksToDelete.push_back(link.id);
                break;
            }
        }
    }

    for (ed::LinkId linkId : linksToDelete)
    {
        deleteLink(linkId);
    }

    uint32_t sourceIdToDelete = nodeToDelete->sourceDataID;
    auto& bpNodes = m_currentBlueprint->GetBlueprintData().Nodes;
    std::erase_if(bpNodes, [sourceIdToDelete](const BlueprintNode& bpNode)
    {
        return bpNode.ID == sourceIdToDelete;
    });

    std::erase_if(m_nodes, [nodeId](const BNode& node)
    {
        return node.id == nodeId;
    });
}

void BlueprintPanel::deleteLink(ed::LinkId linkId)
{
    BLink* linkToDelete = findLinkById(linkId);
    if (!linkToDelete) return;

    BPin* startPin = findPinById(linkToDelete->startPinId);
    BPin* endPin = findPinById(linkToDelete->endPinId);

    std::erase_if(m_links, [linkId](const BLink& link)
    {
        return link.id == linkId;
    });

    if (startPin)
    {
        bool stillConnected = false;
        for (const auto& link : m_links)
        {
            if (link.startPinId == startPin->id)
            {
                stillConnected = true;
                break;
            }
        }
        startPin->isConnected = stillConnected;
    }
    if (endPin)
    {
        bool stillConnected = false;
        for (const auto& link : m_links)
        {
            if (link.endPinId == endPin->id)
            {
                stillConnected = true;
                break;
            }
        }
        endPin->isConnected = stillConnected;
    }
}

bool BlueprintPanel::canCreateLink(const BPin* startPin, const BPin* endPin) const
{
    if (!startPin || !endPin || startPin == endPin) return false;
    if (startPin->nodeId == endPin->nodeId) return false;
    if (startPin->kind == endPin->kind) return false;

    const BPin* pOut = (startPin->kind == ed::PinKind::Output) ? startPin : endPin;
    const BPin* pIn = (startPin->kind == ed::PinKind::Output) ? endPin : startPin;

    if (pIn->isConnected) return false;

    if (pOut->type == "Exec")
    {
        return pIn->type == "Exec";
    }

    auto getCanonicalType = [](const std::string& typeName) -> std::string_view
    {
        if (typeName == "float") return "System.Single";
        if (typeName == "double") return "System.Double";
        if (typeName == "int") return "System.Int32";
        if (typeName == "long") return "System.Int64";
        if (typeName == "bool") return "System.Boolean";
        if (typeName == "string") return "System.String";
        if (typeName == "object") return "System.Object";
        if (typeName == "short") return "System.Int16";
        if (typeName == "byte") return "System.Byte";
        if (typeName == "char") return "System.Char";


        if (typeName == "DynamicObject") return "System.Object";


        return typeName;
    };

    std::string_view pInCanonicalType = getCanonicalType(pIn->type);
    std::string_view pOutCanonicalType = getCanonicalType(pOut->type);

    if (pInCanonicalType == "System.Object")
    {
        return pOut->type != "Exec";
    }

    return pOutCanonicalType == pInCanonicalType;
}

void BlueprintPanel::rebuildPinConnections()
{
    for (auto& node : m_nodes)
    {
        for (auto& pin : node.inputPins) pin.isConnected = false;
        for (auto& pin : node.outputPins) pin.isConnected = false;
    }

    for (const auto& link : m_links)
    {
        BPin* startPin = findPinById(link.startPinId);
        BPin* endPin = findPinById(link.endPinId);
        if (startPin) startPin->isConnected = true;
        if (endPin) endPin->isConnected = true;
    }
}

BlueprintPanel::BNode* BlueprintPanel::findNodeById(ed::NodeId nodeId)
{
    for (auto& node : m_nodes)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

BlueprintPanel::BPin* BlueprintPanel::findPinById(ed::PinId pinId)
{
    if (!pinId) return nullptr;

    for (auto& node : m_nodes)
    {
        for (auto& pin : node.inputPins)
        {
            if (pin.id == pinId)
                return &pin;
        }
        for (auto& pin : node.outputPins)
        {
            if (pin.id == pinId)
                return &pin;
        }
    }
    return nullptr;
}

BlueprintPanel::BLink* BlueprintPanel::findLinkById(ed::LinkId linkId)
{
    for (auto& link : m_links)
    {
        if (link.id == linkId)
            return &link;
    }
    return nullptr;
}

BlueprintNode* BlueprintPanel::findSourceDataById(uint32_t id)
{
    if (!m_currentBlueprint) return nullptr;

    auto& bpNodes = m_currentBlueprint->GetBlueprintData().Nodes;
    auto it = std::find_if(bpNodes.begin(), bpNodes.end(), [id](const BlueprintNode& node)
    {
        return node.ID == id;
    });

    if (it != bpNodes.end())
    {
        return &(*it);
    }

    return nullptr;
}

bool BlueprintPanel::doesEventNodeExist(const std::string& fullName)
{
    for (const auto& node : m_nodes)
    {
        const BlueprintNode* sourceData = findSourceDataById(node.sourceDataID);
        if (sourceData && sourceData->Type == BlueprintNodeType::Event)
        {
            std::string existingNodeFullName = sourceData->TargetClassFullName + "." + sourceData->TargetMemberName;
            if (existingNodeFullName == fullName)
            {
                return true;
            }
        }
    }
    return false;
}
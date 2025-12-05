#include "AnimationControllerEditorPanel.h"
#include "../Utils/Logger.h"
#include "../Resources/Loaders/AnimationControllerLoader.h"
#include "../Utils/PopupManager.h"
#include <imgui.h>
#include <algorithm>
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "AssetManager.h"
#include "Path.h"
#include "Profiler.h"
#include "Input/Keyboards.h"
#include "Loaders/AnimationClipLoader.h"
AnimationControllerEditorPanel::~AnimationControllerEditorPanel()
{
    if (m_nodeEditorContext)
    {
        ed::DestroyEditor(m_nodeEditorContext);
        m_nodeEditorContext = nullptr;
    }
}
void AnimationControllerEditorPanel::Initialize(EditorContext* context)
{
    m_context = context;
    ed::Config config;
    config.SettingsFile = nullptr;
    m_nodeEditorContext = ed::CreateEditor(&config);
}
void AnimationControllerEditorPanel::Update(float deltaTime)
{
    PROFILE_FUNCTION();
    if (m_context->currentEditingAnimationControllerGuid.Valid())
    {
        OpenAnimationController(m_context->currentEditingAnimationControllerGuid);
    }
}
void AnimationControllerEditorPanel::Draw()
{
    PROFILE_FUNCTION();
    if (!m_isVisible)
        return;
    if (m_requestFocus)
    {
        ImGui::SetNextWindowFocus();
        m_requestFocus = false;
    }
    if (ImGui::Begin(GetPanelName(), &m_isVisible, ImGuiWindowFlags_MenuBar))
    {
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("文件"))
            {
                if (ImGui::MenuItem("保存", "Ctrl+S", false, m_currentController != nullptr))
                {
                    saveToControllerData();
                }
                if (ImGui::MenuItem("关闭", "Ctrl+W", false, m_currentController != nullptr))
                {
                    CloseCurrentController();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("编辑"))
            {
                if (ImGui::MenuItem("添加状态", "N", false, m_currentController != nullptr))
                {
                    createStateNode("新状态", ImVec2(100, 100));
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("视图"))
            {
                ImGui::MenuItem("变量面板", nullptr, &m_variablesPanelOpen);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        if (!m_currentController)
        {
            ImVec2 center = ImGui::GetContentRegionAvail();
            center.x *= 0.5f;
            center.y *= 0.5f;
            ImGui::SetCursorPos(center);
            ImGui::Text("请双击动画控制器资源以开始编辑");
        }
        else
        {
            if (ImGui::BeginChild("MainContent", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
            {
                static float splitterWidth = 300.0f;
                ImVec2 contentSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("NodeEditor", ImVec2(contentSize.x - splitterWidth - 10, 0), true))
                {
                    drawNodeEditor();
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::Button("##splitter", ImVec2(10, -1));
                if (ImGui::IsItemActive())
                {
                    splitterWidth -= ImGui::GetIO().MouseDelta.x;
                    splitterWidth = std::clamp(splitterWidth, 200.0f, contentSize.x - 200.0f);
                }
                ImGui::SameLine();
                if (m_variablesPanelOpen)
                {
                    if (ImGui::BeginChild("VariablesPanel", ImVec2(splitterWidth, 0), true))
                    {
                        drawVariablesPanel();
                    }
                    ImGui::EndChild();
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    handleShortcutInput();
    if (m_transitionEditWindowOpen)
    {
        drawTransitionEditor();
    }
}
void AnimationControllerEditorPanel::Shutdown()
{
    CloseCurrentController();
    if (m_nodeEditorContext)
    {
        ed::DestroyEditor(m_nodeEditorContext);
        m_nodeEditorContext = nullptr;
    }
}
void AnimationControllerEditorPanel::OpenAnimationController(const Guid& controllerGuid)
{
    if (m_currentControllerGuid == controllerGuid && m_currentController)
        return;
    CloseCurrentController();
    auto loader = AnimationControllerLoader();
    m_currentController = loader.LoadAsset(controllerGuid);
    if (!m_currentController)
    {
        LogError("无法加载动画控制器，GUID: {}", controllerGuid.ToString());
        return;
    }
    m_currentControllerGuid = controllerGuid;
    m_currentControllerName = Path::GetFileNameWithoutExtension(
        AssetManager::GetInstance().GetMetadata(controllerGuid)->assetPath.string());
    m_controllerData = m_currentController->GetAnimationControllerData();
    initializeFromControllerData();
    SetVisible(true);
    m_context->currentEditingAnimationControllerGuid = controllerGuid;
    LogInfo("打开动画控制器进行编辑: {}", m_currentControllerName);
}
void AnimationControllerEditorPanel::CloseCurrentController()
{
    if (!m_currentController)
        return;
    LogInfo("关闭动画控制器: {}", m_currentControllerName);
    m_currentController = nullptr;
    m_currentControllerGuid = Guid();
    m_currentControllerName.clear();
    m_nodes.clear();
    m_links.clear();
    m_stateToNodeIndex.clear();
    m_transitionEditWindowOpen = false;
    m_nextNodeId = 1;
    m_nextLinkId = 1;
    m_nextPinId = 1;
    m_context->currentEditingAnimationControllerGuid = Guid();
}
void AnimationControllerEditorPanel::Focus()
{
    m_requestFocus = true;
}
void AnimationControllerEditorPanel::initializeFromControllerData()
{
    m_nodes.clear();
    m_links.clear();
    m_stateToNodeIndex.clear();
    ANode entryNode;
    entryNode.id = getNextNodeId();
    entryNode.stateGuid = SpecialStateGuids::Entry();
    entryNode.name = "Entry";
    entryNode.type = NodeType::Entry;
    entryNode.position = ImVec2(50, 200);
    entryNode.color = ImVec4(0.1f, 0.6f, 0.2f, 1.0f);
    entryNode.inputPinId = 0;
    entryNode.outputPinId = getNextPinId();
    m_stateToNodeIndex[entryNode.stateGuid] = static_cast<int>(m_nodes.size());
    m_nodes.push_back(entryNode);
    ANode anyStateNode;
    anyStateNode.id = getNextNodeId();
    anyStateNode.stateGuid = SpecialStateGuids::AnyState();
    anyStateNode.name = "Any State";
    anyStateNode.type = NodeType::AnyState;
    anyStateNode.position = ImVec2(50, 400);
    anyStateNode.color = ImVec4(0.7f, 0.2f, 0.7f, 1.0f);
    anyStateNode.inputPinId = 0;
    anyStateNode.outputPinId = getNextPinId();
    m_stateToNodeIndex[anyStateNode.stateGuid] = static_cast<int>(m_nodes.size());
    m_nodes.push_back(anyStateNode);
    float nodeSpacing = 250.0f;
    float startX = 350.0f;
    float startY = 100.0f;
    int nodeIndex = 0;
    for (const auto& [stateGuid, state] : m_controllerData.States)
    {
        if (stateGuid == SpecialStateGuids::Entry() || stateGuid == SpecialStateGuids::AnyState())
            continue;
        std::string clipName = "状态";
        for (const auto& [name, guid] : m_controllerData.Clips)
        {
            if (guid == stateGuid)
            {
                clipName = name;
                break;
            }
        }
        ANode node;
        node.id = getNextNodeId();
        node.stateGuid = stateGuid;
        node.name = clipName;
        node.type = NodeType::State;
        node.position = ImVec2(startX + (nodeIndex % 3) * nodeSpacing, startY + (nodeIndex / 3) * nodeSpacing);
        node.color = ImVec4(0.4f, 0.4f, 0.5f, 1.0f);
        node.inputPinId = getNextPinId();
        node.outputPinId = getNextPinId();
        m_stateToNodeIndex[stateGuid] = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
        nodeIndex++;
    }
    for (const auto& [fromStateGuid, state] : m_controllerData.States)
    {
        ANode* fromNode = findNodeByStateGuid(fromStateGuid);
        if (!fromNode) continue;
        for (const auto& transition : state.Transitions)
        {
            ANode* toNode = findNodeByStateGuid(transition.ToGuid);
            if (!toNode) continue;
            ALink link;
            link.id = getNextLinkId();
            link.startPinId = fromNode->outputPinId;
            link.endPinId = toNode->inputPinId;
            link.fromStateGuid = fromStateGuid;
            link.toStateGuid = transition.ToGuid;
            link.transitionName = transition.TransitionName;
            link.duration = transition.TransitionDuration;
            link.hasExitTime = transition.hasExitTime;
            link.conditions = transition.Conditions;
            link.priority = transition.priority;
            m_links.push_back(link);
        }
    }
    m_forceLayoutUpdate = true;
}
void AnimationControllerEditorPanel::saveToControllerData()
{
    if (!m_currentController) return;
    m_controllerData.States.clear();
    for (const ANode& node : m_nodes)
    {
        AnimationState state;
        for (const ALink& link : m_links)
        {
            if (link.fromStateGuid == node.stateGuid)
            {
                Transition transition;
                transition.ToGuid = link.toStateGuid;
                transition.TransitionName = link.transitionName;
                transition.TransitionDuration = link.duration;
                transition.Conditions = link.conditions;
                transition.priority = link.priority;
                transition.hasExitTime = link.hasExitTime;
                state.Transitions.push_back(transition);
            }
        }
        m_controllerData.States[node.stateGuid] = state;
    }
    auto meta = AssetManager::GetInstance().GetMetadata(m_currentControllerGuid);
    auto filePath = AssetManager::GetInstance().GetAssetsRootPath() / meta->assetPath;
    std::string content = YAML::Dump(YAML::convert<AnimationControllerData>::encode(m_controllerData));
    Path::WriteFile(filePath.string(), content);
    LogInfo("动画控制器数据已保存");
}
void AnimationControllerEditorPanel::drawTransitionEditor()
{
    if (!ImGui::Begin("过渡编辑器", &m_transitionEditWindowOpen))
    {
        ImGui::End();
        return;
    }
    if (m_editingLinkIndex < 0 || m_editingLinkIndex >= static_cast<int>(m_links.size()))
    {
        ImGui::Text("无效的过渡");
        ImGui::End();
        return;
    }
    ALink& link = m_links[m_editingLinkIndex];
    ImGui::Text("编辑过渡");
    ImGui::Separator();
    char nameBuffer[256];
    strncpy(nameBuffer, link.transitionName.c_str(), sizeof(nameBuffer));
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    if (ImGui::InputText("过渡名称", nameBuffer, sizeof(nameBuffer)))
    {
        link.transitionName = nameBuffer;
    }
    ImGui::DragFloat("持续时间", &link.duration, 0.01f, 0.0f, 10.0f, "%.2fs");
    ImGui::InputInt("优先级", &link.priority);
    ImGui::Checkbox("拥有退出时间", &link.hasExitTime);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("如果勾选，此过渡只会在当前动画播放完毕后才会进行条件检查。\n如果不勾选，则会立即中断当前动画进行过渡。");
    }
    ImGui::Separator();
    ImGui::Text("过渡条件");
    drawConditionEditor(link.conditions);
    if (ImGui::Button("保存"))
    {
        m_transitionEditWindowOpen = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("取消"))
    {
        m_transitionEditWindowOpen = false;
    }
    ImGui::End();
}
void AnimationControllerEditorPanel::drawNodeContextMenu()
{
    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        ANode* contextNode = findNodeById(m_contextNodeId);
        if (contextNode && contextNode->type == NodeType::State)
        {
            if (ImGui::MenuItem("删除状态"))
            {
                deleteNode(m_contextNodeId);
            }
        }
        else
        {
            ImGui::TextDisabled("特殊状态无法修改");
        }
        ImGui::EndPopup();
    }
}
void AnimationControllerEditorPanel::drawLinkContextMenu()
{
    if (ImGui::BeginPopup("LinkContextMenu"))
    {
        if (ImGui::MenuItem("编辑过渡"))
        {
            ALink* contextLink = findLinkById(m_contextLinkId);
            if (contextLink)
            {
                m_editingLinkIndex = static_cast<int>(contextLink - m_links.data());
                m_transitionEditWindowOpen = true;
            }
        }
        if (ImGui::MenuItem("删除过渡"))
        {
            deleteLink(m_contextLinkId);
        }
        ImGui::EndPopup();
    }
}
void AnimationControllerEditorPanel::drawConditionEditor(std::vector<Condition>& conditions)
{
    for (size_t i = 0; i < conditions.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        Condition& condition = conditions[i];
        std::visit([this, &condition, &conditions, i](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, FloatCondition>)
            {
                if (ImGui::BeginCombo("Float变量", arg.VarName.c_str()))
                {
                    for (const auto& var : m_controllerData.Variables)
                    {
                        if (var.Type == VariableType::VariableType_Float)
                        {
                            if (ImGui::Selectable(var.Name.c_str(), arg.VarName == var.Name))
                                arg.VarName = var.Name;
                        }
                    }
                    ImGui::EndCombo();
                }
                const char* floatOps[] = {"大于", "小于"};
                int currentOp = static_cast<int>(arg.op);
                if (ImGui::Combo("比较", &currentOp, floatOps, IM_ARRAYSIZE(floatOps)))
                    arg.op = static_cast<FloatCondition::Comparison>(currentOp);
                ImGui::DragFloat("值", &arg.Value);
            }
            else if constexpr (std::is_same_v<T, BoolCondition>)
            {
                if (ImGui::BeginCombo("Bool变量", arg.VarName.c_str()))
                {
                    for (const auto& var : m_controllerData.Variables)
                    {
                        if (var.Type == VariableType::VariableType_Bool)
                        {
                            if (ImGui::Selectable(var.Name.c_str(), arg.VarName == var.Name))
                                arg.VarName = var.Name;
                        }
                    }
                    ImGui::EndCombo();
                }
                const char* boolOps[] = {"为真", "为假"};
                int currentOp = static_cast<int>(arg.op);
                if (ImGui::Combo("比较", &currentOp, boolOps, IM_ARRAYSIZE(boolOps)))
                    arg.op = static_cast<BoolCondition::Comparison>(currentOp);
            }
            else if constexpr (std::is_same_v<T, IntCondition>)
            {
                if (ImGui::BeginCombo("Int变量", arg.VarName.c_str()))
                {
                    for (const auto& var : m_controllerData.Variables)
                    {
                        if (var.Type == VariableType::VariableType_Int)
                        {
                            if (ImGui::Selectable(var.Name.c_str(), arg.VarName == var.Name))
                                arg.VarName = var.Name;
                        }
                    }
                    ImGui::EndCombo();
                }
                const char* intOps[] = {"大于", "小于", "等于", "不等于"};
                int currentOp = static_cast<int>(arg.op);
                if (ImGui::Combo("比较", &currentOp, intOps, IM_ARRAYSIZE(intOps)))
                    arg.op = static_cast<IntCondition::Comparison>(currentOp);
                ImGui::DragInt("值", &arg.Value);
            }
            else if constexpr (std::is_same_v<T, TriggerCondition>)
            {
                if (ImGui::BeginCombo("Trigger变量", arg.VarName.c_str()))
                {
                    for (const auto& var : m_controllerData.Variables)
                    {
                        if (var.Type == VariableType::VariableType_Trigger)
                        {
                            if (ImGui::Selectable(var.Name.c_str(), arg.VarName == var.Name))
                                arg.VarName = var.Name;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }, condition);
        ImGui::SameLine();
        if (ImGui::Button("删除"))
        {
            conditions.erase(conditions.begin() + i);
            ImGui::PopID();
            return;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    if (ImGui::Button("添加Float条件"))
        conditions.emplace_back(FloatCondition{FloatCondition::GreaterThan, "", 0.0f});
    ImGui::SameLine();
    if (ImGui::Button("添加Bool条件"))
        conditions.emplace_back(BoolCondition{BoolCondition::IsTrue, ""});
    ImGui::SameLine();
    if (ImGui::Button("添加Int条件"))
        conditions.emplace_back(IntCondition{IntCondition::Equal, "", 0});
    ImGui::SameLine();
    if (ImGui::Button("添加Trigger条件"))
        conditions.emplace_back(TriggerCondition{""});
}
AnimationControllerEditorPanel::ANode* AnimationControllerEditorPanel::findNodeByStateGuid(const Guid& stateGuid)
{
    auto it = m_stateToNodeIndex.find(stateGuid);
    if (it != m_stateToNodeIndex.end() && it->second < static_cast<int>(m_nodes.size()))
    {
        return &m_nodes[it->second];
    }
    return nullptr;
}
AnimationControllerEditorPanel::ALink* AnimationControllerEditorPanel::findLinkById(ed::LinkId linkId)
{
    for (auto& link : m_links)
    {
        if (link.id == linkId)
            return &link;
    }
    return nullptr;
}
AnimationControllerEditorPanel::ANode* AnimationControllerEditorPanel::findNodeById(ed::NodeId nodeId)
{
    for (auto& node : m_nodes)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}
void AnimationControllerEditorPanel::createStateNode(const std::string& name, ImVec2 position, bool isEntry,
                                                     bool isDefault)
{
    ANode newNode;
    newNode.id = getNextNodeId();
    newNode.stateGuid = Guid::NewGuid();
    newNode.name = name;
    newNode.position = position;
    newNode.isEntry = isEntry;
    newNode.isDefault = isDefault;
    newNode.inputPinId = getNextPinId();
    newNode.outputPinId = getNextPinId();
    if (isEntry)
        newNode.color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    else if (isDefault)
        newNode.color = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    else
        newNode.color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    AnimationState newState;
    m_controllerData.States[newNode.stateGuid] = newState;
    m_stateToNodeIndex[newNode.stateGuid] = static_cast<int>(m_nodes.size());
    m_nodes.push_back(newNode);
    m_forceLayoutUpdate = true;
    LogInfo("创建新状态节点: {}", name);
}
void AnimationControllerEditorPanel::deleteNode(ed::NodeId nodeId)
{
    ANode* nodeToDelete = findNodeById(nodeId);
    if (!nodeToDelete)
        return;
    m_controllerData.Clips.erase(nodeToDelete->name);
    Guid stateGuidToDelete = nodeToDelete->stateGuid;
    std::erase_if(m_links,
                  [stateGuidToDelete](const ALink& link)
                  {
                      return link.fromStateGuid == stateGuidToDelete || link.toStateGuid ==
                          stateGuidToDelete;
                  });
    m_controllerData.States.erase(stateGuidToDelete);
    m_stateToNodeIndex.erase(stateGuidToDelete);
    std::erase_if(m_nodes,
                  [nodeId](const ANode& node)
                  {
                      return node.id == nodeId;
                  });
    m_stateToNodeIndex.clear();
    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        m_stateToNodeIndex[m_nodes[i].stateGuid] = static_cast<int>(i);
    }
    LogInfo("删除状态节点");
}
void AnimationControllerEditorPanel::deleteLink(ed::LinkId linkId)
{
    m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
                                 [linkId](const ALink& link)
                                 {
                                     return link.id == linkId;
                                 }), m_links.end());
    LogInfo("删除过渡连接");
}
void AnimationControllerEditorPanel::handleShortcutInput()
{
    if (!m_isFocused) return;
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::S.IsPressed())
    {
        saveToControllerData();
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::W.IsPressed())
    {
        CloseCurrentController();
    }
}
void AnimationControllerEditorPanel::drawVariablesPanel()
{
    ImGui::Text("变量");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 240);
    if (ImGui::Button("Float"))
    {
        AnimationVariable newVar;
        newVar.Name = "新Float变量";
        newVar.Type = VariableType::VariableType_Float;
        newVar.Value = 0.0f;
        m_controllerData.Variables.push_back(newVar);
    }
    ImGui::SameLine();
    if (ImGui::Button("Bool"))
    {
        AnimationVariable newVar;
        newVar.Name = "新Bool变量";
        newVar.Type = VariableType::VariableType_Bool;
        newVar.Value = false;
        m_controllerData.Variables.push_back(newVar);
    }
    ImGui::SameLine();
    if (ImGui::Button("Int"))
    {
        AnimationVariable newVar;
        newVar.Name = "新Int变量";
        newVar.Type = VariableType::VariableType_Int;
        newVar.Value = 0;
        m_controllerData.Variables.push_back(newVar);
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger"))
    {
        AnimationVariable newVar;
        newVar.Name = "新Trigger";
        newVar.Type = VariableType::VariableType_Trigger;
        newVar.Value = false;
        m_controllerData.Variables.push_back(newVar);
    }
    ImGui::Separator();
    if (ImGui::BeginChild("VariablesList"))
    {
        for (size_t i = 0; i < m_controllerData.Variables.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            AnimationVariable& var = m_controllerData.Variables[i];
            char nameBuffer[256];
            strncpy(nameBuffer, var.Name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputText("##VarName", nameBuffer, sizeof(nameBuffer)))
            {
                var.Name = nameBuffer;
            }
            ImGui::SameLine();
            switch (var.Type)
            {
            case VariableType::VariableType_Float:
                {
                    float value = std::get<float>(var.Value);
                    if (ImGui::DragFloat("值", &value)) var.Value = value;
                    break;
                }
            case VariableType::VariableType_Bool:
                {
                    bool value = std::get<bool>(var.Value);
                    if (ImGui::Checkbox("值", &value)) var.Value = value;
                    break;
                }
            case VariableType::VariableType_Int:
                {
                    int value = std::get<int>(var.Value);
                    if (ImGui::DragInt("值", &value)) var.Value = value;
                    break;
                }
            case VariableType::VariableType_Trigger:
                {
                    ImGui::TextDisabled("(Trigger)");
                    break;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("删除"))
            {
                m_controllerData.Variables.erase(m_controllerData.Variables.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}
void AnimationControllerEditorPanel::drawNodeEditor()
{
    ed::SetCurrentEditor(m_nodeEditorContext);
    ed::Begin("AnimationStateMachine");
    if (m_forceLayoutUpdate)
    {
        for (const auto& node : m_nodes)
        {
            ed::SetNodePosition(node.id, node.position);
        }
        m_forceLayoutUpdate = false;
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle assetHandle = *static_cast<const AssetHandle*>(payload->Data);
            if (assetHandle.assetType == AssetType::AnimationClip)
            {
                ImVec2 nodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
                handleAnimationClipDrop(assetHandle, nodePosition);
            }
        }
        ImGui::EndDragDropTarget();
    }
    for (auto& node : m_nodes)
    {
        ed::BeginNode(node.id);
        ImGui::PushID(node.id.Get());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_Button, node.color);
        if (ImGui::Button(node.name.c_str(), ImVec2(120, 0)))
        {
        }
        ImGui::PopStyleColor(2);
        if (node.type == NodeType::State)
        {
            ed::BeginPin(node.inputPinId, ed::PinKind::Input);
            ImGui::Text("-> 输入");
            ed::EndPin();
            ImGui::SameLine();
        }
        ed::BeginPin(node.outputPinId, ed::PinKind::Output);
        ImGui::Text("输出 ->");
        ed::EndPin();
        if (node.isEntry)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Entry]");
        }
        if (node.isDefault)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[Default]");
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
            {
                AssetHandle assetHandle = *static_cast<const AssetHandle*>(payload->Data);
                if (assetHandle.assetType == AssetType::AnimationClip)
                {
                    handleAnimationClipDropOnNode(assetHandle, node);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
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
            if (startPinId && endPinId)
            {
                ANode* startNode = nullptr;
                ANode* endNode = nullptr;
                for (auto& node : m_nodes)
                {
                    if (node.outputPinId == startPinId) startNode = &node;
                    if (node.inputPinId == endPinId) endNode = &node;
                }
                if (startNode && endNode && startNode != endNode)
                {
                    if (ed::AcceptNewItem())
                    {
                        ALink newLink;
                        newLink.id = getNextLinkId();
                        newLink.startPinId = startPinId;
                        newLink.endPinId = endPinId;
                        newLink.fromStateGuid = startNode->stateGuid;
                        newLink.toStateGuid = endNode->stateGuid;
                        newLink.transitionName = "新过渡";
                        newLink.duration = 0.3f;
                        m_links.push_back(newLink);
                        LogInfo("创建过渡: {} -> {}", startNode->name, endNode->name);
                    }
                }
                else
                {
                    ed::RejectNewItem();
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
    ed::LinkId doubleClickedLinkId = ed::GetDoubleClickedLink();
    ALink* clickedLink = findLinkById(doubleClickedLinkId);
    if (clickedLink)
    {
        m_editingLinkIndex = static_cast<int>(clickedLink - m_links.data());
        m_transitionEditWindowOpen = true;
    }
    ed::Suspend();
    ed::NodeId contextNodeId = 0;
    ed::LinkId contextLinkId = 0;
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
    drawNodeContextMenu();
    drawLinkContextMenu();
    if (ImGui::BeginPopup("CreateNodeMenu"))
    {
        if (ImGui::MenuItem("创建状态"))
        {
            ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
            createStateNode("新状态", ed::ScreenToCanvas(mousePos));
        }
        ImGui::EndPopup();
    }
    ed::Resume();
    ed::End();
}
void AnimationControllerEditorPanel::handleAnimationClipDrop(const AssetHandle& assetHandle, ImVec2 nodePosition)
{
    if (findNodeByStateGuid(assetHandle.assetGuid))
    {
        LogWarn("动画剪辑 {} 已经存在于状态图中", assetHandle.assetGuid.ToString());
        return;
    }
    auto loader = AnimationClipLoader();
    auto clip = loader.LoadAsset(assetHandle.assetGuid);
    std::string clipName = clip->GetName();
    ANode newNode;
    newNode.id = getNextNodeId();
    newNode.stateGuid = assetHandle.assetGuid;
    newNode.name = clipName;
    newNode.position = nodePosition;
    newNode.isEntry = false;
    newNode.isDefault = m_nodes.empty();
    newNode.inputPinId = getNextPinId();
    newNode.outputPinId = getNextPinId();
    newNode.color = newNode.isDefault ? ImVec4(0.0f, 0.0f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    AnimationState newState;
    m_controllerData.States[newNode.stateGuid] = newState;
    m_controllerData.Clips[clipName] = assetHandle.assetGuid;
    m_stateToNodeIndex[newNode.stateGuid] = static_cast<int>(m_nodes.size());
    m_nodes.push_back(newNode);
    m_forceLayoutUpdate = true;
    if (m_nodes.size() == 3)
    {
        ANode* entryNode = findNodeByStateGuid(SpecialStateGuids::Entry());
        if (entryNode)
        {
            ALink newLink;
            newLink.id = getNextLinkId();
            newLink.startPinId = entryNode->outputPinId;
            newLink.endPinId = newNode.inputPinId;
            newLink.fromStateGuid = entryNode->stateGuid;
            newLink.toStateGuid = newNode.stateGuid;
            newLink.transitionName = "入口过渡";
            newLink.duration = 0.0f;
            m_links.push_back(newLink);
        }
    }
    LogInfo("从拖拽创建状态节点: {} (GUID: {})", clipName, assetHandle.assetGuid.ToString());
}
void AnimationControllerEditorPanel::handleAnimationClipDropOnNode(const AssetHandle& assetHandle, ANode& targetNode)
{
    const Guid& newGuid = assetHandle.assetGuid;
    const Guid& oldGuid = targetNode.stateGuid;
    if (newGuid == oldGuid)
    {
        return;
    }
    ANode* existingNode = findNodeByStateGuid(newGuid);
    if (existingNode && existingNode->id != targetNode.id)
    {
        LogError("无法关联动画剪辑 {}，因为它已经关联到另一个状态节点 {}", newGuid.ToString(), existingNode->name);
        return;
    }
    auto loader = AnimationClipLoader();
    auto clip = loader.LoadAsset(newGuid);
    std::string newClipName = clip->GetName();
    targetNode.name = newClipName;
    targetNode.stateGuid = newGuid;
    if (m_controllerData.States.count(oldGuid))
    {
        auto stateData = m_controllerData.States.at(oldGuid);
        m_controllerData.States.erase(oldGuid);
        m_controllerData.States[newGuid] = stateData;
    }
    else
    {
        m_controllerData.States[newGuid] = AnimationState();
    }
    for (auto it = m_controllerData.Clips.begin(); it != m_controllerData.Clips.end();)
    {
        if (it->second == oldGuid)
        {
            it = m_controllerData.Clips.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_controllerData.Clips[newClipName] = newGuid;
    for (ALink& link : m_links)
    {
        if (link.fromStateGuid == oldGuid)
        {
            link.fromStateGuid = newGuid;
        }
        if (link.toStateGuid == oldGuid)
        {
            link.toStateGuid = newGuid;
        }
    }
    if (m_stateToNodeIndex.count(oldGuid))
    {
        int nodeIndex = m_stateToNodeIndex.at(oldGuid);
        m_stateToNodeIndex.erase(oldGuid);
        m_stateToNodeIndex[newGuid] = nodeIndex;
    }
    LogInfo("将节点 {} 的动画剪辑更新为 {}", targetNode.name, newClipName);
}

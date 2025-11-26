#include "../Utils/PCH.h"
#include "InspectorPanel.h"
#include "imgui_internal.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Components/IDComponent.h"
#include "../Components/ComponentRegistry.h"
#include "../Resources/AssetManager.h"
#include "../Utils/PopupManager.h"
#include "../Resources/Loaders/PrefabLoader.h"
#include "../Utils/Logger.h"
#include "InspectorUI.h"
#include "Profiler.h"
#include "RelationshipComponent.h"
#include "Transform.h"
#include "ScriptComponent.h"
#include "../TagManager.h"
#include "TagComponent.h"
#include "../TagManager.h"
#include "TagComponent.h"
#include "Event/EventBus.h"
#include "Event/Events.h"

void InspectorPanel::Initialize(EditorContext* context)
{
    m_context = context;
    m_isLocked = false;
    m_lockedGuids.clear();
    m_lockedSelectionType = SelectionType::NA;


    m_evtGoCreated = EventBus::GetInstance().Subscribe<GameObjectCreatedEvent>([this](const GameObjectCreatedEvent& e)
    {
        try
        {
            if (!m_context || !m_context->activeScene) return;
            if (m_context->editorState != EditorState::Editing)
            {
                m_dirty = true;
                return;
            }
            auto& id = e.registry.get<ECS::IDComponent>(e.entity);
            if (m_context->selectionType == SelectionType::NA || m_context->selectionList.empty())
            {
                m_context->selectionType = SelectionType::GameObject;
                m_context->selectionList.clear();
                m_context->selectionList.push_back(id.guid);
                m_context->objectToFocusInHierarchy = id.guid;
            }
            m_dirty = true;
        }
        catch (...)
        {
        }
    });

    m_evtGoDestroyed = EventBus::GetInstance().Subscribe<GameObjectDestroyedEvent>(
        [this](const GameObjectDestroyedEvent& e)
        {
            try
            {
                if (!m_context) return;
                auto& id = e.registry.get<ECS::IDComponent>(e.entity);
                auto& sel = m_context->selectionList;
                auto it = std::find(sel.begin(), sel.end(), id.guid);
                if (it != sel.end())
                {
                    sel.erase(it);
                    if (sel.empty())
                    {
                        m_context->selectionType = SelectionType::NA;
                    }
                }
                m_dirty = true;
            }
            catch (...)
            {
            }
        });

    m_evtCompAdded = EventBus::GetInstance().Subscribe<ComponentAddedEvent>([this](const ComponentAddedEvent& e)
    {
        try
        {
            if (!m_context) return;
            auto& id = e.registry.get<ECS::IDComponent>(e.entity);
            if (std::find(m_context->selectionList.begin(), m_context->selectionList.end(), id.guid) != m_context->
                selectionList.end())
            {
                m_dirty = true;
            }
        }
        catch (...)
        {
        }
    });

    m_evtCompRemoved = EventBus::GetInstance().Subscribe<ComponentRemovedEvent>([this](const ComponentRemovedEvent& e)
    {
        try
        {
            if (!m_context) return;
            auto& id = e.registry.get<ECS::IDComponent>(e.entity);
            if (std::find(m_context->selectionList.begin(), m_context->selectionList.end(), id.guid) != m_context->
                selectionList.end())
            {
                m_dirty = true;
            }
        }
        catch (...)
        {
        }
    });

    m_evtCompUpdated = EventBus::GetInstance().Subscribe<ComponentUpdatedEvent>([this](const ComponentUpdatedEvent& e)
    {
        try
        {
            if (!m_context) return;
            if (m_context->selectionList.size() > 1)
            {
                auto& id = e.registry.get<ECS::IDComponent>(e.entity);
                if (std::find(m_context->selectionList.begin(), m_context->selectionList.end(), id.guid) != m_context->
                    selectionList.end())
                {
                    m_dirty = true;
                }
            }
        }
        catch (...)
        {
        }
    });
}

void InspectorPanel::Update(float deltaTime)
{
}

void InspectorPanel::Draw()
{
    PROFILE_FUNCTION();
    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    drawLockButton();
    ImGui::Separator();

    SelectionType currentSelectionType;
    std::vector<Guid> currentSelectionGuids;

    if (isSelectionLocked())
    {
        currentSelectionType = m_lockedSelectionType;
        currentSelectionGuids = m_lockedGuids;
    }
    else
    {
        currentSelectionType = m_context->selectionType;
        currentSelectionGuids = m_context->selectionList;
    }


    size_t fp = computeSelectionFingerprint(currentSelectionType, currentSelectionGuids);
    if (fp != m_selectionFingerprint)
    {
        m_selectionFingerprint = fp;
        m_dirty = true;
    }
    if (m_dirty)
    {
        rebuildCache(currentSelectionGuids, currentSelectionType);
        m_dirty = false;
    }

    switch (currentSelectionType)
    {
    case SelectionType::NA:
        drawNoSelection();
        break;
    case SelectionType::GameObject:
        drawGameObjectInspectorWithGuids(currentSelectionGuids);
        break;
    case SelectionType::SceneCamera:
        drawSceneCameraInspector();
        break;
    }

    ImGui::End();
}

void InspectorPanel::Shutdown()
{
    unlockSelection();
    EventBus::GetInstance().Unsubscribe(m_evtGoCreated);
    EventBus::GetInstance().Unsubscribe(m_evtGoDestroyed);
    EventBus::GetInstance().Unsubscribe(m_evtCompAdded);
    EventBus::GetInstance().Unsubscribe(m_evtCompRemoved);
    EventBus::GetInstance().Unsubscribe(m_evtCompUpdated);
}

void InspectorPanel::drawLockButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button,
                          m_isLocked ? ImVec4(0.8f, 0.4f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          m_isLocked ? ImVec4(0.9f, 0.5f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          m_isLocked ? ImVec4(0.7f, 0.3f, 0.1f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

    const char* buttonText = m_isLocked ? "解锁检视器" : "固定检视器";

    float buttonWidth = ImGui::CalcTextSize(buttonText).x + 20.0f;
    float windowWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(windowWidth - buttonWidth);

    if (ImGui::Button(buttonText, ImVec2(buttonWidth, 0)))
    {
        if (m_isLocked)
        {
            unlockSelection();
        }
        else
        {
            lockSelection();
        }
    }

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
    {
        if (m_isLocked)
        {
            ImGui::SetTooltip("点击解锁检视器，恢复跟随选择");
        }
        else
        {
            ImGui::SetTooltip("点击固定检视器，防止拖拽时切换对象");
        }
    }

    if (m_isLocked)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(10.0f);
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.2f, 1.0f), "已固定");

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("检视器已固定到 %d 个对象", static_cast<int>(m_lockedGuids.size()));
        }
    }
}

void InspectorPanel::lockSelection()
{
    if (m_context->selectionType == SelectionType::NA)
    {
        return;
    }

    m_isLocked = true;
    m_lockedSelectionType = m_context->selectionType;
    m_lockedGuids = m_context->selectionList;

    LogInfo("检视器已固定到 {} 个对象", m_lockedGuids.size());
}

void InspectorPanel::unlockSelection()
{
    m_isLocked = false;
    m_lockedSelectionType = SelectionType::NA;
    m_lockedGuids.clear();
}

bool InspectorPanel::isSelectionLocked() const
{
    return m_isLocked;
}

std::vector<Guid> InspectorPanel::getCurrentSelectionGuids() const
{
    if (isSelectionLocked())
    {
        return m_lockedGuids;
    }
    else
    {
        return m_context->selectionList;
    }
}

void InspectorPanel::drawGameObjectInspectorWithGuids(const std::vector<Guid>& guids)
{
    if (!m_context->activeScene) return;
    if (guids.empty())
    {
        drawNoSelection();
        return;
    }

    std::vector<RuntimeGameObject> selectedObjects;
    for (const auto& guid : guids)
    {
        RuntimeGameObject obj = m_context->activeScene->FindGameObjectByGuid(guid);
        if (obj.IsValid())
        {
            selectedObjects.push_back(obj);
        }
    }

    if (selectedObjects.empty())
    {
        if (isSelectionLocked())
        {
            ImGui::Text("固定的对象已失效。");
            if (ImGui::Button("解锁检视器"))
            {
                unlockSelection();
            }
        }
        else
        {
            m_context->selectionType = SelectionType::NA;
            m_context->selectionList.clear();
            drawNoSelection();
        }
        return;
    }

    if (isSelectionLocked() && selectedObjects.size() != m_lockedGuids.size())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Text("警告: %d 个固定对象中有 %d 个已失效",
                    static_cast<int>(m_lockedGuids.size()),
                    static_cast<int>(m_lockedGuids.size() - selectedObjects.size()));
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    if (selectedObjects.size() == 1)
    {
        drawSingleObjectInspector(selectedObjects[0]);
    }
    else
    {
        drawMultiObjectInspector(selectedObjects);
    }
}

void InspectorPanel::drawNoSelection()
{
    ImGui::Text("未选择对象。");
}

void InspectorPanel::drawGameObjectInspector()
{
    drawGameObjectInspectorWithGuids(m_context->selectionList);
}

void InspectorPanel::drawSingleObjectInspector(RuntimeGameObject& gameObject)
{
    drawGameObjectName(gameObject);
    ImGui::Separator();

    drawComponents(gameObject);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawAddComponentButton();

    drawDragDropTarget();
}

void InspectorPanel::drawMultiObjectInspector(std::vector<RuntimeGameObject>& selectedObjects)
{
    ImGui::Text("已选择 %d 个对象", static_cast<int>(selectedObjects.size()));

    if (isSelectionLocked())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.2f, 1.0f), "[固定]");
    }

    ImGui::Separator();

    drawBatchGameObjectName(selectedObjects);
    ImGui::Separator();

    drawCommonComponents(selectedObjects);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawBatchAddComponentButton();
}

void InspectorPanel::drawSceneCameraInspector()
{
    if (m_context->activeScene)
    {
        if (ImGui::CollapsingHeader("场景相机", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto camProps = m_context->activeScene->GetCameraProperties();
            bool changed = false;

            if (ImGui::IsItemActivated()) { m_context->uiCallbacks->onValueChanged.Invoke(); }
            changed |= CustomDrawing::WidgetDrawer<SkPoint>::Draw("位置", camProps.position, *m_context->uiCallbacks);
            if (ImGui::IsItemActivated()) { m_context->uiCallbacks->onValueChanged.Invoke(); }
            changed |= CustomDrawing::WidgetDrawer<SkPoint>::Draw("缩放", camProps.zoom, *m_context->uiCallbacks);
            if (ImGui::IsItemActivated()) { m_context->uiCallbacks->onValueChanged.Invoke(); }
            changed |= CustomDrawing::WidgetDrawer<float>::Draw("旋转", camProps.rotation, *m_context->uiCallbacks);
            if (ImGui::IsItemActivated()) { m_context->uiCallbacks->onValueChanged.Invoke(); }
            changed |= CustomDrawing::WidgetDrawer<SkColor4f>::Draw("清除颜色", camProps.clearColor,
                                                                    *m_context->uiCallbacks);
            camProps.zoom.fX = std::max(0.f, camProps.zoom.x());
            camProps.zoom.fY = std::max(0.f, camProps.zoom.y());
            if (changed)
            {
                m_context->activeScene->SetCameraProperties(camProps);
            }
        }
    }
}

void InspectorPanel::drawGameObjectName(RuntimeGameObject& gameObject)
{
    bool isActive = gameObject.IsActive();
    if (ImGui::Checkbox("##IsActiveCheckbox", &isActive))
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
        gameObject.SetActive(isActive);
    }
    ImGui::SameLine();

    std::string currentName = gameObject.GetName();
    char nameBuffer[256] = {0};
#ifdef _WIN32
    strncpy_s(nameBuffer, sizeof(nameBuffer), currentName.c_str(), sizeof(nameBuffer) - 1);
#else
    strncpy(nameBuffer, currentName.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
#endif

    std::string label = isSelectionLocked() ? " [固定]" : "";

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText(("名称" + label).c_str(), nameBuffer, sizeof(nameBuffer)))
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
        gameObject.SetName(nameBuffer);
        m_context->objectToFocusInHierarchy = gameObject.GetGuid();
    }
    ImGui::PopItemWidth();


    if (m_context->activeScene)
    {
        auto& registry = m_context->activeScene->GetRegistry();
        auto entity = static_cast<entt::entity>(gameObject);
        if (!registry.any_of<ECS::TagComponent>(entity))
        {
            gameObject.AddComponent<ECS::TagComponent>();
        }
        auto& tagComp = registry.get<ECS::TagComponent>(entity);

        std::vector<std::string> tags = TagManager::GetAllTags();
        if (tags.empty())
        {
            TagManager::EnsureDefaults();
            tags = TagManager::GetAllTags();
        }

        int currentIndex = -1;
        for (int i = 0; i < (int)tags.size(); ++i)
        {
            if (tags[i] == tagComp.tag)
            {
                currentIndex = i;
                break;
            }
        }

        ImGui::Text("标签");
        ImGui::SameLine();
        const char* preview = (currentIndex >= 0 && currentIndex < (int)tags.size())
                                  ? tags[currentIndex].c_str()
                                  : "(未设置)";
        if (ImGui::BeginCombo("##TagDropdownTop", preview))
        {
            static char newTagBuf[64] = {0};
            ImGui::InputTextWithHint("##NewTagInCombo", "新建标签名", newTagBuf, sizeof(newTagBuf));
            ImGui::SameLine();
            if (ImGui::SmallButton("添加"))
            {
                std::string nt = newTagBuf;
                if (!nt.empty())
                {
                    TagManager::AddTag(nt);
                    memset(newTagBuf, 0, sizeof(newTagBuf));
                    tags = TagManager::GetAllTags();
                }
            }
            ImGui::SameLine();
            bool canDelete = (currentIndex >= 0 && currentIndex < (int)tags.size() && tags[currentIndex] !=
                std::string("Unknown"));
            if (!canDelete) ImGui::BeginDisabled();
            if (ImGui::SmallButton("删除当前"))
            {
                std::string toDel = (currentIndex >= 0 && currentIndex < (int)tags.size())
                                        ? tags[currentIndex]
                                        : std::string();
                if (!toDel.empty())
                {
                    TagManager::RemoveTag(toDel);
                    if (tagComp.tag == toDel)
                    {
                        m_context->uiCallbacks->onValueChanged.Invoke();
                        tagComp.tag = "Unknown";
                    }
                    tags = TagManager::GetAllTags();
                    currentIndex = -1;
                    for (int i = 0; i < (int)tags.size(); ++i)
                        if (tags[i] == tagComp.tag)
                        {
                            currentIndex = i;
                            break;
                        }
                }
            }
            if (!canDelete) ImGui::EndDisabled();

            ImGui::Separator();
            for (int n = 0; n < (int)tags.size(); ++n)
            {
                bool selected = (n == currentIndex);
                if (ImGui::Selectable(tags[n].c_str(), selected))
                {
                    if (tagComp.tag != tags[n])
                    {
                        m_context->uiCallbacks->onValueChanged.Invoke();
                        tagComp.tag = tags[n];
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
}

void InspectorPanel::drawBatchGameObjectName(std::vector<RuntimeGameObject>& selectedObjects)
{
    bool firstIsActive = selectedObjects[0].IsActive();
    bool isMixed = false;
    for (size_t i = 1; i < selectedObjects.size(); ++i)
    {
        if (selectedObjects[i].IsActive() != firstIsActive)
        {
            isMixed = true;
            break;
        }
    }

    if (isMixed)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
    }

    bool checkboxState = firstIsActive;

    if (isMixed)
    {
        checkboxState = false;
    }

    if (ImGui::Checkbox("##BatchIsActiveCheckbox", &checkboxState))
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
        for (const auto& obj : selectedObjects)
        {
            RuntimeGameObject mutableObj = obj;
            mutableObj.SetActive(checkboxState);
        }
    }

    if (isMixed)
    {
        ImGui::PopItemFlag();
    }
    ImGui::SameLine();

    static char batchNameBuffer[256] = {0};

    std::string label = isSelectionLocked() ? "批量重命名 [固定]:" : "批量重命名:";
    ImGui::Text("%s", label.c_str());

    if (ImGui::InputText("新名称", batchNameBuffer, sizeof(batchNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(batchNameBuffer) > 0)
        {
            applyBatchNameChange(selectedObjects, batchNameBuffer);
            memset(batchNameBuffer, 0, sizeof(batchNameBuffer));
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("应用"))
    {
        if (strlen(batchNameBuffer) > 0)
        {
            applyBatchNameChange(selectedObjects, batchNameBuffer);
            memset(batchNameBuffer, 0, sizeof(batchNameBuffer));
            m_context->objectToFocusInHierarchy = selectedObjects[0].GetGuid();
        }
    }
}

void InspectorPanel::applyBatchNameChange(const std::vector<RuntimeGameObject>& selectedObjects, const char* newName)
{
    m_context->uiCallbacks->onValueChanged.Invoke();

    for (size_t i = 0; i < selectedObjects.size(); ++i)
    {
        std::string finalName = std::string(newName);
        if (selectedObjects.size() > 1)
        {
            finalName += " (" + std::to_string(i + 1) + ")";
        }

        RuntimeGameObject obj = selectedObjects[i];
        obj.SetName(finalName);
    }
}

void InspectorPanel::drawComponents(RuntimeGameObject& gameObject)
{
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();
    auto entityHandle = static_cast<entt::entity>(gameObject);
    const auto& componentRegistry = ComponentRegistry::GetInstance();

    for (const auto& componentName : componentRegistry.GetAllRegisteredNames())
    {
        if (componentName == "TransformComponent")
        {
            auto& transform = registry.get<ECS::TransformComponent>(entityHandle);
            bool hasParent = gameObject.HasComponent<ECS::ParentComponent>();

            std::string headerName;
            if (hasParent)
            {
                headerName = isSelectionLocked() ? "Transform (Local) [固定]" : "Transform (Local)";
            }
            else
            {
                headerName = isSelectionLocked() ? "Transform (World) [固定]" : "Transform (World)";
            }

            bool isHeadOpen = ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (isHeadOpen)
            {
                if (hasParent)
                {
                    CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("Position", transform.localPosition,
                                                                     *m_context->uiCallbacks);
                    CustomDrawing::WidgetDrawer<float>::Draw("Rotation", transform.localRotation,
                                                             *m_context->uiCallbacks);
                    CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("Scale", transform.localScale,
                                                                     *m_context->uiCallbacks);
                    if (CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw(
                        "Anchor", transform.anchor, *m_context->uiCallbacks))
                    {
                        transform.anchor.x = std::clamp(transform.anchor.x, 0.0f, 1.0f);
                        transform.anchor.y = std::clamp(transform.anchor.y, 0.0f, 1.0f);
                    }
                }
                else
                {
                    CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("Position", transform.position,
                                                                     *m_context->uiCallbacks);
                    CustomDrawing::WidgetDrawer<float>::Draw("Rotation", transform.rotation, *m_context->uiCallbacks);
                    CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("Scale", transform.scale, *m_context->uiCallbacks);
                    if (CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw(
                        "Anchor", transform.anchor, *m_context->uiCallbacks))
                    {
                        transform.anchor.x = std::clamp(transform.anchor.x, 0.0f, 1.0f);
                        transform.anchor.y = std::clamp(transform.anchor.y, 0.0f, 1.0f);
                    }
                }
            }
            continue;
        }

        if (componentName == "ScriptComponent" || componentName == "ScriptsComponent")
        {
            continue;
        }
        if (componentName == "TagComponent")
        {
            continue;
        }

        const ComponentRegistration* compInfo = componentRegistry.Get(componentName);
        if (!compInfo || !compInfo->isExposedInEditor || !compInfo->has(registry, entityHandle))
        {
            continue;
        }

        drawComponentHeader(componentName, compInfo, entityHandle);
    }

    if (gameObject.HasComponent<ECS::ScriptsComponent>())
    {
        drawScriptsComponentUI(gameObject);
    }
}

void InspectorPanel::drawScriptsComponentUI(RuntimeGameObject& gameObject)
{
    auto entityHandle = static_cast<entt::entity>(gameObject);
    const auto& componentRegistry = ComponentRegistry::GetInstance();
    const ComponentRegistration* compInfo = componentRegistry.Get("ScriptsComponent");
    if (!compInfo) return;

    auto& scriptsComponent = gameObject.GetComponent<ECS::ScriptsComponent>();

    ImGui::PushID("ScriptsComponent");
    if (ImGui::Checkbox("##Enabled", &scriptsComponent.Enable))
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
    }
    ImGui::SameLine();

    std::string headerLabel = "Scripts";
    if (isSelectionLocked()) headerLabel += " [固定]";

    bool isHeaderOpen = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    bool removedComponent = drawComponentContextMenu("ScriptsComponent", compInfo, entityHandle);
    if (removedComponent || !gameObject.HasComponent<ECS::ScriptsComponent>())
    {
        ImGui::PopID();
        return;
    }

    if (isHeaderOpen)
    {
        m_context->uiCallbacks->SelectedGuids = getCurrentSelectionGuids();
        for (size_t i = 0; i < scriptsComponent.scripts.size(); ++i)
        {
            auto& script = scriptsComponent.scripts[i];
            ImGui::PushID(static_cast<int>(i));

            std::string scriptLabel = "Script (Unassigned)";
            if (script.scriptAsset.Valid())
            {
                scriptLabel = AssetManager::GetInstance().GetAssetName(script.scriptAsset.assetGuid);
            }

            if (CustomDrawing::WidgetDrawer<ECS::ScriptComponent>::Draw(scriptLabel, script, *m_context->uiCallbacks))
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
            }

            if (ImGui::BeginPopupContextItem("SingleScriptContextMenu"))
            {
                if (ImGui::MenuItem("移除脚本"))
                {
                    m_context->uiCallbacks->onValueChanged.Invoke();
                    scriptsComponent.scripts.erase(scriptsComponent.scripts.begin() + i);
                    ImGui::CloseCurrentPopup();
                    i--;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("复制脚本"))
                {
                    m_context->componentClipboard_Type = "ScriptComponent";
                    m_context->componentClipboard_Data = YAML::convert<ECS::ScriptComponent>::encode(script);
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::Spacing();
        float buttonWidth = 150.0f;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    }
    ImGui::PopID();
}

void InspectorPanel::drawCommonComponents(const std::vector<RuntimeGameObject>& selectedObjects)
{
    if (selectedObjects.empty()) return;
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();
    const auto& componentRegistry = ComponentRegistry::GetInstance();


    const auto& commonComponents = m_cachedCommonComponents;

    for (const auto& componentName : commonComponents)
    {
        const ComponentRegistration* compInfo = componentRegistry.Get(componentName);

        if (componentName == "Transform")
        {
            drawBatchTransformComponent(selectedObjects);
            continue;
        }

        drawBatchComponentHeader(componentName, compInfo, selectedObjects);
    }

    if (commonComponents.empty())
    {
        ImGui::Text("选中的对象没有共同组件。");
    }
}

void InspectorPanel::drawBatchTransformComponent(const std::vector<RuntimeGameObject>& selectedObjects)
{
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();

    bool allHaveParent = m_cachedBatchTransform.allHaveParent;

    std::string headerName = allHaveParent ? "Transform (Local)" : "Transform (混合)";
    if (isSelectionLocked())
    {
        headerName += " [固定]";
    }

    bool isHeadOpen = ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    if (isHeadOpen)
    {
        displayBatchTransformValues(selectedObjects, allHaveParent);

        ImGui::Separator();
        ImGui::Text("批量设置:");

        static ECS::Vector2f batchPosition = {0.0f, 0.0f};
        if (CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("设置位置", batchPosition, *m_context->uiCallbacks))
        {
            applyBatchTransformPosition(selectedObjects, batchPosition, allHaveParent);
        }

        static float batchRotation = 0.0f;
        if (CustomDrawing::WidgetDrawer<float>::Draw("设置旋转", batchRotation, *m_context->uiCallbacks))
        {
            applyBatchTransformRotation(selectedObjects, batchRotation, allHaveParent);
        }

        static ECS::Vector2f batchScale = {1.0f, 1.0f};
        if (CustomDrawing::WidgetDrawer<ECS::Vector2f>::Draw("设置缩放", batchScale, *m_context->uiCallbacks))
        {
            applyBatchTransformScale(selectedObjects, batchScale, allHaveParent);
        }
    }
}

void InspectorPanel::displayBatchTransformValues(const std::vector<RuntimeGameObject>& selectedObjects,
                                                 bool allHaveParent)
{
    if (!m_context->activeScene) return;
    if (selectedObjects.empty()) return;

    const auto& s = m_cachedBatchTransform;
    ImGui::Text("当前值:");

    if (s.positionSame)
    {
        ImGui::Text("位置: (%.3f, %.3f)", s.refPosition.x, s.refPosition.y);
    }
    else
    {
        ImGui::Text("位置: (不同值...)");
    }

    if (s.rotationSame)
    {
        ImGui::Text("旋转: %.3f", s.refRotation);
    }
    else
    {
        ImGui::Text("旋转: (不同值...)");
    }

    if (s.scaleSame)
    {
        ImGui::Text("缩放: (%.3f, %.3f)", s.refScale.x, s.refScale.y);
    }
    else
    {
        ImGui::Text("缩放: (不同值...)");
    }
}

size_t InspectorPanel::computeSelectionFingerprint(SelectionType type, const std::vector<Guid>& guids) const
{
    auto fnv64 = [](uint64_t h, uint64_t v)
    {
        const uint64_t FNV_PRIME = 1099511628211ull;
        h ^= v;
        h *= FNV_PRIME;
        return h;
    };
    uint64_t h = 1469598103934665603ull;
    h = fnv64(h, static_cast<uint64_t>(type));
    h = fnv64(h, static_cast<uint64_t>(guids.size()));
    for (const auto& g : guids)
    {
        std::string s = g.ToString();

        uint64_t part = 0;
        size_t i = 0;
        for (unsigned char c : s)
        {
            part = (part << 8) | static_cast<uint64_t>(c);
            if ((++i % 8) == 0)
            {
                h = fnv64(h, part);
                part = 0;
            }
        }
        if (i % 8) h = fnv64(h, part);
    }
    return static_cast<size_t>(h);
}

void InspectorPanel::rebuildCache(const std::vector<Guid>& guids, SelectionType type)
{
    m_cachedSelectedObjects.clear();
    m_cachedCommonComponents.clear();
    m_cachedBatchTransform = {};

    if (!m_context || !m_context->activeScene) return;
    if (type != SelectionType::GameObject) return;


    m_cachedSelectedObjects.reserve(guids.size());
    for (const auto& guid : guids)
    {
        RuntimeGameObject obj = m_context->activeScene->FindGameObjectByGuid(guid);
        if (obj.IsValid()) m_cachedSelectedObjects.push_back(obj);
    }

    if (m_cachedSelectedObjects.empty()) return;

    if (m_cachedSelectedObjects.size() > 1)
    {
        auto& registry = m_context->activeScene->GetRegistry();
        const auto& componentRegistry = ComponentRegistry::GetInstance();
        for (const auto& componentName : componentRegistry.GetAllRegisteredNames())
        {
            const ComponentRegistration* compInfo = componentRegistry.Get(componentName);
            if (!compInfo || !compInfo->isExposedInEditor) continue;
            if (componentName == "ScriptComponent") continue;

            bool allHaveComponent = true;
            for (const auto& obj : m_cachedSelectedObjects)
            {
                auto entityHandle = static_cast<entt::entity>(obj);
                if (!compInfo->has(registry, entityHandle))
                {
                    allHaveComponent = false;
                    break;
                }
            }
            if (allHaveComponent) m_cachedCommonComponents.push_back(componentName);
        }


        BatchTransformSummary s{};
        s.valid = true;
        s.allHaveParent = true;
        for (const auto& obj : m_cachedSelectedObjects)
        {
            if (!obj.HasComponent<ECS::ParentComponent>())
            {
                s.allHaveParent = false;
                break;
            }
        }

        auto& registry2 = m_context->activeScene->GetRegistry();
        auto firstEntity = static_cast<entt::entity>(m_cachedSelectedObjects[0]);
        auto& firstTransform = registry2.get<ECS::TransformComponent>(firstEntity);
        s.positionSame = true;
        s.rotationSame = true;
        s.scaleSame = true;
        s.refPosition = s.allHaveParent ? firstTransform.localPosition : firstTransform.position;
        s.refRotation = s.allHaveParent ? firstTransform.localRotation : firstTransform.rotation;
        s.refScale = s.allHaveParent ? firstTransform.localScale : firstTransform.scale;

        for (size_t i = 1; i < m_cachedSelectedObjects.size(); ++i)
        {
            auto entity = static_cast<entt::entity>(m_cachedSelectedObjects[i]);
            auto& t = registry2.get<ECS::TransformComponent>(entity);
            ECS::Vector2f pos = s.allHaveParent ? t.localPosition : t.position;
            float rot = s.allHaveParent ? t.localRotation : t.rotation;
            ECS::Vector2f sc = s.allHaveParent ? t.localScale : t.scale;
            if (std::abs(pos.x - s.refPosition.x) > 0.001f || std::abs(pos.y - s.refPosition.y) > 0.001f)
                s.positionSame
                    = false;
            if (std::abs(rot - s.refRotation) > 0.001f) s.rotationSame = false;
            if (std::abs(sc.x - s.refScale.x) > 0.001f || std::abs(sc.y - s.refScale.y) > 0.001f) s.scaleSame = false;
        }
        m_cachedBatchTransform = s;
    }
}

void InspectorPanel::applyBatchTransformPosition(const std::vector<RuntimeGameObject>& selectedObjects,
                                                 const ECS::Vector2f& position, bool allHaveParent)
{
    if (!m_context->activeScene) return;
    m_context->uiCallbacks->onValueChanged.Invoke();
    auto& registry = m_context->activeScene->GetRegistry();

    for (const auto& obj : selectedObjects)
    {
        auto entityHandle = static_cast<entt::entity>(obj);
        auto& transform = registry.get<ECS::TransformComponent>(entityHandle);

        if (allHaveParent)
        {
            transform.localPosition = position;
        }
        else
        {
            transform.position = position;
        }
    }
}

void InspectorPanel::applyBatchTransformRotation(const std::vector<RuntimeGameObject>& selectedObjects, float rotation,
                                                 bool allHaveParent)
{
    if (!m_context->activeScene) return;
    m_context->uiCallbacks->onValueChanged.Invoke();
    auto& registry = m_context->activeScene->GetRegistry();

    for (const auto& obj : selectedObjects)
    {
        auto entityHandle = static_cast<entt::entity>(obj);
        auto& transform = registry.get<ECS::TransformComponent>(entityHandle);

        if (allHaveParent)
        {
            transform.localRotation = rotation;
        }
        else
        {
            transform.rotation = rotation;
        }
    }
}

void InspectorPanel::applyBatchTransformScale(const std::vector<RuntimeGameObject>& selectedObjects,
                                              const ECS::Vector2f& scale, bool allHaveParent)
{
    if (!m_context->activeScene) return;
    m_context->uiCallbacks->onValueChanged.Invoke();
    auto& registry = m_context->activeScene->GetRegistry();

    for (const auto& obj : selectedObjects)
    {
        auto entityHandle = static_cast<entt::entity>(obj);
        auto& transform = registry.get<ECS::TransformComponent>(entityHandle);

        if (allHaveParent)
        {
            transform.localScale = scale;
        }
        else
        {
            transform.scale = scale;
        }
    }
}

void InspectorPanel::drawBatchComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                                              const std::vector<RuntimeGameObject>& selectedObjects)
{
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();

    std::string headerLabel = componentName + " (批量)";

    if (componentName == "ScriptsComponent")
    {
        headerLabel = "脚本 (批量)";
    }

    if (isSelectionLocked())
    {
        headerLabel += " [固定]";
    }

    bool isHeaderOpen = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    drawBatchComponentContextMenu(componentName, compInfo, selectedObjects);

    if (isHeaderOpen)
    {
        ImGui::Text("正在编辑 %d 个对象的 %s", static_cast<int>(selectedObjects.size()), componentName.c_str());

        if (componentName == "ScriptsComponent")
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "警告: 任何修改将同步到所有选中对象。");

            auto originalCallback = m_context->uiCallbacks->onValueChanged;
            m_context->uiCallbacks->onValueChanged.AddListener([this, selectedObjects, compInfo, originalCallback]()
            {
                if (originalCallback) originalCallback();
                applyPropertyToAllSelected(selectedObjects, compInfo);
            });

            auto firstObject = selectedObjects[0];
            drawScriptsComponentUI(firstObject);

            m_context->uiCallbacks->onValueChanged = originalCallback;
        }
        else
        {
            for (const auto& propInfo : compInfo->properties)
            {
                if (propInfo.draw_ui && propInfo.isExposedInEditor)
                {
                    drawBatchProperty(propInfo.name, propInfo, compInfo, selectedObjects);
                }
            }
        }
    }
}

void InspectorPanel::drawBatchProperty(const std::string& propName, const PropertyRegistration& propInfo,
                                       const ComponentRegistration* compInfo,
                                       const std::vector<RuntimeGameObject>& selectedObjects)
{
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();

    if (selectedObjects.empty()) return;

    ImGui::Text("%s:", propName.c_str());
    ImGui::SameLine();

    if (isSelectionLocked())
    {
        ImGui::TextDisabled("(批量-固定)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("修改此值将应用到所有 %d 个固定的对象", static_cast<int>(selectedObjects.size()));
        }
    }
    else
    {
        ImGui::TextDisabled("(批量)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("修改此值将应用到所有 %d 个选中的对象", static_cast<int>(selectedObjects.size()));
        }
    }

    auto firstEntityHandle = static_cast<entt::entity>(selectedObjects[0]);

    auto originalCallback = m_context->uiCallbacks->onValueChanged;

    m_context->uiCallbacks->onValueChanged.AddListener([this, selectedObjects, compInfo, originalCallback]()
    {
        if (originalCallback)
        {
            originalCallback();
        }

        applyPropertyToAllSelected(selectedObjects, compInfo);
    });

    m_context->uiCallbacks->SelectedGuids = getCurrentSelectionGuids();
    propInfo.draw_ui(propName, registry, firstEntityHandle, *m_context->uiCallbacks);

    m_context->uiCallbacks->onValueChanged = originalCallback;
}

void InspectorPanel::applyPropertyToAllSelected(const std::vector<RuntimeGameObject>& selectedObjects,
                                                const ComponentRegistration* compInfo)
{
    if (selectedObjects.size() <= 1) return;
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();
    auto firstEntityHandle = static_cast<entt::entity>(selectedObjects[0]);

    YAML::Node sourceData = compInfo->serialize(registry, firstEntityHandle);

    for (size_t i = 1; i < selectedObjects.size(); ++i)
    {
        auto entityHandle = static_cast<entt::entity>(selectedObjects[i]);
        compInfo->deserialize(registry, entityHandle, sourceData);
    }
}

void InspectorPanel::drawBatchComponentContextMenu(const std::string& componentName,
                                                   const ComponentRegistration* compInfo,
                                                   const std::vector<RuntimeGameObject>& selectedObjects)
{
    if (!m_context->activeScene) return;
    if (ImGui::BeginPopupContextItem())
    {
        auto& registry = m_context->activeScene->GetRegistry();

        if (compInfo->isRemovable)
        {
            if (ImGui::MenuItem("批量移除组件"))
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
                for (const auto& obj : selectedObjects)
                {
                    auto entityHandle = static_cast<entt::entity>(obj);
                    compInfo->remove(registry, entityHandle);
                }
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::TextDisabled("批量移除组件");
        }

        ImGui::Separator();

        if (ImGui::MenuItem("复制第一个对象的组件"))
        {
            if (!selectedObjects.empty())
            {
                auto firstEntityHandle = static_cast<entt::entity>(selectedObjects[0]);
                m_context->componentClipboard_Type = componentName;
                m_context->componentClipboard_Data = compInfo->serialize(registry, firstEntityHandle);
            }
        }

        bool canPasteValues = !m_context->componentClipboard_Type.empty() &&
            m_context->componentClipboard_Type == componentName;
        if (!canPasteValues) ImGui::BeginDisabled();
        if (ImGui::MenuItem("批量粘贴组件数值"))
        {
            m_context->uiCallbacks->onValueChanged.Invoke();
            for (const auto& obj : selectedObjects)
            {
                auto entityHandle = static_cast<entt::entity>(obj);
                compInfo->deserialize(registry, entityHandle, m_context->componentClipboard_Data);
            }
        }
        if (!canPasteValues) ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}

void InspectorPanel::drawComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                                         entt::entity entityHandle)
{
    if (!m_context->activeScene) return;
    auto& registry = m_context->activeScene->GetRegistry();

    void* componentRawPtr = compInfo->get_raw_ptr(registry, entityHandle);
    ECS::IComponent* componentBase = static_cast<ECS::IComponent*>(componentRawPtr);

    ImGui::PushID(componentName.c_str());
    if (ImGui::Checkbox("##Enabled", &componentBase->Enable))
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
    }
    std::string headerLabel = componentName;

    if (isSelectionLocked())
    {
        headerLabel += " [固定]";
    }
    ImGui::SameLine();
    bool isHeaderOpen = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    bool removedComponent = drawComponentContextMenu(componentName, compInfo, entityHandle);
    if (removedComponent || !compInfo->has(registry, entityHandle))
    {
        ImGui::PopID();
        return;
    }

    if (isHeaderOpen)
    {
        if (componentName == "TagComponent")
        {
            auto& tagComp = registry.get<ECS::TagComponent>(entityHandle);

            std::vector<std::string> tags = TagManager::GetAllTags();
            int currentIndex = -1;
            for (int i = 0; i < (int)tags.size(); ++i)
            {
                if (tags[i] == tagComp.tag)
                {
                    currentIndex = i;
                    break;
                }
            }
            if (currentIndex == -1)
            {
                TagManager::EnsureDefaults();
                tags = TagManager::GetAllTags();
                for (int i = 0; i < (int)tags.size(); ++i)
                    if (tags[i] == tagComp.tag)
                    {
                        currentIndex = i;
                        break;
                    }
            }

            ImGui::Text("标签");
            ImGui::SameLine();
            const char* preview = (currentIndex >= 0 && currentIndex < (int)tags.size())
                                      ? tags[currentIndex].c_str()
                                      : "(未设置)";
            if (ImGui::BeginCombo("##TagDropdown", preview))
            {
                for (int n = 0; n < (int)tags.size(); n++)
                {
                    bool selected = (n == currentIndex);
                    if (ImGui::Selectable(tags[n].c_str(), selected))
                    {
                        if (tagComp.tag != tags[n])
                        {
                            m_context->uiCallbacks->onValueChanged.Invoke();
                            tagComp.tag = tags[n];
                        }
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            static char newTagBuf[64] = {0};
            ImGui::SetNextItemWidth(150);
            ImGui::InputTextWithHint("##NewTag", "新建标签名", newTagBuf, sizeof(newTagBuf));
            ImGui::SameLine();
            if (ImGui::Button("添加"))
            {
                std::string nt = newTagBuf;
                if (!nt.empty())
                {
                    TagManager::AddTag(nt);
                    memset(newTagBuf, 0, sizeof(newTagBuf));
                }
            }
            ImGui::SameLine();
            bool canDelete = currentIndex >= 0 && tags[currentIndex] != std::string("Unknown");
            if (!canDelete) ImGui::BeginDisabled();
            if (ImGui::Button("删除当前"))
            {
                std::string toDel = (currentIndex >= 0 && currentIndex < (int)tags.size())
                                        ? tags[currentIndex]
                                        : std::string();
                if (!toDel.empty())
                {
                    TagManager::RemoveTag(toDel);
                    if (tagComp.tag == toDel)
                    {
                        m_context->uiCallbacks->onValueChanged.Invoke();
                        tagComp.tag = "Unknown";
                    }
                }
            }
            if (!canDelete) ImGui::EndDisabled();
        }
        else
        {
            for (const auto& propInfo : compInfo->properties)
            {
                if (propInfo.draw_ui && propInfo.isExposedInEditor)
                {
                    m_context->uiCallbacks->SelectedGuids = getCurrentSelectionGuids();

                    propInfo.draw_ui(propInfo.name, registry, entityHandle, *m_context->uiCallbacks);
                }
            }
        }
    }

    ImGui::PopID();
}

bool InspectorPanel::drawComponentContextMenu(const std::string& componentName, const ComponentRegistration* compInfo,
                                              entt::entity entityHandle)
{
    bool removed = false;
    if (!m_context->activeScene) return removed;

    if (ImGui::BeginPopupContextItem())
    {
        auto& registry = m_context->activeScene->GetRegistry();

        if (compInfo->isRemovable)
        {
            if (ImGui::MenuItem("移除组件"))
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
                compInfo->remove(registry, entityHandle);
                ImGui::CloseCurrentPopup();
                removed = true;
            }
        }
        else
        {
            ImGui::TextDisabled("移除组件");
        }

        if (!removed)
        {
            ImGui::Separator();

            if (ImGui::MenuItem("复制组件"))
            {
                m_context->componentClipboard_Type = componentName;
                m_context->componentClipboard_Data = compInfo->serialize(registry, entityHandle);
            }

            bool canPasteValues = !m_context->componentClipboard_Type.empty() &&
                m_context->componentClipboard_Type == componentName;
            if (!canPasteValues) ImGui::BeginDisabled();
            if (ImGui::MenuItem("粘贴组件数值"))
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
                compInfo->deserialize(registry, entityHandle, m_context->componentClipboard_Data);
            }
            if (!canPasteValues) ImGui::EndDisabled();

            if (componentName == "ScriptsComponent")
            {
                bool canPasteScript = !m_context->componentClipboard_Type.empty() &&
                    m_context->componentClipboard_Type == "ScriptComponent";
                if (!canPasteScript) ImGui::BeginDisabled();
                if (ImGui::MenuItem("粘贴脚本为新的"))
                {
                    m_context->uiCallbacks->onValueChanged.Invoke();
                    ECS::ScriptComponent newScript;
                    if (YAML::convert<ECS::ScriptComponent>::decode(m_context->componentClipboard_Data, newScript))
                    {
                        auto& scriptsComp = registry.get<ECS::ScriptsComponent>(entityHandle);
                        scriptsComp.scripts.push_back(newScript);
                    }
                }
                if (!canPasteScript) ImGui::EndDisabled();
            }
        }

        ImGui::EndPopup();
    }

    return removed;
}

void InspectorPanel::drawAddComponentButton()
{
    float buttonWidth = 200.0f;
    float windowWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("添加组件", ImVec2(buttonWidth, 25)))
    {
        PopupManager::GetInstance().Open("AddComponentPopup");
    }
}

void InspectorPanel::drawBatchAddComponentButton()
{
    float buttonWidth = 200.0f;
    float windowWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("批量添加组件", ImVec2(buttonWidth, 25)))
    {
        PopupManager::GetInstance().Open("AddComponentPopup");
    }
}

void InspectorPanel::drawDragDropTarget()
{
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (!m_context->activeScene) return;
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            if (handle.assetType == AssetType::CSharpScript)
            {
                std::vector<Guid> currentGuids = getCurrentSelectionGuids();
                bool anyAdded = false;

                for (const auto& guid : currentGuids)
                {
                    auto go = m_context->activeScene->FindGameObjectByGuid(guid);
                    if (go.IsValid())
                    {
                        auto& scriptsComp = go.HasComponent<ECS::ScriptsComponent>()
                                                ? go.GetComponent<ECS::ScriptsComponent>()
                                                : go.AddComponent<ECS::ScriptsComponent>();

                        scriptsComp.AddScript(handle, go.GetEntityHandle());
                        anyAdded = true;
                    }
                }

                if (anyAdded)
                {
                    m_context->uiCallbacks->onValueChanged.Invoke();
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

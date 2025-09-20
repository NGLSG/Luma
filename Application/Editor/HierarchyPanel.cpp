#include "../Utils/PCH.h"
#include "HierarchyPanel.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Components/IDComponent.h"
#include "../Resources/AssetManager.h"
#include "../SceneManager.h"
#include "../Utils/Logger.h"
#include "../Resources/Loaders/PrefabLoader.h"
#include "../Resources/RuntimeAsset/RuntimePrefab.h"
#include "../Utils/Profiler.h"
#include <algorithm>

#include "PopupManager.h"
#include "RelationshipComponent.h"
#include "JobSystem.h"
#include <cctype>
#include <memory>

void HierarchyPanel::Initialize(EditorContext* context)
{
    m_context = context;
    needsRebuildCache = true;
    itemHeight = NODE_HEIGHT;
    totalNodeCount = 0;
    visibleNodeCount = 0;
    lastBuildTime = 0.0f;
    m_sceneChangeListener = EventBus::GetInstance().Subscribe<SceneUpdateEvent>(
        [this](const SceneUpdateEvent&)
        {
            this->needsRebuildCache = true;
        });
}

void HierarchyPanel::Update(float deltaTime)
{PROFILE_FUNCTION();
    if (!m_context->gameObjectsToDelete.empty() && m_context->activeScene)
    {
        SceneManager::GetInstance().PushUndoState(m_context->activeScene);
        for (const auto& objGuid : m_context->gameObjectsToDelete)
        {
            RuntimeGameObject objToDel = m_context->activeScene->FindGameObjectByGuid(objGuid);
            if (objToDel.IsValid())
            {
                auto it = std::find(m_context->selectionList.begin(), m_context->selectionList.end(), objGuid);
                if (it != m_context->selectionList.end())
                {
                    m_context->selectionList.erase(it);
                }

                bool hasParent = objToDel.HasComponent<ECS::ParentComponent>();

                if (hasParent)
                {
                    objToDel.SetRoot();
                }
                m_context->activeScene->DestroyGameObject(objToDel);
            }
        }


        if (m_context->selectionList.empty())
        {
            clearSelection();
        }

        m_context->gameObjectsToDelete.clear();
        needsRebuildCache = true;
    }


    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_context->selectionList.empty() && m_isFocused)
    {
        m_context->gameObjectsToDelete = m_context->selectionList;
    }

    PopupManager::GetInstance().Register("HierarchyContextMenu", [this]()
                                         {
                                             this->drawContextMenu();
                                         },
                                         false, ImGuiWindowFlags_AlwaysAutoResize);

    if (needsRebuildCache)
    {
        buildHierarchyCache();
        needsRebuildCache = false;
    }
}

void HierarchyPanel::Draw()
{
    PROFILE_FUNCTION();


    if (m_context->objectToFocusInHierarchy.Valid())
    {
        needsRebuildCache = true;
        buildHierarchyCache();


        expandPathToObject(m_context->objectToFocusInHierarchy);


        m_context->objectToFocusInHierarchy = Guid();


        needsRebuildCache = true;
    }

    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (m_context->editingMode == EditingMode::Prefab)
    {
        if (ImGui::Button("< 返回场景"))
        {
            if (m_context->editingMode != EditingMode::Prefab)
            {
                ImGui::End();
                return;
            }


            m_context->editingMode = EditingMode::Scene;
            m_context->editingPrefabGuid = Guid();


            m_context->activeScene.reset();


            m_context->activeScene = m_context->sceneBeforePrefabEdit;
            SceneManager::GetInstance().SetCurrentScene(m_context->activeScene);
            m_context->sceneBeforePrefabEdit.reset();

            clearSelection();
            needsRebuildCache = true;
        }
        ImGui::Separator();
    }


#ifdef _DEBUG
    if (ImGui::CollapsingHeader("层级性能信息"))
    {
        ImGui::Text("总节点数: %d", totalNodeCount);
        ImGui::Text("可见节点数: %d", visibleNodeCount);
        ImGui::Text("缓存构建时间: %.2f ms", lastBuildTime);
        ImGui::Text("选中对象数: %d", static_cast<int>(m_context->selectionList.size()));
        ImGui::Separator();
    }
#endif

    
    ImGui::InputTextWithHint("##HierarchySearch", "搜索名称…", m_searchBuffer, sizeof(m_searchBuffer));
    ImGui::Separator();

    drawSceneCamera();

    drawVirtualizedGameObjects();


    handlePanelInteraction();


    handleDragDrop();

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        PopupManager::GetInstance().Open("HierarchyContextMenu");
    }

    ImGui::End();
}

void HierarchyPanel::Shutdown()
{
    hierarchyCache.clear();
    visibleNodeIndices.clear();
    expandedStates.clear();
    EventBus::GetInstance().Unsubscribe(m_sceneChangeListener);
}

void HierarchyPanel::drawSceneCamera()
{
    ImGuiTreeNodeFlags cameraFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (m_context->selectionType == SelectionType::SceneCamera)
    {
        cameraFlags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::TreeNodeEx("场景相机", cameraFlags);
    if (ImGui::IsItemClicked())
    {
        selectSceneCamera();
    }
}

void HierarchyPanel::drawVirtualizedGameObjects()
{
    PROFILE_FUNCTION();

    if (!m_context->activeScene) return;


    visibleNodeIndices.clear();
    for (int i = 0; i < static_cast<int>(hierarchyCache.size()); ++i)
    {
        if (hierarchyCache[i].isVisible)
        {
            visibleNodeIndices.push_back(i);
        }
    }
    visibleNodeCount = static_cast<int>(visibleNodeIndices.size());

    ImGui::BeginChild("HierarchyScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_None);


    
    if (m_searchBuffer[0] != '\0')
    {
        
        std::string q = m_searchBuffer;
        auto tolower_ascii = [](unsigned char c){ return static_cast<char>(std::tolower(c)); };
        std::string qlower = q; std::transform(qlower.begin(), qlower.end(), qlower.begin(), tolower_ascii);

        std::vector<int> matches;
        matches.reserve(128);
        for (int i = 0; i < static_cast<int>(hierarchyCache.size()); ++i)
        {
            const auto& name = hierarchyCache[i].displayName;
            std::string nlower = name; std::transform(nlower.begin(), nlower.end(), nlower.begin(), tolower_ascii);
            if (nlower.find(qlower) != std::string::npos)
            {
                matches.push_back(i);
            }
        }

        int total = static_cast<int>(matches.size());
        int drawCount = std::min(total, MAX_VISIBLE_NODES);

        if (total == 0)
        {
            ImGui::TextDisabled("未找到匹配项");
        }
        else
        {
            ImGui::Text("匹配 %d 个，显示前 %d 个", total, drawCount);
            ImGui::Separator();
            for (int i = 0; i < drawCount; ++i)
            {
                int nodeIdx = matches[i];
                const auto& node = hierarchyCache[nodeIdx];
                bool selected = std::find(m_context->selectionList.begin(), m_context->selectionList.end(), node.objectGuid) != m_context->selectionList.end();
                if (ImGui::Selectable(node.displayName.c_str(), selected))
                {
                    expandPathToObject(node.objectGuid);
                    selectSingleGameObject(node.objectGuid);
                    m_context->objectToFocusInHierarchy = node.objectGuid;
                }
            }
        }
        ImGui::EndChild();
        return;
    }

    
    if (visibleNodeCount == 0)
    {
        drawDropSeparator(0);
    }
    else
    {
        int cappedCount = std::min(visibleNodeCount, MAX_VISIBLE_NODES);
        ImGuiListClipper clipper;
        clipper.Begin(cappedCount, itemHeight);

        while (clipper.Step())
        {
            if (clipper.DisplayStart == 0)
            {
                drawDropSeparator(0);
            }

            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                int nodeIndex = visibleNodeIndices[i];
                if (nodeIndex < static_cast<int>(hierarchyCache.size()))
                {
                    drawVirtualizedNode(hierarchyCache[nodeIndex], nodeIndex);
                    drawDropSeparator(i + 1);
                }
            }
        }
    }

    ImGui::EndChild();
}

void HierarchyPanel::drawDropSeparator(int index)
{
    if (!m_context->activeScene) return;
    ImGui::PushID(std::string("Separator##" + std::to_string(index)).c_str());

    float width = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton("##drop_target", ImVec2(width, 4.0f));

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS"))
        {
            size_t guidCount = payload->DataSize / sizeof(Guid);
            if (guidCount == 0 || guidCount > 1000)
            {
                LogError("拖拽的对象数量异常: {}", guidCount);
                ImGui::EndDragDropTarget();
                ImGui::PopID();
                return;
            }

            const Guid* guidArray = static_cast<const Guid*>(payload->Data);
            std::vector<Guid> draggedGuids;
            draggedGuids.reserve(guidCount);

            for (size_t i = 0; i < guidCount; ++i)
            {
                if (guidArray[i].Valid())
                {
                    draggedGuids.push_back(guidArray[i]);
                }
            }

            if (draggedGuids.empty())
            {
                LogWarn("没有有效的拖拽对象");
                ImGui::EndDragDropTarget();
                ImGui::PopID();
                return;
            }


            RuntimeGameObject targetParent(entt::null, nullptr);
            int targetIndex = 0;
            bool validDrop = false;


            if (index < 0 || index > visibleNodeCount)
            {
                LogError("分隔符索引超出范围: {} (可见节点数: {})", index, visibleNodeCount);
                ImGui::EndDragDropTarget();
                ImGui::PopID();
                return;
            }

            if (index < visibleNodeCount && index < static_cast<int>(visibleNodeIndices.size()))
            {
                int nodeIndex = visibleNodeIndices[index];
                if (nodeIndex >= 0 && nodeIndex < static_cast<int>(hierarchyCache.size()))
                {
                    const auto& nextNode = hierarchyCache[nodeIndex];
                    RuntimeGameObject nextObject = m_context->activeScene->FindGameObjectByGuid(nextNode.objectGuid);

                    if (nextObject.IsValid())
                    {
                        targetParent = nextObject.GetParent();

                        if (targetParent.IsValid())
                        {
                            targetIndex = nextObject.GetSiblingIndex();
                        }
                        else
                        {
                            targetIndex = m_context->activeScene->GetRootSiblingIndex(nextObject);
                        }
                        validDrop = true;
                    }
                }
            }
            else if (visibleNodeCount > 0 && !visibleNodeIndices.empty())
            {
                int lastVisibleIndex = visibleNodeIndices.back();
                if (lastVisibleIndex >= 0 && lastVisibleIndex < static_cast<int>(hierarchyCache.size()))
                {
                    const auto& lastNode = hierarchyCache[lastVisibleIndex];
                    RuntimeGameObject lastObject = m_context->activeScene->FindGameObjectByGuid(lastNode.objectGuid);

                    if (lastObject.IsValid())
                    {
                        targetParent = lastObject.GetParent();

                        if (targetParent.IsValid())
                        {
                            auto children = targetParent.GetChildren();
                            targetIndex = static_cast<int>(children.size());
                        }
                        else
                        {
                            auto& rootObjects = m_context->activeScene->GetRootGameObjects();
                            targetIndex = static_cast<int>(rootObjects.size());
                        }
                        validDrop = true;
                    }
                }
            }
            else
            {
                targetIndex = 0;
                validDrop = true;
            }

            if (!validDrop)
            {
                LogError("无法确定有效的拖放目标位置");
                ImGui::EndDragDropTarget();
                ImGui::PopID();
                return;
            }


            SceneManager::GetInstance().PushUndoState(m_context->activeScene);

            bool anyMoved = false;
            int currentTargetIndex = targetIndex;

            for (const auto& draggedGuid : draggedGuids)
            {
                RuntimeGameObject draggedObject = m_context->activeScene->FindGameObjectByGuid(draggedGuid);
                if (!draggedObject.IsValid())
                {
                    LogWarn("找不到拖拽的对象: {}", draggedGuid.ToString());
                    continue;
                }


                if (draggedObject.GetParent() == targetParent)
                {
                    int currentIndex = targetParent.IsValid()
                                           ? draggedObject.GetSiblingIndex()
                                           : m_context->activeScene->GetRootSiblingIndex(draggedObject);

                    if (currentIndex == currentTargetIndex)
                    {
                        continue;
                    }
                }


                if (targetParent.IsValid() && targetParent.IsDescendantOf(draggedObject))
                {
                    LogWarn("跳过循环依赖的移动: {} -> {}",
                            draggedObject.GetName(), targetParent.GetName());
                    continue;
                }

                try
                {
                    if (targetParent.IsValid())
                    {
                        draggedObject.SetParent(targetParent);
                        draggedObject.SetSiblingIndex(currentTargetIndex);
                    }
                    else
                    {
                        draggedObject.SetRoot();
                        m_context->activeScene->SetRootSiblingIndex(draggedObject, currentTargetIndex);
                    }

                    anyMoved = true;
                    currentTargetIndex++;

                    LogInfo("成功移动对象: {} 到位置 {}",
                            draggedObject.GetName(), currentTargetIndex - 1);
                }
                catch (const std::exception& e)
                {
                    LogError("移动对象时发生错误: {} - {}",
                             draggedObject.GetName(), e.what());
                }
            }

            if (anyMoved)
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
                needsRebuildCache = true;
                LogInfo("批量移动完成，共移动 {} 个对象",
                        static_cast<int>(draggedGuids.size()));
            }
        }
        ImGui::EndDragDropTarget();
    }


    if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) && ImGui::IsMouseDragging(0))
    {
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::GetColorU32(ImGuiCol_DragDropTarget)
        );
    }

    ImGui::PopID();
}

void HierarchyPanel::drawVirtualizedNode(const HierarchyNode& node, int virtualIndex)
{
    PROFILE_FUNCTION();
    if (!m_context->activeScene) return;
    RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(node.objectGuid);
    if (!gameObject.IsValid()) return;

    ImGui::PushID(virtualIndex);


    float indentWidth = node.depth * 20.0f;


    ImVec2 originalCursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(originalCursorPos.x + indentWidth);


    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;


    bool isSelected = std::find(m_context->selectionList.begin(), m_context->selectionList.end(), node.objectGuid) !=
        m_context->selectionList.end();
    if (isSelected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }


    if (!node.hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    else
    {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        if (isNodeExpanded(node.objectGuid))
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
    }


    bool nodeOpen = false;
    if (node.hasChildren)
    {
        nodeOpen = ImGui::TreeNodeEx(node.displayName.c_str(), flags);


        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            ImVec2 itemMin = ImGui::GetItemRectMin();
            ImVec2 mousePos = ImGui::GetMousePos();


            if (mousePos.x >= itemMin.x && mousePos.x <= itemMin.x + 20.0f)
            {
                bool currentExpanded = isNodeExpanded(node.objectGuid);
                setNodeExpanded(node.objectGuid, !currentExpanded);
                needsRebuildCache = true;
            }
            else
            {
                handleNodeSelection(node.objectGuid);
            }
        }
    }
    else
    {
        ImGui::TreeNodeEx(node.displayName.c_str(), flags);


        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            handleNodeSelection(node.objectGuid);
        }
    }


    handleNodeDragDrop(node);


    if (ImGui::BeginPopupContextItem())
    {
        if (!isSelected)
        {
            selectSingleGameObject(node.objectGuid);
        }

        if (ImGui::MenuItem("创建空子对象"))
        {
            if (!m_context->selectionList.empty())
            {
                RuntimeGameObject firstSelected = m_context->activeScene->FindGameObjectByGuid(
                    m_context->selectionList[0]);
                if (firstSelected.IsValid())
                {
                    CreateEmptyGameObjectAsChild(firstSelected);
                }
            }
        }

        if (ImGui::MenuItem("复制"))
        {
            CopySelectedGameObjects();
        }

        if (ImGui::MenuItem("删除"))
        {
            m_context->gameObjectsToDelete = m_context->selectionList;
        }

        if (ImGui::MenuItem("粘贴为子对象", nullptr, false, m_context->gameObjectClipboard.has_value()))
        {
            PasteGameObjects(&gameObject);
        }
        ImGui::EndPopup();
    }


    if (nodeOpen)
    {
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void HierarchyPanel::handleNodeSelection(const Guid& objectGuid)
{
    bool ctrlPressed = ImGui::GetIO().KeyCtrl;
    bool shiftPressed = ImGui::GetIO().KeyShift;

    if (shiftPressed && m_context->selectionAnchor.Valid())
    {
        handleRangeSelection(objectGuid);
    }
    else if (ctrlPressed)
    {
        toggleGameObjectSelection(objectGuid);
    }
    else
    {
        selectSingleGameObject(objectGuid);
    }
}

void HierarchyPanel::handleRangeSelection(const Guid& endGuid)
{
    if (!m_context->selectionAnchor.Valid())
    {
        selectSingleGameObject(endGuid);
        return;
    }


    int anchorIndex = -1;
    int endIndex = -1;

    for (int i = 0; i < visibleNodeCount; ++i)
    {
        int nodeIndex = visibleNodeIndices[i];
        const Guid& nodeGuid = hierarchyCache[nodeIndex].objectGuid;

        if (nodeGuid == m_context->selectionAnchor)
        {
            anchorIndex = i;
        }
        if (nodeGuid == endGuid)
        {
            endIndex = i;
        }
    }

    if (anchorIndex != -1 && endIndex != -1)
    {
        int startIndex = std::min(anchorIndex, endIndex);
        int endIndexRange = std::max(anchorIndex, endIndex);


        m_context->selectionList.clear();
        for (int i = startIndex; i <= endIndexRange; ++i)
        {
            int nodeIndex = visibleNodeIndices[i];
            m_context->selectionList.push_back(hierarchyCache[nodeIndex].objectGuid);
        }

        m_context->selectionType = SelectionType::GameObject;
    }
}

void HierarchyPanel::toggleGameObjectSelection(const Guid& objectGuid)
{
    auto it = std::find(m_context->selectionList.begin(), m_context->selectionList.end(), objectGuid);
    if (it != m_context->selectionList.end())
    {
        m_context->selectionList.erase(it);
        if (m_context->selectionList.empty())
        {
            m_context->selectionType = SelectionType::NA;
        }
    }
    else
    {
        m_context->selectionList.push_back(objectGuid);
        m_context->selectionType = SelectionType::GameObject;
    }
}

void HierarchyPanel::selectSingleGameObject(const Guid& objectGuid)
{
    m_context->selectionType = SelectionType::GameObject;
    m_context->selectionList.clear();
    m_context->selectionList.push_back(objectGuid);
    m_context->selectionAnchor = objectGuid;
}

void HierarchyPanel::buildHierarchyCache()
{
    PROFILE_FUNCTION();

    auto startTime = std::chrono::steady_clock::now();

    hierarchyCache.clear();
    totalNodeCount = 0;

    if (!m_context->activeScene) return;

    
    std::vector<Guid> rootGuids;
    {
        auto& roots = m_context->activeScene->GetRootGameObjects();
        rootGuids.reserve(roots.size());
        for (auto& go : roots)
        {
            if (go.IsValid()) rootGuids.push_back(go.GetGuid());
        }
    }

    
    struct BuildJob : public IJob
    {
        EditorContext* ctx;
        Guid rootGuid;
        const std::unordered_map<Guid, bool>* expandedStates;
        std::vector<HierarchyPanel::HierarchyNode>* out;
        int startDepth = 0;
        bool startVisible = true;

        static void BuildRec(EditorContext* ctx,
                             const std::unordered_map<Guid, bool>* expandedStates,
                             RuntimeGameObject& go,
                             int depth,
                             bool parentVisible,
                             std::vector<HierarchyPanel::HierarchyNode>& out)
        {
            if (!go.IsValid()) return;

            auto children = go.GetChildren();
            bool hasChildren = !children.empty();

            HierarchyPanel::HierarchyNode node(go.GetGuid(), go.GetName(), depth, hasChildren);
            node.isVisible = parentVisible;
            out.push_back(std::move(node));

            bool isExpanded = true;
            if (expandedStates)
            {
                auto it = expandedStates->find(go.GetGuid());
                if (it != expandedStates->end()) isExpanded = it->second;
            }

            if (hasChildren && isExpanded)
            {
                bool childrenVisible = parentVisible && isExpanded;
                for (auto& child : children)
                {
                    if (child.IsValid())
                    {
                        BuildRec(ctx, expandedStates, child, depth + 1, childrenVisible, out);
                    }
                }
            }
        }

        void Execute() override
        {
            if (!ctx || !ctx->activeScene || !out) return;
            RuntimeGameObject root = ctx->activeScene->FindGameObjectByGuid(rootGuid);
            if (!root.IsValid()) return;
            out->clear();
            out->reserve(64);
            BuildRec(ctx, expandedStates, root, startDepth, startVisible, *out);
        }
    };

    size_t rootCount = rootGuids.size();
    std::vector<std::vector<HierarchyPanel::HierarchyNode>> partialResults(rootCount);

    auto& jobSystem = JobSystem::GetInstance();
    std::vector<JobHandle> handles;
    handles.reserve(rootCount);

    
    int threadCount = std::max(1, jobSystem.GetThreadCount());
    bool useParallel = (rootCount > 1) && (threadCount > 1);

    if (useParallel)
    {
        std::vector<std::unique_ptr<BuildJob>> jobs;
        jobs.reserve(rootCount);
        for (size_t i = 0; i < rootCount; ++i)
        {
            auto job = std::make_unique<BuildJob>();
            job->ctx = m_context;
            job->rootGuid = rootGuids[i];
            job->expandedStates = &expandedStates;
            job->out = &partialResults[i];
            job->startDepth = 0;
            job->startVisible = true;
            handles.emplace_back(jobSystem.Schedule(job.get()));
            jobs.push_back(std::move(job));
        }
        JobSystem::CompleteAll(handles);
    }
    else
    {
        
        if (rootCount == 1 && threadCount > 1)
        {
            RuntimeGameObject root = m_context->activeScene->FindGameObjectByGuid(rootGuids[0]);
            if (root.IsValid())
            {
                auto children = root.GetChildren();
                bool hasChildren = !children.empty();

                
                std::vector<HierarchyPanel::HierarchyNode> childResults;
                partialResults[0].clear();
                partialResults[0].reserve(64);
                HierarchyPanel::HierarchyNode rootNode(root.GetGuid(), root.GetName(), 0, hasChildren);
                rootNode.isVisible = true;
                partialResults[0].push_back(std::move(rootNode));

                bool isExpanded = true;
                auto it = expandedStates.find(root.GetGuid());
                if (it != expandedStates.end()) isExpanded = it->second;

                if (hasChildren && isExpanded)
                {
                    
                    std::vector<Guid> childGuids;
                    childGuids.reserve(children.size());
                    for (auto& c : children) if (c.IsValid()) childGuids.push_back(c.GetGuid());

                    std::vector<std::unique_ptr<BuildJob>> childJobs;
                    std::vector<JobHandle> childHandles;
                    childJobs.reserve(childGuids.size());
                    childHandles.reserve(childGuids.size());

                    std::vector<std::vector<HierarchyPanel::HierarchyNode>> perChild(childGuids.size());
                    for (size_t ci = 0; ci < childGuids.size(); ++ci)
                    {
                        auto job = std::make_unique<BuildJob>();
                        job->ctx = m_context;
                        job->rootGuid = childGuids[ci];
                        job->expandedStates = &expandedStates;
                        job->out = &perChild[ci];
                        job->startDepth = 1;
                        job->startVisible = true;
                        childHandles.emplace_back(jobSystem.Schedule(job.get()));
                        childJobs.push_back(std::move(job));
                    }
                    JobSystem::CompleteAll(childHandles);

                    
                    size_t totalPerChild = 0; for (auto& v : perChild) totalPerChild += v.size();
                    partialResults[0].reserve(partialResults[0].size() + totalPerChild);
                    for (auto& v : perChild)
                    {
                        if (!v.empty())
                        {
                            partialResults[0].insert(partialResults[0].end(), std::make_move_iterator(v.begin()),
                                                     std::make_move_iterator(v.end()));
                        }
                    }
                }
            }
        }
        else
        {
            
            for (size_t i = 0; i < rootCount; ++i)
            {
                RuntimeGameObject root = m_context->activeScene->FindGameObjectByGuid(rootGuids[i]);
                if (!root.IsValid()) continue;
                partialResults[i].clear();
                partialResults[i].reserve(64);
                BuildJob::BuildRec(m_context, &expandedStates, root, 0, true, partialResults[i]);
            }
        }
    }

    
    size_t totalNodes = 0;
    for (auto& vec : partialResults) totalNodes += vec.size();
    hierarchyCache.reserve(totalNodes);
    for (auto& vec : partialResults)
    {
        if (!vec.empty())
        {
            hierarchyCache.insert(hierarchyCache.end(), std::make_move_iterator(vec.begin()),
                                  std::make_move_iterator(vec.end()));
        }
    }

    totalNodeCount = static_cast<int>(hierarchyCache.size());
    updateNodeVisibility();

    auto endTime = std::chrono::steady_clock::now();
    lastBuildTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    LogInfo("层级缓存重建完成，节点数: {}, 耗时: {:.2f}ms", totalNodeCount, lastBuildTime);
}

void HierarchyPanel::buildNodeRecursive(RuntimeGameObject& gameObject, int depth, bool parentVisible)
{
    if (!gameObject.IsValid()) return;

    auto children = gameObject.GetChildren();
    bool hasChildren = !children.empty();


    HierarchyNode node(gameObject.GetGuid(), gameObject.GetName(), depth, hasChildren);
    node.isVisible = parentVisible;
    hierarchyCache.push_back(node);
    totalNodeCount++;


    if (hasChildren && isNodeExpanded(gameObject.GetGuid()))
    {
        bool childrenVisible = parentVisible && isNodeExpanded(gameObject.GetGuid());
        for (auto& child : children)
        {
            if (child.IsValid())
            {
                buildNodeRecursive(child, depth + 1, childrenVisible);
            }
        }
    }
}

void HierarchyPanel::updateNodeVisibility()
{
    for (size_t i = 0; i < hierarchyCache.size(); ++i)
    {
        HierarchyNode& node = hierarchyCache[i];

        if (node.depth == 0)
        {
            node.isVisible = true;
        }
        else
        {
            node.isVisible = false;


            bool allAncestorsExpanded = true;
            int currentDepth = node.depth;

            for (int j = static_cast<int>(i) - 1; j >= 0 && currentDepth > 0; --j)
            {
                const HierarchyNode& ancestorNode = hierarchyCache[j];

                if (ancestorNode.depth == currentDepth - 1)
                {
                    if (!isNodeExpanded(ancestorNode.objectGuid))
                    {
                        allAncestorsExpanded = false;
                        break;
                    }
                    currentDepth--;
                }
            }

            node.isVisible = allAncestorsExpanded;
        }
    }
}

void HierarchyPanel::handleNodeDragDrop(const HierarchyNode& node)
{
    if (!m_context->activeScene) return;
    RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(node.objectGuid);
    if (!gameObject.IsValid()) return;


    if (ImGui::BeginDragDropSource())
    {
        bool isCurrentSelected = std::find(m_context->selectionList.begin(), m_context->selectionList.end(),
                                           node.objectGuid) != m_context->selectionList.end();
        if (!isCurrentSelected)
        {
            selectSingleGameObject(node.objectGuid);
        }


        std::vector<Guid> selectedGuids = m_context->selectionList;

        if (!selectedGuids.empty())
        {
            bool allGuidsValid = true;
            for (const auto& guid : selectedGuids)
            {
                if (!guid.Valid())
                {
                    LogError("检测到无效的GUID在选择列表中: {}", guid.ToString());
                    allGuidsValid = false;
                    break;
                }
            }

            if (allGuidsValid)
            {
                ImGui::SetDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS",
                                          selectedGuids.data(),
                                          selectedGuids.size() * sizeof(Guid));


                if (selectedGuids.size() == 1)
                {
                    ImGui::Text("%s", node.displayName.c_str());
                }
                else
                {
                    ImGui::Text("拖拽 %d 个对象", static_cast<int>(selectedGuids.size()));
                }
            }
            else
            {
                LogError("检测到无效GUID，取消拖拽操作");
            }
        }
        else
        {
            LogError("选择列表为空，无法开始拖拽");
        }

        ImGui::EndDragDropSource();
    }


    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            handlePrefabDrop(handle, &gameObject);
        }


        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS"))
        {
            size_t guidCount = payload->DataSize / sizeof(Guid);
            const Guid* guidArray = static_cast<const Guid*>(payload->Data);

            std::vector<Guid> draggedGuids;
            draggedGuids.reserve(guidCount);

            for (size_t i = 0; i < guidCount; ++i)
            {
                draggedGuids.push_back(guidArray[i]);
            }

            LogInfo("接收到 {} 个GameObject的拖拽", draggedGuids.size());
            handleGameObjectsDrop(draggedGuids, &gameObject);
        }

        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::handlePrefabDrop(const AssetHandle& handle, RuntimeGameObject* targetParent)
{
    if (!m_context->activeScene) return;
    const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
    if (!meta || meta->type != AssetType::Prefab || !targetParent || !targetParent->IsValid())
        return;

    auto prefabLoader = PrefabLoader();
    sk_sp<RuntimePrefab> prefab = prefabLoader.LoadAsset(handle.assetGuid);

    if (prefab)
    {
        SceneManager::GetInstance().PushUndoState(m_context->activeScene);
        RuntimeGameObject newInstance = m_context->activeScene->Instantiate(*prefab, targetParent);

        if (newInstance.IsValid())
        {
            selectSingleGameObject(newInstance.GetGuid());
            needsRebuildCache = true;
            LogInfo("Prefab实例化为子对象成功，父对象: {}", targetParent->GetName());
        }
    }
    else
    {
        LogError("加载Prefab失败，GUID: {}", handle.assetGuid.ToString());
    }
}

void HierarchyPanel::handleGameObjectsDrop(const std::vector<Guid>& draggedGuids, RuntimeGameObject* targetParent)
{
    if (!m_context->activeScene) return;
    if (!targetParent || !targetParent->IsValid())
    {
        LogError("目标父对象无效");
        return;
    }

    if (draggedGuids.empty())
    {
        LogWarn("拖拽的对象列表为空");
        return;
    }

    bool anyMoved = false;
    std::vector<std::string> errorMessages;

    SceneManager::GetInstance().PushUndoState(m_context->activeScene);

    for (const auto& draggedGuid : draggedGuids)
    {
        if (!draggedGuid.Valid())
        {
            errorMessages.push_back(std::format("无效的GUID: {}", draggedGuid.ToString()));
            continue;
        }

        RuntimeGameObject draggedObject = m_context->activeScene->FindGameObjectByGuid(draggedGuid);
        if (!draggedObject.IsValid())
        {
            errorMessages.push_back(std::format("找不到GUID对应的对象: {}", draggedGuid.ToString()));
            continue;
        }


        if (draggedObject == *targetParent)
        {
            errorMessages.push_back("不能将对象拖拽到自己身上");
            continue;
        }

        if (targetParent->IsDescendantOf(draggedObject))
        {
            errorMessages.push_back(std::format("不能将对象 '{}' 拖拽到自己的子对象上，这会造成循环依赖",
                                                draggedObject.GetName()));
            continue;
        }


        try
        {
            draggedObject.SetParent(*targetParent);
            anyMoved = true;
            LogInfo("GameObject重新父子化成功: {} -> {}", draggedObject.GetName(), targetParent->GetName());
        }
        catch (const std::exception& e)
        {
            errorMessages.push_back(std::format("重新父子化对象 '{}' 时发生错误: {}",
                                                draggedObject.GetName(), e.what()));
        }
    }

    if (anyMoved)
    {
        m_context->uiCallbacks->onValueChanged.Invoke();
        needsRebuildCache = true;
        LogInfo("拖拽操作完成，已移动 {} 个对象",
                static_cast<int>(draggedGuids.size() - errorMessages.size()));
    }


    for (const auto& error : errorMessages)
    {
        LogWarn("拖拽错误: {}", error);
    }
}

void HierarchyPanel::handlePanelInteraction()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
    {
        clearSelection();
    }
}

void HierarchyPanel::handleDragDrop()
{
    if (!m_context->activeScene) return;
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            handlePrefabDrop(handle, nullptr);
        }


        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS"))
        {
            size_t guidCount = payload->DataSize / sizeof(Guid);
            const Guid* guidArray = static_cast<const Guid*>(payload->Data);

            std::vector<Guid> draggedGuids;
            draggedGuids.reserve(guidCount);

            for (size_t i = 0; i < guidCount; ++i)
            {
                draggedGuids.push_back(guidArray[i]);
            }

            LogInfo("在根目录处理 {} 个GameObject的拖拽", draggedGuids.size());
            SceneManager::GetInstance().PushUndoState(m_context->activeScene);
            bool anyMoved = false;
            for (const auto& draggedGuid : draggedGuids)
            {
                RuntimeGameObject draggedObject = m_context->activeScene->FindGameObjectByGuid(draggedGuid);
                if (draggedObject.IsValid())
                {
                    draggedObject.SetRoot();
                    anyMoved = true;
                    LogInfo("GameObject设置为根对象: {}", draggedObject.GetName());
                }
            }

            if (anyMoved)
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
                needsRebuildCache = true;
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::drawContextMenu()
{
    if (ImGui::MenuItem("创建空对象"))
    {
        CreateEmptyGameObject();
    }
    if (ImGui::MenuItem("粘贴", nullptr, false, m_context->gameObjectClipboard.has_value()))
    {
        PasteGameObjects(nullptr);
    }
}

void HierarchyPanel::CreateEmptyGameObject()
{
    if (!m_context->activeScene) return;

    SceneManager::GetInstance().PushUndoState(m_context->activeScene);

    RuntimeGameObject newGo = m_context->activeScene->CreateGameObject("GameObject");
    selectSingleGameObject(newGo.GetGuid());
    needsRebuildCache = true;
}

void HierarchyPanel::CreateEmptyGameObjectAsChild(RuntimeGameObject& parent)
{
    if (!parent.IsValid()) return;
    if (!m_context->activeScene) return;
    SceneManager::GetInstance().PushUndoState(m_context->activeScene);

    RuntimeGameObject newGo = m_context->activeScene->CreateGameObject("GameObject");
    newGo.SetParent(parent);
    selectSingleGameObject(newGo.GetGuid());
    needsRebuildCache = true;
}

void HierarchyPanel::CopySelectedGameObjects()
{
    if (!m_context->activeScene) return;
    if (m_context->selectionList.empty()) return;

    std::vector<Data::PrefabNode> clipboardData;

    for (const auto& objGuid : m_context->selectionList)
    {
        RuntimeGameObject selectedObject = m_context->activeScene->FindGameObjectByGuid(objGuid);
        if (selectedObject.IsValid())
        {
            clipboardData.push_back(selectedObject.SerializeToPrefabData());
        }
    }

    if (!clipboardData.empty())
    {
        m_context->gameObjectClipboard = clipboardData;
        LogInfo("已复制 {} 个GameObject到剪贴板。", clipboardData.size());
    }
}

void HierarchyPanel::PasteGameObjects(RuntimeGameObject* parent)
{
    if (!m_context->activeScene || !m_context->gameObjectClipboard.has_value()) return;

    SceneManager::GetInstance().PushUndoState(m_context->activeScene);

    std::vector<Guid> newObjects;

    for (const auto& nodeToPaste : m_context->gameObjectClipboard.value())
    {
        RuntimeGameObject pastedObject = m_context->activeScene->CreateHierarchyFromNode(nodeToPaste, parent);
        if (pastedObject.IsValid())
        {
            newObjects.push_back(pastedObject.GetGuid());
            LogInfo("从 {} 粘贴GameObject '{}' 成功。", nodeToPaste.localGuid.ToString(), pastedObject.GetGuid().ToString());
        }
    }


    if (!newObjects.empty())
    {
        m_context->selectionType = SelectionType::GameObject;
        m_context->selectionList = newObjects;
        m_context->selectionAnchor = newObjects[0];
    }

    needsRebuildCache = true;
}

void HierarchyPanel::selectSceneCamera()
{
    m_context->selectionType = SelectionType::SceneCamera;
    m_context->selectionList.clear();
    m_context->selectionAnchor = Guid();
}

void HierarchyPanel::clearSelection()
{
    m_context->selectionType = SelectionType::NA;
    m_context->selectionList.clear();
    m_context->selectionAnchor = Guid();
}

bool HierarchyPanel::isNodeExpanded(const Guid& objectGuid) const
{
    auto it = expandedStates.find(objectGuid);
    return it != expandedStates.end() ? it->second : true;
}

void HierarchyPanel::setNodeExpanded(const Guid& objectGuid, bool expanded)
{
    expandedStates[objectGuid] = expanded;
}

void HierarchyPanel::expandPathToObject(const Guid& targetGuid)
{
    if (!m_context->activeScene) return;


    RuntimeGameObject targetObject = m_context->activeScene->FindGameObjectByGuid(targetGuid);
    if (!targetObject.IsValid()) return;


    std::vector<Guid> pathGuids;
    RuntimeGameObject currentObject = targetObject;

    while (currentObject.IsValid())
    {
        pathGuids.push_back(currentObject.GetGuid());


        if (currentObject.HasComponent<ECS::ParentComponent>())
        {
            auto& parentComp = currentObject.GetComponent<ECS::ParentComponent>();
            currentObject = m_context->activeScene->FindGameObjectByEntity(parentComp.parent);
        }
        else
        {
            break;
        }
    }


    std::reverse(pathGuids.begin(), pathGuids.end());


    for (size_t i = 0; i < pathGuids.size() - 1; ++i)
    {
        setNodeExpanded(pathGuids[i], true);
    }

    LogInfo("展开对象路径，目标: {}", targetGuid.ToString());
}

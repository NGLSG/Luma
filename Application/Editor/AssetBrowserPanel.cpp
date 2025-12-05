#include "../Utils/PCH.h"
#include "AssetBrowserPanel.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "AnimationControllerEditorPanel.h"
#include "ShaderEditorPanel.h"
#include "BlueprintData.h"
#include "ButtonSystem.h"
#include "Editor.h"
#include "IDEIntegration.h"
#include "ImGuiRenderer.h"
#include "InputTextSystem.h"
#include "PreferenceSettings.h"
#include "ProjectSettings.h"
#include "RuleTile.h"
#include "Tileset.h"
#include "Window.h"
#include "Resources/AssetManager.h"
#include "Utils/PopupManager.h"
#include "Utils/Logger.h"
#include "Renderer/GraphicsBackend.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Systems/HydrateResources.h"
#include "Systems/TransformSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/InteractionSystem.h"
#include "Systems/ScriptingSystem.h"
#include "../SceneManager.h"
#include "../Resources/Loaders/PrefabLoader.h"
#include "../Resources/RuntimeAsset/RuntimePrefab.h"
#include "../Data/PrefabData.h"
#include "../../Systems/HydrateResources.h"
#include "Input/Cursor.h"
static void Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1,
                     float min_size2)
{
    ImVec2 backup_pos = ImGui::GetCursorPos();
    if (split_vertically)
        ImGui::SetCursorPosY(backup_pos.y + *size1);
    else
        ImGui::SetCursorPosX(backup_pos.x + *size1);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.1f));
    ImGui::Button("##Splitter", ImVec2(!split_vertically ? thickness : -1.0f, split_vertically ? thickness : -1.0f));
    ImGui::PopStyleColor(3);
    ImGui::SetItemAllowOverlap();
    if (ImGui::IsItemActive())
    {
        float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
        if (mouse_delta < min_size1 - *size1)
            mouse_delta = min_size1 - *size1;
        if (mouse_delta > *size2 - min_size2)
            mouse_delta = *size2 - min_size2;
        *size1 += mouse_delta;
        *size2 -= mouse_delta;
    }
    ImGui::SetCursorPos(backup_pos);
}
void AssetBrowserPanel::Initialize(EditorContext* context)
{
    m_context = context;
    buildAssetTree();
    loadEditorIcons();
    PopupManager::GetInstance().Register("ConfirmDeleteAssets", [this]()
    {
        drawConfirmDeleteAssetsPopupContent();
    }, true, ImGuiWindowFlags_AlwaysAutoResize);
    m_context->currentAssetDirectory = m_context->assetTreeRoot.get();
    PopupManager::GetInstance().Register("AssetBrowserContextMenu", [this]() { this->drawAssetBrowserContextMenu(); },
                                         false);
    EventBus::GetInstance().Subscribe<DragDorpFileEvent>([this](const DragDorpFileEvent& e)
    {
        m_context->droppedFilesQueue.insert(m_context->droppedFilesQueue.end(), e.filePaths.begin(), e.filePaths.end());
    });
}
void AssetBrowserPanel::Update(float deltaTime)
{
    m_context->assetBrowserRefreshTimer += deltaTime;
    if (m_context->assetBrowserRefreshTimer >= m_context->assetBrowserRefreshInterval)
    {
        m_context->assetBrowserRefreshTimer = 0.0f;
        std::filesystem::path currentRelativePath;
        if (m_context->currentAssetDirectory)
        {
            currentRelativePath = m_context->currentAssetDirectory->path;
        }
        buildAssetTree();
        DirectoryNode* newNode = findNodeByPath(currentRelativePath);
        if (newNode)
        {
            m_context->currentAssetDirectory = newNode;
        }
        else
        {
            m_context->currentAssetDirectory = m_context->assetTreeRoot.get();
        }
    }
    if (m_context->assetToFocusInBrowser.Valid())
    {
        RequestFocusOnAsset(m_context->assetToFocusInBrowser);
        m_context->assetToFocusInBrowser = Guid();
    }
}
void AssetBrowserPanel::Draw()
{
    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    drawToolbar();
    static float leftPaneWidth = 200.0f;
    float rightPaneWidth = ImGui::GetContentRegionAvail().x - leftPaneWidth - 4.0f;
    Splitter(false, 4.0f, &leftPaneWidth, &rightPaneWidth, 100.0f, 200.0f);
    ImGui::BeginChild("DirectoryTree", ImVec2(leftPaneWidth, 0), true);
    drawDirectoryTree();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ContentView", ImVec2(rightPaneWidth, 0), true);
    drawContentView();
    ImGui::EndChild();
    ImGui::End();
}
void AssetBrowserPanel::Shutdown()
{
    m_context->assetTreeRoot.reset();
}
void AssetBrowserPanel::drawToolbar()
{
    bool isAtRoot = (!m_context->currentAssetDirectory || m_context->currentAssetDirectory->path.empty());
    if (!isAtRoot)
    {
        if (ImGui::Button("<- 返回"))
        {
            std::filesystem::path parentPath = m_context->currentAssetDirectory->path.parent_path();
            DirectoryNode* parentNode = findNodeByPath(parentPath);
            if (parentNode)
            {
                m_context->currentAssetDirectory = parentNode;
                m_pathToExpand = parentPath;
            }
        }
        ImGui::SameLine();
    }
    if (ImGui::Button(m_context->assetBrowserViewMode == AssetBrowserViewMode::List ? "[列表视图]" : "列表视图"))
        m_context->assetBrowserViewMode = AssetBrowserViewMode::List;
    ImGui::SameLine();
    if (ImGui::Button(m_context->assetBrowserViewMode == AssetBrowserViewMode::Grid ? "[网格视图]" : "网格视图"))
        m_context->assetBrowserViewMode = AssetBrowserViewMode::Grid;
    ImGui::SameLine();
    if (ImGui::Button(m_context->assetBrowserSortAscending ? "排序 A-Z" : "排序 Z-A"))
        m_context->assetBrowserSortAscending = !m_context->assetBrowserSortAscending;
    ImGui::SameLine();
    if (ImGui::Button("创建"))
        PopupManager::GetInstance().Open("AssetBrowserContextMenu");
    ImGui::Separator();
}
void AssetBrowserPanel::drawDirectoryTree()
{
    if (m_context->assetTreeRoot)
    {
        if (!m_pathToExpand.empty())
        {
            ensurePathLoaded(m_pathToExpand);
        }
        drawDirectoryNode(*m_context->assetTreeRoot);
    }
    if (!m_pathToExpand.empty())
    {
        m_pathToExpand.clear();
    }
}
void AssetBrowserPanel::drawContentView()
{
    if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
    {
        m_gridCellSize += ImGui::GetIO().MouseWheel * 5.0f;
        m_gridCellSize = std::clamp(m_gridCellSize, 64.0f, 256.0f);
    }
    drawAssetContentView();
    processFileDrop();
    handleContentAreaDragDrop();
}
void AssetBrowserPanel::ProcessDoubleClick(const Item& item)
{
    switch (item.type)
    {
    case AssetType::Scene:
        {
            sk_sp<RuntimeScene> newScene = SceneManager::GetInstance().LoadScene(item.guid);
            if (newScene)
            {
                m_context->activeScene = newScene;
                m_context->activeScene->AddEssentialSystem<Systems::HydrateResources>();
                m_context->activeScene->AddEssentialSystem<Systems::TransformSystem>();
                m_context->activeScene->Activate(*m_context->engineContext);
                m_context->selectionType = SelectionType::NA;
                m_context->selectionList = std::vector<Guid>();
                triggerHierarchyUpdate();
                LogInfo("场景加载成功: {}", item.name);
            }
            else
            {
                LogError("场景加载失败: {}", item.name);
            }
        }
        break;
    case AssetType::Prefab:
        {
            if (m_context->editingMode == EditingMode::Prefab)
            {
                LogWarn("已经在Prefab编辑模式中，请先退出当前模式");
                break;
            }
            auto loader = PrefabLoader();
            sk_sp<RuntimePrefab> prefab = loader.LoadAsset(item.guid);
            if (!prefab)
            {
                LogError("加载用于编辑的Prefab失败，GUID: {}", item.guid.ToString());
                break;
            }
            m_context->editingMode = EditingMode::Prefab;
            m_context->editingPrefabGuid = item.guid;
            m_context->sceneBeforePrefabEdit = m_context->activeScene;
            m_context->activeScene = sk_make_sp<RuntimeScene>();
            m_context->activeScene->SetName("Prefab编辑模式");
            m_context->activeScene->Instantiate(*prefab, nullptr);
            m_context->activeScene->AddEssentialSystem<Systems::HydrateResources>();
            m_context->activeScene->AddEssentialSystem<Systems::TransformSystem>();
            m_context->activeScene->Activate(*m_context->engineContext);
            SceneManager::GetInstance().SetCurrentScene(m_context->activeScene);
            m_context->selectionList = std::vector<Guid>();
            triggerHierarchyUpdate();
            LogInfo("进入Prefab编辑模式: {}", item.name);
        }
        break;
    case AssetType::CSharpScript:
        {
            OpenScriptInIDE(item.guid);
        }
        break;
    case AssetType::AnimationClip:
        {
            m_context->currentEditingAnimationClipGuid = item.guid;
            auto animEditorPanel = m_context->editor->GetPanelByName("动画编辑器");
            if (animEditorPanel)
            {
                animEditorPanel->SetVisible(true);
                animEditorPanel->Focus();
            }
            else
            {
                LogError("未找到动画编辑器面板");
            }
            LogInfo("双击打开动画切片: {}", item.name);
        }
    case AssetType::Blueprint:
        {
            m_context->currentEditingBlueprintGuid = item.guid;
            auto blueprintEditorPanel = m_context->editor->GetPanelByName("蓝图编辑器");
            if (blueprintEditorPanel)
            {
                blueprintEditorPanel->SetVisible(true);
                blueprintEditorPanel->Focus();
            }
            else
            {
                LogError("未找到蓝图编辑器面板");
            }
            LogInfo("双击打开蓝图: {}", item.name);
        }
        break;
    case AssetType::Shader:
        {
            auto shaderEditorPanel = dynamic_cast<ShaderEditorPanel*>(m_context->editor->GetPanelByName("着色器编辑器"));
            if (shaderEditorPanel)
            {
                AssetHandle shaderHandle(item.guid, AssetType::Shader);
                shaderEditorPanel->OpenShader(shaderHandle);
                shaderEditorPanel->SetVisible(true);
                shaderEditorPanel->Focus();
            }
            else
            {
                LogError("未找到着色器编辑器面板");
            }
            LogInfo("双击打开着色器: {}", item.name);
        }
        break;
    case AssetType::AnimationController:
        {
            m_context->currentEditingAnimationControllerGuid = item.guid;
            auto animControllerEditorPanel = m_context->editor->GetPanelByName("动画控制器编辑器");
            if (animControllerEditorPanel)
            {
                animControllerEditorPanel->SetVisible(true);
                animControllerEditorPanel->Focus();
            }
            else
            {
                LogError("未找到动画控制器编辑器面板");
            }
            LogInfo("双击打开动画控制器: {}", item.name);
        }
        break;
    case AssetType::Tileset:
        {
            m_context->currentEditingTilesetGuid = item.guid;
            LogInfo("请求打开 Tileset 编辑器: {}", item.name);
        }
        break;
    case AssetType::RuleTile:
        {
            m_context->currentEditingRuleTileGuid = item.guid;
            auto panel = m_context->editor->GetPanelByName("规则瓦片编辑器");
            if (panel)
            {
                panel->SetVisible(true);
                panel->Focus();
            }
            else
            {
                LogError("未找到规则瓦片编辑器面板");
            }
            LogInfo("请求打开 RuleTile 编辑器: {}", item.name);
        }
        break;
    default:
        LogInfo("双击打开资产: {}", item.name);
        break;
    }
}
void AssetBrowserPanel::drawAssetContentView()
{
    if (!m_context->currentAssetDirectory) return;
    if (!m_context->currentAssetDirectory->scanned)
    {
        scanDirectoryNode(m_context->currentAssetDirectory);
    }
    static AssetHandle s_draggedAssetHandle;
    static std::vector<AssetHandle> s_draggedAssetHandlesMulti;
    bool contentItemClicked = false;
    std::vector<Item> items;
    for (const auto& [name, subNode] : m_context->currentAssetDirectory->subdirectories)
    {
        items.push_back({name, subNode->path, Guid(), AssetType::Unknown, true});
    }
    for (const auto& assetMeta : m_context->currentAssetDirectory->assets)
    {
        items.push_back({
            assetMeta.assetPath.filename().string(), assetMeta.assetPath, assetMeta.guid, assetMeta.type, false
        });
    }
    std::ranges::sort(items, [&](const Item& a, const Item& b)
    {
        if (a.isDirectory != b.isDirectory)
            return a.isDirectory;
        return m_context->assetBrowserSortAscending ? (a.name < b.name) : (a.name > b.name);
    });
    auto handleDragSource = [&](const Item& item)
    {
        if (ImGui::BeginDragDropSource())
        {
            bool isDraggingMulti = m_context->selectedAssets.size() > 1 &&
                std::ranges::find(m_context->selectedAssets, item.path) != m_context->selectedAssets.end();
            if (isDraggingMulti)
            {
                s_draggedAssetHandlesMulti.clear();
                for (const auto& path : m_context->selectedAssets)
                {
                    if (std::filesystem::is_directory(path))
                    {
                        s_draggedAssetHandlesMulti.emplace_back(Guid(), AssetType::Unknown);
                    }
                    else
                    {
                        const auto* meta = AssetManager::GetInstance().GetMetadata(path);
                        if (meta) s_draggedAssetHandlesMulti.emplace_back(meta->guid, meta->type);
                    }
                }
                ImGui::SetDragDropPayload("DRAG_DROP_ASSET_HANDLES_MULTI", s_draggedAssetHandlesMulti.data(),
                                          s_draggedAssetHandlesMulti.size() * sizeof(AssetHandle));
                ImGui::Text("正在拖拽 %zu 个项目", m_context->selectedAssets.size());
            }
            else
            {
                if (item.isDirectory)
                {
                    s_draggedAssetHandle = AssetHandle(Guid(), AssetType::Unknown);
                    ImGui::SetDragDropPayload("DRAG_DROP_ASSET_HANDLE", &s_draggedAssetHandle, sizeof(AssetHandle));
                }
                else
                {
                    s_draggedAssetHandle = AssetHandle(item.guid, item.type);
                    ImGui::SetDragDropPayload("DRAG_DROP_ASSET_HANDLE", &s_draggedAssetHandle, sizeof(AssetHandle));
                }
                ImGui::Text("正在拖拽 %s", item.name.c_str());
            }
            ImGui::EndDragDropSource();
        }
    };
    auto handleDropTarget = [&](const Item& item)
    {
        if (item.isDirectory && ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
            {
                auto p = static_cast<const AssetHandle*>(payload->Data);
                m_context->selectedAssets = {AssetManager::GetInstance().GetMetadata(p->assetGuid)->assetPath};
                moveSelectedItems(item.path);
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLES_MULTI"))
            {
                auto p = static_cast<const AssetHandle*>(payload->Data);
                size_t count = payload->DataSize / sizeof(AssetHandle);
                m_context->selectedAssets.clear();
                for (size_t i = 0; i < count; ++i)
                {
                    const auto& handle = p[i];
                    if (handle.assetGuid.Valid())
                    {
                        const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
                        if (meta) m_context->selectedAssets.push_back(meta->assetPath);
                    }
                    else
                    {
                        for (const auto& [name, subNode] : m_context->assetTreeRoot->subdirectories)
                        {
                            if (subNode->path == item.path)
                            {
                                m_context->selectedAssets.push_back(subNode->path);
                                break;
                            }
                        }
                    }
                }
                moveSelectedItems(item.path);
            }
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            drawList->AddRect(min, max, IM_COL32(255, 255, 0, 255), 2.0f);
            ImGui::EndDragDropTarget();
        }
    };
    if (m_context->assetBrowserViewMode == AssetBrowserViewMode::List)
    {
        const float iconSize = 20.0f;
        const float rowHeight = std::max(ImGui::GetTextLineHeightWithSpacing(), iconSize) + ImGui::GetStyle().
            CellPadding.y * 2;
        ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg;
        if (ImGui::BeginTable("AssetContentList", 1, tableFlags))
        {
            for (int i = 0; i < (int)items.size(); ++i)
            {
                const auto& item = items[i];
                ImGui::TableNextRow(0, rowHeight);
                ImGui::TableNextColumn();
                ImGui::PushID(item.path.generic_string().c_str());
                bool isSelected = std::ranges::find(m_context->selectedAssets, item.path) != m_context->
                    selectedAssets.end();
                if (ImGui::Selectable("##row", isSelected,
                                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
                                      ImVec2(0, rowHeight)))
                {
                    contentItemClicked = true;
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        if (isSelected) std::erase(m_context->selectedAssets, item.path);
                        else m_context->selectedAssets.push_back(item.path);
                    }
                    else if (ImGui::GetIO().KeyShift)
                    {
                        auto anchorIt = std::ranges::find_if(items, [&](const Item& itm)
                        {
                            return itm.path == m_context->assetBrowserSelectionAnchor;
                        });
                        if (anchorIt != items.end())
                        {
                            m_context->selectedAssets.clear();
                            int anchorIndex = (int)std::distance(items.begin(), anchorIt);
                            for (int j = std::min(anchorIndex, i); j <= std::max(anchorIndex, i); ++j)
                                m_context->selectedAssets.push_back(items[j].path);
                        }
                    }
                    else
                    {
                        m_context->selectedAssets.clear();
                        m_context->selectedAssets.push_back(item.path);
                    }
                    m_context->assetBrowserSelectionAnchor = item.path;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    contentItemClicked = true;
                    if (!isSelected)
                    {
                        m_context->selectedAssets.clear();
                        m_context->selectedAssets.push_back(item.path);
                        m_context->assetBrowserSelectionAnchor = item.path;
                    }
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (item.isDirectory)
                    {
                        m_context->currentAssetDirectory = m_context->currentAssetDirectory->subdirectories.at(
                            item.name).get();
                        m_pathToExpand = item.path;
                        ImGui::PopID();
                        break;
                    }
                    else { ProcessDoubleClick(item); }
                }
                handleDragSource(item);
                handleDropTarget(item);
                drawAssetItemContextMenu(item.path);
                ImGui::SameLine();
                auto iconTexturePtr = item.isDirectory ? m_icons.directory : getIconForAssetType(item.type);
                wgpu::Texture iconTexture = iconTexturePtr ? iconTexturePtr->GetTexture() : wgpu::Texture();
                ImTextureID iconId = iconTexture
                                         ? m_context->imguiRenderer->GetOrCreateTextureIdFor(iconTexture)
                                         : (ImTextureID)-1;
                if (iconId != (ImTextureID)-1) ImGui::Image(iconId, ImVec2(iconSize, iconSize));
                else ImGui::Dummy(ImVec2(iconSize, iconSize));
                ImGui::SameLine();
                if (m_context->itemToRename == item.path)
                {
                    contentItemClicked = true;
                    ImGui::SetKeyboardFocusHere();
                    if (ImGui::InputText("##Rename", m_context->renameBuffer, sizeof(m_context->renameBuffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        renameItem(item.path, m_context->renameBuffer);
                        m_context->itemToRename.clear();
                    }
                    if (ImGui::IsItemDeactivated() && !ImGui::IsKeyPressed(ImGuiKey_Enter))
                    {
                        m_context->itemToRename.clear();
                    }
                }
                else { ImGui::TextUnformatted(item.name.c_str()); }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    else
    {
        const float cellSize = m_gridCellSize;
        const float iconPadding = 12.0f;
        const float iconSize = cellSize - iconPadding * 2.0f;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));
        if (ImGui::BeginTable("AssetContentGrid", columnCount))
        {
            for (int i = 0; i < (int)items.size(); ++i)
            {
                const auto& item = items[i];
                ImGui::TableNextColumn();
                ImGui::PushID(item.path.generic_string().c_str());
                bool isSelected = std::ranges::find(m_context->selectedAssets, item.path) != m_context->
                    selectedAssets.end();
                ImGui::BeginGroup();
                ImVec4 color = isSelected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0, 0, 0, 0);
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                auto iconTexturePtr = item.isDirectory ? m_icons.directory : getIconForAssetType(item.type);
                wgpu::Texture iconTexture = iconTexturePtr ? iconTexturePtr->GetTexture() : wgpu::Texture();
                ImTextureID iconId = iconTexture
                                         ? m_context->imguiRenderer->GetOrCreateTextureIdFor(iconTexture)
                                         : (ImTextureID)-1;
                if (ImGui::ImageButton(item.path.generic_string().c_str(), iconId, ImVec2(iconSize, iconSize)))
                {
                    contentItemClicked = true;
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        if (isSelected) std::erase(m_context->selectedAssets, item.path);
                        else m_context->selectedAssets.push_back(item.path);
                    }
                    else if (ImGui::GetIO().KeyShift)
                    {
                        auto anchorIt = std::ranges::find_if(items, [&](const Item& itm)
                        {
                            return itm.path == m_context->assetBrowserSelectionAnchor;
                        });
                        if (anchorIt != items.end())
                        {
                            m_context->selectedAssets.clear();
                            int anchorIndex = (int)std::distance(items.begin(), anchorIt);
                            for (int j = std::min(anchorIndex, i); j <= std::max(anchorIndex, i); ++j)
                                m_context->selectedAssets.push_back(items[j].path);
                        }
                    }
                    else
                    {
                        m_context->selectedAssets.clear();
                        m_context->selectedAssets.push_back(item.path);
                    }
                    m_context->assetBrowserSelectionAnchor = item.path;
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    contentItemClicked = true;
                    if (!isSelected)
                    {
                        m_context->selectedAssets.clear();
                        m_context->selectedAssets.push_back(item.path);
                        m_context->assetBrowserSelectionAnchor = item.path;
                    }
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (item.isDirectory)
                    {
                        m_context->currentAssetDirectory = m_context->currentAssetDirectory->subdirectories.at(
                            item.name).get();
                        m_pathToExpand = item.path;
                        ImGui::EndGroup();
                        ImGui::PopID();
                        break;
                    }
                    else { ProcessDoubleClick(item); }
                }
                handleDragSource(item);
                handleDropTarget(item);
                drawAssetItemContextMenu(item.path);
                if (m_context->itemToRename == item.path)
                {
                    contentItemClicked = true;
                    ImGui::SetKeyboardFocusHere();
                    ImGui::SetNextItemWidth(cellSize);
                    if (ImGui::InputText("##Rename", m_context->renameBuffer, sizeof(m_context->renameBuffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        renameItem(item.path, m_context->renameBuffer);
                        m_context->itemToRename.clear();
                    }
                    if (ImGui::IsItemDeactivated() && !ImGui::IsKeyPressed(ImGuiKey_Enter))
                    {
                        m_context->itemToRename.clear();
                    }
                }
                else { ImGui::TextWrapped("%s", item.name.c_str()); }
                ImGui::EndGroup();
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
            m_context->selectedAssets.clear();
            for (const auto& item : items)
            {
                m_context->selectedAssets.push_back(item.path);
            }
            if (!items.empty())
            {
                m_context->assetBrowserSelectionAnchor = items.back().path;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !m_context->selectedAssets.empty())
        {
            PopupManager::GetInstance().Open("ConfirmDeleteAssets");
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && m_context->selectedAssets.size() == 1)
        {
            const auto& itemToRenamePath = m_context->selectedAssets[0];
            m_context->itemToRename = itemToRenamePath;
            std::string nameToEdit = itemToRenamePath.stem().string();
#ifdef  _WIN32
            strncpy_s(m_context->renameBuffer, sizeof(m_context->renameBuffer),
                      nameToEdit.c_str(), sizeof(m_context->renameBuffer) - 1);
#else
            std::strncpy(m_context->renameBuffer, nameToEdit.c_str(), sizeof(m_context->renameBuffer) - 1);
            m_context->renameBuffer[sizeof(m_context->renameBuffer) - 1] = '\0';
#endif
        }
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
    {
        m_context->selectedAssets.clear();
        m_context->assetBrowserSelectionAnchor.clear();
        m_context->itemToRename.clear();
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)
        && m_context->selectedAssets.empty())
    {
        PopupManager::GetInstance().Open("AssetBrowserContextMenu");
    }
    size_t currentSelectionCount = m_context->selectedAssets.size();
    if (currentSelectionCount != m_lastSelectionCount)
    {
        if (m_context->editor)
        {
            IEditorPanel* inspectorPanel = m_context->editor->GetPanelByName("资产设置");
            if (inspectorPanel)
            {
                inspectorPanel->SetVisible(currentSelectionCount > 0);
            }
        }
    }
    m_lastSelectionCount = currentSelectionCount;
}
void AssetBrowserPanel::drawAssetBrowserContextMenu()
{
    auto closeMenu = []() { PopupManager::GetInstance().Close("AssetBrowserContextMenu"); };
    if (ImGui::BeginMenu("创建"))
    {
        if (ImGui::MenuItem("文件夹")) { createNewAsset(AssetType::Unknown); closeMenu(); }
        ImGui::Separator();
        if (ImGui::MenuItem("C# 脚本")) { createNewAsset(AssetType::CSharpScript); closeMenu(); }
        if (ImGui::MenuItem("场景")) { createNewAsset(AssetType::Scene); closeMenu(); }
        if (ImGui::MenuItem("材质")) { createNewAsset(AssetType::Material); closeMenu(); }
        if (ImGui::MenuItem("着色器")) { createNewAsset(AssetType::Shader); closeMenu(); }
        if (ImGui::MenuItem("物理材质")) { createNewAsset(AssetType::PhysicsMaterial); closeMenu(); }
        if (ImGui::MenuItem("蓝图")) { createNewAsset(AssetType::Blueprint); closeMenu(); }
        ImGui::Separator();
        if (ImGui::MenuItem("动画切片")) { createNewAsset(AssetType::AnimationClip); closeMenu(); }
        if (ImGui::MenuItem("动画控制器")) { createNewAsset(AssetType::AnimationController); closeMenu(); }
        ImGui::Separator();
        if (ImGui::MenuItem("瓦片")) { createNewAsset(AssetType::Tile); closeMenu(); }
        if (ImGui::MenuItem("规则瓦片")) { createNewAsset(AssetType::RuleTile); closeMenu(); }
        if (ImGui::MenuItem("瓦片集")) { createNewAsset(AssetType::Tileset); closeMenu(); }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("粘贴", nullptr, false, !m_context->assetClipboard.empty()))
    {
        pasteCopiedItems();
        closeMenu();
    }
    if (ImGui::MenuItem("打开资源文件夹"))
    {
        std::filesystem::path fullPath = ProjectSettings::GetInstance().GetAssetsDirectory() / m_context->
            currentAssetDirectory->path;
        Utils::OpenFileExplorerAt(fullPath);
        closeMenu();
    }
}
void AssetBrowserPanel::handleContentAreaDragDrop()
{
    bool shouldShowOverlay = false;
    if (const ImGuiPayload* activePayload = ImGui::GetDragDropPayload())
    {
        if (std::strcmp(activePayload->DataType, "DRAG_DROP_GAMEOBJECT_GUIDS") == 0 ||
            std::strcmp(activePayload->DataType, "DRAG_DROP_GAMEOBJECT_GUID") == 0)
        {
            shouldShowOverlay = true;
        }
    }
    if (!shouldShowOverlay)
        return;
    ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 dropAreaMin = ImVec2(windowPos.x + contentRegionMin.x, windowPos.y + contentRegionMin.y);
    ImVec2 dropAreaMax = ImVec2(windowPos.x + contentRegionMax.x, windowPos.y + contentRegionMax.y);
    ImVec2 dropAreaSize = ImVec2(dropAreaMax.x - dropAreaMin.x, dropAreaMax.y - dropAreaMin.y);
    ImGui::SetCursorScreenPos(dropAreaMin);
    ImGui::InvisibleButton("##ContentAreaDropTarget", dropAreaSize);
    bool isDragHovering = false;
    if (ImGui::BeginDragDropTarget())
    {
        isDragHovering = true;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS"))
        {
            size_t guidCount = payload->DataSize / sizeof(Guid);
            const Guid* guidArray = static_cast<const Guid*>(payload->Data);
            std::vector<Guid> goGuids;
            goGuids.reserve(guidCount);
            for (size_t i = 0; i < guidCount; ++i)
                goGuids.push_back(guidArray[i]);
            LogInfo("接收到 {} 个GameObject拖拽，开始创建预制体", goGuids.size());
            for (const auto& goGuid : goGuids)
            {
                LogInfo("为GameObject GUID: {} 创建预制体", goGuid.ToString());
                createPrefabFromGameObject(goGuid);
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUID"))
        {
            Guid goGuid = *static_cast<const Guid*>(payload->Data);
            LogInfo("接收到单个GameObject拖拽，GUID: {}", goGuid.ToString());
            createPrefabFromGameObject(goGuid);
        }
        ImGui::EndDragDropTarget();
    }
    if (isDragHovering)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 highlightColor = IM_COL32(255, 255, 0, 100);
        ImU32 borderColor = IM_COL32(255, 255, 0, 255);
        drawList->AddRectFilled(dropAreaMin, dropAreaMax, highlightColor);
        drawList->AddRect(dropAreaMin, dropAreaMax, borderColor, 0.0f, 0, 2.0f);
        ImVec2 textSize = ImGui::CalcTextSize("拖拽到此处创建预制体");
        ImVec2 textPos = ImVec2(
            dropAreaMin.x + (dropAreaSize.x - textSize.x) * 0.5f,
            dropAreaMin.y + (dropAreaSize.y - textSize.y) * 0.5f
        );
        ImVec2 textBgMin = ImVec2(textPos.x - 10, textPos.y - 5);
        ImVec2 textBgMax = ImVec2(textPos.x + textSize.x + 10, textPos.y + textSize.y + 5);
        drawList->AddRectFilled(textBgMin, textBgMax, IM_COL32(0, 0, 0, 150), 4.0f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), "拖拽到此处创建预制体");
    }
}
void AssetBrowserPanel::createPrefabFromGameObject(const Guid& goGuid)
{
    if (!m_context->activeScene)
    {
        LogError("无法创建Prefab：当前没有活动场景");
        return;
    }
    RuntimeGameObject sourceGo = m_context->activeScene->FindGameObjectByGuid(goGuid);
    if (!sourceGo.IsValid())
    {
        LogError("找不到用于创建Prefab的GameObject，GUID: {}", goGuid.ToString());
        return;
    }
    Data::PrefabNode rootNode = sourceGo.SerializeToPrefabData();
    Data::PrefabData prefabData;
    prefabData.root = rootNode;
    auto& assetManager = AssetManager::GetInstance();
    std::filesystem::path parentDir = assetManager.GetAssetsRootPath() / m_context->currentAssetDirectory->path;
    std::string baseName = sourceGo.GetName();
    std::string extension = ".prefab";
    std::filesystem::path finalPath = parentDir / (baseName + extension);
    int counter = 1;
    while (std::filesystem::exists(finalPath))
    {
        finalPath = parentDir / (baseName + " " + std::to_string(counter++) + extension);
    }
    try
    {
        YAML::Emitter emitter;
        emitter.SetIndent(2);
        emitter << YAML::convert<Data::PrefabData>::encode(prefabData);
        std::ofstream fout(finalPath);
        fout << emitter.c_str();
        fout.close();
        LogInfo("成功从GameObject '{}' 创建预制体: {}", sourceGo.GetName(), finalPath.filename().string());
        m_context->assetBrowserRefreshTimer = 0.0f;
    }
    catch (const std::exception& e)
    {
        LogError("创建预制体失败: {}", e.what());
    }
}
void AssetBrowserPanel::triggerHierarchyUpdate()
{
    if (!m_context->selectionList.empty())
    {
        m_context->objectToFocusInHierarchy = m_context->selectionList[0];
    }
    else
    {
        m_context->objectToFocusInHierarchy = Guid::NewGuid();
    }
}
void AssetBrowserPanel::drawDirectoryNode(DirectoryNode& node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (&node == m_context->currentAssetDirectory)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (node.scanned && node.subdirectories.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (!m_pathToExpand.empty())
    {
        std::string targetPathStr = m_pathToExpand.generic_string();
        std::string nodePathStr = node.path.generic_string();
        if (targetPathStr == nodePathStr || targetPathStr.starts_with(nodePathStr + "/"))
        {
            ImGui::SetNextItemOpen(true);
        }
    }
    ImGui::PushID(node.path.generic_string().c_str());
    auto iconTexturePtr = m_icons.directory;
    wgpu::Texture iconTexture = iconTexturePtr ? iconTexturePtr->GetTexture() : wgpu::Texture();
    if (iconTexture)
    {
        ImTextureID iconId = m_context->imguiRenderer->GetOrCreateTextureIdFor(iconTexture);
        ImGui::Image(iconId, ImVec2(16, 16));
    }
    else
    {
        ImGui::Dummy(ImVec2(16, 16));
    }
    ImGui::SameLine();
    bool nodeOpen = ImGui::TreeNodeEx(node.name.c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        m_context->currentAssetDirectory = &node;
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_ITEM_MOVE"))
        {
            moveSelectedItems(node.path);
            ImGui::EndDragDropTarget();
            ImGui::PopID();
            if (nodeOpen && !(flags & ImGuiTreeNodeFlags_Leaf)) ImGui::TreePop();
            return;
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        drawList->AddRect(min, max, IM_COL32(255, 255, 0, 255), 2.0f);
        ImGui::EndDragDropTarget();
    }
    if (nodeOpen && !(flags & ImGuiTreeNodeFlags_Leaf))
    {
        if (!node.scanned)
        {
            scanDirectoryNode(&node);
        }
        std::vector<const DirectoryNode*> sortedSubdirs;
        for (const auto& pair : node.subdirectories)
        {
            sortedSubdirs.push_back(pair.second.get());
        }
        std::ranges::sort(sortedSubdirs, [](const DirectoryNode* a, const DirectoryNode* b)
        {
            return a->name < b->name;
        });
        for (const auto* subNode : sortedSubdirs)
        {
            drawDirectoryNode(*const_cast<DirectoryNode*>(subNode));
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
void AssetBrowserPanel::drawAssetItemContextMenu(const std::filesystem::path& itemPath)
{
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("删除"))
        {
            PopupManager::GetInstance().Open("ConfirmDeleteAssets");
        }
        if (ImGui::MenuItem("重命名"))
        {
            m_context->itemToRename = itemPath;
            std::string nameToEdit = m_context->itemToRename.stem().string();
#ifdef  _WIN32
            strncpy_s(m_context->renameBuffer, sizeof(m_context->renameBuffer),
                      nameToEdit.c_str(), sizeof(m_context->renameBuffer) - 1);
#else
            std::strncpy(m_context->renameBuffer, nameToEdit.c_str(), sizeof(m_context->renameBuffer) - 1);
            m_context->renameBuffer[sizeof(m_context->renameBuffer) - 1] = '\0';
#endif
        }
        if (ImGui::MenuItem("复制"))
        {
            copySelectedItems();
        }
        ImGui::EndPopup();
    }
}
void AssetBrowserPanel::drawConfirmDeleteAssetsPopupContent()
{
    if (m_context->selectedAssets.empty())
    {
        ImGui::Text("没有选中的项目。");
        if (ImGui::Button("关闭")) PopupManager::GetInstance().Close("ConfirmDeleteAssets");
        return;
    }
    ImGui::Text("您确定要删除 %d 个选中的项目吗？", static_cast<int>(m_context->selectedAssets.size()));
    ImGui::Text("此操作无法撤销。");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (120 * 2 + ImGui::GetStyle().ItemSpacing.x)) * 0.5f);
    if (ImGui::Button("删除", ImVec2(120, 0)))
    {
        deleteSelectedItems();
        PopupManager::GetInstance().Close("ConfirmDeleteAssets");
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("取消", ImVec2(120, 0)))
        PopupManager::GetInstance().Close("ConfirmDeleteAssets");
}
void AssetBrowserPanel::buildAssetTree()
{
    auto& assetManager = AssetManager::GetInstance();
    const auto assetsRoot = assetManager.GetAssetsRootPath();
    if (assetsRoot.empty() || !std::filesystem::exists(assetsRoot))
    {
        m_context->assetTreeRoot.reset();
        return;
    }
    m_context->assetTreeRoot = std::make_unique<DirectoryNode>();
    m_context->assetTreeRoot->path = "";
    m_context->assetTreeRoot->name = "Assets";
    m_context->assetTreeRoot->scanned = false;
    scanDirectoryNode(m_context->assetTreeRoot.get());
}
void AssetBrowserPanel::scanDirectoryNode(DirectoryNode* parentNode)
{
    if (!parentNode) return;
    if (parentNode->scanned)
    {
        return;
    }
    parentNode->subdirectories.clear();
    parentNode->assets.clear();
    std::filesystem::path absoluteParentPath = AssetManager::GetInstance().GetAssetsRootPath() / parentNode->path;
    std::error_code ec;
    std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::directory_iterator it(absoluteParentPath, opts, ec);
    if (ec)
    {
        LogWarn("遍历目录失败: {}，错误: {}", absoluteParentPath.string(), ec.message());
        return;
    }
    for (; it != std::filesystem::directory_iterator(); it.increment(ec))
    {
        if (ec)
        {
            LogWarn("遍历目录项失败: {}，错误: {}", absoluteParentPath.string(), ec.message());
            ec.clear();
            continue;
        }
        const auto& entry = *it;
        if (entry.is_directory(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }
            std::string dirName = entry.path().filename().string();
            auto newNode = std::make_unique<DirectoryNode>();
            newNode->path = parentNode->path / dirName;
            newNode->name = dirName;
            newNode->scanned = false;
            parentNode->subdirectories[dirName] = std::move(newNode);
        }
        else
        {
            if (ec)
            {
                ec.clear();
                continue;
            }
            std::filesystem::path relPath = std::filesystem::relative(entry.path(),
                                                                      AssetManager::GetInstance().GetAssetsRootPath());
            if (relPath.extension() == ".meta")
            {
                continue;
            }
            if (const AssetMetadata* meta = AssetManager::GetInstance().GetMetadata(relPath))
            {
                parentNode->assets.push_back(*meta);
            }
        }
    }
    parentNode->scanned = true;
}
DirectoryNode* AssetBrowserPanel::findNodeByPath(const std::filesystem::path& path)
{
    if (!m_context->assetTreeRoot) return nullptr;
    if (path.empty() || path.string() == ".")
    {
        return m_context->assetTreeRoot.get();
    }
    DirectoryNode* currentNode = m_context->assetTreeRoot.get();
    for (const auto& part : path)
    {
        std::string partStr = part.string();
        if (partStr == ".") continue;
        if (!currentNode->scanned)
        {
            scanDirectoryNode(currentNode);
        }
        auto it = currentNode->subdirectories.find(partStr);
        if (it == currentNode->subdirectories.end())
        {
            return nullptr;
        }
        currentNode = it->second.get();
    }
    return currentNode;
}
void AssetBrowserPanel::ensurePathLoaded(const std::filesystem::path& path)
{
    if (!m_context->assetTreeRoot) return;
    DirectoryNode* current = m_context->assetTreeRoot.get();
    for (const auto& part : path)
    {
        std::string partStr = part.string();
        if (partStr == ".") continue;
        if (!current->scanned)
        {
            scanDirectoryNode(current);
        }
        auto it = current->subdirectories.find(partStr);
        if (it == current->subdirectories.end())
        {
            return;
        }
        current = it->second.get();
    }
}
void AssetBrowserPanel::createNewAsset(AssetType type)
{
    if (!m_context->currentAssetDirectory)
    {
        LogError("无法创建资产：没有选中的目录。");
        return;
    }
    std::filesystem::path currentDir = AssetManager::GetInstance().GetAssetsRootPath() / m_context->
        currentAssetDirectory->path;
    std::string defaultContent, baseName, extension;
    bool isDirectory = false;
    switch (type)
    {
    case AssetType::Unknown:
        baseName = "新建文件夹";
        isDirectory = true;
        break;
    case AssetType::Blueprint:
        {
            Blueprint bp;
            baseName = "新建蓝图";
            extension = ".blueprint";
            bp.Name = baseName;
            defaultContent = YAML::Dump(YAML::convert<Blueprint>::encode(bp));
        }
        break;
    case AssetType::CSharpScript:
        baseName = "NewScript";
        extension = ".cs";
        defaultContent =
            "using Luma.SDK;\nusing Luma.SDK.Components;\n\nnamespace GameScripts\n{\n    public class NewScript : Script\n    {\n        public override void OnCreate() {}\n        public override void OnUpdate(float deltaTime) {}\n        public override void OnDestroy() {}\n    }\n}\n";
        break;
    case AssetType::Scene:
        baseName = "新建场景";
        extension = ".scene";
        defaultContent = "name: 新建场景\nentities: []";
        break;
    case AssetType::Material:
        {
            baseName = "新建材质";
            extension = ".mat";
            Data::MaterialDefinition defaultMatDef;
            defaultContent = YAML::Dump(YAML::convert<Data::MaterialDefinition>::encode(defaultMatDef));
        }
        break;
    case AssetType::Shader:
        {
            baseName = "新建着色器";
            extension = ".shader";
            Data::ShaderData defaultData;
            defaultData.source = R"(
/// @file Common2D.wgsl
/// @brief 2D渲染通用着色器模板
/// @author Luma Engine
/// @version 1.0
/// @date 2025
import Std;
/// @brief 顶点着色器主函数
/// @details 处理顶点变换、UV变换和颜色传递，支持实例化渲染
/// @param input 顶点输入数据
/// @param instanceIdx 实例索引，用于访问实例数据数组
/// @return 处理后的顶点输出数据
@vertex
fn vs_main(input: VertexInput, @builtin(instance_index) instanceIdx: u32) -> VertexOutput {
    // 从实例数据数组中获取当前实例的数据
    let instance = instanceDatas[instanceIdx];
    // 将局部坐标按实例尺寸进行缩放
    let localPos = input.position * instance.size;
    // 将局部坐标变换到裁剪空间
    let clipPosition = TransformVertex(localPos, instance, engineData);
    // 对UV坐标进行变换，应用实例的UV矩形
    let transformedUV = TransformUV(input.uv, instance.uvRect);
    // 构建顶点输出结构
    var out: VertexOutput;
    out.clipPosition = clipPosition;    ///< 裁剪空间位置
    out.uv = transformedUV;             ///< 变换后的UV坐标
    out.color = instance.color;         ///< 实例颜色（包含透明度）
    return out;
}
/// @brief 片段着色器主函数
/// @details 采样纹理并与顶点颜色混合，输出最终像素颜色
/// @param in 顶点着色器传递过来的插值数据
/// @return 输出到颜色附件的RGBA颜色值
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    // 从主纹理采样颜色，使用主采样器
    let texColor = textureSample(mainTexture, mainSampler, in.uv);
    // 将纹理颜色与顶点颜色相乘（支持透明度混合）
    return texColor * in.color;
}
)";
            defaultData.language = Data::ShaderLanguage::WGSL;
            defaultData.type = Data::ShaderType::VertFrag;
            defaultContent = YAML::Dump(YAML::convert<Data::ShaderData>::encode(defaultData));
        }
        break;
    case AssetType::PhysicsMaterial:
        baseName = "新建物理材质";
        extension = ".physmat";
        defaultContent = "friction: 0.4\nrestitution: 0.0";
        break;
    case AssetType::AnimationClip:
        {
            AnimationClip defaultClip;
            baseName = "新建动画切片";
            extension = ".anim";
            defaultContent = YAML::Dump(YAML::convert<AnimationClip>::encode(defaultClip));
            break;
        }
    case AssetType::AnimationController:
        {
            baseName = "新建动画控制器";
            extension = ".animctrl";
            AnimationControllerData defaultController;
            defaultContent = YAML::Dump(YAML::convert<AnimationControllerData>::encode(defaultController));
            break;
        }
    case AssetType::Tileset:
        {
            baseName = "新建瓦片集";
            extension = ".tileset";
            TilesetData defaultTileset;
            defaultContent = YAML::Dump(YAML::convert<TilesetData>::encode(defaultTileset));
            break;
        }
    case AssetType::Tile:
        {
            baseName = "新建瓦片";
            extension = ".tile";
            TileAssetData defaultTile;
            defaultTile = SpriteTileData{};
            defaultContent = YAML::Dump(YAML::convert<TileAssetData>::encode(defaultTile));
            break;
        }
    case AssetType::RuleTile:
        {
            baseName = "新建规则瓦片";
            extension = ".ruletile";
            RuleTileAssetData defaultRuleTile;
            defaultContent = YAML::Dump(YAML::convert<RuleTileAssetData>::encode(defaultRuleTile));
            break;
        }
    default:
        return;
    }
    std::filesystem::path finalPath;
    int counter = 1;
    if (isDirectory)
    {
        finalPath = currentDir / baseName;
        while (std::filesystem::exists(finalPath))
            finalPath = currentDir / (baseName + "_" + std::to_string(counter++));
    }
    else
    {
        finalPath = currentDir / (baseName + extension);
        while (std::filesystem::exists(finalPath))
            finalPath = currentDir / (baseName + "_" + std::to_string(counter++) + extension);
    }
    if (isDirectory)
        std::filesystem::create_directory(finalPath);
    else
    {
        std::ofstream fout(finalPath);
        fout << defaultContent;
    }
    m_context->assetBrowserRefreshTimer = 0.0f;
}
void AssetBrowserPanel::deleteSelectedItems()
{
    if (m_context->selectedAssets.empty()) return;
    auto& assetManager = AssetManager::GetInstance();
    std::filesystem::path assetsRoot = assetManager.GetAssetsRootPath();
    for (const auto& relativePath : m_context->selectedAssets)
    {
        std::filesystem::path fullPath = assetsRoot / relativePath;
        try
        {
            if (std::filesystem::exists(fullPath))
            {
                std::filesystem::remove_all(fullPath);
                std::filesystem::path metaPath = fullPath.string() + ".meta";
                if (std::filesystem::exists(metaPath))
                    std::filesystem::remove(metaPath);
            }
        }
        catch (const std::exception& e)
        {
            LogError("删除失败 {}: {}", relativePath.string(), e.what());
        }
    }
    m_context->selectedAssets.clear();
    m_context->assetBrowserRefreshTimer = 0.0f;
}
void AssetBrowserPanel::copySelectedItems()
{
    m_context->assetClipboard.clear();
    if (m_context->selectedAssets.empty()) return;
    for (const auto& path : m_context->selectedAssets)
    {
        m_context->assetClipboard.push_back(path);
    }
}
void AssetBrowserPanel::pasteCopiedItems()
{
    if (m_context->assetClipboard.empty()) return;
    auto& assetManager = AssetManager::GetInstance();
    std::filesystem::path assetsRoot = assetManager.GetAssetsRootPath();
    std::filesystem::path destDir = assetsRoot / m_context->currentAssetDirectory->path;
    for (const auto& relativePath : m_context->assetClipboard)
    {
        const AssetMetadata* sourceMetadata = assetManager.GetMetadata(relativePath);
        if (!sourceMetadata)
        {
            LogWarn("源资产 '{}' 不存在或元数据无效，已跳过粘贴。", relativePath.string());
            continue;
        }
        try
        {
            std::filesystem::path sourcePath = assetsRoot / relativePath;
            std::filesystem::path destPath = destDir / sourcePath.filename();
            int counter = 1;
            std::string stem = destPath.stem().string();
            std::string extension = destPath.extension().string();
            while (std::filesystem::exists(destPath))
            {
                destPath = destDir / (stem + " (副本 " + std::to_string(counter++) + ")" + extension);
            }
            std::filesystem::copy(sourcePath, destPath, std::filesystem::copy_options::recursive);
            AssetMetadata newMetadata;
            newMetadata.guid = Guid::NewGuid();
            newMetadata.assetPath = std::filesystem::relative(destPath, assetsRoot);
            newMetadata.type = sourceMetadata->type;
            assetManager.ReImport(newMetadata);
            LogInfo("资产已粘贴为新副本: {}", newMetadata.assetPath.string());
        }
        catch (const std::exception& e)
        {
            LogError("粘贴资产 '{}' 失败: {}", relativePath.string(), e.what());
        }
    }
    m_context->assetBrowserRefreshTimer = 0.0f;
}
void AssetBrowserPanel::renameItem(const std::filesystem::path& oldRelativePath, const std::string& newName)
{
    if (newName.empty()) return;
    auto& assetManager = AssetManager::GetInstance();
    std::filesystem::path assetsRoot = assetManager.GetAssetsRootPath();
    std::filesystem::path oldFullPath = assetsRoot / oldRelativePath;
    std::filesystem::path newFullPath = oldFullPath.parent_path() / newName;
    if (!std::filesystem::is_directory(oldFullPath))
    {
        newFullPath.replace_extension(oldFullPath.extension());
    }
    if (std::filesystem::exists(newFullPath))
    {
        LogError("名为 '{}' 的项目已存在。", newName);
        return;
    }
    try
    {
        std::filesystem::rename(oldFullPath, newFullPath);
        std::filesystem::path oldMetaPath = oldFullPath.string() + ".meta";
        if (std::filesystem::exists(oldMetaPath))
        {
            if (std::filesystem::is_directory(newFullPath))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(newFullPath))
                {
                    if (entry.is_regular_file() && entry.path().extension() != ".meta")
                    {
                        std::filesystem::path metaPath = entry.path().string() + ".meta";
                        if (std::filesystem::exists(metaPath))
                        {
                            YAML::Node metaNode = YAML::LoadFile(metaPath.string());
                            metaNode["assetPath"] = std::filesystem::relative(entry.path(), assetsRoot).
                                generic_string();
                            std::ofstream fout(metaPath);
                            fout << metaNode;
                        }
                    }
                }
            }
            else
            {
                std::filesystem::path newMetaPath = newFullPath.string() + ".meta";
                std::filesystem::rename(oldMetaPath, newMetaPath);
                YAML::Node metaNode = YAML::LoadFile(newMetaPath.string());
                metaNode["assetPath"] = std::filesystem::relative(newFullPath, assetsRoot).generic_string();
                std::ofstream fout(newMetaPath);
                fout << metaNode;
            }
        }
        m_context->assetBrowserRefreshTimer = 0.0f;
    }
    catch (const std::exception& e)
    {
        LogError("重命名项目失败: {}", e.what());
    }
}
void AssetBrowserPanel::processFileDrop()
{
    if (!m_context->droppedFilesQueue.empty())
    {
        if (m_context->currentAssetDirectory)
        {
            std::filesystem::path destDir = AssetManager::GetInstance().GetAssetsRootPath() /
                m_context->currentAssetDirectory->path;
            for (const auto& sourcePathStr : m_context->droppedFilesQueue)
            {
                std::filesystem::path sourcePath(sourcePathStr);
                std::error_code ecExists;
                if (!std::filesystem::exists(sourcePath, ecExists) || ecExists)
                {
                    LogWarn("拖入的路径不存在或不可访问，已忽略: {}", sourcePathStr);
                    continue;
                }
                std::filesystem::path destPath = destDir / sourcePath.filename();
                std::error_code ec;
                if (std::filesystem::exists(destPath, ec) && !ec)
                {
                    m_context->conflictSourcePath = sourcePath.string();
                    m_context->conflictDestPath = destPath.string();
                    PopupManager::GetInstance().Open("File Exists");
                    continue;
                }
                ec.clear();
                if (std::filesystem::is_directory(sourcePath, ec) && !ec)
                {
                    ec.clear();
                    std::filesystem::copy(
                        sourcePath,
                        destPath,
                        std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::skip_symlinks,
                        ec
                    );
                }
                else
                {
                    ec.clear();
                    std::filesystem::copy_file(
                        sourcePath,
                        destPath,
                        std::filesystem::copy_options::none,
                        ec
                    );
                }
                if (ec)
                {
                    LogError("导入资产失败: {} -> {}，错误: {}", sourcePath.string(), destPath.string(), ec.message());
                }
                else
                {
                    LogInfo("资产已导入: {} -> {}", sourcePath.filename().string(), destDir.string());
                    m_context->assetBrowserRefreshTimer = 0.0f;
                }
            }
        }
        m_context->droppedFilesQueue.clear();
    }
}
void AssetBrowserPanel::loadEditorIcons()
{
    auto backend = m_context->graphicsBackend;
    m_icons.directory = backend->LoadTextureFromFile("Icons/directory.png");
    m_icons.image = backend->LoadTextureFromFile("Icons/image.png");
    m_icons.script = backend->LoadTextureFromFile("Icons/script.png");
    m_icons.scene = backend->LoadTextureFromFile("Icons/scene.png");
    m_icons.audio = backend->LoadTextureFromFile("Icons/audio.png");
    m_icons.file = backend->LoadTextureFromFile("Icons/file.png");
    m_icons.prefab = backend->LoadTextureFromFile("Icons/prefab.png");
}
Nut::TextureAPtr AssetBrowserPanel::getIconForAssetType(AssetType type)
{
    switch (type)
    {
    case AssetType::Texture: return m_icons.image ? m_icons.image : m_icons.file;
    case AssetType::Scene: return m_icons.scene ? m_icons.scene : m_icons.file;
    case AssetType::CSharpScript: return m_icons.script ? m_icons.script : m_icons.file;
    case AssetType::Audio: return m_icons.audio ? m_icons.audio : m_icons.file;
    case AssetType::Prefab: return m_icons.prefab ? m_icons.prefab : m_icons.file;
    default: return m_icons.file;
    }
}
void AssetBrowserPanel::RequestFocusOnAsset(const Guid& guid)
{
    const auto* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (metadata)
    {
        std::filesystem::path directoryPath = metadata->assetPath.parent_path();
        DirectoryNode* targetNode = findNodeByPath(directoryPath);
        if (targetNode)
            m_context->currentAssetDirectory = targetNode;
    }
}
void AssetBrowserPanel::OpenScriptInIDE(const Guid& scriptAssetGuid)
{
    const AssetMetadata* meta = AssetManager::GetInstance().GetMetadata(scriptAssetGuid);
    if (!meta || meta->type != AssetType::CSharpScript)
    {
        LogError("Failed to open script: Invalid asset or not a C# script.");
        return;
    }
    IDE detectedIDE = PreferenceSettings::GetInstance().GetPreferredIDE();
    if (detectedIDE == IDE::Unknown)
    {
        detectedIDE = IDEIntegration::DetectInstalledIDE();
    }
    if (detectedIDE == IDE::Unknown)
    {
        LogError("No supported IDE (Rider, Visual Studio, VS Code) found on this system.");
        return;
    }
    std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
    std::filesystem::path solutionPath = projectRoot / "LumaScripting.sln";
    std::filesystem::path scriptPath = projectRoot / "Assets" / meta->assetPath;
    if (!IDEIntegration::Open(detectedIDE, solutionPath, scriptPath))
    {
        LogError("Failed to launch IDE for script: {}", scriptPath.string());
    }
}
void AssetBrowserPanel::moveSelectedItems(const std::filesystem::path& destinationRelativePath)
{
    if (m_context->selectedAssets.empty()) return;
    auto& assetManager = AssetManager::GetInstance();
    std::filesystem::path assetsRoot = assetManager.GetAssetsRootPath();
    std::filesystem::path destDirFullPath = assetsRoot / destinationRelativePath;
    for (const auto& sourceRelativePath : m_context->selectedAssets)
    {
        if (sourceRelativePath == destinationRelativePath ||
            destinationRelativePath.generic_string().starts_with(sourceRelativePath.generic_string() + "/"))
        {
            LogWarn("无法将文件夹 '{}' 移动到其自身或子目录中。", sourceRelativePath.string());
            continue;
        }
        std::filesystem::path sourceFullPath = assetsRoot / sourceRelativePath;
        std::filesystem::path newFullPath = destDirFullPath / sourceFullPath.filename();
        if (std::filesystem::exists(newFullPath))
        {
            LogError("移动失败：目标路径 '{}' 已存在同名文件或文件夹。", newFullPath.string());
            continue;
        }
        const AssetMetadata* originalMetadata = assetManager.GetMetadata(sourceRelativePath);
        if (!originalMetadata)
        {
            LogError("移动失败：找不到源资产 '{}' 的元数据。", sourceRelativePath.string());
            continue;
        }
        try
        {
            std::filesystem::rename(sourceFullPath, newFullPath);
            std::filesystem::path oldMetaPath = sourceFullPath.string() + ".meta";
            if (std::filesystem::exists(oldMetaPath))
            {
                std::filesystem::path newMetaPath = newFullPath.string() + ".meta";
                std::filesystem::rename(oldMetaPath, newMetaPath);
            }
            AssetMetadata updatedMetadata = *originalMetadata;
            updatedMetadata.assetPath = std::filesystem::relative(newFullPath, assetsRoot);
            assetManager.ReImport(updatedMetadata);
            LogInfo("已移动 '{}' -> '{}'", sourceRelativePath.string(), destinationRelativePath.string());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LogError("移动资产 '{}' 时发生文件系统错误: {}", sourceRelativePath.string(), e.what());
        }
    }
    m_context->selectedAssets.clear();
    m_context->assetBrowserRefreshTimer = 0.0f;
}

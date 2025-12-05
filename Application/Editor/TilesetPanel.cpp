#include "TilesetPanel.h"
#include "imgui.h"
#include "AssetManager.h"
#include "Path.h"
#include "Loaders/TileLoader.h"
#include "Loaders/RuleTileLoader.h"
#include "Utils/Logger.h"
#include <fstream>
#include <Resources/Loaders/RuleTileLoader.h>
#include <Resources/Loaders/TilesetLoader.h>
#include "Profiler.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "Loaders/TextureLoader.h"
void TilesetPanel::Initialize(EditorContext* context)
{
    m_context = context;
}
void TilesetPanel::Update(float deltaTime)
{
    PROFILE_FUNCTION();
    if (!m_isVisible) return;
    if (m_context->currentEditingTilesetGuid.Valid() && m_context->currentEditingTilesetGuid != m_currentTilesetGuid)
    {
        openTileset(m_context->currentEditingTilesetGuid);
    }
    else if (!m_context->currentEditingTilesetGuid.Valid() && m_currentTileset)
    {
        closeCurrentTileset();
    }
    if (m_context->activeTileBrush.assetGuid != m_lastActiveBrushHandle.assetGuid)
    {
        updateBrushPreview();
        m_lastActiveBrushHandle = m_context->activeTileBrush;
    }
}
void TilesetPanel::Draw()
{
    PROFILE_FUNCTION();
    if (!m_isVisible) return;
    std::string windowTitle = std::string(GetPanelName());
    if (m_currentTileset)
    {
        const auto* meta = AssetManager::GetInstance().GetMetadata(m_currentTilesetGuid);
        if (meta)
        {
            windowTitle += " - " + meta->assetPath.filename().string();
        }
    }
    if (ImGui::Begin(windowTitle.c_str(), &m_isVisible))
    {
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (!m_currentTileset)
        {
            ImVec2 center = ImGui::GetContentRegionAvail();
            center.x *= 0.5f;
            center.y *= 0.5f;
            ImGui::SetCursorPos(center);
            ImGui::Text("请从资源浏览器双击一个 Tileset 资产以开始编辑");
        }
        else
        {
            if (ImGui::Button("保存")) { saveCurrentTileset(); }
            ImGui::SameLine();
            if (ImGui::Button("关闭"))
            {
                m_context->currentEditingTilesetGuid = Guid();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::SliderFloat("缩放", &m_thumbnailSize, 32.0f, 128.0f, "%.0f");
            ImGui::Separator();
            if (ImGui::BeginChild("TilesetContent"))
            {
                drawTilesetContent();
                handleDropTarget();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}
void TilesetPanel::Shutdown()
{
    closeCurrentTileset();
}
void TilesetPanel::openTileset(Guid tilesetGuid)
{
    closeCurrentTileset();
    TilesetLoader loader;
    m_currentTileset = loader.LoadAsset(tilesetGuid);
    if (!m_currentTileset)
    {
        LogError("无法加载Tileset资产: {}", tilesetGuid.ToString());
        m_context->currentEditingTilesetGuid = Guid();
        return;
    }
    m_currentTilesetGuid = tilesetGuid;
    m_tileHandles = m_currentTileset->GetData().tiles;
    TileLoader tileLoader;
    RuleTileLoader ruleTileLoader;
    for (const auto& handle : m_tileHandles)
    {
        if (handle.assetType == AssetType::Tile && !m_hydratedTiles.contains(handle.assetGuid))
        {
            m_hydratedTiles[handle.assetGuid] = tileLoader.LoadAsset(handle.assetGuid);
        }
        else if (handle.assetType == AssetType::RuleTile && !m_hydratedRuleTiles.contains(handle.assetGuid))
        {
            m_hydratedRuleTiles[handle.assetGuid] = ruleTileLoader.LoadAsset(handle.assetGuid);
        }
    }
}
void TilesetPanel::closeCurrentTileset()
{
    m_currentTileset = nullptr;
    m_currentTilesetGuid = Guid();
    m_tileHandles.clear();
    m_hydratedTiles.clear();
    m_hydratedRuleTiles.clear();
}
void TilesetPanel::saveCurrentTileset()
{
    if (!m_currentTileset) return;
    const auto* meta = AssetManager::GetInstance().GetMetadata(m_currentTilesetGuid);
    if (!meta)
    {
        LogError("找不到Tileset元数据，保存失败");
        return;
    }
    TilesetData data;
    data.tiles = m_tileHandles;
    YAML::Node node = YAML::convert<TilesetData>::encode(data);
    std::ofstream fout(AssetManager::GetInstance().GetAssetsRootPath() / meta->assetPath);
    fout << node;
    fout.close();
    LogInfo("Tileset资产已保存: {}", meta->assetPath.string());
}
void TilesetPanel::drawTilesetContent()
{
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = std::max(1, static_cast<int>(panelWidth / (m_thumbnailSize + 20.0f)));
    if (ImGui::BeginTable("TilesetGrid", columnCount))
    {
        for (const auto& handle : m_tileHandles)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(handle.assetGuid.ToString().c_str());
            const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
            std::string name = meta ? meta->assetPath.stem().string() : "无效资产";
            bool isSelected = m_context->activeTileBrush.assetGuid == handle.assetGuid;
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            // TODO: 此处应绘制资产的真实缩略图
            if (ImGui::Button(name.c_str(), ImVec2(m_thumbnailSize, m_thumbnailSize)))
            {
                m_context->activeTileBrush = handle;
            }
            if (ImGui::BeginPopupContextItem("TileItemContext"))
            {
                if (ImGui::MenuItem("删除"))
                {
                    auto it = std::find_if(m_tileHandles.begin(), m_tileHandles.end(), [&](const AssetHandle& h)
                    { return h.assetGuid == handle.assetGuid; });
                    if (it != m_tileHandles.end())
                    {
                        if (m_context->activeTileBrush.assetGuid == handle.assetGuid) { m_context->activeTileBrush = {}; }
                        m_hydratedTiles.erase(handle.assetGuid);
                        m_hydratedRuleTiles.erase(handle.assetGuid);
                        m_tileHandles.erase(it);
                    }
                }
                ImGui::EndPopup();
            }
            if (isSelected)
            {
                ImGui::PopStyleColor();
            }
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("DRAG_DROP_ASSET_HANDLE", &handle, sizeof(AssetHandle));
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::TextWrapped("%s", name.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
void TilesetPanel::handleDropTarget()
{
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle droppedHandle = *static_cast<const AssetHandle*>(payload->Data);
            if (droppedHandle.assetType == AssetType::Texture || droppedHandle.assetType == AssetType::Prefab)
            {
                createTileAssetFromSource(droppedHandle);
            }
            else if (droppedHandle.assetType == AssetType::Tile || droppedHandle.assetType == AssetType::RuleTile)
            {
                bool exists = false;
                for (const auto& h : m_tileHandles)
                {
                    if (h.assetGuid == droppedHandle.assetGuid)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    m_tileHandles.push_back(droppedHandle);
                    LogInfo("已将资产 {} 添加到Tileset", droppedHandle.assetGuid.ToString());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}
void TilesetPanel::createTileAssetFromSource(const AssetHandle& sourceAssetHandle)
{
    auto& assetManager = AssetManager::GetInstance();
    const auto* sourceMeta = assetManager.GetMetadata(sourceAssetHandle.assetGuid);
    if (!sourceMeta)
    {
        LogError("找不到源资产的元数据");
        return;
    }
    std::filesystem::path sourcePath = sourceMeta->assetPath;
    std::filesystem::path tilesDir = sourcePath.parent_path() / "Tiles";
    std::filesystem::create_directories(assetManager.GetAssetsRootPath() / tilesDir);
    std::string newAssetName = sourcePath.stem().string() + ".tile";
    std::filesystem::path newAssetRelativePath = tilesDir / newAssetName;
    std::filesystem::path newAssetFullPath = assetManager.GetAssetsRootPath() / newAssetRelativePath;
    TileAssetData tileData;
    if (sourceAssetHandle.assetType == AssetType::Texture)
    {
        SpriteTileData spriteData;
        spriteData.textureHandle = sourceAssetHandle;
        tileData = spriteData;
    }
    else if (sourceAssetHandle.assetType == AssetType::Prefab)
    {
        PrefabTileData prefabData;
        prefabData.prefabHandle = sourceAssetHandle;
        tileData = prefabData;
    }
    else
    {
        return;
    }
    YAML::Node node = YAML::convert<TileAssetData>::encode(tileData);
    std::ofstream fout(newAssetFullPath);
    fout << node;
    fout.close();
    LogInfo("已自动创建Tile资产: {}. 请从资源浏览器中将其拖入本面板。", newAssetRelativePath.string());
}
void TilesetPanel::updateBrushPreview()
{
    m_context->activeBrushPreviewImage = nullptr;
    m_context->activeBrushPreviewSourceRect = SkRect::MakeEmpty();
    const AssetHandle& brushHandle = m_context->activeTileBrush;
    if (!brushHandle.Valid()) return;
    TileLoader tileLoader;
    RuleTileLoader ruleTileLoader;
    TextureLoader textureLoader(*m_context->graphicsBackend);
    AssetHandle finalTileHandle;
    if (brushHandle.assetType == AssetType::Tile)
    {
        finalTileHandle = brushHandle;
    }
    else if (brushHandle.assetType == AssetType::RuleTile)
    {
        sk_sp<RuntimeRuleTile> ruleTile = ruleTileLoader.LoadAsset(brushHandle.assetGuid);
        if (ruleTile)
        {
            finalTileHandle = ruleTile->GetData().defaultTileHandle;
        }
    }
    if (!finalTileHandle.Valid()) return;
    sk_sp<RuntimeTile> tileAsset = tileLoader.LoadAsset(finalTileHandle.assetGuid);
    if (tileAsset && std::holds_alternative<SpriteTileData>(tileAsset->GetData()))
    {
        const auto& spriteData = std::get<SpriteTileData>(tileAsset->GetData());
        if (spriteData.textureHandle.Valid())
        {
            m_context->activeBrushPreviewImage = textureLoader.LoadAsset(spriteData.textureHandle.assetGuid);
            if (m_context->activeBrushPreviewImage)
            {
                if (spriteData.sourceRect.Width() <= 0 || spriteData.sourceRect.Height() <= 0)
                {
                    m_context->activeBrushPreviewSourceRect = SkRect::MakeWH(
                        m_context->activeBrushPreviewImage->getImage()->width(),
                        m_context->activeBrushPreviewImage->getImage()->height());
                }
                else
                {
                    m_context->activeBrushPreviewSourceRect = SkRect::MakeXYWH(
                        spriteData.sourceRect.x, spriteData.sourceRect.y, spriteData.sourceRect.Width(),
                        spriteData.sourceRect.Height());
                }
            }
        }
    }
}

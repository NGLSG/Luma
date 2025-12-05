#ifndef LUMAENGINE_TILESETPANEL_H
#define LUMAENGINE_TILESETPANEL_H
#pragma once
#include "EditorContext.h"
#include "Resources/RuntimeAsset/IRuntimeAsset.h"
#include "Resources/RuntimeAsset/RuntimeTileset.h"
#include "Renderer/GraphicsBackend.h"
#include <vector>
#include "IEditorPanel.h"
#include "RuntimeAsset/RuntimeRuleTile.h"
#include "RuntimeAsset/RuntimeTile.h"
class TilesetPanel : public IEditorPanel
{
public:
    TilesetPanel() = default;
    ~TilesetPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "瓦片集编辑器"; }
private:
    void openTileset(Guid tilesetGuid);
    void closeCurrentTileset();
    void saveCurrentTileset();
    void drawTilesetContent();
    void handleDropTarget();
    void createTileAssetFromSource(const AssetHandle& sourceAssetHandle);
    EditorContext* m_context = nullptr; 
    Guid m_currentTilesetGuid; 
    sk_sp<RuntimeTileset> m_currentTileset = nullptr; 
    std::vector<AssetHandle> m_tileHandles; 
    std::unordered_map<Guid, sk_sp<RuntimeTile>> m_hydratedTiles; 
    std::unordered_map<Guid, sk_sp<RuntimeRuleTile>> m_hydratedRuleTiles; 
    AssetHandle m_lastActiveBrushHandle; 
    void updateBrushPreview();
    float m_thumbnailSize = 64.0f; 
};
#endif

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

/**
 * @brief 瓦片集编辑器面板。
 *
 * 负责显示和编辑瓦片集资源。
 */
class TilesetPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    TilesetPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~TilesetPanel() override = default;

    /**
     * @brief 初始化编辑器面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新编辑器面板逻辑。
     * @param deltaTime 帧时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制编辑器面板UI。
     */
    void Draw() override;
    /**
     * @brief 关闭编辑器面板。
     */
    void Shutdown() override;

    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C字符串。
     */
    const char* GetPanelName() const override { return "瓦片集编辑器"; }

private:
    /**
     * @brief 打开指定的瓦片集。
     * @param tilesetGuid 瓦片集的全局唯一标识符。
     */
    void openTileset(Guid tilesetGuid);

    /**
     * @brief 关闭当前打开的瓦片集。
     */
    void closeCurrentTileset();

    /**
     * @brief 保存当前打开的瓦片集。
     */
    void saveCurrentTileset();

    /**
     * @brief 绘制瓦片集的内容。
     */
    void drawTilesetContent();

    /**
     * @brief 处理拖放目标事件。
     */
    void handleDropTarget();

    /**
     * @brief 从源资产创建瓦片资产。
     * @param sourceAssetHandle 源资产的句柄。
     */
    void createTileAssetFromSource(const AssetHandle& sourceAssetHandle);

    EditorContext* m_context = nullptr; ///< 编辑器上下文指针。

    Guid m_currentTilesetGuid; ///< 当前打开瓦片集的GUID。
    sk_sp<RuntimeTileset> m_currentTileset = nullptr; ///< 当前打开的运行时瓦片集。

    std::vector<AssetHandle> m_tileHandles; ///< 瓦片资产句柄列表。
    std::unordered_map<Guid, sk_sp<RuntimeTile>> m_hydratedTiles; ///< 已加载的运行时瓦片映射。
    std::unordered_map<Guid, sk_sp<RuntimeRuleTile>> m_hydratedRuleTiles; ///< 已加载的运行时规则瓦片映射。

    AssetHandle m_lastActiveBrushHandle; ///< 上次激活的画刷句柄。

    /**
     * @brief 更新画刷预览。
     */
    void updateBrushPreview();
    float m_thumbnailSize = 64.0f; ///< 瓦片缩略图的大小。
};

#endif
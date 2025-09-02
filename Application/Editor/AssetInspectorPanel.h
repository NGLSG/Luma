#ifndef LUMAENGINE_ASSETINSPECTORPANEL_H
#define LUMAENGINE_ASSETINSPECTORPANEL_H

#include "EditorContext.h"
#include "IEditorPanel.h"
#include "Resources/AssetMetadata.h"
#include <unordered_set>

/**
 * @brief 资产检查器面板，用于显示和编辑资产的元数据和设置。
 */
class AssetInspectorPanel : public IEditorPanel
{
public:
    /**
     * @brief 初始化资产检查器面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 绘制资产检查器面板的用户界面。
     */
    void Draw() override;

    /**
     * @brief 关闭资产检查器面板。
     */
    void Shutdown() override;

    /**
     * @brief 更新资产检查器面板的状态。
     * @param deltaTime 帧之间的时间间隔。
     */
    void Update(float deltaTime) override
    {
    }

    /**
     * @brief 获取面板的名称。
     * @return 面板的名称字符串。
     */
    const char* GetPanelName() const override { return "资产设置"; }

private:
    /// @brief 绘制检查器用户界面。
    void drawInspectorUI();
    /// @brief 应用所有更改。
    void applyChanges();
    /// @brief 根据当前选择重置面板状态。
    void resetStateFromSelection();
    /// @brief 将元数据和设置保存到文件。
    /// @param metadata 要保存的资产元数据。
    /// @param settings 要保存的YAML设置。
    void saveMetadataToFile(const AssetMetadata& metadata, const YAML::Node& settings);

private:
    EditorContext* m_context = nullptr; ///< 编辑器上下文指针。

    std::vector<std::filesystem::path> m_currentEditingPaths; ///< 当前正在编辑的资产文件路径列表。
    YAML::Node m_editingSettings; ///< 当前正在编辑的资产设置。
    AssetType m_editingAssetType = AssetType::Unknown; ///< 当前正在编辑的资产类型。

    std::unordered_set<std::string> m_mixedValueProperties; ///< 具有混合值的属性集合（当多选资产时）。
    std::unordered_set<std::string> m_dirtyProperties; ///< 已修改的属性集合。
};

#endif
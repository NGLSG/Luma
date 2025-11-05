#pragma once

#include "IEditorPanel.h" // 包含 IEditorPanel 基类
#include "Resources/AssetMetadata.h" // 包含 AssetType 和 AssetMetadata
#include "EventBus.h"
#include <filesystem>
#include <vector>
#include <set>
#include <any> // 用于类型擦除的 m_deserializedSettings
#include <yaml-cpp/yaml.h>

// 前向声明
class EditorContext;

/**
 * @brief 资产检视器面板
 *
 * 负责在编辑器中显示和编辑所选资产（Assets）的导入设置（Importer Settings）。
 * 支持多选相同类型的资产并批量编辑。
 * 使用类型擦除（std::any）来存储反序列化后的设置，以优化性能。
 */
class AssetInspectorPanel : public IEditorPanel
{
public:
    /**
     * @brief 默认构造函数。
     */
    AssetInspectorPanel() = default;

    /**
     * @brief 默认析构函数。
     */
    ~AssetInspectorPanel() override = default;

    /**
     * @brief 初始化面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 更新面板状态（此面板中当前未使用）。
     * @param deltaTime 帧间时间。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 绘制面板的ImGui界面。
     */
    void Draw() override;

    /**
     * @brief 关闭面板并清理资源。
     */
    void Shutdown() override;

    /**
     * @brief 获取面板的显示名称。
     * @return 面板名称。
     */
    const char* GetPanelName() const override { return "资产检视器"; }

private:
    /**
     * @brief 当编辑器的资产选择发生变化时，重置面板内部状态。
     *
     * 这是面板的核心性能优化点之一。
     * 它会执行一次昂贵的反序列化（Deserialize），将YAML::Node转换为
     * 存储在 m_deserializedSettings (std::any) 中的C++结构体。
     */
    void resetStateFromSelection();

    /**
     * @brief 绘制检视器的主UI。
     *
     * 此函数现在只对已反序列化的C++对象（通过 void*）进行操作，
     * 避免了在循环中进行昂贵的 `settings.as<T>()` 调用。
     */
    void drawInspectorUI();

    /**
     * @brief 应用所有已修改（"dirty"）的属性。
     *
     * 此函数会执行一次昂贵的序列化（Serialize），将C++结构体
     * (m_deserializedSettings) 转换回 YAML::Node，
     * 然后仅将已更改的属性写回 .meta 文件。
     */
    void applyChanges();

    /**
     * @brief 将新的设置保存到资产的 .meta 文件，并触发重新导入。
     * @param originalMetadata 原始的资产元数据。
     * @param newSettings 包含已更改属性的新 YAML::Node。
     */
    void saveMetadataToFile(const AssetMetadata& originalMetadata, const YAML::Node& newSettings);

    /**
     * @brief 打开纹理切片编辑器（仅用于纹理资产）。
     */
    void openTextureSlicer();

private:
    ///< 当前在面板中编辑的资产路径列表。
    std::vector<std::filesystem::path> m_currentEditingPaths;

    ///< 当前编辑的资产类型。
    AssetType m_editingAssetType = AssetType::Unknown;

    ///< 存储类型擦除后的C++设置对象（例如 TextureImporterSettings）。
    std::any m_deserializedSettings;

    ///< 标记 m_deserializedSettings 是否有效。
    bool m_isDeserialized = false;

    ///< 存储在多选时具有混合值的属性名称。
    std::set<std::string> m_mixedValueProperties;

    ///< 存储在UI中被修改过的属性名称。
    std::set<std::string> m_dirtyProperties;
};
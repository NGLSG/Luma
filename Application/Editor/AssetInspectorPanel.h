#pragma once
#include "IEditorPanel.h" 
#include "Resources/AssetMetadata.h" 
#include "EventBus.h"
#include <filesystem>
#include <vector>
#include <set>
#include <any> 
#include <yaml-cpp/yaml.h>
class EditorContext;
class AssetInspectorPanel : public IEditorPanel
{
public:
    AssetInspectorPanel() = default;
    ~AssetInspectorPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "资产检视器"; }
private:
    void resetStateFromSelection();
    void drawInspectorUI();
    void applyChanges();
    void saveMetadataToFile(const AssetMetadata& updatedMetadata, const YAML::Node& newSettings, bool writeAssetFile);
    void openTextureSlicer();
    void openShaderEditor();
private:
    std::vector<std::filesystem::path> m_currentEditingPaths;
    AssetType m_editingAssetType = AssetType::Unknown;
    std::any m_deserializedSettings;
    bool m_isDeserialized = false;
    std::set<std::string> m_mixedValueProperties;
    std::set<std::string> m_dirtyProperties;
    std::string m_addressName;
    std::string m_groupNamesInput;
    bool m_addressMixed = false;
    bool m_groupMixed = false;
    bool m_addressDirty = false;
    bool m_groupDirty = false;
};

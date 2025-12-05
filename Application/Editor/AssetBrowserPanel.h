#ifndef ASSETBROWSERPANEL_H
#define ASSETBROWSERPANEL_H
#include "IEditorPanel.h"
#include "Renderer/Nut/TextureA.h"
struct DirectoryNode
{
    std::filesystem::path path; 
    std::string name; 
    std::map<std::string, std::unique_ptr<DirectoryNode>> subdirectories; 
    std::vector<AssetMetadata> assets; 
    bool scanned = false; 
};
struct Item
{
    std::string name; 
    std::filesystem::path path; 
    Guid guid; 
    AssetType type; 
    bool isDirectory; 
};
class AssetBrowserPanel : public IEditorPanel
{
public:
    AssetBrowserPanel() = default;
    ~AssetBrowserPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "资产浏览器"; }
    void RequestFocusOnAsset(const Guid& guid);
    void OpenScriptInIDE(const Guid& scriptAssetGuid);
    void moveSelectedItems(const std::filesystem::path& destinationRelativePath);
private:
    void drawToolbar();
    void drawDirectoryTree();
    void drawContentView();
    void ProcessDoubleClick(const Item& item);
    void drawAssetContentView();
    void handleContentAreaDragDrop();
    void createPrefabFromGameObject(const Guid& goGuid);
    void triggerHierarchyUpdate();
    void drawDirectoryNode(DirectoryNode& node);
    void drawAssetBrowserContextMenu();
    void drawAssetItemContextMenu(const std::filesystem::path& itemPath);
    void drawConfirmDeleteAssetsPopupContent();
    void buildAssetTree();
    void scanDirectoryNode(DirectoryNode* parentNode);
    DirectoryNode* findNodeByPath(const std::filesystem::path& path);
    void ensurePathLoaded(const std::filesystem::path& path);
    void createNewAsset(AssetType type);
    void deleteSelectedItems();
    void copySelectedItems();
    void pasteCopiedItems();
    void renameItem(const std::filesystem::path& oldRelativePath, const std::string& newName);
    void processFileDrop();
    void loadEditorIcons();
    Nut::TextureAPtr getIconForAssetType(AssetType type);
    struct IconSet
    {
        Nut::TextureAPtr directory; 
        Nut::TextureAPtr image;     
        Nut::TextureAPtr script;    
        Nut::TextureAPtr scene;     
        Nut::TextureAPtr audio;     
        Nut::TextureAPtr prefab;    
        Nut::TextureAPtr file;      
    };
    IconSet m_icons; 
    std::filesystem::path m_pathToExpand; 
    float m_gridCellSize = 90.0f; 
    size_t m_lastSelectionCount; 
};
#endif

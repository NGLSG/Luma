#ifndef ASSETBROWSERPANEL_H
#define ASSETBROWSERPANEL_H

#include "IEditorPanel.h"
#include "Renderer/Nut/TextureA.h"

/**
 * @brief 表示资产浏览器中的一个目录节点。
 */
struct DirectoryNode
{
    std::filesystem::path path; ///< 目录的完整文件系统路径。
    std::string name; ///< 目录的名称。
    std::map<std::string, std::unique_ptr<DirectoryNode>> subdirectories; ///< 子目录的映射。
    std::vector<AssetMetadata> assets; ///< 该目录下的资产元数据列表。
    bool scanned = false; ///< 是否已扫描（仅扫描当前目录的直接子项与资产）。
};

/**
 * @brief 表示资产浏览器中显示的一个项目（文件或目录）。
 */
struct Item
{
    std::string name; ///< 项目的名称。
    std::filesystem::path path; ///< 项目的完整文件系统路径。
    Guid guid; ///< 项目的全局唯一标识符（如果适用）。
    AssetType type; ///< 项目的资产类型。
    bool isDirectory; ///< 指示项目是否为目录。
};


/**
 * @brief 资产浏览器面板，用于显示和管理项目中的资产。
 *
 * 该面板允许用户浏览文件系统中的资产，执行创建、删除、重命名、移动等操作。
 */
class AssetBrowserPanel : public IEditorPanel
{
public:
    AssetBrowserPanel() = default;
    ~AssetBrowserPanel() override = default;

    /**
     * @brief 初始化资产浏览器面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 更新资产浏览器面板的逻辑。
     * @param deltaTime 帧之间的时间间隔。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 绘制资产浏览器面板的用户界面。
     */
    void Draw() override;

    /**
     * @brief 关闭资产浏览器面板。
     */
    void Shutdown() override;

    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C风格字符串。
     */
    const char* GetPanelName() const override { return "资产浏览器"; }


    /**
     * @brief 请求将焦点设置到指定的资产上。
     * @param guid 要聚焦的资产的GUID。
     */
    void RequestFocusOnAsset(const Guid& guid);

    /**
     * @brief 在外部IDE中打开指定的脚本资产。
     * @param scriptAssetGuid 要打开的脚本资产的GUID。
     */
    void OpenScriptInIDE(const Guid& scriptAssetGuid);

    /**
     * @brief 移动选定的项目到指定的目标相对路径。
     * @param destinationRelativePath 目标目录的相对路径。
     */
    void moveSelectedItems(const std::filesystem::path& destinationRelativePath);

private:
    /// 绘制工具栏。
    void drawToolbar();
    /// 绘制目录树视图。
    void drawDirectoryTree();
    /// 绘制内容视图。
    void drawContentView();
    /// 处理项目的双击事件。
    void ProcessDoubleClick(const Item& item);
    /// 绘制资产内容视图。
    void drawAssetContentView();
    /// 处理内容区域的拖放事件。
    void handleContentAreaDragDrop();
    /// 从游戏对象创建预制体。
    void createPrefabFromGameObject(const Guid& goGuid);
    /// 触发层级面板更新。
    void triggerHierarchyUpdate();
    /// 递归绘制目录节点。
    void drawDirectoryNode(DirectoryNode& node);
    /// 绘制资产浏览器上下文菜单。
    void drawAssetBrowserContextMenu();
    /// 绘制资产项目的上下文菜单。
    void drawAssetItemContextMenu(const std::filesystem::path& itemPath);
    /// 绘制确认删除资产的弹出窗口内容。
    void drawConfirmDeleteAssetsPopupContent();


    /// 构建资产树（轻量，构建根并按需扫描）。
    void buildAssetTree();
    /// 扫描指定目录节点（仅当前目录，非递归）。
    void scanDirectoryNode(DirectoryNode* parentNode);
    /// 根据路径查找目录节点。
    DirectoryNode* findNodeByPath(const std::filesystem::path& path);
    /// 确保给定相对路径对应的树节点沿途均已加载。
    void ensurePathLoaded(const std::filesystem::path& path);


    /// 创建新资产。
    void createNewAsset(AssetType type);
    /// 删除选定的项目。
    void deleteSelectedItems();
    /// 复制选定的项目。
    void copySelectedItems();
    /// 粘贴已复制的项目。
    void pasteCopiedItems();
    /// 重命名项目。
    void renameItem(const std::filesystem::path& oldRelativePath, const std::string& newName);
    /// 处理文件拖放事件。
    void processFileDrop();


    /// 加载编辑器图标。
    void loadEditorIcons();
    /// 获取指定资产类型的图标。
    Nut::TextureAPtr getIconForAssetType(AssetType type);


    /**
     * @brief 存储各种资产类型的图标纹理。
     */
    struct IconSet
    {
        Nut::TextureAPtr directory; ///< 目录图标。
        Nut::TextureAPtr image;     ///< 图像图标。
        Nut::TextureAPtr script;    ///< 脚本图标。
        Nut::TextureAPtr scene;     ///< 场景图标。
        Nut::TextureAPtr audio;     ///< 音频图标。
        Nut::TextureAPtr prefab;    ///< 预制体图标。
        Nut::TextureAPtr file;      ///< 通用文件图标。
    };

    IconSet m_icons; ///< 存储所有编辑器图标的集合。
    std::filesystem::path m_pathToExpand; ///< 需要展开的目录路径。
    float m_gridCellSize = 90.0f; ///< 资产内容视图中网格单元格的大小。
    size_t m_lastSelectionCount; ///< 上次选择的项目数量。
};

#endif

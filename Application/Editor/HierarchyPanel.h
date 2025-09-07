#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "IEditorPanel.h"
#include <vector>


class RuntimeGameObject;


/**
 * @brief 层级面板类，用于显示和管理场景中的游戏对象层级结构。
 *
 * 该面板允许用户查看、选择、创建、删除和重新组织场景中的游戏对象。
 */
class HierarchyPanel : public IEditorPanel
{
public:
    /**
     * @brief 默认构造函数。
     */
    HierarchyPanel() = default;
    /**
     * @brief 默认析构函数。
     */
    ~HierarchyPanel() override = default;

    /**
     * @brief 初始化层级面板。
     * @param context 编辑器上下文指针，提供编辑器相关服务。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新层级面板的逻辑。
     * @param deltaTime 距离上一帧的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制层级面板的用户界面。
     */
    void Draw() override;
    /**
     * @brief 关闭层级面板，进行资源清理。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C风格字符串。
     */
    const char* GetPanelName() const override { return "层级"; }


    /**
     * @brief 创建一个空的根级游戏对象。
     */
    void CreateEmptyGameObject();
    /**
     * @brief 在指定的父对象下创建一个空的子游戏对象。
     * @param parent 作为新游戏对象父级的运行时游戏对象。
     */
    void CreateEmptyGameObjectAsChild(RuntimeGameObject& parent);
    /**
     * @brief 复制当前选中的游戏对象。
     */
    void CopySelectedGameObjects();
    /**
     * @brief 将剪贴板中的游戏对象粘贴到指定父对象下。
     * @param parent 作为新游戏对象父级的运行时游戏对象，如果为nullptr则粘贴为根对象。
     */
    void PasteGameObjects(RuntimeGameObject* parent);

private:
    /**
     * @brief 表示层级结构中的一个节点。
     *
     * 用于缓存游戏对象在层级面板中的显示信息。
     */
    struct HierarchyNode
    {
        Guid objectGuid; ///< 游戏对象的全局唯一标识符。
        std::string displayName; ///< 游戏对象的显示名称。
        int depth; ///< 游戏对象在层级中的深度。
        bool hasChildren; ///< 指示游戏对象是否有子对象。
        bool isExpanded; ///< 指示该节点在UI中是否展开。
        bool isVisible; ///< 指示该节点在UI中是否可见。

        /**
         * @brief 构造一个新的层级节点。
         * @param guid 游戏对象的GUID。
         * @param name 游戏对象的名称。
         * @param d 游戏对象的深度。
         * @param children 游戏对象是否有子对象。
         */
        HierarchyNode(const Guid& guid, const std::string& name, int d, bool children)
            : objectGuid(guid), displayName(name), depth(d), hasChildren(children),
              isExpanded(true), isVisible(true)
        {
        }
    };


    void drawSceneCamera();
    void drawVirtualizedGameObjects();
    void drawDropSeparator(int index);
    void drawVirtualizedNode(const HierarchyNode& node, int virtualIndex);
    void handlePanelInteraction();
    void drawContextMenu();
    void handleDragDrop();
    void buildHierarchyCache();
    void buildNodeRecursive(RuntimeGameObject& gameObject, int depth, bool parentVisible);
    void updateNodeVisibility();


    void handleNodeSelection(const Guid& objectGuid);
    void handleRangeSelection(const Guid& endGuid);
    void toggleGameObjectSelection(const Guid& objectGuid);
    void selectSingleGameObject(const Guid& objectGuid);


    void selectSceneCamera();
    void clearSelection();
    bool isNodeExpanded(const Guid& objectGuid) const;
    void setNodeExpanded(const Guid& objectGuid, bool expanded);
    void expandPathToObject(const Guid& targetGuid);


    void handleNodeDragDrop(const HierarchyNode& node);
    void handlePrefabDrop(const AssetHandle& handle, RuntimeGameObject* targetParent);
    void handleGameObjectsDrop(const std::vector<Guid>& draggedGuids, RuntimeGameObject* targetParent);

private:
    std::vector<HierarchyNode> hierarchyCache; ///< 缓存的层级节点列表，用于虚拟化渲染。
    std::vector<int> visibleNodeIndices; ///< 当前可见节点的索引列表。
    bool needsRebuildCache; ///< 指示是否需要重建层级缓存。
    float itemHeight; ///< 每个层级项的高度。


    static constexpr int MAX_VISIBLE_NODES = 16; ///< 同时显示的最大可见节点数。
    static constexpr float NODE_HEIGHT = 20.0f; ///< 单个节点在UI中的高度。


    std::unordered_map<Guid, bool> expandedStates; ///< 存储每个节点的展开状态。
    ListenerHandle m_sceneChangeListener; ///< 监听场景更改事件的句柄。


    int totalNodeCount; ///< 缓存中所有节点的总数。
    int visibleNodeCount; ///< 当前可见节点的总数。
    float lastBuildTime; ///< 上次重建缓存的时间。
};

#endif

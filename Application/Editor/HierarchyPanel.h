#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H
#include "IEditorPanel.h"
#include <vector>
class RuntimeGameObject;
class HierarchyPanel : public IEditorPanel
{
public:
    HierarchyPanel() = default;
    ~HierarchyPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "层级"; }
    void CreateEmptyGameObject();
    void CreateEmptyGameObjectAsChild(RuntimeGameObject& parent);
    void CopySelectedGameObjects();
    void CopyGameObjects(const std::vector<Guid>& guids);
    void PasteGameObjects(RuntimeGameObject* parent);
private:
    struct HierarchyNode
    {
        Guid objectGuid; 
        std::string displayName; 
        int depth; 
        bool hasChildren; 
        bool isExpanded; 
        bool isVisible; 
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
    std::vector<HierarchyNode> hierarchyCache; 
    std::vector<int> visibleNodeIndices; 
    bool needsRebuildCache; 
    float itemHeight; 
    static constexpr int MAX_VISIBLE_NODES = 32; 
    static constexpr float NODE_HEIGHT = 20.0f; 
    std::unordered_map<Guid, bool> expandedStates; 
    ListenerHandle m_sceneChangeListener; 
    int totalNodeCount; 
    int visibleNodeCount; 
    float lastBuildTime; 
    char m_searchBuffer[256] = {0}; 
};
#endif

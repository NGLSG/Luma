#ifndef ANIMATIONCONTROLLEREDITORPANEL_H
#define ANIMATIONCONTROLLEREDITORPANEL_H
#include "IEditorPanel.h"
#include "Data/AnimationControllerData.h"
#include <imgui_node_editor.h>
#include <unordered_map>
#include <string>
#include "RuntimeAsset/RuntimeAnimationController.h"
namespace ed = ax::NodeEditor;
class AnimationControllerEditorPanel : public IEditorPanel
{
private:
    ed::EditorContext* m_nodeEditorContext = nullptr;
    sk_sp<RuntimeAnimationController> m_currentController;
    Guid m_currentControllerGuid;
    std::string m_currentControllerName;
    AnimationControllerData m_controllerData;
    int m_nextNodeId = 1;
    int m_nextLinkId = 1;
    int m_nextPinId = 1;
    ed::NodeId m_contextNodeId = 0;
    ed::LinkId m_contextLinkId = 0;
    enum class NodeType {
        State,      
        Entry,      
        AnyState    
    };
    struct ANode
    {
        NodeType type = NodeType::State; 
        ed::NodeId id;                   
        Guid stateGuid;                  
        std::string name;                
        ImVec2 position;                 
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); 
        bool isEntry = false;            
        bool isDefault = false;          
        ed::PinId inputPinId;            
        ed::PinId outputPinId;           
    };
    struct ALink
    {
        ed::LinkId id;                   
        ed::PinId startPinId;            
        ed::PinId endPinId;              
        Guid fromStateGuid;              
        Guid toStateGuid;                
        std::string transitionName;      
        float duration = 0.0f;           
        int priority = 0;                
        std::vector<Condition> conditions; 
        bool hasExitTime = true;         
    };
    std::vector<ANode> m_nodes;
    std::vector<ALink> m_links;
    std::unordered_map<Guid, int> m_stateToNodeIndex;
    bool m_variablesPanelOpen = true;
    bool m_transitionEditWindowOpen = false;
    int m_editingLinkIndex = -1;
    void initializeFromControllerData();
    void saveToControllerData();
    void drawNodeEditor();
    void handleAnimationClipDrop(const AssetHandle& assetHandle, ImVec2 dropPosition);
    void handleAnimationClipDropOnNode(const AssetHandle& assetHandle, ANode& targetNode);
    void drawVariablesPanel();
    void drawTransitionEditor();
    void drawNodeContextMenu();
    void drawLinkContextMenu();
    void drawConditionEditor(std::vector<Condition>& conditions);
    ANode* findNodeByStateGuid(const Guid& stateGuid);
    ALink* findLinkById(ed::LinkId linkId);
    ANode* findNodeById(ed::NodeId nodeId);
    void createStateNode(const std::string& name, ImVec2 position, bool isEntry = false, bool isDefault = false);
    void deleteNode(ed::NodeId nodeId);
    void deleteLink(ed::LinkId linkId);
    void handleShortcutInput();
    ed::NodeId getNextNodeId()
    {
        return ed::NodeId(m_nextNodeId++);
    }
    ed::LinkId getNextLinkId()
    {
        return ed::LinkId(100000 + m_nextLinkId++);
    }
    ed::PinId getNextPinId()
    {
        return ed::PinId(200000 + m_nextPinId++);
    }
public:
    AnimationControllerEditorPanel() = default;
    ~AnimationControllerEditorPanel() override;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "动画控制器编辑器"; }
    void OpenAnimationController(const Guid& controllerGuid);
    void CloseCurrentController();
    bool HasActiveController() const { return m_currentController != nullptr; }
    void Focus() override;
    bool m_forceLayoutUpdate = false;
    bool m_requestFocus = false;
};
#endif

#ifndef LUMAENGINE_BLUEPRINTPANEL_H
#define LUMAENGINE_BLUEPRINTPANEL_H
#include "IEditorPanel.h"
#include "Data/BlueprintData.h"
#include "Event/EventBus.h"
#include "Resources/RuntimeAsset/RuntimeBlueprint.h"
#include <imgui_node_editor.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "BlueprintNodeRegistry.h"
namespace ed = ax::NodeEditor;
namespace std
{
    template <>
    struct hash<ed::PinId>
    {
        size_t operator()(const ed::PinId& id) const noexcept
        {
            return std::hash<uint64_t>()(id.Get());
        }
    };
    template <>
    struct hash<ed::NodeId>
    {
        size_t operator()(const ed::NodeId& id) const noexcept
        {
            return std::hash<uint64_t>()(id.Get());
        }
    };
    template <>
    struct hash<std::pair<uint32_t, std::string>>
    {
        size_t operator()(const std::pair<uint32_t, std::string>& p) const noexcept
        {
            auto h1 = std::hash<uint32_t>()(p.first);
            auto h2 = std::hash<std::string>()(p.second);
            return h1 ^ (h2 << 1);
        }
    };
}
class BlueprintPanel : public IEditorPanel
{
public:
    BlueprintPanel() = default;
    ~BlueprintPanel() override;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    void OpenBlueprint(const Guid& blueprintGuid);
    void ClearEditorState();
    void CloseCurrentBlueprint();
    void Focus() override;
    const char* GetPanelName() const override
    {
        return "蓝图编辑器";
    }
private:
    struct BPin
    {
        ed::PinId id; 
        ed::NodeId nodeId; 
        std::string name; 
        std::string type; 
        ed::PinKind kind; 
        bool isConnected = false; 
    };
    struct BNode
    {
        ed::NodeId id; 
        uint32_t sourceDataID; 
        std::string name; 
        ImVec2 position; 
        std::vector<BPin> inputPins; 
        std::vector<BPin> outputPins; 
    };
    struct BLink
    {
        ed::LinkId id; 
        ed::PinId startPinId; 
        ed::PinId endPinId; 
    };
    struct BRegion
    {
        uint32_t id; 
        std::string title; 
        ImVec2 position; 
        ImVec2 size; 
        uint32_t functionId = 0; 
        ImVec4 color; 
    };
    enum class ERegionInteractionType
    {
        None, 
        Dragging, 
        Resizing 
    };
    struct RegionInteractionState
    {
        ERegionInteractionType type = ERegionInteractionType::None; 
        BRegion* activeRegion = nullptr; 
        ImVec2 startMousePos; 
        std::vector<BNode*> nodesToDrag; 
    };
    struct InputStringWindow
    {
        uint32_t nodeId; 
        std::string pinName; 
        ImVec2 position; 
        ImVec2 size; 
        bool isOpen; 
        bool needsFocus; 
        std::string windowId; 
    };
    struct SelectTypeWindow
    {
        uint32_t nodeId; 
        std::string pinName; 
        bool isOpen = false; 
        bool needsFocus = false; 
        std::string windowId; 
        char searchBuffer[256] = {0}; 
    };
    struct SelectFunctionWindow
    {
        bool isOpen = false;
        bool needsFocus = false;
        std::string windowId;
        uint32_t nodeId; 
        std::string pinName;
        char searchBuffer[128] = "";
    };
    void drawMenuBar();
    void drawNodeEditor();
    void drawNodeListPanel();
    void drawVariablesPanel();
    void drawFunctionsPanel();
    void drawRegions();
    void drawInputStringWindows();
    void drawSelectTypeWindows();
    void drawBackgroundContextMenu();
    void drawNodeContextMenu();
    void drawLinkContextMenu();
    void drawRegionContextMenu();
    void drawCreateFunctionPopup();
    void drawCreateRegionPopup();
    void drawSelectFunctionWindows();
    void initializeFromBlueprintData();
    void saveToBlueprintData();
    void handleRegionInteraction();
    void updateInputStringWindows();
    void rebuildPinConnections();
    void rebuildFunctionNodePins(const std::string& oldName, const BlueprintFunction& updatedFunc);
    void createNodeFromDefinition(const BlueprintNodeDefinition* definition, ImVec2 position);
    void createVariableNode(const BlueprintVariable& variable, BlueprintNodeType type, ImVec2 position);
    void createFunctionCallNode(const BlueprintFunction& func, ImVec2 position);
    void deleteNode(ed::NodeId nodeId);
    void deleteLink(ed::LinkId linkId);
    void deleteFunction(const std::string& functionName);
    void updateSelfNodePinTypes();
    bool canCreateLink(const BPin* startPin, const BPin* endPin) const;
    BNode* findNodeById(ed::NodeId nodeId);
    BPin* findPinById(ed::PinId pinId);
    BLink* findLinkById(ed::LinkId linkId);
    BlueprintNode* findSourceDataById(uint32_t id);
    InputStringWindow* findInputStringWindow(uint32_t nodeId, const std::string& pinName);
    SelectTypeWindow* findSelectTypeWindow(uint32_t nodeId, const std::string& pinName);
    bool doesEventNodeExist(const std::string& fullName);
    SelectFunctionWindow* findSelectFunctionWindow(uint32_t nodeId, const std::string& pinName);
    void handleShortcutInput();
    uint32_t getNextNodeId() { return m_nextNodeId++; }
    ed::PinId getNextPinId() { return ed::PinId(1000000 + m_nextPinId++); }
    ed::LinkId getNextLinkId() { return ed::LinkId(2000000 + m_nextLinkId++); }
    uint32_t getNextFunctionId() { return 3000000 + m_nextFunctionId++; }
    uint32_t getNextRegionId() { return 4000000 + m_nextRegionId++; }
private:
    ed::EditorContext* m_nodeEditorContext = nullptr; 
    Guid m_currentBlueprintGuid; 
    sk_sp<RuntimeBlueprint> m_currentBlueprint = nullptr; 
    std::string m_currentBlueprintName; 
    std::vector<BNode> m_nodes; 
    std::vector<BLink> m_links; 
    std::vector<BRegion> m_regions; 
    std::vector<InputStringWindow> m_inputStringWindows; 
    std::vector<SelectTypeWindow> m_selectTypeWindows; 
    std::unordered_map<ed::NodeId, BNode*> m_nodeMap; 
    std::unordered_map<ed::PinId, BPin*> m_pinMap; 
    uint32_t m_nextNodeId = 1; 
    uint32_t m_nextPinId = 1; 
    uint32_t m_nextLinkId = 1; 
    uint32_t m_nextFunctionId = 1; 
    uint32_t m_nextRegionId = 1; 
    bool m_requestFocus = false; 
    bool m_variablesPanelOpen = true; 
    bool m_showCreateFunctionPopup = false; 
    bool m_showCreateRegionPopup = false; 
    bool m_isEditingFunction = false; 
    ed::NodeId m_contextNodeId = 0; 
    ed::LinkId m_contextLinkId = 0; 
    uint32_t m_contextRegionId = 0; 
    std::string m_contextFunctionName; 
    RegionInteractionState m_regionInteraction; 
    char m_variableTypeSearchBuffer[256] = {0}; 
    char m_newRegionTitleBuffer[128] = {0}; 
    float m_newRegionColorBuffer[3] = {0.3f, 0.3f, 0.7f}; 
    float m_newRegionSizeBuffer[2] = {400.0f, 300.0f}; 
    BlueprintFunction m_functionEditorBuffer; 
    char m_functionNameBuffer[128]; 
    char m_functionTypeSearchBuffer[256] = {0}; 
    char m_blueprintNameBuffer[128] = {0}; 
    std::vector<std::string> m_sortedTypeNames; 
    ListenerHandle m_scriptCompiledListener; 
    std::vector<SelectFunctionWindow> m_selectFunctionWindows;
};
#endif 

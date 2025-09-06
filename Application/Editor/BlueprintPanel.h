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

// Forward declarations
namespace ed = ax::NodeEditor;

// --- std::hash 特化 ---

namespace std
{
    /**
     * @brief ed::PinId 的 std::hash 特化。
     */
    template <>
    struct hash<ed::PinId>
    {
        size_t operator()(const ed::PinId& id) const noexcept
        {
            return std::hash<uint64_t>()(id.Get());
        }
    };

    /**
     * @brief ed::NodeId 的 std::hash 特化。
     */
    template <>
    struct hash<ed::NodeId>
    {
        size_t operator()(const ed::NodeId& id) const noexcept
        {
            return std::hash<uint64_t>()(id.Get());
        }
    };

    /**
     * @brief std::pair<uint32_t, std::string> 的 std::hash 特化。
     */
    template <>
    struct hash<std::pair<uint32_t, std::string>>
    {
        size_t operator()(const std::pair<uint32_t, std::string>& p) const noexcept
        {
            auto h1 = std::hash<uint32_t>()(p.first);
            auto h2 = std::hash<std::string>()(p.second);
            // 简单组合哈希值
            return h1 ^ (h2 << 1);
        }
    };
}

/**
 * @class BlueprintPanel
 * @brief 蓝图编辑器面板。
 * @details 提供一个可视化的节点编辑器，用于创建和修改蓝图资产 (.blueprint)。
 * 蓝图最终会被解析并生成C#脚本。
 */
class BlueprintPanel : public IEditorPanel
{
public:
    // =========================================================================
    // 公共接口 (Public Interface)
    // =========================================================================

    /**
     * @brief 默认构造函数。
     */
    BlueprintPanel() = default;

    /**
     * @brief 析构函数，负责销毁节点编辑器上下文。
     */
    ~BlueprintPanel() override;

    /**
     * @brief 初始化面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 每帧更新面板逻辑。
     * @param deltaTime 增量时间。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 绘制面板UI。
     */
    void Draw() override;

    /**
     * @brief 关闭面板并清理资源。
     */
    void Shutdown() override;

    /**
     * @brief 打开指定的蓝图资源进行编辑。
     * @param blueprintGuid 要打开的蓝图资源的GUID。
     */
    void OpenBlueprint(const Guid& blueprintGuid);

    /**
     * @brief 清除编辑器内部的UI状态。
     * @details 清除节点、连接、区域等UI元素，但不卸载当前蓝图资源。
     */
    void ClearEditorState();

    /**
     * @brief 关闭当前正在编辑的蓝图。
     * @details 清理UI状态并卸载蓝图资源。
     */
    void CloseCurrentBlueprint();

    /**
     * @brief 请求聚焦到此面板。
     */
    void Focus() override;

    /**
     * @brief 获取面板的显示名称。
     * @return 返回 "蓝图编辑器"。
     */
    const char* GetPanelName() const override
    {
        return "蓝图编辑器";
    }

private:
    // =========================================================================
    // 内部数据结构 (Internal Data Structures)
    // =========================================================================

    /**
     * @struct BPin
     * @brief 编辑器内部表示一个节点的引脚 (Pin)。
     */
    struct BPin
    {
        ed::PinId id; ///< 引脚的唯一ID。
        ed::NodeId nodeId; ///< 所属节点的ID。
        std::string name; ///< 引脚的显示名称。
        std::string type; ///< 引脚的数据类型。
        ed::PinKind kind; ///< 引脚类型 (输入/输出)。
        bool isConnected = false; ///< 标记此引脚是否已连接。
    };

    /**
     * @struct BNode
     * @brief 编辑器内部表示一个蓝图节点 (Node)。
     */
    struct BNode
    {
        ed::NodeId id; ///< 节点的唯一ID。
        uint32_t sourceDataID; ///< 对应于 `BlueprintData` 中的节点ID。
        std::string name; ///< 节点的显示名称。
        ImVec2 position; ///< 节点在画布上的位置。
        std::vector<BPin> inputPins; ///< 节点的输入引脚列表。
        std::vector<BPin> outputPins; ///< 节点的输出引脚列表。
    };

    /**
     * @struct BLink
     * @brief 编辑器内部表示一个连接 (Link)。
     */
    struct BLink
    {
        ed::LinkId id; ///< 连接的唯一ID。
        ed::PinId startPinId; ///< 连接的起始引脚ID。
        ed::PinId endPinId; ///< 连接的结束引脚ID。
    };

    /**
     * @struct BRegion
     * @brief 编辑器内部表示一个逻辑区域/注释框。
     */
    struct BRegion
    {
        uint32_t id; ///< 区域的唯一ID。
        std::string title; ///< 区域的标题。
        ImVec2 position; ///< 区域在画布上的位置。
        ImVec2 size; ///< 区域的尺寸。
        uint32_t functionId = 0; ///< 如果是函数区域，则关联的函数ID。
        ImVec4 color; ///< 区域的背景颜色。
    };

    /**
     * @enum ERegionInteractionType
     * @brief 定义对逻辑区域的交互类型。
     */
    enum class ERegionInteractionType
    {
        None, ///< 无交互。
        Dragging, ///< 正在拖动。
        Resizing ///< 正在调整大小。
    };

    /**
     * @struct RegionInteractionState
     * @brief 存储当前对逻辑区域的交互状态。
     */
    struct RegionInteractionState
    {
        ERegionInteractionType type = ERegionInteractionType::None; ///< 当前交互类型。
        BRegion* activeRegion = nullptr; ///< 当前正在交互的区域。
        ImVec2 startMousePos; ///< 交互开始时的鼠标位置。
        std::vector<BNode*> nodesToDrag; ///< 拖动区域时需要一起移动的节点。
    };

    /**
     * @struct InputStringWindow
     * @brief 管理用于输入长字符串的弹出窗口的状态。
     */
    struct InputStringWindow
    {
        uint32_t nodeId; ///< 关联的节点源ID。
        std::string pinName; ///< 关联的引脚名称。
        ImVec2 position; ///< 窗口在屏幕上的位置。
        ImVec2 size; ///< 窗口的尺寸。
        bool isOpen; ///< 窗口是否可见。
        bool needsFocus; ///< 窗口是否需要获取输入焦点。
        std::string windowId; ///< ImGui窗口的唯一标识符。
    };

    /**
     * @struct SelectTypeWindow
     * @brief 管理用于选择数据类型的弹出窗口状态。
     */
    struct SelectTypeWindow
    {
        uint32_t nodeId; ///< 关联的节点源ID。
        std::string pinName; ///< 关联的引脚名称。
        bool isOpen = false; ///< 窗口是否可见。
        bool needsFocus = false; ///< 窗口是否需要获取输入焦点。
        std::string windowId; ///< ImGui窗口的唯一标识符。
        char searchBuffer[256] = {0}; ///< 类型搜索缓冲区。
    };

    /**
     * @struct SelectFunctionWindow
     * @brief 管理用于选择函数的弹出窗口状态。
     */
    struct SelectFunctionWindow
    {
        bool isOpen = false;
        bool needsFocus = false;
        std::string windowId;
        uint32_t nodeId; // 节点的 sourceDataID
        std::string pinName;
        char searchBuffer[128] = "";
    };


    // =========================================================================
    // 绘制函数 (Drawing Methods)
    // =========================================================================

    /**
     * @brief 绘制面板的菜单栏。
     */
    void drawMenuBar();

    /**
     * @brief 绘制核心的节点编辑器区域。
     */
    void drawNodeEditor();

    /**
     * @brief 在侧边栏绘制可拖拽的节点列表。
     */
    void drawNodeListPanel();

    /**
     * @brief 在侧边栏绘制变量面板。
     */
    void drawVariablesPanel();

    /**
     * @brief 在侧边栏绘制函数面板。
     */
    void drawFunctionsPanel();

    /**
     * @brief 在节点编辑器背景中绘制所有逻辑区域。
     */
    void drawRegions();

    /**
     * @brief 绘制所有活动的 InputString 窗口。
     */
    void drawInputStringWindows();

    /**
     * @brief 绘制所有活动的 SelectType 窗口。
     */
    void drawSelectTypeWindows();

    // --- 上下文菜单和弹窗 (Context Menus & Popups) ---

    /**
     * @brief 绘制节点编辑器背景的右键上下文菜单。
     */
    void drawBackgroundContextMenu();

    /**
     * @brief 绘制节点的右键上下文菜单。
     */
    void drawNodeContextMenu();

    /**
     * @brief 绘制连接的右键上下文菜单。
     */
    void drawLinkContextMenu();

    /**
     * @brief 绘制逻辑区域的右键上下文菜单。
     */
    void drawRegionContextMenu();

    /**
     * @brief 绘制用于创建新函数的模态弹窗。
     */
    void drawCreateFunctionPopup();

    /**
     * @brief 绘制用于创建新逻辑区域的模态弹窗。
     */
    void drawCreateRegionPopup();
    /**
     * @brief 绘制所有活动的 SelectFunction 窗口。
     */
    void drawSelectFunctionWindows();
    // =========================================================================
    // 数据处理与状态管理 (Data Handling & State Management)
    // =========================================================================

    /**
     * @brief 从当前加载的蓝图数据初始化编辑器UI状态。
     */
    void initializeFromBlueprintData();

    /**
     * @brief 将编辑器当前的UI状态保存回蓝图数据，并写入文件。
     */
    void saveToBlueprintData();

    /**
     * @brief 处理用户对逻辑区域的拖动和缩放交互。
     */
    void handleRegionInteraction();

    /**
     * @brief 更新所有 InputString 窗口的位置和状态。
     */
    void updateInputStringWindows();

    /**
     * @brief 遍历所有连接，重建每个引脚的连接状态。
     */
    void rebuildPinConnections();

    /**
     * @brief 根据函数定义更新所有相关的函数调用节点的引脚。
     * @param oldName 函数更新前的旧名称。
     * @param updatedFunc 更新后的函数数据。
     */
    void rebuildFunctionNodePins(const std::string& oldName, const BlueprintFunction& updatedFunc);

    // =========================================================================
    // 实体操作 (Entity Operations)
    // =========================================================================

    /**
     * @brief 根据节点定义在画布上创建一个新节点。
     * @param definition 节点的定义数据。
     * @param position 节点在画布上的初始位置。
     */
    void createNodeFromDefinition(const BlueprintNodeDefinition* definition, ImVec2 position);

    /**
     * @brief 根据变量定义创建一个新的变量节点。
     * @param variable 变量的数据模板。
     * @param type 节点类型 (Get/Set)。
     * @param position 节点在画布上的初始位置。
     */
    void createVariableNode(const BlueprintVariable& variable, BlueprintNodeType type, ImVec2 position);

    /**
     * @brief 根据函数定义创建一个新的函数调用节点。
     * @param func 函数的数据模板。
     * @param position 节点在画布上的初始位置。
     */
    void createFunctionCallNode(const BlueprintFunction& func, ImVec2 position);

    /**
     * @brief 删除指定的节点及其所有相关连接。
     * @param nodeId 要删除的节点的ID。
     */
    void deleteNode(ed::NodeId nodeId);

    /**
     * @brief 删除指定的连接。
     * @param linkId 要删除的连接的ID。
     */
    void deleteLink(ed::LinkId linkId);

    /**
     * @brief 删除指定的函数及其所有调用节点。
     * @param functionName 要删除的函数名称。
     */
    void deleteFunction(const std::string& functionName);
    void updateSelfNodePinTypes();
    /**
     * @brief 检查两个引脚之间是否可以创建连接。
     * @param startPin 起始引脚。
     * @param endPin 目标引脚。
     * @return 如果可以连接，返回 true；否则返回 false。
     */
    bool canCreateLink(const BPin* startPin, const BPin* endPin) const;

    // =========================================================================
    // 辅助函数 (Helper Methods)
    // =========================================================================

    /**
     * @brief 根据节点ID查找编辑器内部的节点对象。
     * @param nodeId 节点ID。
     * @return 指向 BNode 对象的指针，如果未找到则返回 nullptr。
     */
    BNode* findNodeById(ed::NodeId nodeId);

    /**
     * @brief 根据引脚ID查找编辑器内部的引脚对象。
     * @param pinId 引脚ID。
     * @return 指向 BPin 对象的指针，如果未找到则返回 nullptr。
     */
    BPin* findPinById(ed::PinId pinId);

    /**
     * @brief 根据连接ID查找编辑器内部的连接对象。
     * @param linkId 连接ID。
     * @return 指向 BLink 对象的指针，如果未找到则返回 nullptr。
     */
    BLink* findLinkById(ed::LinkId linkId);

    /**
     * @brief 根据源数据ID查找蓝图数据中的节点对象。
     * @param id 节点的源ID。
     * @return 指向 BlueprintNode 对象的指针，如果未找到则返回 nullptr。
     */
    BlueprintNode* findSourceDataById(uint32_t id);

    /**
     * @brief 查找与指定节点和引脚关联的 InputString 窗口。
     * @param nodeId 节点源ID。
     * @param pinName 引脚名称。
     * @return 指向 InputStringWindow 对象的指针，如果未找到则返回 nullptr。
     */
    InputStringWindow* findInputStringWindow(uint32_t nodeId, const std::string& pinName);

    /**
     * @brief 查找与指定节点和引脚关联的 SelectType 窗口。
     * @param nodeId 节点源ID。
     * @param pinName 引脚名称。
     * @return 指向 SelectTypeWindow 对象的指针，如果未找到则返回 nullptr。
     */
    SelectTypeWindow* findSelectTypeWindow(uint32_t nodeId, const std::string& pinName);

    /**
     * @brief 检查具有指定名称的事件节点是否已存在于图中。
     * @param fullName 事件节点的完整名称 (例如 "Luma.SDK.Script.OnUpdate")。
     * @return 如果已存在，返回 true；否则返回 false。
     */
    bool doesEventNodeExist(const std::string& fullName);

    /**
     * @brief 查找与指定节点和引脚关联的 SelectFunction 窗口。
     * @param nodeId 节点源ID。
     * @param pinName 引脚名称。
     * @return 指向 SelectFunctionWindow 对象的指针，如果未找到则返回 nullptr。
     */
    SelectFunctionWindow* findSelectFunctionWindow(uint32_t nodeId, const std::string& pinName);
    // =========================================================================
    // ID 生成器 (ID Generators)
    // =========================================================================

    /**
     * @brief 获取下一个可用的节点源数据ID。
     * @return 新的节点ID。
     */
    uint32_t getNextNodeId() { return m_nextNodeId++; }

    /**
     * @brief 获取下一个可用的UI引脚ID。
     * @return 新的引脚ID。
     */
    ed::PinId getNextPinId() { return ed::PinId(1000000 + m_nextPinId++); }

    /**
     * @brief 获取下一个可用的UI连接ID。
     * @return 新的连接ID。
     */
    ed::LinkId getNextLinkId() { return ed::LinkId(2000000 + m_nextLinkId++); }

    /**
     * @brief 获取下一个可用的函数ID。
     * @return 新的函数ID。
     */
    uint32_t getNextFunctionId() { return 3000000 + m_nextFunctionId++; }

    /**
     * @brief 获取下一个可用的逻辑区域ID。
     * @return 新的逻辑区域ID。
     */
    uint32_t getNextRegionId() { return 4000000 + m_nextRegionId++; }

private:
    // =========================================================================
    // 成员变量 (Member Variables)
    // =========================================================================

    // --- 核心状态与上下文 (Core State & Context) ---
    ed::EditorContext* m_nodeEditorContext = nullptr; ///< ImGui Node Editor 的上下文实例。
    Guid m_currentBlueprintGuid; ///< 当前正在编辑的蓝图资产的GUID。
    sk_sp<RuntimeBlueprint> m_currentBlueprint = nullptr; ///< 当前加载的蓝图资产运行时对象。
    std::string m_currentBlueprintName; ///< 当前蓝图的名称，用于显示。

    // --- UI 数据容器 (UI Data Containers) ---
    std::vector<BNode> m_nodes; ///< 编辑器中的所有节点实例。
    std::vector<BLink> m_links; ///< 编辑器中的所有连接实例。
    std::vector<BRegion> m_regions; ///< 编辑器中的所有逻辑区域实例。
    std::vector<InputStringWindow> m_inputStringWindows; ///< 当前活动的字符串输入窗口列表。
    std::vector<SelectTypeWindow> m_selectTypeWindows; ///< 当前活动的类型选择窗口列表。

    // --- 快速查找映射 (Fast Lookup Maps) ---
    std::unordered_map<ed::NodeId, BNode*> m_nodeMap; ///< 从节点ID到BNode对象的快速映射。
    std::unordered_map<ed::PinId, BPin*> m_pinMap; ///< 从引脚ID到BPin对象的快速映射。

    // --- ID 生成器计数器 (ID Generator Counters) ---
    uint32_t m_nextNodeId = 1; ///< 用于生成唯一的节点源数据ID。
    uint32_t m_nextPinId = 1; ///< 用于生成唯一的UI引脚ID。
    uint32_t m_nextLinkId = 1; ///< 用于生成唯一的UI连接ID。
    uint32_t m_nextFunctionId = 1; ///< 用于生成唯一的函数ID。
    uint32_t m_nextRegionId = 1; ///< 用于生成唯一的逻辑区域ID。

    // --- UI 状态与标志 (UI State & Flags) ---
    bool m_requestFocus = false; ///< 是否请求在下一帧聚焦此面板。
    bool m_variablesPanelOpen = true; ///< 变量面板是否展开。
    bool m_showCreateFunctionPopup = false; ///< 是否显示“创建函数”弹窗。
    bool m_showCreateRegionPopup = false; ///< 是否显示“创建区域”弹窗。
    bool m_isEditingFunction = false; ///< 标记当前是创建新函数还是编辑现有函数。

    // --- 上下文相关ID (Context-Sensitive IDs) ---
    ed::NodeId m_contextNodeId = 0; ///< 右键菜单上下文中的节点ID。
    ed::LinkId m_contextLinkId = 0; ///< 右键菜单上下文中的连接ID。
    uint32_t m_contextRegionId = 0; ///< 右键菜单上下文中的区域ID。
    std::string m_contextFunctionName; ///< 右键菜单上下文中的函数名称。

    // --- 交互状态 (Interaction State) ---
    RegionInteractionState m_regionInteraction; ///< 当前对逻辑区域的交互状态。

    // --- UI 缓冲区 (UI Buffers) ---
    char m_variableTypeSearchBuffer[256] = {0}; ///< 变量面板中用于类型搜索的缓冲区。
    char m_newRegionTitleBuffer[128] = {0}; ///< “创建区域”弹窗中的标题缓冲区。
    float m_newRegionColorBuffer[3] = {0.3f, 0.3f, 0.7f}; ///< “创建区域”弹窗中的颜色缓冲区。
    float m_newRegionSizeBuffer[2] = {400.0f, 300.0f}; ///< “创建区域”弹窗中的尺寸缓冲区。

    // --- 函数编辑器相关 (Function Editor) ---
    BlueprintFunction m_functionEditorBuffer; ///< 用于创建和编辑函数的缓冲区。
    char m_functionNameBuffer[128]; ///< 函数编辑器中的名称缓冲区。
    char m_functionTypeSearchBuffer[256] = {0}; ///< 函数编辑器中用于类型搜索的缓冲区。
    char m_blueprintNameBuffer[128] = {0}; /// 蓝图名称编辑缓冲区。

    // --- 其他 (Miscellaneous) ---
    std::vector<std::string> m_sortedTypeNames; ///< 缓存排序后的可用类型名称列表。
    ListenerHandle m_scriptCompiledListener; ///< 监听脚本编译事件的句柄。
    std::vector<SelectFunctionWindow> m_selectFunctionWindows;
};

#endif //LUMAENGINE_BLUEPRINTPANEL_H

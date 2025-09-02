#ifndef ANIMATIONCONTROLLEREDITORPANEL_H
#define ANIMATIONCONTROLLEREDITORPANEL_H

#include "IEditorPanel.h"
#include "Data/AnimationControllerData.h"
#include <imgui_node_editor.h>
#include <unordered_map>
#include <string>

#include "RuntimeAsset/RuntimeAnimationController.h"

namespace ed = ax::NodeEditor;

/**
 * @brief 动画控制器编辑器面板类。
 *
 * 负责在编辑器中显示和编辑动画控制器逻辑，使用节点编辑器界面。
 */
class AnimationControllerEditorPanel : public IEditorPanel
{
private:
    /**
     * @brief 节点编辑器上下文。
     * 用于管理节点编辑器的状态和交互。
     */
    ed::EditorContext* m_nodeEditorContext = nullptr;

    /**
     * @brief 当前正在编辑的运行时动画控制器。
     */
    sk_sp<RuntimeAnimationController> m_currentController;
    /**
     * @brief 当前动画控制器的全局唯一标识符。
     */
    Guid m_currentControllerGuid;
    /**
     * @brief 当前动画控制器的名称。
     */
    std::string m_currentControllerName;
    /**
     * @brief 当前动画控制器的数据。
     */
    AnimationControllerData m_controllerData;

    /**
     * @brief 下一个可用的节点ID。
     */
    int m_nextNodeId = 1;
    /**
     * @brief 下一个可用的链接ID。
     */
    int m_nextLinkId = 1;
    /**
     * @brief 下一个可用的引脚ID。
     */
    int m_nextPinId = 1;

    /**
     * @brief 上下文菜单关联的节点ID。
     */
    ed::NodeId m_contextNodeId = 0;
    /**
     * @brief 上下文菜单关联的链接ID。
     */
    ed::LinkId m_contextLinkId = 0;

    /**
     * @brief 节点类型枚举。
     */
    enum class NodeType {
        State,      ///< 普通状态节点。
        Entry,      ///< 入口状态节点。
        AnyState    ///< 任意状态节点。
    };

    /**
     * @brief 表示编辑器中的一个节点。
     */
    struct ANode
    {
        NodeType type = NodeType::State; ///< 节点的类型。
        ed::NodeId id;                   ///< 节点的唯一ID。
        Guid stateGuid;                  ///< 关联的状态的全局唯一标识符。
        std::string name;                ///< 节点的名称。
        ImVec2 position;                 ///< 节点在编辑器中的位置。
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); ///< 节点的颜色。
        bool isEntry = false;            ///< 是否为入口节点。
        bool isDefault = false;          ///< 是否为默认状态节点。

        ed::PinId inputPinId;            ///< 节点的输入引脚ID。
        ed::PinId outputPinId;           ///< 节点的输出引脚ID。
    };

    /**
     * @brief 表示编辑器中的一个链接（状态转换）。
     */
    struct ALink
    {
        ed::LinkId id;                   ///< 链接的唯一ID。
        ed::PinId startPinId;            ///< 起始引脚的ID。
        ed::PinId endPinId;              ///< 结束引脚的ID。
        Guid fromStateGuid;              ///< 源状态的全局唯一标识符。
        Guid toStateGuid;                ///< 目标状态的全局唯一标识符。
        std::string transitionName;      ///< 转换的名称。
        float duration = 0.0f;           ///< 转换的持续时间。
        int priority = 0;                ///< 转换的优先级。
        std::vector<Condition> conditions; ///< 转换的条件列表。
        bool hasExitTime = true;         ///< 转换是否具有退出时间。
    };

    /**
     * @brief 存储所有节点的列表。
     */
    std::vector<ANode> m_nodes;
    /**
     * @brief 存储所有链接的列表。
     */
    std::vector<ALink> m_links;
    /**
     * @brief 状态GUID到节点索引的映射。
     */
    std::unordered_map<Guid, int> m_stateToNodeIndex;

    /**
     * @brief 变量面板是否打开。
     */
    bool m_variablesPanelOpen = true;
    /**
     * @brief 转换编辑窗口是否打开。
     */
    bool m_transitionEditWindowOpen = false;
    /**
     * @brief 当前正在编辑的链接在m_links中的索引。
     */
    int m_editingLinkIndex = -1;

    /**
     * @brief 从控制器数据初始化编辑器面板。
     * 根据m_controllerData构建节点和链接。
     */
    void initializeFromControllerData();
    /**
     * @brief 将编辑器面板的当前状态保存到控制器数据。
     * 将节点和链接信息更新到m_controllerData。
     */
    void saveToControllerData();
    /**
     * @brief 绘制节点编辑器界面。
     * 负责渲染所有节点、链接和处理用户交互。
     */
    void drawNodeEditor();
    /**
     * @brief 处理动画剪辑资源拖放到编辑器区域的事件。
     * @param assetHandle 拖放的动画剪辑资源的句柄。
     * @param dropPosition 拖放发生时的屏幕位置。
     */
    void handleAnimationClipDrop(const AssetHandle& assetHandle, ImVec2 dropPosition);
    /**
     * @brief 处理动画剪辑资源拖放到特定节点上的事件。
     * @param assetHandle 拖放的动画剪辑资源的句柄。
     * @param targetNode 目标节点。
     */
    void handleAnimationClipDropOnNode(const AssetHandle& assetHandle, ANode& targetNode);
    /**
     * @brief 绘制变量面板。
     * 显示和编辑动画控制器中的变量。
     */
    void drawVariablesPanel();
    /**
     * @brief 绘制转换编辑器窗口。
     * 用于编辑特定链接（状态转换）的属性。
     */
    void drawTransitionEditor();
    /**
     * @brief 绘制节点上下文菜单。
     * 当用户右键点击节点时显示。
     */
    void drawNodeContextMenu();
    /**
     * @brief 绘制链接上下文菜单。
     * 当用户右键点击链接时显示。
     */
    void drawLinkContextMenu();
    /**
     * @brief 绘制条件编辑器。
     * 用于编辑转换的条件列表。
     * @param conditions 要编辑的条件列表。
     */
    void drawConditionEditor(std::vector<Condition>& conditions);

    /**
     * @brief 根据状态GUID查找节点。
     * @param stateGuid 要查找的状态的GUID。
     * @return 指向找到的节点的指针，如果未找到则为nullptr。
     */
    ANode* findNodeByStateGuid(const Guid& stateGuid);
    /**
     * @brief 根据链接ID查找链接。
     * @param linkId 要查找的链接的ID。
     * @return 指向找到的链接的指针，如果未找到则为nullptr。
     */
    ALink* findLinkById(ed::LinkId linkId);
    /**
     * @brief 根据节点ID查找节点。
     * @param nodeId 要查找的节点的ID。
     * @return 指向找到的节点的指针，如果未找到则为nullptr。
     */
    ANode* findNodeById(ed::NodeId nodeId);

    /**
     * @brief 创建一个新的状态节点。
     * @param name 节点的名称。
     * @param position 节点在编辑器中的初始位置。
     * @param isEntry 是否为入口节点。
     * @param isDefault 是否为默认状态节点。
     */
    void createStateNode(const std::string& name, ImVec2 position, bool isEntry = false, bool isDefault = false);
    /**
     * @brief 删除指定ID的节点。
     * @param nodeId 要删除的节点的ID。
     */
    void deleteNode(ed::NodeId nodeId);
    /**
     * @brief 删除指定ID的链接。
     * @param linkId 要删除的链接的ID。
     */
    void deleteLink(ed::LinkId linkId);

    /**
     * @brief 获取下一个可用的节点ID。
     * @return 新的节点ID。
     */
    ed::NodeId getNextNodeId()
    {
        return ed::NodeId(m_nextNodeId++);
    }

    /**
     * @brief 获取下一个可用的链接ID。
     * @return 新的链接ID。
     */
    ed::LinkId getNextLinkId()
    {
        return ed::LinkId(100000 + m_nextLinkId++);
    }

    /**
     * @brief 获取下一个可用的引脚ID。
     * @return 新的引脚ID。
     */
    ed::PinId getNextPinId()
    {
        return ed::PinId(200000 + m_nextPinId++);
    }

public:
    /**
     * @brief 动画控制器编辑器面板的默认构造函数。
     */
    AnimationControllerEditorPanel() = default;
    /**
     * @brief 动画控制器编辑器面板的析构函数。
     */
    ~AnimationControllerEditorPanel() override;

    /**
     * @brief 初始化编辑器面板。
     * @param context 编辑器上下文。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新编辑器面板逻辑。
     * @param deltaTime 帧时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制编辑器面板的用户界面。
     */
    void Draw() override;
    /**
     * @brief 关闭编辑器面板。
     * 释放相关资源。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C字符串。
     */
    const char* GetPanelName() const override { return "动画控制器编辑器"; }

    /**
     * @brief 打开指定的动画控制器进行编辑。
     * @param controllerGuid 要打开的动画控制器的GUID。
     */
    void OpenAnimationController(const Guid& controllerGuid);

    /**
     * @brief 关闭当前正在编辑的动画控制器。
     */
    void CloseCurrentController();

    /**
     * @brief 检查是否有活动的动画控制器正在编辑。
     * @return 如果有活动控制器则返回true，否则返回false。
     */
    bool HasActiveController() const { return m_currentController != nullptr; }
    /**
     * @brief 使编辑器面板获得焦点。
     */
    void Focus() override;
    /**
     * @brief 强制布局更新标志。
     * 如果设置为true，编辑器将强制重新计算节点布局。
     */
    bool m_forceLayoutUpdate = false;
    /**
     * @brief 请求焦点标志。
     * 如果设置为true，编辑器面板将在下一帧获得焦点。
     */
    bool m_requestFocus = false;
};

#endif
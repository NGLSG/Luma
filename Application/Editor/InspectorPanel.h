#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include "IEditorPanel.h"
#include "../Components/Transform.h"
#include "../Event/LumaEvent.h"
#include "RuntimeAsset/RuntimeGameObject.h"
struct ComponentRegistration;
struct PropertyRegistration;


/**
 * @brief 属性面板类，用于显示和编辑选中游戏对象或场景组件的属性。
 *
 * 该面板继承自IEditorPanel，提供了初始化、更新、绘制和关闭等编辑器面板的基本功能。
 * 它能够处理单个或多个游戏对象的属性显示和批量编辑。
 */
class InspectorPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    InspectorPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~InspectorPanel() override = default;

    /**
     * @brief 初始化属性面板。
     * @param context 编辑器上下文，提供编辑器运行所需的环境信息。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新属性面板的逻辑。
     * @param deltaTime 距离上一帧的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制属性面板的用户界面。
     */
    void Draw() override;
    /**
     * @brief 关闭属性面板，进行资源清理。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板的名称字符串。
     */
    const char* GetPanelName() const override { return "属性"; }

private:
    // Cache/invalidation helpers
    void rebuildCache(const std::vector<Guid>& guids, SelectionType type);
    size_t computeSelectionFingerprint(SelectionType type, const std::vector<Guid>& guids) const;

    /// 绘制未选中任何对象时的提示信息。
    void drawNoSelection();
    /// 绘制游戏对象的属性检查器。
    void drawGameObjectInspector();
    /// 绘制场景摄像机的属性检查器。
    void drawSceneCameraInspector();


    /// 绘制单个游戏对象的属性检查器。
    /// @param gameObject 要绘制属性的游戏对象。
    void drawSingleObjectInspector(RuntimeGameObject& gameObject);
    /// 绘制单个游戏对象的名称。
    /// @param gameObject 要绘制名称的游戏对象。
    void drawGameObjectName(RuntimeGameObject& gameObject);
    /// 绘制单个游戏对象的所有组件。
    /// @param gameObject 要绘制组件的游戏对象。
    void drawComponents(RuntimeGameObject& gameObject);
    void drawScriptsComponentUI(RuntimeGameObject& gameObject);
    /// 绘制组件的头部，包含组件名称和折叠/删除按钮等。
    /// @param componentName 组件的名称。
    /// @param compInfo 组件的注册信息。
    /// @param entityHandle 实体句柄。
    void drawComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                             entt::entity entityHandle);
    /// 绘制组件的上下文菜单。
    /// @param componentName 组件的名称。
    /// @param compInfo 组件的注册信息。
    /// @param entityHandle 实体句柄。
    bool drawComponentContextMenu(const std::string& componentName, const ComponentRegistration* compInfo,
                                  entt::entity entityHandle);
    /// 绘制添加组件按钮。
    void drawAddComponentButton();
    /// 绘制拖放目标区域。
    void drawDragDropTarget();


    /// 绘制多个游戏对象的属性检查器（批量编辑）。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawMultiObjectInspector(std::vector<RuntimeGameObject>& selectedObjects);
    /// 绘制批量选中的游戏对象的名称。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawBatchGameObjectName(std::vector<RuntimeGameObject>& selectedObjects);
    /// 绘制多个游戏对象共有的组件。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawCommonComponents(const std::vector<RuntimeGameObject>& selectedObjects);
    /// 绘制批量组件的头部。
    /// @param componentName 组件的名称。
    /// @param compInfo 组件的注册信息。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawBatchComponentHeader(const std::string& componentName, const ComponentRegistration* compInfo,
                                  const std::vector<RuntimeGameObject>& selectedObjects);
    /// 绘制批量组件的上下文菜单。
    /// @param componentName 组件的名称。
    /// @param compInfo 组件的注册信息。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawBatchComponentContextMenu(const std::string& componentName, const ComponentRegistration* compInfo,
                                       const std::vector<RuntimeGameObject>& selectedObjects);
    /// 绘制批量添加组件按钮。
    void drawBatchAddComponentButton();


    /// 绘制批量变换组件的属性。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawBatchTransformComponent(const std::vector<RuntimeGameObject>& selectedObjects);
    /// 显示批量变换组件的值。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param allHaveParent 指示所有选中对象是否都有父级。
    void displayBatchTransformValues(const std::vector<RuntimeGameObject>& selectedObjects, bool allHaveParent);
    /// 应用批量变换的位置。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param position 要应用的新位置。
    /// @param allHaveParent 指示所有选中对象是否都有父级。
    void applyBatchTransformPosition(const std::vector<RuntimeGameObject>& selectedObjects,
                                     const ECS::Vector2f& position, bool allHaveParent);
    /// 应用批量变换的旋转。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param rotation 要应用的新旋转角度。
    /// @param allHaveParent 指示所有选中对象是否都有父级。
    void applyBatchTransformRotation(const std::vector<RuntimeGameObject>& selectedObjects, float rotation,
                                     bool allHaveParent);
    /// 应用批量变换的缩放。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param scale 要应用的新缩放值。
    /// @param allHaveParent 指示所有选中对象是否都有父级。
    void applyBatchTransformScale(const std::vector<RuntimeGameObject>& selectedObjects, const ECS::Vector2f& scale,
                                  bool allHaveParent);


    /// 绘制批量属性。
    /// @param propName 属性名称。
    /// @param propInfo 属性注册信息。
    /// @param compInfo 组件注册信息。
    /// @param selectedObjects 选中的游戏对象列表。
    void drawBatchProperty(const std::string& propName, const PropertyRegistration& propInfo,
                           const ComponentRegistration* compInfo,
                           const std::vector<RuntimeGameObject>& selectedObjects);
    /// 将属性应用到所有选中的游戏对象。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param compInfo 组件注册信息。
    void applyPropertyToAllSelected(const std::vector<RuntimeGameObject>& selectedObjects,
                                    const ComponentRegistration* compInfo);


    /// 绘制锁定按钮。
    void drawLockButton();
    /// 锁定当前选择。
    void lockSelection();
    /// 解锁当前选择。
    void unlockSelection();
    /// 检查当前选择是否被锁定。
    /// @return 如果选择被锁定则返回true，否则返回false。
    bool isSelectionLocked() const;
    /// 获取当前选中对象的GUID列表。
    /// @return 当前选中对象的GUID列表。
    std::vector<Guid> getCurrentSelectionGuids() const;
    /// 根据GUID列表绘制游戏对象的属性检查器。
    /// @param guids 游戏对象的GUID列表。
    void drawGameObjectInspectorWithGuids(const std::vector<Guid>& guids);


    /// 应用批量名称更改到选中的游戏对象。
    /// @param selectedObjects 选中的游戏对象列表。
    /// @param newName 新的名称。
    void applyBatchNameChange(const std::vector<RuntimeGameObject>& selectedObjects, const char* newName);

private:
    bool m_isLocked = false; ///< 指示选择是否被锁定。
    std::vector<Guid> m_lockedGuids; ///< 存储被锁定的游戏对象GUID列表。
    SelectionType m_lockedSelectionType = SelectionType::NA; ///< 存储被锁定时的选择类型。

    // Event listeners and incremental caching to improve performance with many selections
    ListenerHandle m_evtGoCreated{};
    ListenerHandle m_evtGoDestroyed{};
    ListenerHandle m_evtCompAdded{};
    ListenerHandle m_evtCompRemoved{};
    ListenerHandle m_evtCompUpdated{};
    bool m_dirty = true;
    size_t m_selectionFingerprint = 0;

    // Cached selection expansion
    std::vector<RuntimeGameObject> m_cachedSelectedObjects;

    // Cached common components for multi-selection
    std::vector<std::string> m_cachedCommonComponents;

    struct BatchTransformSummary
    {
        bool valid = false;
        bool allHaveParent = false;
        bool positionSame = false;
        bool rotationSame = false;
        bool scaleSame = false;
        ECS::Vector2f refPosition{0.f, 0.f};
        float refRotation = 0.f;
        ECS::Vector2f refScale{1.f, 1.f};
    };
    BatchTransformSummary m_cachedBatchTransform;
};

#endif

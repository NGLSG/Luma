/**
 * @file SceneViewPanel.h
 * @brief 定义了场景视图面板类，用于在编辑器中显示和交互场景内容。
 */
#ifndef SCENEVIEWPANEL_H
#define SCENEVIEWPANEL_H

#include "ColliderComponent.h"
#include "IEditorPanel.h"
#include "Sprite.h"
#include "TextComponent.h"
#include "TilemapComponent.h"
#include "Transform.h"
#include "UIComponents.h"
#include "../Renderer/RenderTarget.h"
#include "Renderer/Camera.h"
#include "../Utils/Guid.h"


struct AssetHandle;
class RuntimeGameObject;

/**
 * @brief 场景视图面板类。
 *
 * 负责在编辑器中渲染游戏场景，处理用户交互，如对象选择、拖拽、碰撞体编辑等。
 */
class SceneViewPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    SceneViewPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~SceneViewPanel() override = default;

    /**
     * @brief 初始化场景视图面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新场景视图面板的逻辑。
     * @param deltaTime 帧时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制场景视图面板的UI和内容。
     */
    void Draw() override;

    /**
     * @brief 关闭场景视图面板。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C字符串。
     */
    const char* GetPanelName() const override { return "场景"; }

private:
    /**
     * @brief 表示被拖拽对象的结构体。
     */
    struct DraggedObject
    {
        Guid guid; ///< 被拖拽对象的全局唯一标识符。
        ECS::Vector2f dragOffset; ///< 拖拽时的鼠标偏移量。
    };


    /**
     * @brief 表示碰撞体编辑手柄的结构体。
     */
    struct ColliderHandle
    {
        Guid entityGuid; ///< 关联实体的全局唯一标识符。
        int handleIndex; ///< 手柄的索引。
        ImVec2 screenPosition; ///< 手柄在屏幕上的位置。
        float radius; ///< 手柄的半径。
    };

    /**
     * @brief 表示当前激活的碰撞体编辑手柄的结构体。
     */
    struct ActiveColliderHandle
    {
        Guid entityGuid; ///< 关联实体的全局唯一标识符。
        int handleIndex = -1; ///< 手柄的索引，-1表示无效。

        ECS::Vector2f fixedPointWorldPos; ///< 拖拽时固定点的世界坐标。
        ECS::Vector2f dragOffset; ///< 拖拽时的鼠标偏移量。

        /**
         * @brief 检查手柄是否有效。
         * @return 如果手柄索引不是-1，则返回true；否则返回false。
         */
        bool IsValid() const { return handleIndex != -1; }

        /**
         * @brief 重置手柄状态为无效。
         */
        void Reset()
        {
            handleIndex = -1;
            entityGuid = Guid();
            fixedPointWorldPos = {0, 0};
            dragOffset = {0, 0};
        }
    };

    // UI 矩形编辑手柄（基于 IUIComponent::rect 的尺寸编辑）
    struct UIRectHandle
    {
        Guid entityGuid;
        ImVec2 screenPosition;
        float size;
    };

    /**
     * @brief 根据变换组件查找对应的实体。
     * @param targetTransform 目标变换组件。
     * @return 找到的实体，如果未找到则返回entt::null。
     */
    entt::entity findEntityByTransform(const ECS::TransformComponent& targetTransform);
    /**
     * @brief 检查世界坐标点是否在空对象（无渲染组件）的范围内。
     * @param worldPoint 世界坐标点。
     * @param transform 空对象的变换组件。
     * @return 如果点在空对象范围内，则返回true；否则返回false。
     */
    bool isPointInEmptyObject(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform);


    /**
     * @brief 绘制选中对象的轮廓。
     * @param viewportScreenPos 视口在屏幕上的起始位置。
     * @param viewportSize 视口的大小。
     */
    void drawSelectionOutlines(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    /**
     * @brief 绘制盒碰撞体的轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 盒碰撞体所属的变换组件。
     * @param boxCollider 盒碰撞体组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawBoxColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                const ECS::BoxColliderComponent& boxCollider, ImU32 outlineColor, ImU32 fillColor,
                                float thickness);
    /**
     * @brief 绘制圆碰撞体的轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 圆碰撞体所属的变换组件。
     * @param circleCollider 圆碰撞体组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawCircleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                   const ECS::CircleColliderComponent& circleCollider, ImU32 outlineColor,
                                   ImU32 fillColor, float thickness);
    /**
     * @brief 绘制多边形碰撞体的轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 多边形碰撞体所属的变换组件。
     * @param polygonCollider 多边形碰撞体组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawPolygonColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::PolygonColliderComponent& polygonCollider, ImU32 outlineColor,
                                    ImU32 fillColor, float thickness);
    /**
     * @brief 绘制边缘碰撞体的轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 边缘碰撞体所属的变换组件。
     * @param edgeCollider 边缘碰撞体组件。
     * @param outlineColor 轮廓颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawEdgeColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                 const ECS::EdgeColliderComponent& edgeCollider, ImU32 outlineColor, float thickness);
    // 绘制Tilemap碰撞体的轮廓（由生成的链段组成）
    void drawTilemapColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::TilemapColliderComponent& tilemapCollider, ImU32 outlineColor,
                                    float thickness);
    /**
     * @brief 绘制精灵的选中轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 精灵所属的变换组件。
     * @param sprite 精灵组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawSpriteSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::SpriteComponent& sprite, ImU32 outlineColor, ImU32 fillColor,
                                    float thickness);

    /**
     * @brief 绘制按钮组件的选中轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 按钮所属的变换组件。
     * @param buttonComp 按钮组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawButtonSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::ButtonComponent& buttonComp, ImU32 outlineColor, ImU32 fillColor,
                                    float thickness);
    /**
     * @brief 绘制胶囊碰撞体的轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 胶囊碰撞体所属的变换组件。
     * @param capsuleCollider 胶囊碰撞体组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawCapsuleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::CapsuleColliderComponent& capsuleCollider, ImU32 outlineColor,
                                    ImU32 fillColor, float thickness);
    /**
     * @brief 绘制碰撞体编辑手柄。
     * @param drawList ImGui绘图列表。
     * @param gameObject 运行时游戏对象。
     * @param transform 碰撞体所属的变换组件。
     */
    void drawColliderEditHandles(ImDrawList* drawList, RuntimeGameObject& gameObject,
                                 const ECS::TransformComponent& transform);
    /**
     * @brief 绘制虚线。
     * @param drawList ImGui绘图列表。
     * @param start 起始点。
     * @param end 结束点。
     * @param color 颜色。
     * @param thickness 线粗细。
     * @param dashSize 虚线段大小。
     */
    void drawDashedLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness,
                        float dashSize);
    /**
     * @brief 绘制输入文本框的选中轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 输入文本框所属的变换组件。
     * @param inputTextComp 输入文本框组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawInputTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                       const ECS::InputTextComponent& inputTextComp, ImU32 outlineColor,
                                       ImU32 fillColor, float thickness);
    /**
     * @brief 绘制文本组件的选中轮廓。
     * @param drawList ImGui绘图列表。
     * @param transform 文本组件所属的变换组件。
     * @param textComp 文本组件。
     * @param outlineColor 轮廓颜色。
     * @param fillColor 填充颜色。
     * @param thickness 轮廓线粗细。
     */
    void drawTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                  const ECS::TextComponent& textComp, ImU32 outlineColor, ImU32 fillColor,
                                  float thickness);
    /**
     * @brief 绘制空对象的选中表示。
     * @param drawList ImGui绘图列表。
     * @param transform 空对象所属的变换组件。
     * @param objectName 对象名称。
     * @param outlineColor 轮廓颜色。
     * @param labelBgColor 标签背景颜色。
     * @param labelTextColor 标签文本颜色。
     */
    void drawEmptyObjectSelection(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                  const std::string& objectName,
                                  ImU32 outlineColor, ImU32 labelBgColor, ImU32 labelTextColor);
    /**
     * @brief 绘制对象名称标签。
     * @param drawList ImGui绘图列表。
     * @param transform 对象所属的变换组件。
     * @param objectName 对象名称。
     * @param labelBgColor 标签背景颜色。
     * @param labelTextColor 标签文本颜色。
     */
    void drawObjectNameLabel(ImDrawList* drawList, const ECS::TransformComponent& transform,
                             const std::string& objectName,
                             ImU32 labelBgColor, ImU32 labelTextColor);


    /**
     * @brief 绘制编辑器网格。
     * @param viewportScreenPos 视口在屏幕上的起始位置。
     * @param viewportSize 视口的大小。
     */
    void drawEditorGrid(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    /**
     * @brief 处理瓦片地图的绘制操作。
     * @param tilemapGo 瓦片地图游戏对象。
     */
    void handleTilePainting(RuntimeGameObject& tilemapGo);
    /**
     * @brief 处理导航和对象拾取。
     * @param viewportScreenPos 视口在屏幕上的起始位置。
     * @param viewportSize 视口的大小。
     */
    void handleNavigationAndPick(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    /**
     * @brief 处理拖放操作。
     */
    void handleDragDrop();
    /**
     * @brief 处理资产拖放到场景中。
     * @param handle 资产句柄。
     * @param worldPosition 拖放发生的世界坐标。
     */
    void processAssetDrop(const AssetHandle& handle, const ECS::Vector2f& worldPosition);
    /**
     * @brief 触发层级面板更新。
     */
    void triggerHierarchyUpdate();
    /**
     * @brief 绘制编辑器Gizmo。
     * @param viewportScreenPos 视口在屏幕上的起始位置。
     * @param viewportSize 视口的大小。
     */
    void drawEditorGizmos(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    /**
     * @brief 绘制瓦片地图网格。
     * @param drawList ImGui绘图列表。
     * @param tilemap 瓦片地图组件。
     * @param viewportScreenPos
     * @param viewportSize
     */
    void drawTilemapGrid(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform, const ECS::TilemapComponent& tilemap, const
                         ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    /**
     * @brief 绘制瓦片画刷预览。
     * @param drawList ImGui绘图列表。
     * @param tilemap 瓦片地图组件。
     */
    void drawTileBrushPreview(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform, const ECS::TilemapComponent& tilemap);


    /**
     * @brief 处理对象拾取。
     * @param worldMousePos 鼠标的世界坐标。
     * @return 被拾取的实体，如果未拾取到则返回entt::null。
     */
    entt::entity handleObjectPicking(const ECS::Vector2f& worldMousePos);
    /**
     * @brief 处理对象拖拽。
     * @param worldMousePos 鼠标的世界坐标。
     */
    void handleObjectDragging(const ECS::Vector2f& worldMousePos);
    /**
     * @brief 启动对象拖拽。
     * @param worldMousePos 鼠标的世界坐标。
     */
    void initiateDragging(const ECS::Vector2f& worldMousePos);
    /**
     * @brief 处理碰撞体手柄拾取。
     * @param worldMousePos 鼠标的世界坐标。
     * @return 如果拾取到碰撞体手柄，则返回true；否则返回false。
     */
    bool handleColliderHandlePicking(const ECS::Vector2f& worldMousePos);
    /**
     * @brief 处理碰撞体手柄拖拽。
     * @param worldMousePos 鼠标的世界坐标。
     */
    void handleColliderHandleDragging(const ECS::Vector2f& worldMousePos);

    // UI 矩形编辑：绘制与交互
    void drawUIRectOutline(ImDrawList* drawList, const ECS::TransformComponent& transform, const ECS::RectF& rect,
                           ImU32 outlineColor, ImU32 fillColor, float thickness);
    void drawUIRectEditHandle(ImDrawList* drawList, const ECS::TransformComponent& transform, const ECS::RectF& rect,
                              std::vector<UIRectHandle>& outHandles);
    bool handleUIRectHandlePicking(const ECS::Vector2f& worldMousePos);
    void handleUIRectHandleDragging(const ECS::Vector2f& worldMousePos);


    /**
     * @brief 选中单个对象。
     * @param objectGuid 对象的全局唯一标识符。
     */
    void selectSingleObject(const Guid& objectGuid);
    /**
     * @brief 切换对象的选中状态。
     * @param objectGuid 对象的全局唯一标识符。
     */
    void toggleObjectSelection(const Guid& objectGuid);
    /**
     * @brief 清除所有选中对象。
     */
    void clearSelection();


    /**
     * @brief 将屏幕坐标转换为世界坐标。
     * @param props 摄像机属性。
     * @param screenPos 屏幕坐标。
     * @return 对应的世界坐标。
     */
    ECS::Vector2f screenToWorldWith(const Camera::CamProperties& props, const ImVec2& screenPos) const;
    /**
     * @brief 将世界坐标转换为屏幕坐标。
     * @param props 摄像机属性。
     * @param worldPos 世界坐标。
     * @return 对应的屏幕坐标。
     */
    ImVec2 worldToScreenWith(const Camera::CamProperties& props, const ECS::Vector2f& worldPos) const;
    /**
     * @brief 绘制摄像机Gizmo。
     * @param drawList ImGui绘图列表。
     */
    void drawCameraGizmo(ImDrawList* drawList);
    /**
     * @brief 绘制设计分辨率边框。
     * 当视口布局模式不是None时，在场景视图中显示设计分辨率的边界。
     * @param viewportScreenPos 视口在屏幕上的起始位置。
     * @param viewportSize 视口的大小。
     */
    void drawDesignResolutionFrame(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);

private:
    std::vector<ColliderHandle> m_colliderHandles; ///< 存储碰撞体编辑手柄的列表。

    std::shared_ptr<RenderTarget> m_sceneViewTarget; ///< 场景视图的渲染目标。
    Camera::CamProperties m_editorCameraProperties; ///< 编辑器摄像机的属性。
    bool m_editorCameraInitialized; ///< 标记编辑器摄像机是否已初始化。


    bool m_isDragging; ///< 标记当前是否正在拖拽对象。
    bool m_isEditingCollider; ///< 标记当前是否正在编辑碰撞体。
    bool m_isEditingUIRect = false; ///< 标记当前是否正在编辑 UI 矩形。
    ActiveColliderHandle m_activeColliderHandle; ///< 当前激活的碰撞体编辑手柄。
    std::vector<DraggedObject> m_draggedObjects; ///< 存储被拖拽对象的列表。
    std::vector<UIRectHandle> m_uiRectHandles; ///< UI 矩形的编辑手柄集合（底角手柄）。
    Guid m_activeUIRectEntity; ///< 当前激活的 UI 矩形所在实体。

    entt::entity m_potentialDragEntity = entt::null; ///< 潜在的拖拽实体。
    ImVec2 m_mouseDownScreenPos; ///< 鼠标按下时的屏幕位置。

    std::vector<entt::entity> m_lastPickCandidates; ///< 上次拾取到的候选实体列表。
    int m_currentPickIndex = -1; ///< 当前拾取索引。
    ImVec2 m_lastPickScreenPos; ///< 上次拾取时的屏幕位置。


    bool m_isPainting = false; ///< 标记当前是否正在进行瓦片绘制。
    ECS::Vector2i m_paintStartCoord; ///< 瓦片绘制的起始坐标。
    std::unordered_set<ECS::Vector2i, ECS::Vector2iHash> m_paintedCoordsThisStroke; ///< 本次绘制笔触中已绘制的瓦片坐标集合。
};

#endif

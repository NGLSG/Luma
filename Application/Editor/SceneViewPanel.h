#ifndef SCENEVIEWPANEL_H
#define SCENEVIEWPANEL_H
#include "AmbientZoneComponent.h"
#include "AreaLightComponent.h"
#include "ColliderComponent.h"
#include "DirectionalLightComponent.h"
#include "IEditorPanel.h"
#include "LightProbeComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "Sprite.h"
#include "TextComponent.h"
#include "TilemapComponent.h"
#include "Transform.h"
#include "UIComponents.h"
#include "../Renderer/RenderTarget.h"
#include "Renderer/Camera.h"
#include "../Utils/Guid.h"
#include "Particles/ParticleRenderer.h"
#include "../../Components/ParticleComponent.h"
#include "TouchGestureHandler.h"
struct AssetHandle;
class RuntimeGameObject;
class SceneViewPanel : public IEditorPanel
{
public:
    SceneViewPanel() = default;
    ~SceneViewPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "场景"; }
private:
    struct DraggedObject
    {
        Guid guid; 
        ECS::Vector2f dragOffset; 
    };
    struct ColliderHandle
    {
        Guid entityGuid; 
        int handleIndex; 
        ImVec2 screenPosition; 
        float radius; 
    };
    struct ActiveColliderHandle
    {
        Guid entityGuid; 
        int handleIndex = -1; 
        ECS::Vector2f fixedPointWorldPos; 
        ECS::Vector2f dragOffset; 
        bool IsValid() const { return handleIndex != -1; }
        void Reset()
        {
            handleIndex = -1;
            entityGuid = Guid();
            fixedPointWorldPos = {0, 0};
            dragOffset = {0, 0};
        }
    };
    struct UIRectHandle
    {
        Guid entityGuid;
        ImVec2 screenPosition;
        float size;
    };
    entt::entity findEntityByTransform(const ECS::TransformComponent& targetTransform);
    bool isPointInEmptyObject(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform);
    void drawSelectionOutlines(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void drawBoxColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                const ECS::BoxColliderComponent& boxCollider, ImU32 outlineColor, ImU32 fillColor,
                                float thickness);
    void drawCircleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                   const ECS::CircleColliderComponent& circleCollider, ImU32 outlineColor,
                                   ImU32 fillColor, float thickness);
    void drawPolygonColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::PolygonColliderComponent& polygonCollider, ImU32 outlineColor,
                                    ImU32 fillColor, float thickness);
    void drawEdgeColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                 const ECS::EdgeColliderComponent& edgeCollider, ImU32 outlineColor, float thickness);
    void drawTilemapColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::TilemapColliderComponent& tilemapCollider, ImU32 outlineColor,
                                    float thickness);
    void drawSpriteSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::SpriteComponent& sprite, ImU32 outlineColor, ImU32 fillColor,
                                    float thickness);
    void drawButtonSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::ButtonComponent& buttonComp, ImU32 outlineColor, ImU32 fillColor,
                                    float thickness);
    void drawCapsuleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                    const ECS::CapsuleColliderComponent& capsuleCollider, ImU32 outlineColor,
                                    ImU32 fillColor, float thickness);
    void drawColliderEditHandles(ImDrawList* drawList, RuntimeGameObject& gameObject,
                                 const ECS::TransformComponent& transform);
    void drawDashedLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness,
                        float dashSize);
    void drawInputTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                       const ECS::InputTextComponent& inputTextComp, ImU32 outlineColor,
                                       ImU32 fillColor, float thickness);
    void drawTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                  const ECS::TextComponent& textComp, ImU32 outlineColor, ImU32 fillColor,
                                  float thickness);
    void drawEmptyObjectSelection(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                  const std::string& objectName,
                                  ImU32 outlineColor, ImU32 labelBgColor, ImU32 labelTextColor);
    void drawObjectNameLabel(ImDrawList* drawList, const ECS::TransformComponent& transform,
                             const std::string& objectName,
                             ImU32 labelBgColor, ImU32 labelTextColor);
    void drawEditorGrid(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void handleTilePainting(RuntimeGameObject& tilemapGo);
    void handleNavigationAndPick(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void handleDragDrop();
    void processAssetDrop(const AssetHandle& handle, const ECS::Vector2f& worldPosition);
    void triggerHierarchyUpdate();
    void drawEditorGizmos(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void drawTilemapGrid(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform, const ECS::TilemapComponent& tilemap, const
                         ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void drawTileBrushPreview(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform, const ECS::TilemapComponent& tilemap);
    entt::entity handleObjectPicking(const ECS::Vector2f& worldMousePos);
    void handleObjectDragging(const ECS::Vector2f& worldMousePos);
    void initiateDragging(const ECS::Vector2f& worldMousePos);
    bool handleColliderHandlePicking(const ECS::Vector2f& worldMousePos);
    void handleColliderHandleDragging(const ECS::Vector2f& worldMousePos);
    void drawUIRectOutline(ImDrawList* drawList, const ECS::TransformComponent& transform, const ECS::RectF& rect,
                           ImU32 outlineColor, ImU32 fillColor, float thickness);
    void drawUIRectEditHandle(ImDrawList* drawList, const ECS::TransformComponent& transform, const ECS::RectF& rect,
                              std::vector<UIRectHandle>& outHandles);
    bool handleUIRectHandlePicking(const ECS::Vector2f& worldMousePos);
    void handleUIRectHandleDragging(const ECS::Vector2f& worldMousePos);
    void selectSingleObject(const Guid& objectGuid);
    void toggleObjectSelection(const Guid& objectGuid);
    void clearSelection();
    ECS::Vector2f screenToWorldWith(const Camera::CamProperties& props, const ImVec2& screenPos) const;
    ImVec2 worldToScreenWith(const Camera::CamProperties& props, const ECS::Vector2f& worldPos) const;
    void drawCameraGizmo(ImDrawList* drawList);
    void drawDesignResolutionFrame(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void updateParticlePreview(float deltaTime);
    void drawParticlePreview(ImDrawList* drawList, const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void setupTouchGestureCallbacks();
    void handleTouchNavigation(const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    
    // 光源 Gizmo 绘制
    void drawLightGizmos(ImDrawList* drawList, const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void drawPointLightGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform, 
                             const struct ECS::PointLightComponent& light, bool isSelected);
    void drawSpotLightGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform,
                            const struct ECS::SpotLightComponent& light, bool isSelected);
    void drawDirectionalLightGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                   const struct ECS::DirectionalLightComponent& light, bool isSelected);
    
    // 增强光照组件 Gizmo 绘制 (Requirements: 13.1, 13.2, 13.3)
    void drawAreaLightGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform,
                            const struct ECS::AreaLightComponent& light, bool isSelected);
    void drawAmbientZoneGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform,
                              const struct ECS::AmbientZoneComponent& zone, bool isSelected);
    void drawLightProbeGizmo(ImDrawList* drawList, const ECS::TransformComponent& transform,
                             const struct ECS::LightProbeComponent& probe, bool isSelected);
    
    // 光照调试视图
    void drawLightingDebugOverlay(ImDrawList* drawList, const ImVec2& viewportScreenPos, const ImVec2& viewportSize);
    void drawLightingDebugUI();
private:
    std::vector<ColliderHandle> m_colliderHandles; 
    std::shared_ptr<RenderTarget> m_sceneViewTarget; 
    Camera::CamProperties m_editorCameraProperties; 
    bool m_editorCameraInitialized; 
    bool m_isDragging; 
    bool m_isEditingCollider; 
    bool m_isEditingUIRect = false; 
    ActiveColliderHandle m_activeColliderHandle; 
    std::vector<DraggedObject> m_draggedObjects; 
    std::vector<UIRectHandle> m_uiRectHandles; 
    Guid m_activeUIRectEntity; 
    entt::entity m_potentialDragEntity = entt::null; 
    ImVec2 m_mouseDownScreenPos; 
    std::vector<entt::entity> m_lastPickCandidates; 
    int m_currentPickIndex = -1; 
    ImVec2 m_lastPickScreenPos; 
    bool m_isPainting = false; 
    ECS::Vector2i m_paintStartCoord; 
    std::unordered_set<ECS::Vector2i, ECS::Vector2iHash> m_paintedCoordsThisStroke; 
    std::unique_ptr<Particles::ParticleRenderer> m_particleRenderer; 
    std::vector<Guid> m_lastParticleSelection; 
    float m_particlePreviewTime = 0.0f;
    // 触摸手势支持(Android Pad)
    TouchGestureHandler m_touchGesture;
    bool m_touchGestureInitialized = false;
    
    // 光照调试视图
    enum class LightingDebugMode
    {
        None,           ///< 正常渲染
        LightingOnly,   ///< 仅显示光照贡献
        LightLayers,    ///< 显示光照层
        // 缓冲区调试视图 (Requirements: 12.5)
        LightBuffer,    ///< 显示光照缓冲区
        ShadowBuffer,   ///< 显示阴影缓冲区
        EmissionBuffer, ///< 显示自发光缓冲区
        NormalBuffer,   ///< 显示法线缓冲区
        GBuffer         ///< 显示 G-Buffer
    };
    LightingDebugMode m_lightingDebugMode = LightingDebugMode::None;
    uint32_t m_debugLayerMask = 0xFFFFFFFF;  ///< 调试时显示的光照层掩码
};
#endif

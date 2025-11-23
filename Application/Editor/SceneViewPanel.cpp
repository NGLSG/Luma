#include "../Utils/PCH.h"
#include "SceneViewPanel.h"

#include "ImGuiRenderer.h"
#include "../Resources/AssetManager.h"
#include "../Components/IDComponent.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/RenderSystem.h"
#include "SceneRenderer.h"
#include "Sprite.h"
#include "Transform.h"
#include "../Utils/Profiler.h"
#include "Renderer/Camera.h"
#include "../Resources/Loaders/PrefabLoader.h"
#include "../Resources/RuntimeAsset/RuntimePrefab.h"
#include "../SceneManager.h"
#include "../Utils/Logger.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <entt/entt.hpp>
#include <ranges>

#include "ColliderComponent.h"
#include "RelationshipComponent.h"
#include "RenderableManager.h"
#include "TextComponent.h"
#include "UIComponents.h"
#include "glm/gtx/transform.hpp"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"

#ifndef PIXELS_PER_METER
#define PIXELS_PER_METER 32.0f;
#endif

namespace
{
    inline ECS::Vector2f ComputeAnchoredCenter(const ECS::TransformComponent& transform, float width, float height)
    {
        float offsetX = (0.5f - transform.anchor.x) * width;
        float offsetY = (0.5f - transform.anchor.y) * height;


        offsetX *= transform.scale.x;
        offsetY *= transform.scale.y;


        if (std::abs(transform.rotation) > 0.0001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);
            const float tempX = offsetX;
            offsetX = offsetX * cosR - offsetY * sinR;
            offsetY = tempX * sinR + offsetY * cosR;
        }

        return ECS::Vector2f(transform.position.x + offsetX, transform.position.y + offsetY);
    }
}

static bool IsPointInSprite(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                            const ECS::SpriteComponent& sprite)
{
    const float halfWidth = 100.f / sprite.image->getImportSettings().pixelPerUnit * (sprite.sourceRect.Width() > 0
        ? sprite.sourceRect.Width()
        : sprite.image->getImage()->width()) * 0.5f;
    const float halfHeight = 100.f / sprite.image->getImportSettings().pixelPerUnit * (sprite.sourceRect.Height() > 0
            ? sprite.sourceRect.Height()
            : sprite.image->getImage()->height()) *
        0.5f;

    if (halfWidth <= 0 || halfHeight <= 0) return false;


    const float width = halfWidth * 2.0f;
    const float height = halfHeight * 2.0f;


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    ECS::Vector2f localPoint = worldPoint - anchoredCenter;


    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(-transform.rotation);
        const float cosR = cosf(-transform.rotation);
        const float tempX = localPoint.x;
        localPoint.x = localPoint.x * cosR - localPoint.y * sinR;
        localPoint.y = tempX * sinR + localPoint.y * cosR;
    }


    localPoint.x /= transform.scale.x;
    localPoint.y /= transform.scale.y;


    return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
        localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
}


static std::vector<std::string> splitTextByNewlines(const std::string& str)
{
    std::vector<std::string> lines;
    if (str.empty())
    {
        lines.emplace_back("");
        return lines;
    }

    std::string line;
    std::istringstream stream(str);
    while (std::getline(stream, line))
    {
        lines.push_back(line);
    }
    return lines;
}


static SkRect GetLocalTextBounds(const ECS::TextComponent& textComp, float padding = 0.0f)
{
    if (!textComp.typeface)
    {
        return SkRect::MakeEmpty();
    }

    SkFont font(textComp.typeface, textComp.fontSize);
    auto lines = splitTextByNewlines(textComp.text);

    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    const float lineHeight = font.getSpacing();

    float maxWidth = 0.0f;
    for (const auto& line : lines)
    {
        maxWidth = std::max(maxWidth, font.measureText(line.c_str(), line.length(), SkTextEncoding::kUTF8));
    }


    const float inkTop = metrics.fAscent;
    const float inkBottom = (lines.size() - 1) * lineHeight + metrics.fDescent;
    const float inkWidth = maxWidth;
    const float inkHeight = inkBottom - inkTop;


    float offsetX = 0.0f;
    float offsetY = 0.0f;

    switch (textComp.alignment)
    {
    case TextAlignment::TopLeft:
    case TextAlignment::MiddleLeft:
    case TextAlignment::BottomLeft:
        offsetX = 0;
        break;
    case TextAlignment::TopCenter:
    case TextAlignment::MiddleCenter:
    case TextAlignment::BottomCenter:
        offsetX = -inkWidth / 2.0f;
        break;
    case TextAlignment::TopRight:
    case TextAlignment::MiddleRight:
    case TextAlignment::BottomRight:
        offsetX = -inkWidth;
        break;
    }

    switch (textComp.alignment)
    {
    case TextAlignment::TopLeft:
    case TextAlignment::TopCenter:
    case TextAlignment::TopRight:
        offsetY = -inkTop;
        break;
    case TextAlignment::MiddleLeft:
    case TextAlignment::MiddleCenter:
    case TextAlignment::MiddleRight:
        offsetY = -inkTop - inkHeight / 2.0f;
        break;
    case TextAlignment::BottomLeft:
    case TextAlignment::BottomCenter:
    case TextAlignment::BottomRight:
        offsetY = -inkTop - inkHeight;
        break;
    }


    SkRect localBounds = SkRect::MakeWH(inkWidth, inkHeight);
    localBounds.offset(offsetX, offsetY + inkTop);
    localBounds.outset(padding, padding);

    return localBounds;
}


static bool isPointInText(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                          const ECS::TextComponent& textComp)
{
    if (!textComp.typeface)
    {
        return false;
    }

    const SkRect localBounds = GetLocalTextBounds(textComp);
    if (localBounds.isEmpty())
    {
        return false;
    }

    ECS::Vector2f localPoint = worldPoint - transform.position;

    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(-transform.rotation);
        const float cosR = cosf(-transform.rotation);
        const float tempX = localPoint.x;
        localPoint.x = localPoint.x * cosR - localPoint.y * sinR;
        localPoint.y = tempX * sinR + localPoint.y * cosR;
    }

    const SkRect scaledBounds = SkRect::MakeLTRB(
        localBounds.fLeft * transform.scale.x,
        localBounds.fTop * transform.scale.y,
        localBounds.fRight * transform.scale.x,
        localBounds.fBottom * transform.scale.y
    );

    return scaledBounds.contains(localPoint.x, localPoint.y);
}


static bool isPointInButton(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                            const ECS::ButtonComponent& button)
{
    const float width = button.rect.Width();
    const float height = button.rect.Height();

    if (width <= 0 || height <= 0) return false;


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    ECS::Vector2f localPoint = worldPoint - anchoredCenter;


    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(-transform.rotation);
        const float cosR = cosf(-transform.rotation);
        const float tempX = localPoint.x;
        localPoint.x = localPoint.x * cosR - localPoint.y * sinR;
        localPoint.y = tempX * sinR + localPoint.y * cosR;
    }


    localPoint.x /= transform.scale.x;
    localPoint.y /= transform.scale.y;


    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
        localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
}


static bool isPointInUIRect(const ECS::Vector2f& worldPoint,
                            const ECS::TransformComponent& transform,
                            float width, float height)
{
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;
    if (halfWidth <= 0 || halfHeight <= 0) return false;

    ECS::Vector2f localPoint = worldPoint - transform.position;
    if (transform.rotation != 0.0f)
    {
        const float sinR = sinf(-transform.rotation);
        const float cosR = cosf(-transform.rotation);
        float tempX = localPoint.x;
        localPoint.x = localPoint.x * cosR - localPoint.y * sinR;
        localPoint.y = tempX * sinR + localPoint.y * cosR;
    }
    if (std::abs(transform.scale.x) > 1e-5f) localPoint.x /= transform.scale.x;
    if (std::abs(transform.scale.y) > 1e-5f) localPoint.y /= transform.scale.y;

    return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
        localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
}

entt::entity SceneViewPanel::findEntityByTransform(const ECS::TransformComponent& targetTransform)
{
    auto& registry = m_context->activeScene->GetRegistry();
    auto view = registry.view<ECS::TransformComponent>();

    for (auto entity : view)
    {
        const auto& transform = view.get<ECS::TransformComponent>(entity);

        if (&transform == &targetTransform)
        {
            return entity;
        }
    }

    return entt::null;
}

bool SceneViewPanel::isPointInEmptyObject(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform)
{
    ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, transform.position);
    ImVec2 worldMouseScreen = worldToScreenWith(m_editorCameraProperties, worldPoint);


    const float crossSize = 8.0f;
    bool inCrossArea = (std::abs(worldMouseScreen.x - screenPos.x) <= crossSize &&
        std::abs(worldMouseScreen.y - screenPos.y) <= crossSize);

    if (inCrossArea)
        return true;


    auto& registry = m_context->activeScene->GetRegistry();
    auto* idComponent = registry.try_get<ECS::IDComponent>(
        findEntityByTransform(transform));

    if (idComponent)
    {
        RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(idComponent->guid);
        if (gameObject.IsValid())
        {
            std::string objectName = gameObject.GetName();
            ImVec2 textSize = ImGui::CalcTextSize(objectName.c_str());


            ImVec2 labelPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y + crossSize + 5.0f);
            ImVec2 labelSize = ImVec2(textSize.x + 8.0f, textSize.y + 4.0f);


            bool inLabelArea = (worldMouseScreen.x >= labelPos.x - 4.0f &&
                worldMouseScreen.x <= labelPos.x + labelSize.x - 4.0f &&
                worldMouseScreen.y >= labelPos.y - 2.0f &&
                worldMouseScreen.y <= labelPos.y + labelSize.y - 2.0f);

            return inLabelArea;
        }
    }

    return false;
}

void SceneViewPanel::Initialize(EditorContext* context)
{
    m_context = context;
    m_editorCameraInitialized = false;
    m_isDragging = false;
    m_isEditingCollider = false;
    m_activeColliderHandle.Reset();
    m_draggedObjects.clear();
}

void SceneViewPanel::Shutdown()
{
    m_sceneViewTarget.reset();
    m_editorCameraInitialized = false;
    m_isDragging = false;
    m_isEditingCollider = false;
    m_activeColliderHandle.Reset();
    m_draggedObjects.clear();
}

void SceneViewPanel::Update(float)
{
}

void SceneViewPanel::Draw()
{
    PROFILE_FUNCTION();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    const ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();


    if (m_context->editorState == EditorState::Editing)
    {
        m_context->engineContext->sceneViewRect = ECS::RectF(viewportScreenPos.x, viewportScreenPos.y, viewportSize.x,
                                                             viewportSize.y);
    }

    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        m_sceneViewTarget = m_context->graphicsBackend->CreateOrGetRenderTarget(
            "SceneView",
            static_cast<uint16_t>(viewportSize.x),
            static_cast<uint16_t>(viewportSize.y));
        m_context->engineContext->isSceneViewFocused =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
            ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        if (m_sceneViewTarget && m_context->activeScene)
        {
            if (!m_editorCameraInitialized)
            {
                m_editorCameraProperties = m_context->activeScene->GetCameraProperties();
                m_editorCameraInitialized = true;
            }

            if (viewportSize.x <= 1 || viewportSize.y <= 1)
            {
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }
            m_editorCameraProperties.viewport = SkRect::MakeXYWH(
                viewportScreenPos.x, viewportScreenPos.y,
                viewportSize.x, viewportSize.y
            );


            Camera& cam = Camera::GetInstance();
            const Camera::CamProperties prevCamProps = cam.m_properties;
            cam.SetProperties(m_editorCameraProperties);

            m_context->graphicsBackend->SetActiveRenderTarget(m_sceneViewTarget);
            for (const auto& packet : m_context->renderQueue)
            {
                m_context->engineContext->renderSystem->Submit(packet);
            }
            m_context->engineContext->renderSystem->Flush();
            m_context->graphicsBackend->Submit();


            ImTextureID textureId = m_context->imguiRenderer->GetOrCreateTextureIdFor(m_sceneViewTarget->GetTexture());
            ImGui::Image(textureId, viewportSize, ImVec2(0, 0), ImVec2(1, 1));


            ImGui::SetCursorScreenPos(viewportScreenPos);
            ImGui::InvisibleButton(
                "##scene_interactive_layer",
                viewportSize,
                ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
            );


            drawEditorGizmos(viewportScreenPos, viewportSize);
            drawCameraGizmo(ImGui::GetWindowDrawList());


            handleNavigationAndPick(viewportScreenPos, viewportSize);


            drawSelectionOutlines(viewportScreenPos, viewportSize);


            handleDragDrop();


            cam.SetProperties(prevCamProps);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void SceneViewPanel::drawSelectionOutlines(const ImVec2& viewportScreenPos, const ImVec2& viewportSize)
{
    if (m_context->selectionType != SelectionType::GameObject || m_context->selectionList.empty())
        return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    m_colliderHandles.clear();
    m_uiRectHandles.clear();
    auto& registry = m_context->activeScene->GetRegistry();


    const ImU32 outlineColor = IM_COL32(255, 165, 0, 255);
    const ImU32 fillColor = IM_COL32(255, 165, 0, 30);
    const ImU32 colliderColor = IM_COL32(0, 255, 0, 255);
    const ImU32 colliderFillColor = IM_COL32(0, 255, 0, 40);
    const ImU32 labelBgColor = IM_COL32(0, 0, 0, 180);
    const ImU32 labelTextColor = IM_COL32(255, 255, 255, 255);
    const float outlineThickness = 2.0f;

    for (const auto& selectedGuid : m_context->selectionList)
    {
        RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(selectedGuid);
        if (!gameObject.IsValid() || !gameObject.HasComponent<ECS::TransformComponent>())
            continue;

        const auto& transform = gameObject.GetComponent<ECS::TransformComponent>();
        bool hasVisualRepresentation = false;


        if (gameObject.HasComponent<ECS::BoxColliderComponent>())
        {
            const auto& boxCollider = gameObject.GetComponent<ECS::BoxColliderComponent>();
            drawBoxColliderOutline(drawList, transform, boxCollider, colliderColor, colliderFillColor,
                                   outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::CircleColliderComponent>())
        {
            const auto& circleCollider = gameObject.GetComponent<ECS::CircleColliderComponent>();
            drawCircleColliderOutline(drawList, transform, circleCollider, colliderColor, colliderFillColor,
                                      outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::PolygonColliderComponent>())
        {
            const auto& polygonCollider = gameObject.GetComponent<ECS::PolygonColliderComponent>();
            drawPolygonColliderOutline(drawList, transform, polygonCollider, colliderColor, colliderFillColor,
                                       outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::EdgeColliderComponent>())
        {
            const auto& edgeCollider = gameObject.GetComponent<ECS::EdgeColliderComponent>();
            drawEdgeColliderOutline(drawList, transform, edgeCollider, colliderColor, outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::TilemapColliderComponent>())
        {
            const auto& tilemapCollider = gameObject.GetComponent<ECS::TilemapColliderComponent>();
            drawTilemapColliderOutline(drawList, transform, tilemapCollider, colliderColor, outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::CapsuleColliderComponent>())
        {
            const auto& capsuleCollider = gameObject.GetComponent<ECS::CapsuleColliderComponent>();
            drawCapsuleColliderOutline(drawList, transform, capsuleCollider, colliderColor, colliderFillColor,
                                       outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::SpriteComponent>())
        {
            const auto& sprite = gameObject.GetComponent<ECS::SpriteComponent>();
            if (sprite.image)
            {
                drawSpriteSelectionOutline(drawList, transform, sprite, outlineColor, fillColor, outlineThickness);
                hasVisualRepresentation = true;
            }
        }

        else if (gameObject.HasComponent<ECS::ButtonComponent>())
        {
            const auto& buttonComp = gameObject.GetComponent<ECS::ButtonComponent>();
            drawButtonSelectionOutline(drawList, transform, buttonComp, outlineColor, fillColor, outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::TextComponent>())
        {
            const auto& textComp = gameObject.GetComponent<ECS::TextComponent>();
            drawTextSelectionOutline(drawList, transform, textComp, outlineColor, fillColor, outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::InputTextComponent>())
        {
            const auto& inputTextComp = gameObject.GetComponent<ECS::InputTextComponent>();
            drawInputTextSelectionOutline(drawList, transform, inputTextComp, outlineColor, fillColor,
                                          outlineThickness);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::ListBoxComponent>())
        {
            const auto& listBox = gameObject.GetComponent<ECS::ListBoxComponent>();
            drawUIRectOutline(drawList, transform, listBox.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, listBox.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::ToggleButtonComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::ToggleButtonComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::RadioButtonComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::RadioButtonComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::CheckBoxComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::CheckBoxComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::SliderComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::SliderComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::ComboBoxComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::ComboBoxComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::ExpanderComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::ExpanderComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::ProgressBarComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::ProgressBarComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }
        else if (gameObject.HasComponent<ECS::TabControlComponent>())
        {
            const auto& comp = gameObject.GetComponent<ECS::TabControlComponent>();
            drawUIRectOutline(drawList, transform, comp.rect, outlineColor, fillColor, outlineThickness);
            drawUIRectEditHandle(drawList, transform, comp.rect, m_uiRectHandles);
            hasVisualRepresentation = true;
        }


        if (!hasVisualRepresentation)
        {
            drawEmptyObjectSelection(drawList, transform, gameObject.GetName(), outlineColor, labelBgColor,
                                     labelTextColor);
        }
        else
        {
            drawObjectNameLabel(drawList, transform, gameObject.GetName(), labelBgColor, labelTextColor);
        }


        drawColliderEditHandles(drawList, gameObject, transform);
    }
}

void SceneViewPanel::drawUIRectOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                       const ECS::RectF& rect,
                                       ImU32 outlineColor, ImU32 fillColor, float thickness)
{
    const float halfW = rect.z * 0.5f;
    const float halfH = rect.w * 0.5f;
    std::vector<ECS::Vector2f> local = {{-halfW, -halfH}, {halfW, -halfH}, {halfW, halfH}, {-halfW, halfH}};
    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);
    std::vector<ImVec2> screen;
    screen.reserve(4);
    for (auto p : local)
    {
        p.x *= transform.scale.x;
        p.y *= transform.scale.y;
        if (std::abs(transform.rotation) > 0.001f)
        {
            float tx = p.x;
            p.x = p.x * cosR - p.y * sinR;
            p.y = tx * sinR + p.y * cosR;
        }
        const ECS::Vector2f wp = transform.position + p;
        screen.push_back(worldToScreenWith(m_editorCameraProperties, wp));
    }
    drawList->AddConvexPolyFilled(screen.data(), 4, fillColor);
    drawList->AddPolyline(screen.data(), 4, outlineColor, ImDrawFlags_Closed, thickness);
}

void SceneViewPanel::drawUIRectEditHandle(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                          const ECS::RectF& rect,
                                          std::vector<UIRectHandle>& outHandles)
{
    ECS::Vector2f brLocal = {rect.z * 0.5f * transform.scale.x, rect.w * 0.5f * transform.scale.y};
    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(transform.rotation), cosR = cosf(transform.rotation);
        float tx = brLocal.x;
        brLocal.x = brLocal.x * cosR - brLocal.y * sinR;
        brLocal.y = tx * sinR + brLocal.y * cosR;
    }
    const ECS::Vector2f brWorld = transform.position + brLocal;
    const ImVec2 brScreen = worldToScreenWith(m_editorCameraProperties, brWorld);
    const float s = 12.0f;
    ImU32 col = IM_COL32(255, 255, 255, 255);
    drawList->AddTriangleFilled(brScreen, ImVec2(brScreen.x + s, brScreen.y), ImVec2(brScreen.x + s, brScreen.y + s),
                                col);
    entt::entity e = findEntityByTransform(transform);
    if (e != entt::null)
    {
        Guid g = m_context->activeScene->FindGameObjectByEntity(e).GetGuid();
        outHandles.push_back({g, brScreen, s});
    }
}

void SceneViewPanel::drawBoxColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                            const ECS::BoxColliderComponent& boxCollider, ImU32 outlineColor,
                                            ImU32 fillColor, float thickness)
{
    const float halfWidth = boxCollider.size.x * 0.5f;
    const float halfHeight = boxCollider.size.y * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWidth, -halfHeight},
        {halfWidth, -halfHeight},
        {halfWidth, halfHeight},
        {-halfWidth, halfHeight}
    };


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);

    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);

    for (auto corner : localCorners)
    {
        corner += boxCollider.offset;


        corner.x *= transform.scale.x;
        corner.y *= transform.scale.y;


        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = corner.x;
            corner.x = corner.x * cosR - corner.y * sinR;
            corner.y = tempX * sinR + corner.y * cosR;
        }


        const ECS::Vector2f worldPos = transform.position + corner;
        screenCorners.push_back(worldToScreenWith(m_editorCameraProperties, worldPos));
    }


    drawList->AddConvexPolyFilled(screenCorners.data(), 4, fillColor);
    drawList->AddPolyline(screenCorners.data(), 4, outlineColor, ImDrawFlags_Closed, thickness);
}

void SceneViewPanel::drawCircleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                               const ECS::CircleColliderComponent& circleCollider, ImU32 outlineColor,
                                               ImU32 fillColor, float thickness)
{
    ECS::Vector2f offsetPos = circleCollider.offset;


    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(transform.rotation);
        const float cosR = cosf(transform.rotation);
        float tempX = offsetPos.x;
        offsetPos.x = offsetPos.x * cosR - offsetPos.y * sinR;
        offsetPos.y = tempX * sinR + offsetPos.y * cosR;
    }

    ECS::Vector2f worldCenter = transform.position + offsetPos;
    ImVec2 screenCenter = worldToScreenWith(m_editorCameraProperties, worldCenter);


    float radius = circleCollider.radius * std::max(transform.scale.x, transform.scale.y);
    float screenRadius = radius * m_editorCameraProperties.zoom;


    drawList->AddCircleFilled(screenCenter, screenRadius, fillColor, 32);


    drawList->AddCircle(screenCenter, screenRadius, outlineColor, 32, thickness);


    ImVec2 directionEnd = ImVec2(
        screenCenter.x + screenRadius * cosf(transform.rotation),
        screenCenter.y + screenRadius * sinf(transform.rotation)
    );
    drawList->AddLine(screenCenter, directionEnd, outlineColor, thickness);
}

void SceneViewPanel::drawPolygonColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                const ECS::PolygonColliderComponent& polygonCollider,
                                                ImU32 outlineColor,
                                                ImU32 fillColor, float thickness)
{
    if (polygonCollider.vertices.size() < 3) return;

    std::vector<ImVec2> screenVertices;
    screenVertices.reserve(polygonCollider.vertices.size());

    for (const auto& vertex : polygonCollider.vertices)
    {
        ECS::Vector2f offsetVertex = vertex + polygonCollider.offset;


        offsetVertex.x *= transform.scale.x;
        offsetVertex.y *= transform.scale.y;


        if (std::abs(transform.rotation) > 0.001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);
            float tempX = offsetVertex.x;
            offsetVertex.x = offsetVertex.x * cosR - offsetVertex.y * sinR;
            offsetVertex.y = tempX * sinR + offsetVertex.y * cosR;
        }


        ECS::Vector2f worldPos = transform.position + offsetVertex;
        ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, worldPos);
        screenVertices.push_back(screenPos);
    }


    drawList->AddConvexPolyFilled(screenVertices.data(), static_cast<int>(screenVertices.size()), fillColor);


    for (size_t i = 0; i < screenVertices.size(); ++i)
    {
        size_t nextI = (i + 1) % screenVertices.size();
        drawList->AddLine(screenVertices[i], screenVertices[nextI], outlineColor, thickness);
    }
}

void SceneViewPanel::drawEdgeColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                             const ECS::EdgeColliderComponent& edgeCollider, ImU32 outlineColor,
                                             float thickness)
{
    if (edgeCollider.vertices.size() < 2) return;

    std::vector<ImVec2> screenVertices;
    screenVertices.reserve(edgeCollider.vertices.size());

    for (const auto& vertex : edgeCollider.vertices)
    {
        ECS::Vector2f offsetVertex = vertex + edgeCollider.offset;


        offsetVertex.x *= transform.scale.x;
        offsetVertex.y *= transform.scale.y;


        if (std::abs(transform.rotation) > 0.001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);
            float tempX = offsetVertex.x;
            offsetVertex.x = offsetVertex.x * cosR - offsetVertex.y * sinR;
            offsetVertex.y = tempX * sinR + offsetVertex.y * cosR;
        }


        ECS::Vector2f worldPos = transform.position + offsetVertex;
        ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, worldPos);
        screenVertices.push_back(screenPos);
    }


    for (size_t i = 0; i < screenVertices.size() - 1; ++i)
    {
        drawList->AddLine(screenVertices[i], screenVertices[i + 1], outlineColor, thickness);
    }


    if (edgeCollider.loop && screenVertices.size() > 2)
    {
        drawList->AddLine(screenVertices.back(), screenVertices.front(), outlineColor, thickness);
    }


    for (const auto& vertex : screenVertices)
    {
        drawList->AddCircleFilled(vertex, 3.0f, outlineColor);
    }
}

void SceneViewPanel::drawTilemapColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                const ECS::TilemapColliderComponent& tilemapCollider,
                                                ImU32 outlineColor,
                                                float thickness)
{
    if (tilemapCollider.generatedChains.empty()) return;

    for (const auto& chain : tilemapCollider.generatedChains)
    {
        if (chain.size() < 2) continue;

        std::vector<ImVec2> screenVertices;
        screenVertices.reserve(chain.size());

        for (const auto& v : chain)
        {
            ECS::Vector2f local = {v.x + tilemapCollider.offset.x, v.y + tilemapCollider.offset.y};

            local.x *= transform.scale.x;
            local.y *= transform.scale.y;

            if (std::abs(transform.rotation) > 0.001f)
            {
                const float sinR = sinf(transform.rotation);
                const float cosR = cosf(transform.rotation);
                float tempX = local.x;
                local.x = local.x * cosR - local.y * sinR;
                local.y = tempX * sinR + local.y * cosR;
            }

            ECS::Vector2f worldPos = transform.position + local;
            ImVec2 sp = worldToScreenWith(m_editorCameraProperties, worldPos);
            screenVertices.push_back(sp);
        }

        for (size_t i = 0; i + 1 < screenVertices.size(); ++i)
        {
            drawList->AddLine(screenVertices[i], screenVertices[i + 1], outlineColor, thickness);
        }
    }
}

void SceneViewPanel::drawSpriteSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                const ECS::SpriteComponent& sprite, ImU32 outlineColor,
                                                ImU32 fillColor, float thickness)
{
    float width = 100.f / sprite.image->getImportSettings().pixelPerUnit * (sprite.sourceRect.Width() > 0
                                                                                ? sprite.sourceRect.Width()
                                                                                : sprite.image->getImage()->width());
    float height = 100.f / sprite.image->getImportSettings().pixelPerUnit * (sprite.sourceRect.Height() > 0
                                                                                 ? sprite.sourceRect.Height()
                                                                                 : sprite.image->getImage()->height());


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    const float scaledWidth = width * transform.scale.x;
    const float scaledHeight = height * transform.scale.y;
    const float halfWidth = scaledWidth * 0.5f;
    const float halfHeight = scaledHeight * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWidth, -halfHeight},
        {halfWidth, -halfHeight},
        {halfWidth, halfHeight},
        {-halfWidth, halfHeight}
    };

    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);

    for (const auto& corner : localCorners)
    {
        ECS::Vector2f rotatedCorner = corner;
        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = corner.x;
            rotatedCorner.x = corner.x * cosR - corner.y * sinR;
            rotatedCorner.y = tempX * sinR + corner.y * cosR;
        }


        ECS::Vector2f worldPos = anchoredCenter + rotatedCorner;
        ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, worldPos);
        screenCorners.push_back(screenPos);
    }


    drawList->AddConvexPolyFilled(screenCorners.data(), 4, fillColor);

    for (int i = 0; i < 4; ++i)
    {
        int nextI = (i + 1) % 4;
        drawList->AddLine(screenCorners[i], screenCorners[nextI], outlineColor, thickness);
    }
}


void SceneViewPanel::drawButtonSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                const ECS::ButtonComponent& buttonComp, ImU32 outlineColor,
                                                ImU32 fillColor, float thickness)
{
    const float width = buttonComp.rect.Width();
    const float height = buttonComp.rect.Height();


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    const float scaledWidth = width * transform.scale.x;
    const float scaledHeight = height * transform.scale.y;
    const float halfWidth = scaledWidth * 0.5f;
    const float halfHeight = scaledHeight * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWidth, -halfHeight}, {halfWidth, -halfHeight},
        {halfWidth, halfHeight}, {-halfWidth, halfHeight}
    };

    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);

    for (const auto& corner : localCorners)
    {
        ECS::Vector2f rotatedCorner = corner;
        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = corner.x;
            rotatedCorner.x = corner.x * cosR - corner.y * sinR;
            rotatedCorner.y = tempX * sinR + corner.y * cosR;
        }


        ECS::Vector2f worldPos = anchoredCenter + rotatedCorner;
        screenCorners.push_back(worldToScreenWith(m_editorCameraProperties, worldPos));
    }

    drawList->AddConvexPolyFilled(screenCorners.data(), 4, fillColor);
    drawList->AddPolyline(screenCorners.data(), 4, outlineColor, ImDrawFlags_Closed, thickness);
}

void SceneViewPanel::drawCapsuleColliderOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                const ECS::CapsuleColliderComponent& capsuleCollider,
                                                ImU32 outlineColor,
                                                ImU32 fillColor, float thickness)
{
    float width = capsuleCollider.size.x * transform.scale.x;
    float height = capsuleCollider.size.y * transform.scale.y;


    float radius, length;
    bool isVertical = (capsuleCollider.direction == ECS::CapsuleDirection::Vertical);

    if (isVertical)
    {
        radius = width * 0.5f;
        length = height - width;
    }
    else
    {
        radius = height * 0.5f;
        length = width - height;
    }


    if (length < 0) length = 0;


    ECS::Vector2f offsetPos = capsuleCollider.offset;


    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(transform.rotation);
        const float cosR = cosf(transform.rotation);
        float tempX = offsetPos.x;
        offsetPos.x = offsetPos.x * cosR - offsetPos.y * sinR;
        offsetPos.y = tempX * sinR + offsetPos.y * cosR;
    }

    ECS::Vector2f worldCenter = transform.position + offsetPos;


    ECS::Vector2f offset1, offset2;
    if (isVertical)
    {
        offset1 = {0, -length * 0.5f};
        offset2 = {0, length * 0.5f};
    }
    else
    {
        offset1 = {-length * 0.5f, 0};
        offset2 = {length * 0.5f, 0};
    }


    if (std::abs(transform.rotation) > 0.001f)
    {
        const float sinR = sinf(transform.rotation);
        const float cosR = cosf(transform.rotation);

        float tempX1 = offset1.x;
        offset1.x = offset1.x * cosR - offset1.y * sinR;
        offset1.y = tempX1 * sinR + offset1.y * cosR;

        float tempX2 = offset2.x;
        offset2.x = offset2.x * cosR - offset2.y * sinR;
        offset2.y = tempX2 * sinR + offset2.y * cosR;
    }

    ECS::Vector2f worldCenter1 = worldCenter + offset1;
    ECS::Vector2f worldCenter2 = worldCenter + offset2;

    ImVec2 screenCenter1 = worldToScreenWith(m_editorCameraProperties, worldCenter1);
    ImVec2 screenCenter2 = worldToScreenWith(m_editorCameraProperties, worldCenter2);
    float screenRadius = radius * m_editorCameraProperties.zoom;


    drawList->AddCircleFilled(screenCenter1, screenRadius, fillColor, 16);
    drawList->AddCircleFilled(screenCenter2, screenRadius, fillColor, 16);


    if (length > 0)
    {
        ECS::Vector2f rectOffset1, rectOffset2;
        if (isVertical)
        {
            rectOffset1 = {-radius, 0};
            rectOffset2 = {radius, 0};
        }
        else
        {
            rectOffset1 = {0, -radius};
            rectOffset2 = {0, radius};
        }


        if (std::abs(transform.rotation) > 0.001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);

            float tempX1 = rectOffset1.x;
            rectOffset1.x = rectOffset1.x * cosR - rectOffset1.y * sinR;
            rectOffset1.y = tempX1 * sinR + rectOffset1.y * cosR;

            float tempX2 = rectOffset2.x;
            rectOffset2.x = rectOffset2.x * cosR - rectOffset2.y * sinR;
            rectOffset2.y = tempX2 * sinR + rectOffset2.y * cosR;
        }

        std::vector<ImVec2> rectCorners = {
            worldToScreenWith(m_editorCameraProperties, worldCenter1 + rectOffset1),
            worldToScreenWith(m_editorCameraProperties, worldCenter1 + rectOffset2),
            worldToScreenWith(m_editorCameraProperties, worldCenter2 + rectOffset2),
            worldToScreenWith(m_editorCameraProperties, worldCenter2 + rectOffset1)
        };

        drawList->AddConvexPolyFilled(rectCorners.data(), 4, fillColor);
    }


    drawList->AddCircle(screenCenter1, screenRadius, outlineColor, 16, thickness);
    drawList->AddCircle(screenCenter2, screenRadius, outlineColor, 16, thickness);


    if (length > 0)
    {
        ECS::Vector2f lineOffset1, lineOffset2;
        if (isVertical)
        {
            lineOffset1 = {-radius, 0};
            lineOffset2 = {radius, 0};
        }
        else
        {
            lineOffset1 = {0, -radius};
            lineOffset2 = {0, radius};
        }


        if (std::abs(transform.rotation) > 0.001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);

            float tempX1 = lineOffset1.x;
            lineOffset1.x = lineOffset1.x * cosR - lineOffset1.y * sinR;
            lineOffset1.y = tempX1 * sinR + lineOffset1.y * cosR;

            float tempX2 = lineOffset2.x;
            lineOffset2.x = lineOffset2.x * cosR - lineOffset2.y * sinR;
            lineOffset2.y = tempX2 * sinR + lineOffset2.y * cosR;
        }

        ImVec2 line1Start = worldToScreenWith(m_editorCameraProperties, worldCenter1 + lineOffset1);
        ImVec2 line1End = worldToScreenWith(m_editorCameraProperties, worldCenter2 + lineOffset1);
        ImVec2 line2Start = worldToScreenWith(m_editorCameraProperties, worldCenter1 + lineOffset2);
        ImVec2 line2End = worldToScreenWith(m_editorCameraProperties, worldCenter2 + lineOffset2);

        drawList->AddLine(line1Start, line1End, outlineColor, thickness);
        drawList->AddLine(line2Start, line2End, outlineColor, thickness);
    }
}


void SceneViewPanel::drawColliderEditHandles(ImDrawList* drawList, RuntimeGameObject& gameObject,
                                             const ECS::TransformComponent& transform)
{
    if (!gameObject.HasComponent<ECS::BoxColliderComponent>()) return;

    const auto& boxCollider = gameObject.GetComponent<ECS::BoxColliderComponent>();


    const float halfWidth = boxCollider.size.x * 0.5f;
    const float halfHeight = boxCollider.size.y * 0.5f;


    std::vector<ECS::Vector2f> localHandles = {
        {-halfWidth, -halfHeight},
        {0.0f, -halfHeight},
        {halfWidth, -halfHeight},
        {halfWidth, 0.0f},
        {halfWidth, halfHeight},
        {0.0f, halfHeight},
        {-halfWidth, halfHeight},
        {-halfWidth, 0.0f}
    };


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);
    const float handleSize = 6.0f;
    const ImU32 handleColor = IM_COL32(255, 255, 255, 255);
    const ImU32 handleOutlineColor = IM_COL32(0, 0, 0, 255);

    m_colliderHandles.clear();

    for (size_t i = 0; i < localHandles.size(); ++i)
    {
        ECS::Vector2f currentHandle = localHandles[i];


        currentHandle += boxCollider.offset;
        currentHandle.x *= transform.scale.x;
        currentHandle.y *= transform.scale.y;

        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = currentHandle.x;
            const float tempY = currentHandle.y; 
            currentHandle.x = tempX * cosR - tempY * sinR;
            currentHandle.y = tempX * sinR + tempY * cosR; 
        }

        const ECS::Vector2f worldPos = transform.position + currentHandle;
        const ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, worldPos);

        drawList->AddCircleFilled(screenPos, handleSize, handleColor);
        drawList->AddCircle(screenPos, handleSize, handleOutlineColor, 12, 2.0f);

        m_colliderHandles.push_back({
            gameObject.GetGuid(),
            static_cast<int>(i),
            screenPos,
            handleSize
        });
    }
}


void SceneViewPanel::drawDashedLine(ImDrawList* drawList, const ImVec2& start, const ImVec2& end,
                                    ImU32 color, float thickness, float dashSize)
{
    ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);

    if (length < 0.001f) return;

    direction.x /= length;
    direction.y /= length;

    float currentDistance = 0.0f;
    bool isDash = true;

    while (currentDistance < length)
    {
        float segmentLength = std::min(dashSize, length - currentDistance);

        if (isDash)
        {
            ImVec2 segmentStart = ImVec2(start.x + direction.x * currentDistance,
                                         start.y + direction.y * currentDistance);
            ImVec2 segmentEnd = ImVec2(segmentStart.x + direction.x * segmentLength,
                                       segmentStart.y + direction.y * segmentLength);
            drawList->AddLine(segmentStart, segmentEnd, color, thickness);
        }

        currentDistance += segmentLength;
        isDash = !isDash;
    }
}


void SceneViewPanel::drawTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                              const ECS::TextComponent& textComp, ImU32 outlineColor,
                                              ImU32 fillColor, float thickness)
{
    if (!textComp.typeface) return;

    const SkRect localBounds = GetLocalTextBounds(textComp);
    if (localBounds.isEmpty()) return;

    const float width = localBounds.width();
    const float height = localBounds.height();


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    const float scaledWidth = width * transform.scale.x;
    const float scaledHeight = height * transform.scale.y;
    const float halfWidth = scaledWidth * 0.5f;
    const float halfHeight = scaledHeight * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWidth, -halfHeight},
        {halfWidth, -halfHeight},
        {halfWidth, halfHeight},
        {-halfWidth, halfHeight}
    };

    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);

    for (const auto& corner : localCorners)
    {
        ECS::Vector2f rotatedCorner = corner;
        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = corner.x;
            rotatedCorner.x = corner.x * cosR - corner.y * sinR;
            rotatedCorner.y = tempX * sinR + corner.y * cosR;
        }


        ECS::Vector2f worldPos = anchoredCenter + rotatedCorner;
        screenCorners.push_back(worldToScreenWith(m_editorCameraProperties, worldPos));
    }

    drawList->AddConvexPolyFilled(screenCorners.data(), 4, fillColor);
    drawList->AddPolyline(screenCorners.data(), 4, outlineColor, ImDrawFlags_Closed, thickness);
}


void SceneViewPanel::drawInputTextSelectionOutline(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                                   const ECS::InputTextComponent& inputTextComp, ImU32 outlineColor,
                                                   ImU32 fillColor, float thickness)
{
    const ECS::TextComponent& displayTextComp = (!inputTextComp.text.text.empty() || inputTextComp.isFocused)
                                                    ? inputTextComp.text
                                                    : inputTextComp.placeholder;

    if (!displayTextComp.typeface) return;

    const float padding = 8.0f;
    const SkRect localBounds = GetLocalTextBounds(displayTextComp, padding);
    if (localBounds.isEmpty()) return;

    const float width = localBounds.width();
    const float height = localBounds.height();


    const ECS::Vector2f anchoredCenter = ComputeAnchoredCenter(transform, width, height);


    const float scaledWidth = width * transform.scale.x;
    const float scaledHeight = height * transform.scale.y;
    const float halfWidth = scaledWidth * 0.5f;
    const float halfHeight = scaledHeight * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWidth, -halfHeight},
        {halfWidth, -halfHeight},
        {halfWidth, halfHeight},
        {-halfWidth, halfHeight}
    };

    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);


    const float sinR = sinf(transform.rotation);
    const float cosR = cosf(transform.rotation);

    for (const auto& corner : localCorners)
    {
        ECS::Vector2f rotatedCorner = corner;
        if (std::abs(transform.rotation) > 0.001f)
        {
            const float tempX = corner.x;
            rotatedCorner.x = corner.x * cosR - corner.y * sinR;
            rotatedCorner.y = tempX * sinR + corner.y * cosR;
        }


        ECS::Vector2f worldPos = anchoredCenter + rotatedCorner;
        screenCorners.push_back(worldToScreenWith(m_editorCameraProperties, worldPos));
    }

    drawList->AddConvexPolyFilled(screenCorners.data(), 4, fillColor);
    drawList->AddPolyline(screenCorners.data(), 4, outlineColor, ImDrawFlags_Closed, thickness);

    if (inputTextComp.isFocused)
    {
        ImU32 focusColor = IM_COL32(100, 200, 255, 255);
        drawList->AddPolyline(screenCorners.data(), 4, focusColor, ImDrawFlags_Closed, thickness + 1.0f);
    }
}

void SceneViewPanel::drawEmptyObjectSelection(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                              const std::string& objectName, ImU32 outlineColor,
                                              ImU32 labelBgColor, ImU32 labelTextColor)
{
    ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, transform.position);


    const float crossSize = 8.0f;
    const float crossThickness = 2.0f;


    float actualCrossSize = crossSize;
    float actualThickness = crossThickness;
    ImU32 actualOutlineColor = outlineColor;

    if (m_isDragging)
    {
        auto& registry = m_context->activeScene->GetRegistry();
        auto* idComponent = registry.try_get<ECS::IDComponent>(
            findEntityByTransform(transform));

        if (idComponent)
        {
            for (const auto& draggedObj : m_draggedObjects)
            {
                if (draggedObj.guid == idComponent->guid)
                {
                    actualCrossSize *= 1.3f;
                    actualThickness *= 1.5f;
                    actualOutlineColor = IM_COL32(255, 200, 0, 255);
                    break;
                }
            }
        }
    }


    drawList->AddLine(
        ImVec2(screenPos.x - actualCrossSize, screenPos.y),
        ImVec2(screenPos.x + actualCrossSize, screenPos.y),
        actualOutlineColor, actualThickness
    );


    drawList->AddLine(
        ImVec2(screenPos.x, screenPos.y - actualCrossSize),
        ImVec2(screenPos.x, screenPos.y + actualCrossSize),
        actualOutlineColor, actualThickness
    );


    ImVec2 textSize = ImGui::CalcTextSize(objectName.c_str());
    ImVec2 labelPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y + actualCrossSize + 5.0f);
    ImVec2 labelSize = ImVec2(textSize.x + 8.0f, textSize.y + 4.0f);


    ImU32 actualLabelBgColor = m_isDragging ? IM_COL32(50, 50, 50, 200) : labelBgColor;

    drawList->AddRectFilled(
        ImVec2(labelPos.x - 4.0f, labelPos.y - 2.0f),
        ImVec2(labelPos.x + labelSize.x - 4.0f, labelPos.y + labelSize.y - 2.0f),
        actualLabelBgColor,
        3.0f
    );


    drawList->AddText(labelPos, labelTextColor, objectName.c_str());
}

void SceneViewPanel::drawObjectNameLabel(ImDrawList* drawList, const ECS::TransformComponent& transform,
                                         const std::string& objectName, ImU32 labelBgColor, ImU32 labelTextColor)
{
    ImVec2 screenPos = worldToScreenWith(m_editorCameraProperties, transform.position);


    ImVec2 textSize = ImGui::CalcTextSize(objectName.c_str());
    ImVec2 labelPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y - 15.0f);
    ImVec2 labelSize = ImVec2(textSize.x + 8.0f, textSize.y + 4.0f);


    drawList->AddRectFilled(
        ImVec2(labelPos.x - 4.0f, labelPos.y - 2.0f),
        ImVec2(labelPos.x + labelSize.x - 4.0f, labelPos.y + labelSize.y - 2.0f),
        labelBgColor,
        3.0f
    );


    drawList->AddText(labelPos, labelTextColor, objectName.c_str());
}

void SceneViewPanel::handleDragDrop()
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            ECS::Vector2f worldPosition = screenToWorldWith(
                m_editorCameraProperties,
                ImGui::GetIO().MousePos
            );
            LogInfo("接收到资产拖拽，GUID: {}, 世界坐标: ({:.2f}, {:.2f})",
                    handle.assetGuid.ToString(), worldPosition.x, worldPosition.y);
            processAssetDrop(handle, worldPosition);
        }
        ImGui::EndDragDropTarget();
    }
}

void SceneViewPanel::processAssetDrop(const AssetHandle& handle, const ECS::Vector2f& worldPosition)
{
    const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
    if (!meta) return;

    if (meta->type == AssetType::Prefab)
    {
        auto prefabLoader = PrefabLoader();
        sk_sp<RuntimePrefab> prefab = prefabLoader.LoadAsset(handle.assetGuid);

        if (prefab)
        {
            RuntimeGameObject newInstance = m_context->activeScene->Instantiate(*prefab, nullptr);

            if (newInstance.IsValid())
            {
                if (newInstance.HasComponent<ECS::TransformComponent>())
                {
                    auto& transform = newInstance.GetComponent<ECS::TransformComponent>();
                    transform.position = worldPosition;
                }
                else
                {
                    LogWarn("实例化的预制体缺少Transform组件，手动添加");
                    auto& transform = newInstance.AddComponent<ECS::TransformComponent>();
                    transform.position = worldPosition;
                }


                selectSingleObject(newInstance.GetGuid());
                triggerHierarchyUpdate();
                LogInfo("预制体实例化成功，GUID: {}", newInstance.GetGuid().ToString());
            }
            else
            {
                LogError("预制体实例化失败，GUID: {}", handle.assetGuid.ToString());
            }
        }
        else
        {
            LogError("加载预制体失败，GUID: {}", handle.assetGuid.ToString());
        }
    }
    else if (meta->type == AssetType::Texture)
    {
        if (!m_context->activeScene) return;

        SceneManager::GetInstance().PushUndoState(m_context->activeScene);

        RuntimeGameObject newGo = m_context->activeScene->CreateGameObject("Sprite");

        if (newGo.IsValid())
        {
            if (newGo.HasComponent<ECS::TransformComponent>())
            {
                newGo.GetComponent<ECS::TransformComponent>().position = worldPosition;
            }


            newGo.AddComponent<ECS::SpriteComponent>(handle.assetGuid, ECS::Colors::White);


            selectSingleObject(newGo.GetGuid());
            triggerHierarchyUpdate();
            LogInfo("纹理精灵创建成功，GUID: {}", newGo.GetGuid().ToString());
        }
    }
    else if (meta->type == AssetType::CSharpScript)
    {
        if (m_context->selectionType == SelectionType::GameObject && !m_context->selectionList.empty())
        {
            bool anyAdded = false;
            for (const auto& objGuid : m_context->selectionList)
            {
                RuntimeGameObject selectedGo = m_context->activeScene->FindGameObjectByGuid(objGuid);
                if (selectedGo.IsValid())
                {
                    auto& scriptsComp = selectedGo.HasComponent<ECS::ScriptsComponent>()
                                            ? selectedGo.GetComponent<ECS::ScriptsComponent>()
                                            : selectedGo.AddComponent<ECS::ScriptsComponent>();


                    scriptsComp.AddScript(handle, selectedGo.GetEntityHandle());
                    anyAdded = true;
                    LogInfo("脚本已添加到GameObject: {}", selectedGo.GetName());
                }
            }

            if (anyAdded)
            {
                m_context->uiCallbacks->onValueChanged.Invoke();
            }
            else
            {
                LogWarn("没有向任何有效对象添加脚本");
            }
        }

        else
        {
            if (!m_context->activeScene) return;

            SceneManager::GetInstance().PushUndoState(m_context->activeScene);

            std::string scriptName = AssetManager::GetInstance().GetAssetName(handle.assetGuid);
            RuntimeGameObject newGo = m_context->activeScene->CreateGameObject(scriptName);

            if (newGo.IsValid())
            {
                if (newGo.HasComponent<ECS::TransformComponent>())
                {
                    newGo.GetComponent<ECS::TransformComponent>().position = worldPosition;
                }


                auto& scriptsComp = newGo.AddComponent<ECS::ScriptsComponent>();

                scriptsComp.AddScript(handle, newGo.GetEntityHandle());

                selectSingleObject(newGo.GetGuid());
                triggerHierarchyUpdate();
                LogInfo("脚本GameObject创建成功，GUID: {}", newGo.GetGuid().ToString());
            }
        }
    }
    else
    {
        LogWarn("不支持的资产类型拖拽到场景视图: {}", static_cast<int>(meta->type));
    }
}

void SceneViewPanel::triggerHierarchyUpdate()
{
    if (!m_context->selectionList.empty())
    {
        m_context->objectToFocusInHierarchy = m_context->selectionList[0];
    }
}

void SceneViewPanel::drawEditorGizmos(const ImVec2& viewportScreenPos, const ImVec2& viewportSize)
{
    bool isTilemapEditingMode = false;
    RuntimeGameObject selectedGo;
    if (m_context->activeTileBrush.Valid() &&
        m_context->selectionType == SelectionType::GameObject &&
        m_context->selectionList.size() == 1)
    {
        selectedGo = m_context->activeScene->FindGameObjectByGuid(m_context->selectionList[0]);
        if (selectedGo.IsValid() && selectedGo.HasComponent<ECS::TilemapComponent>())
        {
            isTilemapEditingMode = true;
        }
    }


    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (isTilemapEditingMode)
    {
        const auto& tilemap = selectedGo.GetComponent<ECS::TilemapComponent>();
        const auto& tilemapTransform = selectedGo.GetComponent<ECS::TransformComponent>();

        drawTilemapGrid(drawList, tilemapTransform, tilemap, viewportScreenPos, viewportSize);
        drawTileBrushPreview(drawList, tilemapTransform, tilemap);
    }
    else
    {
        drawEditorGrid(viewportScreenPos, viewportSize);
    }

    drawCameraGizmo(drawList);
}

void SceneViewPanel::drawTilemapGrid(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform,
                                     const ECS::TilemapComponent& tilemap, const ImVec2& viewportScreenPos,
                                     const ImVec2& viewportSize)
{
    const float zoom = m_editorCameraProperties.zoom;
    const float halfW = viewportSize.x * 0.5f / zoom;
    const float halfH = viewportSize.y * 0.5f / zoom;
    const float cx = m_editorCameraProperties.position.x();
    const float cy = m_editorCameraProperties.position.y();
    const float left = cx - halfW;
    const float right = cx + halfW;
    const float top = cy - halfH;
    const float bottom = cy + halfH;

    const float cellWidth = tilemap.cellSize.x;
    const float cellHeight = tilemap.cellSize.y;

    if (cellWidth <= 0 || cellHeight <= 0) return;

    const ImU32 gridColor = IM_COL32(255, 255, 255, 40);


    const float offsetX = 0.5f * cellWidth;
    const float offsetY = 0.5f * cellHeight;


    const float originX = tilemapTransform.position.x + offsetX;
    const float originY = tilemapTransform.position.y + offsetY;


    const float startX = originX + std::floor((left - originX) / cellWidth) * cellWidth;
    for (float x = startX; x <= right; x += cellWidth)
    {
        ImVec2 pTop = worldToScreenWith(m_editorCameraProperties, {x, top});
        drawList->AddLine(
            ImVec2(pTop.x, viewportScreenPos.y),
            ImVec2(pTop.x, viewportScreenPos.y + viewportSize.y),
            gridColor);
    }


    const float startY = originY + std::floor((top - originY) / cellHeight) * cellHeight;
    for (float y = startY; y <= bottom; y += cellHeight)
    {
        ImVec2 pLeft = worldToScreenWith(m_editorCameraProperties, {left, y});
        drawList->AddLine(
            ImVec2(viewportScreenPos.x, pLeft.y),
            ImVec2(viewportScreenPos.x + viewportSize.x, pLeft.y),
            gridColor);
    }
}

void SceneViewPanel::drawTileBrushPreview(ImDrawList* drawList, const ECS::TransformComponent& tilemapTransform,
                                          const ECS::TilemapComponent& tilemap)
{
    if (!m_context->activeTileBrush.Valid()) return;

    ECS::Vector2f worldMousePos = screenToWorldWith(m_editorCameraProperties, ImGui::GetIO().MousePos);


    ECS::Vector2f localMousePos = {
        worldMousePos.x - tilemapTransform.position.x,
        worldMousePos.y - tilemapTransform.position.y
    };


    ECS::Vector2i gridCoord = {
        static_cast<int>(std::floor(localMousePos.x / tilemap.cellSize.x + 0.5f)),
        static_cast<int>(std::floor(localMousePos.y / tilemap.cellSize.y + 0.5f))
    };


    ECS::Vector2f tileWorldPos = {
        tilemapTransform.position.x + (gridCoord.x - 0.5f) * tilemap.cellSize.x,
        tilemapTransform.position.y + (gridCoord.y - 0.5f) * tilemap.cellSize.y
    };

    ECS::Vector2f tileWorldPosEnd = {
        tilemapTransform.position.x + (gridCoord.x + 0.5f) * tilemap.cellSize.x,
        tilemapTransform.position.y + (gridCoord.y + 0.5f) * tilemap.cellSize.y
    };

    ImVec2 screenMin = worldToScreenWith(m_editorCameraProperties, tileWorldPos);
    ImVec2 screenMax = worldToScreenWith(m_editorCameraProperties, tileWorldPosEnd);

    ImU32 previewColor = ImGui::GetIO().KeyAlt ? IM_COL32(255, 80, 80, 100) : IM_COL32(80, 255, 80, 100);
    drawList->AddRectFilled(screenMin, screenMax, previewColor);
}

void SceneViewPanel::drawEditorGrid(const ImVec2& viewportScreenPos, const ImVec2& viewportSize)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();


    const float zoom = m_editorCameraProperties.zoom;
    const float halfW = viewportSize.x * 0.5f / zoom;
    const float halfH = viewportSize.y * 0.5f / zoom;
    const float cx = m_editorCameraProperties.position.x();
    const float cy = m_editorCameraProperties.position.y();
    const float left = cx - halfW;
    const float right = cx + halfW;
    const float top = cy - halfH;
    const float bottom = cy + halfH;


    float baseStep = PIXELS_PER_METER;
    float step = baseStep;
    float pxPerStep = step * zoom;
    while (pxPerStep < 16.0f)
    {
        step *= 2.0f;
        pxPerStep = step * zoom;
    }
    while (pxPerStep > 256.0f)
    {
        step *= 0.5f;
        pxPerStep = step * zoom;
    }


    const ImU32 colMinor = IM_COL32(255, 255, 255, 40);
    const ImU32 colMajor = IM_COL32(255, 255, 255, 80);
    const ImU32 colAxisX = IM_COL32(240, 100, 100, 180);
    const ImU32 colAxisY = IM_COL32(100, 180, 240, 180);
    const float thicknessMinor = 1.0f;
    const float thicknessMajor = 1.5f;
    const float thicknessAxis = 2.0f;


    const float startX = std::floor(left / step) * step;
    for (float x = startX; x <= right; x += step)
    {
        ImVec2 pTop = worldToScreenWith(m_editorCameraProperties, ECS::Vector2f{x, top});
        ImVec2 pBottom = worldToScreenWith(m_editorCameraProperties, ECS::Vector2f{x, bottom});

        bool isAxis = std::abs(x) < 1e-4f;
        bool isMajor = std::fmod(std::abs(x), step * 10.0f) < 1e-4f;
        dl->AddLine(ImVec2(pTop.x, viewportScreenPos.y),
                    ImVec2(pBottom.x, viewportScreenPos.y + viewportSize.y),
                    isAxis ? colAxisY : (isMajor ? colMajor : colMinor),
                    isAxis ? thicknessAxis : (isMajor ? thicknessMajor : thicknessMinor));
    }


    const float startY = std::floor(top / step) * step;
    for (float y = startY; y <= bottom; y += step)
    {
        ImVec2 pLeft = worldToScreenWith(m_editorCameraProperties, ECS::Vector2f{left, y});
        ImVec2 pRight = worldToScreenWith(m_editorCameraProperties, ECS::Vector2f{right, y});

        bool isAxis = std::abs(y) < 1e-4f;
        bool isMajor = std::fmod(std::abs(y), step * 10.0f) < 1e-4f;
        dl->AddLine(ImVec2(viewportScreenPos.x, pLeft.y),
                    ImVec2(viewportScreenPos.x + viewportSize.x, pRight.y),
                    isAxis ? colAxisX : (isMajor ? colMajor : colMinor),
                    isAxis ? thicknessAxis : (isMajor ? thicknessMajor : thicknessMinor));
    }
}

static ImVec2 operator-(const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

void SceneViewPanel::handleTilePainting(RuntimeGameObject& tilemapGo)
{
    if (!tilemapGo.HasComponent<ECS::TilemapComponent>()) return;

    auto& tilemap = tilemapGo.GetComponent<ECS::TilemapComponent>();
    const auto& tilemapTransform = tilemapGo.GetComponent<ECS::TransformComponent>();
    const ImGuiIO& io = ImGui::GetIO();
    ECS::Vector2f worldMousePos = screenToWorldWith(m_editorCameraProperties, io.MousePos);


    ECS::Vector2f localMousePos = {
        worldMousePos.x - tilemapTransform.position.x,
        worldMousePos.y - tilemapTransform.position.y
    };


    ECS::Vector2i gridCoord = {
        static_cast<int>(std::floor(localMousePos.x / tilemap.cellSize.x + 0.5f)),
        static_cast<int>(std::floor(localMousePos.y / tilemap.cellSize.y + 0.5f))
    };

    bool isErasing = io.KeyAlt;

    auto paintTile = [&](const ECS::Vector2i& coord)
    {
        if (m_paintedCoordsThisStroke.count(coord)) return;
        m_paintedCoordsThisStroke.insert(coord);

        if (isErasing)
        {
            tilemap.normalTiles.erase(coord);
            tilemap.ruleTiles.erase(coord);
            EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                m_context->activeScene->GetRegistry(), tilemapGo.GetEntityHandle()
            });
        }
        else
        {
            if (m_context->activeTileBrush.assetType == AssetType::RuleTile)
            {
                tilemap.ruleTiles[coord] = m_context->activeTileBrush;
                tilemap.normalTiles.erase(coord);
                EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                    m_context->activeScene->GetRegistry(), tilemapGo.GetEntityHandle()
                });
            }
            else if (m_context->activeTileBrush.assetType == AssetType::Tile)
            {
                tilemap.normalTiles[coord] = m_context->activeTileBrush;
                tilemap.ruleTiles.erase(coord);
                EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                    m_context->activeScene->GetRegistry(), tilemapGo.GetEntityHandle()
                });
            }
        }
    };

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_isPainting = true;
        m_paintedCoordsThisStroke.clear();
        m_paintStartCoord = gridCoord;
        SceneManager::GetInstance().PushUndoState(m_context->activeScene);
        paintTile(gridCoord);
    }

    if (m_isPainting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        if (io.KeyCtrl)
        {
        }
        else if (io.KeyShift)
        {
        }
        else { paintTile(gridCoord); }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (m_isPainting)
        {
            if (io.KeyCtrl)
            {
                int x1 = m_paintStartCoord.x, y1 = m_paintStartCoord.y;
                int x2 = gridCoord.x, y2 = gridCoord.y;
                int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
                int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
                int err = dx + dy, e2;
                for (;;)
                {
                    paintTile({x1, y1});
                    if (x1 == x2 && y1 == y2) break;
                    e2 = 2 * err;
                    if (e2 >= dy)
                    {
                        err += dy;
                        x1 += sx;
                    }
                    if (e2 <= dx)
                    {
                        err += dx;
                        y1 += sy;
                    }
                }
            }
            else if (io.KeyShift)
            {
                int minX = std::min(m_paintStartCoord.x, gridCoord.x);
                int maxX = std::max(m_paintStartCoord.x, gridCoord.x);
                int minY = std::min(m_paintStartCoord.y, gridCoord.y);
                int maxY = std::max(m_paintStartCoord.y, gridCoord.y);
                for (int x = minX; x <= maxX; ++x)
                {
                    for (int y = minY; y <= maxY; ++y)
                    {
                        paintTile({x, y});
                    }
                }
            }

            EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                m_context->activeScene->GetRegistry(), tilemapGo.GetEntityHandle()
            });
        }
        m_isPainting = false;
    }
}

void SceneViewPanel::handleNavigationAndPick(const ImVec2& viewportScreenPos, const ImVec2& viewportSize)
{
    const bool isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
    if (!m_context->engineContext->isSceneViewFocused || !isHovered)
    {
        m_isDragging = m_isEditingCollider = m_isPainting = false;
        m_activeColliderHandle.Reset();
        m_draggedObjects.clear();
        m_potentialDragEntity = entt::null;
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ECS::Vector2f worldMousePos = screenToWorldWith(m_editorCameraProperties, io.MousePos);


    float wheel = io.MouseWheel;
    if (wheel != 0.0f)
    {
        const ECS::Vector2f worldBeforeZoom = screenToWorldWith(m_editorCameraProperties, io.MousePos);
        const float zoomFactor = 1.1f;
        m_editorCameraProperties.zoom *= (wheel > 0.0f ? zoomFactor : 1.0f / zoomFactor);
        m_editorCameraProperties.zoom = std::clamp(m_editorCameraProperties.zoom, 0.02f, 50.0f);
        const ECS::Vector2f worldAfterZoom = screenToWorldWith(m_editorCameraProperties, io.MousePos);
        const float dx = worldBeforeZoom.x - worldAfterZoom.x;
        const float dy = worldBeforeZoom.y - worldAfterZoom.y;
        m_editorCameraProperties.position = SkPoint::Make(m_editorCameraProperties.position.x() + dx,
                                                          m_editorCameraProperties.position.y() + dy);
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
    {
        const float invZoom = 1.0f / m_editorCameraProperties.zoom;
        m_editorCameraProperties.position = SkPoint::Make(
            m_editorCameraProperties.position.x() - io.MouseDelta.x * invZoom,
            m_editorCameraProperties.position.y() - io.MouseDelta.y * invZoom);
    }


    bool isTilemapEditingMode = false;
    RuntimeGameObject selectedGo;
    if (m_context->activeTileBrush.Valid() && m_context->selectionType == SelectionType::GameObject && m_context->
        selectionList.size() == 1)
    {
        selectedGo = m_context->activeScene->FindGameObjectByGuid(m_context->selectionList[0]);
        if (selectedGo.IsValid() && selectedGo.HasComponent<ECS::TilemapComponent>())
        {
            isTilemapEditingMode = true;
        }
    }


    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (isTilemapEditingMode)
        {
            handleTilePainting(selectedGo);
        }
        else
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (!handleUIRectHandlePicking(worldMousePos) && !handleColliderHandlePicking(worldMousePos))
                {
                    m_potentialDragEntity = handleObjectPicking(worldMousePos);
                    if (m_potentialDragEntity != entt::null) { m_mouseDownScreenPos = io.MousePos; }
                }
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                if (m_potentialDragEntity != entt::null && !m_isDragging && !m_isEditingCollider && !m_isEditingUIRect)
                {
                    const float dragThresholdSq = 5.0f * 5.0f;
                    if (ImLengthSqr(io.MousePos - m_mouseDownScreenPos) > dragThresholdSq)
                    {
                        ECS::Vector2f dragStartWorldPos = screenToWorldWith(
                            m_editorCameraProperties, m_mouseDownScreenPos);
                        initiateDragging(dragStartWorldPos);
                        m_potentialDragEntity = entt::null;
                    }
                }
                if (m_isEditingCollider) { handleColliderHandleDragging(worldMousePos); }
                else if (m_isEditingUIRect) { handleUIRectHandleDragging(worldMousePos); }
                else if (m_isDragging) { handleObjectDragging(worldMousePos); }
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                if (m_isEditingCollider || m_isEditingUIRect || m_isDragging)
                {
                    SceneManager::GetInstance().PushUndoState(m_context->activeScene);
                }
                m_isEditingCollider = false;
                m_isEditingUIRect = false;
                m_activeColliderHandle.Reset();
                m_isDragging = false;
                m_draggedObjects.clear();
                m_potentialDragEntity = entt::null;
            }
        }
    }
}

entt::entity SceneViewPanel::handleObjectPicking(const ECS::Vector2f& worldMousePos)
{
    entt::entity foundEntity = entt::null;
    auto& registry = m_context->activeScene->GetRegistry();
    const ImVec2 currentMousePos = ImGui::GetIO().MousePos;

    std::vector<std::pair<entt::entity, int>> candidates;


    auto buttonView = registry.view<ECS::TransformComponent, ECS::ButtonComponent>();
    for (auto entity : buttonView)
    {
        const auto& transform = buttonView.get<ECS::TransformComponent>(entity);
        const auto& button = buttonView.get<ECS::ButtonComponent>(entity);
        if (isPointInButton(worldMousePos, transform, button))
        {
            candidates.emplace_back(entity, 2000);
        }
    }

    auto inputTextView = registry.view<ECS::TransformComponent, ECS::InputTextComponent>();
    for (auto entity : inputTextView)
    {
        const auto& transform = inputTextView.get<ECS::TransformComponent>(entity);
        const auto& inputText = inputTextView.get<ECS::InputTextComponent>(entity);
        const ECS::TextComponent& displayText = (!inputText.text.text.empty() || inputText.isFocused)
                                                    ? inputText.text
                                                    : inputText.placeholder;
        if (isPointInText(worldMousePos, transform, displayText))
        {
            candidates.emplace_back(entity, 2000);
        }
    }

    auto spriteView = registry.view<ECS::TransformComponent, ECS::SpriteComponent>();
    for (auto entity : spriteView)
    {
        const auto& sprite = spriteView.get<ECS::SpriteComponent>(entity);
        if (sprite.image)
        {
            const auto& transform = spriteView.get<ECS::TransformComponent>(entity);
            if (IsPointInSprite(worldMousePos, transform, sprite))
            {
                candidates.emplace_back(entity, sprite.zIndex + 1000);
            }
        }
    }


    auto listView = registry.view<ECS::TransformComponent, ECS::ListBoxComponent>();
    for (auto entity : listView)
    {
        const auto& transform = listView.get<ECS::TransformComponent>(entity);
        const auto& listBox = listView.get<ECS::ListBoxComponent>(entity);
        if (isPointInUIRect(worldMousePos, transform, listBox.rect.Width(), listBox.rect.Height()))
        {
            candidates.emplace_back(entity, listBox.zIndex + 1500);
        }
    }

    auto textView = registry.view<ECS::TransformComponent, ECS::TextComponent>();
    for (auto entity : textView)
    {
        if (registry.any_of<ECS::InputTextComponent>(entity)) continue;
        const auto& textComp = textView.get<ECS::TextComponent>(entity);
        const auto& transform = textView.get<ECS::TransformComponent>(entity);
        if (isPointInText(worldMousePos, transform, textComp))
        {
            candidates.emplace_back(entity, textComp.zIndex + 1000);
        }
    }

    auto emptyView = registry.view<ECS::TransformComponent>();
    for (auto entity : emptyView)
    {
        if (registry.any_of<ECS::SpriteComponent, ECS::TextComponent, ECS::InputTextComponent,
                            ECS::ButtonComponent>(entity))
            continue;
        const auto& transform = emptyView.get<ECS::TransformComponent>(entity);
        if (isPointInEmptyObject(worldMousePos, transform))
        {
            candidates.emplace_back(entity, 0);
        }
    }

    if (!candidates.empty())
    {
        std::ranges::sort(candidates, [](const auto& a, const auto& b)
        {
            if (a.second != b.second) return a.second > b.second;
            return a.first > b.first;
        });

        std::vector<entt::entity> currentPickCandidates;
        for (const auto& pair : candidates)
        {
            currentPickCandidates.push_back(pair.first);
        }

        const float clickTolerance = 2.0f * 2.0f;
        bool isSameLocation = ImLengthSqr(currentMousePos - m_lastPickScreenPos) < clickTolerance;

        if (!isSameLocation || currentPickCandidates != m_lastPickCandidates)
        {
            m_currentPickIndex = 0;
            m_lastPickCandidates = std::move(currentPickCandidates);
        }
        else
        {
            m_currentPickIndex = (m_currentPickIndex + 1) % m_lastPickCandidates.size();
        }

        foundEntity = m_lastPickCandidates[m_currentPickIndex];
    }

    m_lastPickScreenPos = currentMousePos;

    bool ctrlPressed = ImGui::GetIO().KeyCtrl;
    bool shiftPressed = ImGui::GetIO().KeyShift;

    if (foundEntity != entt::null)
    {
        Guid clickedGuid = registry.get<ECS::IDComponent>(foundEntity).guid;

        if (m_currentPickIndex > 0)
        {
            selectSingleObject(clickedGuid);
        }
        else
        {
            if (shiftPressed && m_context->selectionAnchor.Valid())
            {
                selectSingleObject(clickedGuid);
            }
            else if (ctrlPressed)
            {
                toggleObjectSelection(clickedGuid);
            }
            else
            {
                bool isAlreadySelected = false;
                if (m_context->selectionList.size() == 1 && m_context->selectionList[0] == clickedGuid)
                {
                    isAlreadySelected = true;
                }

                if (!isAlreadySelected)
                {
                    selectSingleObject(clickedGuid);
                }
            }
        }
    }
    else
    {
        if (!ctrlPressed && !shiftPressed)
        {
            clearSelection();
        }
    }

    return foundEntity;
}

void SceneViewPanel::handleObjectDragging(const ECS::Vector2f& worldMousePos)
{
    if (!m_isDragging || m_draggedObjects.empty())
        return;


    for (auto& draggedObj : m_draggedObjects)
    {
        RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(draggedObj.guid);
        if (!gameObject.IsValid()) continue;

        auto& transform = gameObject.GetComponent<ECS::TransformComponent>();
        ECS::Vector2f newWorldPosition = worldMousePos + draggedObj.dragOffset;

        if (gameObject.HasComponent<ECS::ParentComponent>())
        {
            auto& parentComponent = gameObject.GetComponent<ECS::ParentComponent>();
            RuntimeGameObject parentGO = m_context->activeScene->FindGameObjectByEntity(parentComponent.parent);

            if (parentGO.IsValid())
            {
                auto& parentTransform = parentGO.GetComponent<ECS::TransformComponent>();


                transform.localPosition = {
                    newWorldPosition.x - parentTransform.position.x,
                    newWorldPosition.y - parentTransform.position.y
                };
            }
        }
        else
        {
            transform.position = newWorldPosition;
        }
    }
}

void SceneViewPanel::initiateDragging(const ECS::Vector2f& worldMousePos)
{
    m_isDragging = true;
    m_draggedObjects.clear();


    for (const auto& selectedGuid : m_context->selectionList)
    {
        RuntimeGameObject gameObject = m_context->activeScene->FindGameObjectByGuid(selectedGuid);
        if (gameObject.IsValid() && gameObject.HasComponent<ECS::TransformComponent>())
        {
            const auto& transform = gameObject.GetComponent<ECS::TransformComponent>();

            DraggedObject draggedObj;
            draggedObj.guid = selectedGuid;
            draggedObj.dragOffset = transform.position - worldMousePos;

            m_draggedObjects.push_back(draggedObj);
        }
    }
}

bool SceneViewPanel::handleColliderHandlePicking(const ECS::Vector2f& worldMousePos)
{
    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    for (const auto& handle : std::ranges::reverse_view(m_colliderHandles))
    {
        const float distSq = ImLengthSqr(ImVec2(mousePos.x - handle.screenPosition.x,
                                                mousePos.y - handle.screenPosition.y));
        const float radiusSq = handle.radius * handle.radius * 2.25f;

        if (distSq <= radiusSq)
        {
            m_isEditingCollider = true;
            m_activeColliderHandle.entityGuid = handle.entityGuid;
            m_activeColliderHandle.handleIndex = handle.handleIndex;

            RuntimeGameObject go = m_context->activeScene->FindGameObjectByGuid(handle.entityGuid);
            if (go.IsValid() && go.HasComponent<ECS::BoxColliderComponent>())
            {
                const auto& transform = go.GetComponent<ECS::TransformComponent>();
                const auto& boxCollider = go.GetComponent<ECS::BoxColliderComponent>();


                const float halfWidth = boxCollider.size.x * 0.5f;
                const float halfHeight = boxCollider.size.y * 0.5f;
                std::vector<ECS::Vector2f> localHandles = {
                    {-halfWidth, -halfHeight}, {0, -halfHeight}, {halfWidth, -halfHeight}, {halfWidth, 0},
                    {halfWidth, halfHeight}, {0, halfHeight}, {-halfWidth, halfHeight}, {-halfWidth, 0}
                };


                const int clickedIndex = handle.handleIndex;
                const int oppositeIndex = (clickedIndex + 4) % 8;

                auto calculateWorldPos = [&](const ECS::Vector2f& localPos) -> ECS::Vector2f
                {
                    ECS::Vector2f finalPos = localPos + boxCollider.offset;
                    finalPos.x *= transform.scale.x;
                    finalPos.y *= transform.scale.y;

                    if (std::abs(transform.rotation) > 0.001f)
                    {
                        const float sinR = sinf(transform.rotation);
                        const float cosR = cosf(transform.rotation);
                        const float tempX = finalPos.x;
                        finalPos.x = finalPos.x * cosR - finalPos.y * sinR;
                        finalPos.y = tempX * sinR + finalPos.y * cosR;
                    }
                    return transform.position + finalPos;
                };

                ECS::Vector2f clickedHandleWorldPos = calculateWorldPos(localHandles[clickedIndex]);
                m_activeColliderHandle.fixedPointWorldPos = calculateWorldPos(localHandles[oppositeIndex]);
                m_activeColliderHandle.dragOffset = clickedHandleWorldPos - worldMousePos;
            }

            return true;
        }
    }

    return false;
}

void SceneViewPanel::handleColliderHandleDragging(const ECS::Vector2f& worldMousePos)
{
    if (!m_activeColliderHandle.IsValid()) return;

    RuntimeGameObject go = m_context->activeScene->FindGameObjectByGuid(m_activeColliderHandle.entityGuid);
    if (!go.IsValid() || !go.HasComponent<ECS::BoxColliderComponent>()) return;

    auto& transform = go.GetComponent<ECS::TransformComponent>();
    auto& boxCollider = go.GetComponent<ECS::BoxColliderComponent>();
    auto& handle = m_activeColliderHandle;

    const ECS::Vector2f effectiveHandlePos = worldMousePos + handle.dragOffset;
    const float rot = transform.rotation;
    const float cosR = cosf(rot);
    const float sinR = sinf(rot);

    ECS::Vector2f newWorldCenter;


    if (handle.handleIndex % 2 == 0)
    {
        const ECS::Vector2f& fixedCorner = handle.fixedPointWorldPos;
        newWorldCenter = (effectiveHandlePos + fixedCorner) * 0.5f;
        ECS::Vector2f diagVecWorld = effectiveHandlePos - fixedCorner;


        const float worldWidth = std::abs(diagVecWorld.x * cosR + diagVecWorld.y * sinR);
        const float worldHeight = std::abs(diagVecWorld.x * -sinR + diagVecWorld.y * cosR);


        if (std::abs(transform.scale.x) > 1e-5f) boxCollider.size.x = worldWidth / std::abs(transform.scale.x);
        if (std::abs(transform.scale.y) > 1e-5f) boxCollider.size.y = worldHeight / std::abs(transform.scale.y);
    }
    else
    {
        const ECS::Vector2f& fixedPoint = handle.fixedPointWorldPos;
        const ECS::Vector2f localXAxis = {cosR, sinR};
        const ECS::Vector2f localYAxis = {-sinR, cosR};

        ECS::Vector2f delta = effectiveHandlePos - fixedPoint;


        if (handle.handleIndex == 1 || handle.handleIndex == 5)
        {
            float newWorldHeight = std::abs(delta.Dot(localYAxis));
            newWorldCenter = fixedPoint + localYAxis * (delta.Dot(localYAxis) / 2.0f);
            if (std::abs(transform.scale.y) > 1e-5f) boxCollider.size.y = newWorldHeight / std::abs(transform.scale.y);
        }

        else
        {
            float newWorldWidth = std::abs(delta.Dot(localXAxis));
            newWorldCenter = fixedPoint + localXAxis * (delta.Dot(localXAxis) / 2.0f);
            if (std::abs(transform.scale.x) > 1e-5f) boxCollider.size.x = newWorldWidth / std::abs(transform.scale.x);
        }
    }


    ECS::Vector2f offsetInWorld = newWorldCenter - transform.position;


    const float invCosR = cosf(-rot);
    const float invSinR = sinf(-rot);
    ECS::Vector2f localOffsetScaled;
    localOffsetScaled.x = offsetInWorld.x * invCosR - offsetInWorld.y * invSinR;
    localOffsetScaled.y = offsetInWorld.x * invSinR + offsetInWorld.y * invCosR;


    if (std::abs(transform.scale.x) > 1e-5f)
    {
        boxCollider.offset.x = localOffsetScaled.x / transform.scale.x;
    }
    if (std::abs(transform.scale.y) > 1e-5f)
    {
        boxCollider.offset.y = localOffsetScaled.y / transform.scale.y;
    }
}

bool SceneViewPanel::handleUIRectHandlePicking(const ECS::Vector2f& worldMousePos)
{
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    for (auto it = m_uiRectHandles.rbegin(); it != m_uiRectHandles.rend(); ++it)
    {
        const float dx = mousePos.x - it->screenPosition.x;
        const float dy = mousePos.y - it->screenPosition.y;
        const float distSq = dx * dx + dy * dy;
        const float rSq = it->size * it->size * 1.5f;
        if (distSq <= rSq)
        {
            m_isEditingUIRect = true;
            m_activeUIRectEntity = it->entityGuid;
            return true;
        }
    }
    return false;
}

void SceneViewPanel::handleUIRectHandleDragging(const ECS::Vector2f& worldMousePos)
{
    if (!m_activeUIRectEntity.Valid()) return;
    RuntimeGameObject go = m_context->activeScene->FindGameObjectByGuid(m_activeUIRectEntity);
    if (!go.IsValid() || !go.HasComponent<ECS::TransformComponent>()) return;
    auto& transform = go.GetComponent<ECS::TransformComponent>();

    auto applyResize = [&](auto& uiComp)
    {
        ECS::Vector2f delta = worldMousePos - transform.position;
        float newW = std::max(1.0f, std::abs(delta.x) * 2.0f);
        float newH = std::max(1.0f, std::abs(delta.y) * 2.0f);
        uiComp.rect.z = newW;
        uiComp.rect.w = newH;
    };

    if (go.HasComponent<ECS::ListBoxComponent>()) applyResize(go.GetComponent<ECS::ListBoxComponent>());
    else if (go.HasComponent<ECS::ButtonComponent>()) applyResize(go.GetComponent<ECS::ButtonComponent>());
    else if (go.HasComponent<ECS::InputTextComponent>()) applyResize(go.GetComponent<ECS::InputTextComponent>());
    else if (go.HasComponent<ECS::ToggleButtonComponent>()) applyResize(go.GetComponent<ECS::ToggleButtonComponent>());
    else if (go.HasComponent<ECS::RadioButtonComponent>()) applyResize(go.GetComponent<ECS::RadioButtonComponent>());
    else if (go.HasComponent<ECS::CheckBoxComponent>()) applyResize(go.GetComponent<ECS::CheckBoxComponent>());
    else if (go.HasComponent<ECS::SliderComponent>()) applyResize(go.GetComponent<ECS::SliderComponent>());
    else if (go.HasComponent<ECS::ComboBoxComponent>()) applyResize(go.GetComponent<ECS::ComboBoxComponent>());
    else if (go.HasComponent<ECS::ExpanderComponent>()) applyResize(go.GetComponent<ECS::ExpanderComponent>());
    else if (go.HasComponent<ECS::ProgressBarComponent>()) applyResize(go.GetComponent<ECS::ProgressBarComponent>());
    else if (go.HasComponent<ECS::TabControlComponent>()) applyResize(go.GetComponent<ECS::TabControlComponent>());
}

void SceneViewPanel::selectSingleObject(const Guid& objectGuid)
{
    m_context->selectionType = SelectionType::GameObject;
    m_context->selectionList.clear();
    m_context->selectionList.push_back(objectGuid);
    m_context->selectionAnchor = objectGuid;
}

void SceneViewPanel::toggleObjectSelection(const Guid& objectGuid)
{
    auto it = std::find(m_context->selectionList.begin(), m_context->selectionList.end(), objectGuid);
    if (it != m_context->selectionList.end())
    {
        m_context->selectionList.erase(it);
        if (m_context->selectionList.empty())
        {
            m_context->selectionType = SelectionType::NA;
            m_context->selectionAnchor = Guid();
        }
    }
    else
    {
        m_context->selectionList.push_back(objectGuid);
        m_context->selectionType = SelectionType::GameObject;
        if (!m_context->selectionAnchor.Valid())
        {
            m_context->selectionAnchor = objectGuid;
        }
    }
}

void SceneViewPanel::clearSelection()
{
    m_context->selectionType = SelectionType::NA;
    m_context->selectionList.clear();
    m_context->selectionAnchor = Guid();


    m_lastPickCandidates.clear();
    m_currentPickIndex = -1;
}

ECS::Vector2f SceneViewPanel::screenToWorldWith(const Camera::CamProperties& props, const ImVec2& screenPos) const
{
    const float localX = screenPos.x - props.viewport.x();
    const float localY = screenPos.y - props.viewport.y();


    const float worldX = (localX - props.viewport.width() * 0.5f) / props.zoom + props.position.x();
    const float worldY = (localY - props.viewport.height() * 0.5f) / props.zoom + props.position.y();
    return {worldX, worldY};
}

ImVec2 SceneViewPanel::worldToScreenWith(const Camera::CamProperties& props, const ECS::Vector2f& worldPos) const
{
    const float localX = (worldPos.x - props.position.x()) * props.zoom + props.viewport.width() * 0.5f;
    const float localY = (worldPos.y - props.position.y()) * props.zoom + props.viewport.height() * 0.5f;


    const float screenX = localX + props.viewport.x();
    const float screenY = localY + props.viewport.y();
    return ImVec2(screenX, screenY);
}


void SceneViewPanel::drawCameraGizmo(ImDrawList* drawList)
{
    if (!m_context->activeScene)
    {
        return;
    }


    const auto& gameCamProps = m_context->activeScene->GetCameraProperties();


    if (gameCamProps.zoom <= 0.0f) return;
    const float worldViewWidth = m_context->engineContext->sceneViewRect.Width() / gameCamProps.zoom;
    const float worldViewHeight = m_context->engineContext->sceneViewRect.Height() / gameCamProps.zoom;
    const float halfWorldW = worldViewWidth * 0.5f;
    const float halfWorldH = worldViewHeight * 0.5f;


    std::vector<ECS::Vector2f> localCorners = {
        {-halfWorldW, -halfWorldH},
        {halfWorldW, -halfWorldH},
        {halfWorldW, halfWorldH},
        {-halfWorldW, halfWorldH}
    };


    std::vector<ECS::Vector2f> worldCorners;
    worldCorners.reserve(4);
    const float sinR = sinf(gameCamProps.rotation);
    const float cosR = cosf(gameCamProps.rotation);

    for (const auto& corner : localCorners)
    {
        const float rotatedX = corner.x * cosR - corner.y * sinR;
        const float rotatedY = corner.x * sinR + corner.y * cosR;


        worldCorners.emplace_back(
            gameCamProps.position.x() + rotatedX,
            gameCamProps.position.y() + rotatedY
        );
    }


    std::vector<ImVec2> screenCorners;
    screenCorners.reserve(4);
    for (const auto& worldCorner : worldCorners)
    {
        screenCorners.push_back(worldToScreenWith(m_editorCameraProperties, worldCorner));
    }


    const ImU32 gizmoColor = IM_COL32(255, 255, 255, 150);
    const float thickness = 2.0f;
    drawList->AddPolyline(screenCorners.data(), 4, gizmoColor, ImDrawFlags_Closed, thickness);
}

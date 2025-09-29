#include "InteractionSystem.h"

#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Data/EngineContext.h"
#include "../Components/InteractionEvents.h"
#include "../Components/Transform.h"
#include "../Components/Sprite.h"
#include "../Components/UIComponents.h"
#include <algorithm>
#include <ranges>

#include "SceneManager.h"
#include "Input/Cursor.h"

namespace Systems
{
    static ECS::Vector2f screenToWorldCentered(const ECS::Vector2f& globalScreenPos,
                                               const Camera::CamProperties& cameraProps,
                                               const ECS::RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;


        const float centeredX = localX - (viewport.z / 2.0f);
        const float centeredY = localY - (viewport.w / 2.0f);


        const float unzoomedX = centeredX / cameraProps.zoom;
        const float unzoomedY = centeredY / cameraProps.zoom;


        const float worldX = unzoomedX + cameraProps.position.x();
        const float worldY = unzoomedY + cameraProps.position.y();

        return {worldX, worldY};
    }


    static bool isPointInSprite(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                                const ECS::SpriteComponent& sprite)
    {
        const float halfWidth = (sprite.sourceRect.Width() > 0
                                     ? sprite.sourceRect.Width()
                                     : sprite.image->getImage()->width()) *
            0.5f;
        const float halfHeight = (sprite.sourceRect.Height() > 0
                                      ? sprite.sourceRect.Height()
                                      : sprite.image->getImage()->height())
            * 0.5f;

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
        localPoint.x /= transform.scale.x;
        localPoint.y /= transform.scale.y;


        return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
            localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
    }


    static bool isPointInRectUI(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                                const ECS::Vector2f& size)
    {
        const float halfWidth = size.x * 0.5f;
        const float halfHeight = size.y * 0.5f;

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
        localPoint.x /= transform.scale.x;
        localPoint.y /= transform.scale.y;


        return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
            localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
    }

    void InteractionSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        m_hoveredEntity = entt::null;
        m_pressedEntity = entt::null;
        m_wasLeftMouseDownLastFrame = false;
    }

    void InteractionSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();


        registry.clear<PointerEnterEvent, PointerExitEvent, PointerDownEvent, PointerUpEvent, PointerClickEvent>();


        if (context.appMode == ApplicationMode::Editor)
        {
            return;
        }

        ECS::Vector2f worldMousePos;
        bool canInteract = false;

        const auto& input = context.inputState;
        ECS::Vector2f globalMousePos = {(float)input.mousePosition.x, (float)input.mousePosition.y};
        auto cameraProps = scene->GetCameraProperties();
        ECS::RectF viewportRect;

        if (context.appMode == ApplicationMode::PIE)
        {
            viewportRect = context.sceneViewRect;


            if (globalMousePos.x >= viewportRect.x && globalMousePos.x <= (viewportRect.x + viewportRect.z) &&
                globalMousePos.y >= viewportRect.y && globalMousePos.y <= (viewportRect.y + viewportRect.w))
            {
                canInteract = true;
            }
        }
        else
        {
            viewportRect = {0.0f, 0.0f, cameraProps.viewport.width(), cameraProps.viewport.height()};
            canInteract = true;
        }

        entt::entity currentHoveredEntity = entt::null;
        if (canInteract)
        {
            worldMousePos = screenToWorldCentered(globalMousePos, cameraProps, viewportRect);
            currentHoveredEntity = performGeometricPicking(registry, worldMousePos);
        }


        handleHoverEvents(registry, currentHoveredEntity);
        handleClickEvents(registry, input);
    }

    entt::entity InteractionSystem::performGeometricPicking(entt::registry& registry,
                                                            const ECS::Vector2f& worldMousePos)
    {
        std::vector<std::pair<entt::entity, int>> candidates;


        auto buttonView = registry.view<ECS::TransformComponent, ECS::ButtonComponent>();
        for (auto entity : buttonView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& button = buttonView.get<ECS::ButtonComponent>(entity);
            if (!button.Enable) continue;
            const auto& transform = buttonView.get<ECS::TransformComponent>(entity);
            ECS::Vector2f buttonSize = {100.0f, 30.0f};
            if (auto* sprite = registry.try_get<ECS::SpriteComponent>(entity))
            {
                if (sprite->image)
                {
                    buttonSize.x = sprite->sourceRect.Width() > 0
                                       ? sprite->sourceRect.Width()
                                       : sprite->image->getImage()->width();
                    buttonSize.y = sprite->sourceRect.Height() > 0
                                       ? sprite->sourceRect.Height()
                                       : sprite->image->getImage()->height();
                }
            }
            if (isPointInRectUI(worldMousePos, transform, buttonSize))
            {
                candidates.emplace_back(entity, 1000);
            }
        }

        auto inputTextView = registry.view<ECS::TransformComponent, ECS::InputTextComponent>();
        for (auto entity : inputTextView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& inputText = inputTextView.get<ECS::InputTextComponent>(entity);
            if (!inputText.Enable) continue;
            const auto& transform = inputTextView.get<ECS::TransformComponent>(entity);
            ECS::Vector2f inputSize = {200.0f, 25.0f};
            if (auto* sprite = registry.try_get<ECS::SpriteComponent>(entity))
            {
                if (sprite->image)
                {
                    inputSize.x = sprite->sourceRect.Width() > 0
                                      ? sprite->sourceRect.Width()
                                      : sprite->image->getImage()->width();
                    inputSize.y = sprite->sourceRect.Height() > 0
                                      ? sprite->sourceRect.Height()
                                      : sprite->image->getImage()->height();
                }
            }
            if (isPointInRectUI(worldMousePos, transform, inputSize))
            {
                candidates.emplace_back(entity, 1000);
            }
        }



        auto spriteView = registry.view<ECS::TransformComponent, ECS::SpriteComponent>();
        for (auto entity : spriteView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& sprite = spriteView.get<ECS::SpriteComponent>(entity);
            if (!sprite.image->getImage()) continue;
            const auto& transform = spriteView.get<ECS::TransformComponent>(entity);
            if (isPointInSprite(worldMousePos, transform, sprite))
            {
                candidates.emplace_back(entity, sprite.zIndex);
            }
        }

        if (candidates.empty())
        {
            return entt::null;
        }


        std::ranges::sort(candidates, [](const auto& a, const auto& b)
        {
            if (a.second != b.second)
            {
                return a.second > b.second;
            }
            return a.first > b.first;
        });

        return candidates[0].first;
    }

    void InteractionSystem::handleHoverEvents(entt::registry& registry, entt::entity currentHoveredEntity)
    {
        if (currentHoveredEntity != m_hoveredEntity)
        {
            if (registry.valid(m_hoveredEntity))
            {
                registry.emplace<PointerExitEvent>(m_hoveredEntity);
            }
            if (registry.valid(currentHoveredEntity))
            {
                registry.emplace<PointerEnterEvent>(currentHoveredEntity);
            }
            m_hoveredEntity = currentHoveredEntity;
        }
    }

    void InteractionSystem::handleClickEvents(entt::registry& registry, const InputState& inputState)
    {
        bool isLeftMouseDownThisFrame = inputState.isLeftMouseDown;

        if (isLeftMouseDownThisFrame && !m_wasLeftMouseDownLastFrame)
        {
            if (registry.valid(m_hoveredEntity))
            {
                registry.emplace<PointerDownEvent>(m_hoveredEntity);
                m_pressedEntity = m_hoveredEntity;
            }
        }
        else if (!isLeftMouseDownThisFrame && m_wasLeftMouseDownLastFrame)
        {
            if (registry.valid(m_pressedEntity))
            {
                registry.emplace<PointerUpEvent>(m_pressedEntity);
            }


            if (registry.valid(m_hoveredEntity) && m_hoveredEntity == m_pressedEntity)
            {
                registry.emplace<PointerClickEvent>(m_hoveredEntity);
            }

            m_pressedEntity = entt::null;
        }

        m_wasLeftMouseDownLastFrame = isLeftMouseDownThisFrame;
    }

    ECS::Vector2f InteractionSystem::screenToWorld(const ECS::Vector2f& globalScreenPos,
                                                   const Camera::CamProperties& cameraProps,
                                                   const ECS::RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;

        const float worldX = (localX / cameraProps.zoom) + cameraProps.position.x();
        const float worldY = (localY / cameraProps.zoom) + cameraProps.position.y();

        return {worldX, worldY};
    }
}

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
#include "Window.h"
#include "Input/Cursor.h"

namespace
{
    
    static ECS::Vector2f screenToWorldCentered(const ECS::Vector2f& globalScreenPos,
                                               const Camera::CamProperties& cameraProps,
                                               const ECS::RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;

        const float centeredX = localX - (viewport.z / 2.0f);
        const float centeredY = localY - (viewport.w / 2.0f);

        SkPoint effectiveZoom = cameraProps.GetEffectiveZoom();
        const float unzoomedX = centeredX / effectiveZoom.x();
        const float unzoomedY = centeredY / effectiveZoom.y();

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
}

namespace Systems
{
    void InteractionSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        m_hoveredEntity = entt::null;
        m_pressedEntity = entt::null;
        m_wasLeftMouseDownLastFrame = false;

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        m_touchPoints.clear();
#endif
    }

    void InteractionSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();

        
        registry.clear<PointerEnterEvent, PointerExitEvent, PointerDownEvent, PointerUpEvent, PointerClickEvent>();

        
        if (*context.appMode == ApplicationMode::Editor)
        {
            return;
        }

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        
        handleMultiTouchEvents(scene, context);
#else
        
        ECS::Vector2f worldMousePos;
        bool canInteract = false;

        const auto& input = context.inputState;
        ECS::Vector2f globalMousePos = {(float)input.mousePosition.x, (float)input.mousePosition.y};
        auto cameraProps = scene->GetCameraProperties();
        ECS::RectF viewportRect;

        
        if (*context.appMode == ApplicationMode::PIE)
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

        handleHoverEvents(registry, currentHoveredEntity, m_hoveredEntity);
        m_hoveredEntity = currentHoveredEntity;
        handleMouseClickEvents(registry, input);
#endif
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
            const ECS::Vector2f buttonSize = {button.rect.z, button.rect.w};

            if (isPointInRectUI(worldMousePos, transform, buttonSize))
            {
                candidates.emplace_back(entity, button.zIndex);
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
            const ECS::Vector2f inputSize = {inputText.rect.z, inputText.rect.w};

            if (isPointInRectUI(worldMousePos, transform, inputSize))
            {
                candidates.emplace_back(entity, inputText.zIndex);
            }
        }

        auto toggleView = registry.view<ECS::TransformComponent, ECS::ToggleButtonComponent>();
        for (auto entity : toggleView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& toggle = toggleView.get<ECS::ToggleButtonComponent>(entity);
            if (!toggle.Enable || !toggle.isVisible) continue;

            const auto& transform = toggleView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {toggle.rect.z, toggle.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, toggle.zIndex);
            }
        }

        auto radioView = registry.view<ECS::TransformComponent, ECS::RadioButtonComponent>();
        for (auto entity : radioView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& radio = radioView.get<ECS::RadioButtonComponent>(entity);
            if (!radio.Enable || !radio.isVisible) continue;

            const auto& transform = radioView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {radio.rect.z, radio.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, radio.zIndex);
            }
        }

        auto checkboxView = registry.view<ECS::TransformComponent, ECS::CheckBoxComponent>();
        for (auto entity : checkboxView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& checkbox = checkboxView.get<ECS::CheckBoxComponent>(entity);
            if (!checkbox.Enable || !checkbox.isVisible) continue;

            const auto& transform = checkboxView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {checkbox.rect.z, checkbox.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, checkbox.zIndex);
            }
        }

        auto sliderView = registry.view<ECS::TransformComponent, ECS::SliderComponent>();
        for (auto entity : sliderView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& slider = sliderView.get<ECS::SliderComponent>(entity);
            if (!slider.Enable || !slider.isVisible) continue;

            const auto& transform = sliderView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {slider.rect.z, slider.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, slider.zIndex);
            }
        }

        auto comboView = registry.view<ECS::TransformComponent, ECS::ComboBoxComponent>();
        for (auto entity : comboView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& combo = comboView.get<ECS::ComboBoxComponent>(entity);
            if (!combo.Enable || !combo.isVisible) continue;

            const auto& transform = comboView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f baseSize = {combo.rect.z, combo.rect.w};
            bool inside = isPointInRectUI(worldMousePos, transform, baseSize);

            if (!inside && combo.isDropdownOpen && !combo.items.empty())
            {
                const float itemHeight = std::max(combo.displayText.fontSize * 1.4f, combo.rect.w);
                const float additionalHeight = itemHeight * static_cast<float>(combo.items.size());
                ECS::TransformComponent adjustedTransform = transform;
                adjustedTransform.position.y += additionalHeight * 0.5f;
                const ECS::Vector2f dropdownSize = {combo.rect.z, combo.rect.w + additionalHeight};
                inside = isPointInRectUI(worldMousePos, adjustedTransform, dropdownSize);
            }

            if (inside)
            {
                candidates.emplace_back(entity, combo.zIndex);
            }
        }

        auto expanderView = registry.view<ECS::TransformComponent, ECS::ExpanderComponent>();
        for (auto entity : expanderView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& expander = expanderView.get<ECS::ExpanderComponent>(entity);
            if (!expander.Enable || !expander.isVisible) continue;

            const auto& transform = expanderView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {expander.rect.z, expander.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, expander.zIndex);
            }
        }

        auto tabView = registry.view<ECS::TransformComponent, ECS::TabControlComponent>();
        for (auto entity : tabView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& tabControl = tabView.get<ECS::TabControlComponent>(entity);
            if (!tabControl.Enable || !tabControl.isVisible) continue;

            const auto& transform = tabView.get<ECS::TransformComponent>(entity);
            ECS::Vector2f localPoint = worldMousePos - transform.position;
            if (transform.rotation != 0.0f)
            {
                const float sinR = sinf(-transform.rotation);
                const float cosR = cosf(-transform.rotation);
                const float tempX = localPoint.x;
                localPoint.x = localPoint.x * cosR - localPoint.y * sinR;
                localPoint.y = tempX * sinR + localPoint.y * cosR;
            }
            localPoint.x /= transform.scale.x;
            localPoint.y /= transform.scale.y;

            const float halfWidth = tabControl.rect.z * 0.5f;
            const float halfHeight = tabControl.rect.w * 0.5f;
            const float tabsTop = -halfHeight;
            const float tabsBottom = tabsTop + tabControl.tabHeight;

            if (localPoint.y >= tabsTop && localPoint.y <= tabsBottom &&
                localPoint.x >= -halfWidth && localPoint.x <= halfWidth)
            {
                candidates.emplace_back(entity, tabControl.zIndex);
            }
        }

        auto listBoxView = registry.view<ECS::TransformComponent, ECS::ListBoxComponent>();
        for (auto entity : listBoxView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& listBox = listBoxView.get<ECS::ListBoxComponent>(entity);
            if (!listBox.Enable || !listBox.isVisible) continue;

            const auto& transform = listBoxView.get<ECS::TransformComponent>(entity);
            const ECS::Vector2f size = {listBox.rect.z, listBox.rect.w};
            if (isPointInRectUI(worldMousePos, transform, size))
            {
                candidates.emplace_back(entity, listBox.zIndex);
            }
        }

        
        auto spriteView = registry.view<ECS::TransformComponent, ECS::SpriteComponent>();
        for (auto entity : spriteView)
        {
            
            if (registry.any_of<ECS::ButtonComponent, ECS::InputTextComponent, ECS::ToggleButtonComponent,
                                ECS::RadioButtonComponent, ECS::CheckBoxComponent, ECS::SliderComponent,
                                ECS::ComboBoxComponent,
                                ECS::ExpanderComponent, ECS::TabControlComponent, ECS::ListBoxComponent>(entity))
            {
                continue;
            }

            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& sprite = spriteView.get<ECS::SpriteComponent>(entity);
            if (!sprite.image || !sprite.image->getImage()) continue;
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

    void InteractionSystem::handleHoverEvents(entt::registry& registry, entt::entity currentHoveredEntity,
                                              entt::entity previousHoveredEntity)
    {
        if (currentHoveredEntity != previousHoveredEntity)
        {
            
            if (registry.valid(previousHoveredEntity))
            {
                registry.emplace<PointerExitEvent>(previousHoveredEntity);
            }
            
            if (registry.valid(currentHoveredEntity))
            {
                registry.emplace<PointerEnterEvent>(currentHoveredEntity);
            }
        }
    }

    void InteractionSystem::handleMouseClickEvents(entt::registry& registry, const InputState& inputState)
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

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    void InteractionSystem::handleMultiTouchEvents(RuntimeScene* scene, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        auto cameraProps = scene->GetCameraProperties();

        
        int windowWidth, windowHeight;
        SDL_GetWindowSizeInPixels(context.window->GetSdlWindow(), &windowWidth, &windowHeight);

        ECS::RectF viewportRect = {0.0f, 0.0f, (float)windowWidth, (float)windowHeight};

        
        const auto& activeTouches = context.window->GetActiveTouches();

        
        for (const auto& [fingerId, touchPoint] : activeTouches)
        {
            
            ECS::Vector2f screenPos = {
                touchPoint.x * windowWidth,
                touchPoint.y * windowHeight
            };

            
            ECS::Vector2f worldPos = screenToWorldCentered(screenPos, cameraProps, viewportRect);

            
            entt::entity pickedEntity = performGeometricPicking(registry, worldPos);

            
            auto it = m_touchPoints.find(fingerId);
            if (it == m_touchPoints.end())
            {
                
                TouchPointInfo info;
                info.position = worldPos;
                info.hoveredEntity = pickedEntity;
                info.pressedEntity = entt::null;

                
                if (registry.valid(pickedEntity))
                {
                    registry.emplace<PointerEnterEvent>(pickedEntity);
                    registry.emplace<PointerDownEvent>(pickedEntity);
                    info.pressedEntity = pickedEntity;
                }

                m_touchPoints[fingerId] = info;
            }
            else
            {
                
                TouchPointInfo& info = it->second;
                entt::entity previousHoveredEntity = info.hoveredEntity;

                
                info.position = worldPos;

                
                if (pickedEntity != previousHoveredEntity)
                {
                    
                    if (registry.valid(previousHoveredEntity))
                    {
                        registry.emplace<PointerExitEvent>(previousHoveredEntity);

                        
                        if (previousHoveredEntity == info.pressedEntity)
                        {
                            registry.emplace<PointerUpEvent>(previousHoveredEntity);
                            info.pressedEntity = entt::null;
                        }
                    }

                    
                    if (registry.valid(pickedEntity))
                    {
                        registry.emplace<PointerEnterEvent>(pickedEntity);
                    }

                    info.hoveredEntity = pickedEntity;
                }
            }
        }

        
        std::vector<SDL_FingerID> fingersToRemove;
        for (const auto& [fingerId, info] : m_touchPoints)
        {
            
            if (activeTouches.find(fingerId) == activeTouches.end())
            {
                
                if (registry.valid(info.pressedEntity))
                {
                    registry.emplace<PointerUpEvent>(info.pressedEntity);

                    
                    if (info.hoveredEntity == info.pressedEntity)
                    {
                        registry.emplace<PointerClickEvent>(info.pressedEntity);
                    }
                }

                
                if (registry.valid(info.hoveredEntity))
                {
                    registry.emplace<PointerExitEvent>(info.hoveredEntity);
                }

                fingersToRemove.push_back(fingerId);
            }
        }

        
        for (SDL_FingerID fingerId : fingersToRemove)
        {
            m_touchPoints.erase(fingerId);
        }
    }
#endif

    ECS::Vector2f InteractionSystem::screenToWorld(const ECS::Vector2f& globalScreenPos,
                                                   const Camera::CamProperties& cameraProps,
                                                   const ECS::RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;

        SkPoint effectiveZoom = cameraProps.GetEffectiveZoom();
        const float worldX = (localX / effectiveZoom.x()) + cameraProps.position.x();
        const float worldY = (localY / effectiveZoom.y()) + cameraProps.position.y();

        return {worldX, worldY};
    }
}

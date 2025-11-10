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
    /**
     * @brief 将屏幕坐标转换为世界坐标（居中模式）
     * @param globalScreenPos 全局屏幕坐标
     * @param cameraProps 相机属性
     * @param viewport 视口矩形
     * @return 世界坐标
     */
    static ECS::Vector2f screenToWorldCentered(const ECS::Vector2f& globalScreenPos,
                                               const Camera::CamProperties& cameraProps,
                                               const ECS::RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;

        // 转换到以视口中心为原点的坐标系
        const float centeredX = localX - (viewport.z / 2.0f);
        const float centeredY = localY - (viewport.w / 2.0f);

        // 逆向缩放（考虑相机缩放）
        const float unzoomedX = centeredX / cameraProps.zoom;
        const float unzoomedY = centeredY / cameraProps.zoom;

        // 转换到世界坐标
        const float worldX = unzoomedX + cameraProps.position.x();
        const float worldY = unzoomedY + cameraProps.position.y();

        return {worldX, worldY};
    }

    /**
     * @brief 检查点是否在精灵内部
     * @param worldPoint 世界坐标点
     * @param transform 变换组件
     * @param sprite 精灵组件
     * @return 如果点在精灵内部返回true
     */
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

        // 转换到局部坐标
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

        // 边界检测
        return (localPoint.x >= -halfWidth && localPoint.x <= halfWidth &&
            localPoint.y >= -halfHeight && localPoint.y <= halfHeight);
    }

    /**
     * @brief 检查点是否在矩形UI内部
     * @param worldPoint 世界坐标点
     * @param transform 变换组件
     * @param size UI尺寸
     * @return 如果点在UI内部返回true
     */
    static bool isPointInRectUI(const ECS::Vector2f& worldPoint, const ECS::TransformComponent& transform,
                                const ECS::Vector2f& size)
    {
        const float halfWidth = size.x * 0.5f;
        const float halfHeight = size.y * 0.5f;

        if (halfWidth <= 0 || halfHeight <= 0) return false;

        // 转换到局部坐标
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

        // 边界检测
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

        // 清除上一帧的交互事件
        registry.clear<PointerEnterEvent, PointerExitEvent, PointerDownEvent, PointerUpEvent, PointerClickEvent>();

        // 编辑器模式下不处理交互
        if (context.appMode == ApplicationMode::Editor)
        {
            return;
        }

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        // 移动平台使用多点触摸
        handleMultiTouchEvents(scene, context);
#else
        // 桌面平台使用鼠标
        ECS::Vector2f worldMousePos;
        bool canInteract = false;

        const auto& input = context.inputState;
        ECS::Vector2f globalMousePos = {(float)input.mousePosition.x, (float)input.mousePosition.y};
        auto cameraProps = scene->GetCameraProperties();
        ECS::RectF viewportRect;

        // 确定视口区域
        if (context.appMode == ApplicationMode::PIE)
        {
            viewportRect = context.sceneViewRect;
            // 检查鼠标是否在视口内
            if (globalMousePos.x >= viewportRect.x && globalMousePos.x <= (viewportRect.x + viewportRect.z) &&
                globalMousePos.y >= viewportRect.y && globalMousePos.y <= (viewportRect.y + viewportRect.w))
            {
                canInteract = true;
            }
        }
        else // Runtime模式
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

        // 检查所有UI组件（按钮、输入框等）
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

        // 检查精灵组件
        auto spriteView = registry.view<ECS::TransformComponent, ECS::SpriteComponent>();
        for (auto entity : spriteView)
        {
            // 跳过已经作为UI组件处理的实体
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

        // 按zIndex排序，选择最顶层的实体
        std::ranges::sort(candidates, [](const auto& a, const auto& b)
        {
            if (a.second != b.second)
            {
                return a.second > b.second; // zIndex高的在前
            }
            return a.first > b.first; // 相同zIndex时按实体ID排序
        });

        return candidates[0].first;
    }

    void InteractionSystem::handleHoverEvents(entt::registry& registry, entt::entity currentHoveredEntity,
                                              entt::entity previousHoveredEntity)
    {
        if (currentHoveredEntity != previousHoveredEntity)
        {
            // 触发退出事件
            if (registry.valid(previousHoveredEntity))
            {
                registry.emplace<PointerExitEvent>(previousHoveredEntity);
            }
            // 触发进入事件
            if (registry.valid(currentHoveredEntity))
            {
                registry.emplace<PointerEnterEvent>(currentHoveredEntity);
            }
        }
    }

    void InteractionSystem::handleMouseClickEvents(entt::registry& registry, const InputState& inputState)
    {
        bool isLeftMouseDownThisFrame = inputState.isLeftMouseDown;

        // 鼠标按下
        if (isLeftMouseDownThisFrame && !m_wasLeftMouseDownLastFrame)
        {
            if (registry.valid(m_hoveredEntity))
            {
                registry.emplace<PointerDownEvent>(m_hoveredEntity);
                m_pressedEntity = m_hoveredEntity;
            }
        }
        // 鼠标抬起
        else if (!isLeftMouseDownThisFrame && m_wasLeftMouseDownLastFrame)
        {
            if (registry.valid(m_pressedEntity))
            {
                registry.emplace<PointerUpEvent>(m_pressedEntity);
            }

            // 如果抬起时仍在同一个实体上，触发点击事件
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

        // 获取窗口尺寸
        int windowWidth, windowHeight;
        SDL_GetWindowSizeInPixels(context.window->GetSdlWindow(), &windowWidth, &windowHeight);

        ECS::RectF viewportRect = {0.0f, 0.0f, (float)windowWidth, (float)windowHeight};

        // 获取当前活动的触摸点
        const auto& activeTouches = context.window->GetActiveTouches();

        // 处理新的触摸点和更新现有触摸点
        for (const auto& [fingerId, touchPoint] : activeTouches)
        {
            // 将归一化坐标转换为像素坐标
            ECS::Vector2f screenPos = {
                touchPoint.x * windowWidth,
                touchPoint.y * windowHeight
            };

            // 转换为世界坐标
            ECS::Vector2f worldPos = screenToWorldCentered(screenPos, cameraProps, viewportRect);

            // 执行拾取
            entt::entity pickedEntity = performGeometricPicking(registry, worldPos);

            // 查找或创建触摸点信息
            auto it = m_touchPoints.find(fingerId);
            if (it == m_touchPoints.end())
            {
                // 新触摸点
                TouchPointInfo info;
                info.position = worldPos;
                info.hoveredEntity = pickedEntity;
                info.pressedEntity = entt::null;

                // 触发进入事件
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
                // 已存在的触摸点
                TouchPointInfo& info = it->second;
                entt::entity previousHoveredEntity = info.hoveredEntity;

                // 更新位置
                info.position = worldPos;

                // 检查悬停实体是否改变
                if (pickedEntity != previousHoveredEntity)
                {
                    // 触发退出事件
                    if (registry.valid(previousHoveredEntity))
                    {
                        registry.emplace<PointerExitEvent>(previousHoveredEntity);

                        // 如果这是被按下的实体，也触发抬起事件
                        if (previousHoveredEntity == info.pressedEntity)
                        {
                            registry.emplace<PointerUpEvent>(previousHoveredEntity);
                            info.pressedEntity = entt::null;
                        }
                    }

                    // 触发进入事件
                    if (registry.valid(pickedEntity))
                    {
                        registry.emplace<PointerEnterEvent>(pickedEntity);
                    }

                    info.hoveredEntity = pickedEntity;
                }
            }
        }

        // 处理已抬起的触摸点
        std::vector<SDL_FingerID> fingersToRemove;
        for (const auto& [fingerId, info] : m_touchPoints)
        {
            // 如果触摸点不在活动列表中，说明已抬起
            if (activeTouches.find(fingerId) == activeTouches.end())
            {
                // 触发抬起事件
                if (registry.valid(info.pressedEntity))
                {
                    registry.emplace<PointerUpEvent>(info.pressedEntity);

                    // 如果抬起时仍在同一实体上，触发点击事件
                    if (info.hoveredEntity == info.pressedEntity)
                    {
                        registry.emplace<PointerClickEvent>(info.pressedEntity);
                    }
                }

                // 触发退出事件
                if (registry.valid(info.hoveredEntity))
                {
                    registry.emplace<PointerExitEvent>(info.hoveredEntity);
                }

                fingersToRemove.push_back(fingerId);
            }
        }

        // 移除已抬起的触摸点
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

        const float worldX = (localX / cameraProps.zoom) + cameraProps.position.x();
        const float worldY = (localY / cameraProps.zoom) + cameraProps.position.y();

        return {worldX, worldY};
    }
}

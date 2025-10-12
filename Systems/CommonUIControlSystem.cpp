#include "CommonUIControlSystem.h"

#include "Components/InteractionEvents.h"
#include "Components/ScriptComponent.h"
#include "Components/Transform.h"
#include "Components/UIComponents.h"
#include "Components/RelationshipComponent.h"
#include "Event/LumaEvent.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Resources/RuntimeAsset/RuntimeGameObject.h"
#include "yaml-cpp/yaml.h"
#if !defined(LUMA_DISABLE_SCRIPTING)
#include "../Scripting/ManagedHost.h"
#endif
#include "Renderer/Camera.h"
#include "Application/Window.h"
#include <SDL3/SDL_events.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include <cmath>

namespace
{
    using ECS::ButtonState;
    using ECS::RectF;
    using ECS::SerializableEventTarget;
    using ECS::ListBoxLayout;
    using ECS::Vector2f;

    constexpr float EPSILON = 1e-4f;

    Vector2f screenToWorldCentered(const Vector2f& globalScreenPos,
                                   const Camera::CamProperties& cameraProps,
                                   const RectF& viewport)
    {
        const float localX = globalScreenPos.x - viewport.x;
        const float localY = globalScreenPos.y - viewport.y;

        const float centeredX = localX - (viewport.z * 0.5f);
        const float centeredY = localY - (viewport.w * 0.5f);

        const float unzoomedX = centeredX / cameraProps.zoom;
        const float unzoomedY = centeredY / cameraProps.zoom;

        const float worldX = unzoomedX + cameraProps.position.x();
        const float worldY = unzoomedY + cameraProps.position.y();

        return {worldX, worldY};
    }

    Vector2f worldToLocal(const ECS::TransformComponent& transform, const Vector2f& worldPoint)
    {
        Vector2f local = worldPoint - transform.position;
        if (std::abs(transform.rotation) > EPSILON)
        {

            const float sinR = std::sin(-transform.rotation);
            const float cosR = std::cos(-transform.rotation);
            const float tempX = local.x;
            local.x = local.x * cosR - local.y * sinR;
            local.y = tempX * sinR + local.y * cosR;
        }
        if (std::abs(transform.scale.x) > EPSILON)
        {
            local.x /= transform.scale.x;
        }
        if (std::abs(transform.scale.y) > EPSILON)
        {
            local.y /= transform.scale.y;
        }
        return local;
    }

    void updateButtonState(entt::registry& registry, entt::entity entity,
                           bool enable, bool interactable, ButtonState& currentState)
    {
        if (!enable || !interactable)
        {
            currentState = ButtonState::Disabled;
            return;
        }

        const bool hasExited = registry.all_of<PointerExitEvent>(entity);
        const bool hasEntered = registry.all_of<PointerEnterEvent>(entity);
        const bool isPressedDown = registry.all_of<PointerDownEvent>(entity);

        switch (currentState)
        {
        case ButtonState::Normal:
            if (hasEntered) currentState = ButtonState::Hovered;
            break;
        case ButtonState::Hovered:
            if (hasExited) currentState = ButtonState::Normal;
            else if (isPressedDown) currentState = ButtonState::Pressed;
            break;
        case ButtonState::Pressed:
            if (hasExited) currentState = ButtonState::Normal;
            if (!isPressedDown && !registry.all_of<PointerUpEvent>(entity))
            {
                currentState = ButtonState::Hovered;
            }
            break;
        case ButtonState::Disabled:
            currentState = ButtonState::Normal;
            break;
        }
    }

    void invokeScriptTargets(RuntimeScene* scene,
                             const std::vector<SerializableEventTarget>& targets,
                             const std::function<void(YAML::Node&)>& populateArgs = nullptr)
    {
        if (targets.empty()) return;

        for (const auto& target : targets)
        {
            if (!target.targetEntityGuid.Valid()) continue;

            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (!targetGO.IsValid() || !targetGO.HasComponent<ECS::ScriptsComponent>())
            {
                continue;
            }

            YAML::Node args;
            if (populateArgs)
            {
                populateArgs(args);
            }

            InteractScriptEvent scriptEvent;
            scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
            scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
            scriptEvent.methodName = target.targetMethodName;
            scriptEvent.methodArgs = YAML::Dump(args);

            EventBus::GetInstance().Publish(scriptEvent);
        }
    }

    template <typename T>
    bool almostEqual(T a, T b, T tolerance = static_cast<T>(1e-5))
    {
        return std::abs(a - b) <= tolerance;
    }

    bool ensureSceneEntityActive(RuntimeScene* scene, entt::entity entity)
    {
        auto go = scene->FindGameObjectByEntity(entity);
        return go.IsValid() && go.IsActive();
    }

    int floorDiv(int numerator, int denominator)
    {
        if (denominator == 0) return 0;
        int quotient = numerator / denominator;
        int remainder = numerator % denominator;
        if (remainder != 0 && ((remainder > 0) != (denominator > 0)))
        {
            --quotient;
        }
        return quotient;
    }

    int floorMod(int numerator, int denominator)
    {
        if (denominator == 0) return 0;
        int result = numerator % denominator;
        if (result != 0 && ((result < 0) != (denominator < 0)))
        {
            result += denominator;
        }
        return result;
    }
}


namespace Systems
{
    float CommonUIControlSystem::Clamp01(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    void CommonUIControlSystem::OnCreate(RuntimeScene* , EngineContext& )
    {
        m_activeSlider = entt::null;
        m_openCombo = entt::null;
    }

    void CommonUIControlSystem::OnDestroy(RuntimeScene* )
    {
        m_activeSlider = entt::null;
        m_openCombo = entt::null;
    }

    void CommonUIControlSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();

        const bool isRuntime = context.appMode != ApplicationMode::Editor;
        const Vector2f globalMousePos = {
            static_cast<float>((context.window ? context.window->GetInputState().mousePosition.x : context.inputState.mousePosition.x)),
            static_cast<float>((context.window ? context.window->GetInputState().mousePosition.y : context.inputState.mousePosition.y))
        };

        auto cameraProps = scene->GetCameraProperties();
        RectF viewportRect;
        if (context.appMode == ApplicationMode::PIE)
        {
            viewportRect = context.sceneViewRect;
        }
        else
        {
            viewportRect = {0.0f, 0.0f, cameraProps.viewport.width(), cameraProps.viewport.height()};
        }

        const Vector2f worldMousePos = screenToWorldCentered(globalMousePos, cameraProps, viewportRect);
        int scrollWheelX = 0;
        int scrollWheelY = 0;
        for (const auto& event : context.frameEvents.GetView())
        {
            if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                scrollWheelX += event.wheel.x;
                scrollWheelY += event.wheel.y;
            }
        }


        {
            auto view = registry.view<ECS::ToggleButtonComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto& toggle = view.get<ECS::ToggleButtonComponent>(entity);
                if (!toggle.isVisible) continue;

                const bool previousToggle = toggle.isToggled;

                updateButtonState(registry, entity, toggle.Enable, toggle.isInteractable, toggle.currentState);

                if (toggle.isInteractable && toggle.Enable && registry.all_of<PointerClickEvent>(entity))
                {
                    if (!toggle.isToggled)
                    {
                        toggle.isToggled = true;
                        invokeScriptTargets(scene, toggle.onToggleOnTargets);
                    }
                    else if (toggle.allowToggleOff)
                    {
                        toggle.isToggled = false;
                        invokeScriptTargets(scene, toggle.onToggleOffTargets);
                    }
                }

                if (!toggle.Enable || !toggle.isInteractable)
                {
                    toggle.currentState = ButtonState::Disabled;
                }

                if (previousToggle != toggle.isToggled)
                {
                    toggle.currentState = toggle.isToggled ? ButtonState::Pressed : ButtonState::Normal;
                }
            }
        }


        {
            auto view = registry.view<ECS::RadioButtonComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto& radio = view.get<ECS::RadioButtonComponent>(entity);
                if (!radio.isVisible) continue;

                const bool prevSelected = radio.isSelected;

                updateButtonState(registry, entity, radio.Enable, radio.isInteractable, radio.currentState);

                if (radio.isInteractable && radio.Enable &&
                    registry.all_of<PointerClickEvent>(entity) && !radio.isSelected)
                {
                    radio.isSelected = true;

                    for (auto other : view)
                    {
                        if (other == entity) continue;

                        auto& otherRadio = view.get<ECS::RadioButtonComponent>(other);
                        if (otherRadio.groupId != radio.groupId) continue;

                        if (otherRadio.isSelected)
                        {
                            otherRadio.isSelected = false;
                            otherRadio.currentState = ButtonState::Normal;
                        }
                    }
                }

                if (!radio.Enable || !radio.isInteractable)
                {
                    radio.currentState = ButtonState::Disabled;
                }

                if (prevSelected != radio.isSelected)
                {
                    if (radio.isSelected)
                    {
                        invokeScriptTargets(scene, radio.onSelectedTargets);
                    }
                    else
                    {
                        invokeScriptTargets(scene, radio.onDeselectedTargets);
                    }
                }
            }
        }


        {
            auto view = registry.view<ECS::CheckBoxComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto& checkbox = view.get<ECS::CheckBoxComponent>(entity);
                if (!checkbox.isVisible) continue;

                const bool prevChecked = checkbox.isChecked;
                const bool prevIndeterminate = checkbox.isIndeterminate;

                updateButtonState(registry, entity, checkbox.Enable, checkbox.isInteractable, checkbox.currentState);

                if (checkbox.isInteractable && checkbox.Enable && registry.all_of<PointerClickEvent>(entity))
                {
                    if (checkbox.allowIndeterminate)
                    {
                        if (!checkbox.isChecked && !checkbox.isIndeterminate)
                        {
                            checkbox.isChecked = true;
                        }
                        else if (checkbox.isChecked)
                        {
                            checkbox.isChecked = false;
                            checkbox.isIndeterminate = true;
                        }
                        else
                        {
                            checkbox.isIndeterminate = false;
                        }
                    }
                    else
                    {
                        checkbox.isChecked = !checkbox.isChecked;
                        checkbox.isIndeterminate = false;
                    }
                }

                if (!checkbox.Enable || !checkbox.isInteractable)
                {
                    checkbox.currentState = ButtonState::Disabled;
                }

                if (prevChecked != checkbox.isChecked || prevIndeterminate != checkbox.isIndeterminate)
                {
                    invokeScriptTargets(scene, checkbox.onValueChangedTargets, [&](YAML::Node& args)
                    {
                        args["isChecked"] = checkbox.isChecked;
                        args["isIndeterminate"] = checkbox.isIndeterminate;
                    });
                }
            }
        }


        {
            auto view = registry.view<ECS::TransformComponent, ECS::SliderComponent>();
            const bool leftMouseDown = context.window ? context.window->GetInputState().isLeftMouseDown : context.inputState.isLeftMouseDown;

            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto [transform, slider] = view.get<ECS::TransformComponent, ECS::SliderComponent>(entity);
                if (!slider.isVisible) continue;

                const float range = slider.maxValue - slider.minValue;
                const float safeRange = std::max(range, EPSILON);

                if (slider.step > 0.0f && range > EPSILON)
                {
                    float steps = std::round((slider.value - slider.minValue) / slider.step);
                    slider.value = slider.minValue + steps * slider.step;
                }

                float prevValue = slider.value;
                bool startedDrag = false;
                bool endedDrag = false;

                if (!slider.Enable || !slider.isInteractable)
                {
                    if (slider.isDragging && m_activeSlider == entity)
                    {
                        endedDrag = true;
                    }
                    slider.isDragging = false;
                    if (m_activeSlider == entity) m_activeSlider = entt::null;
                }
                else
                {
                    if (registry.all_of<PointerDownEvent>(entity))
                    {
                        slider.isDragging = true;
                        m_activeSlider = entity;
                        startedDrag = true;
                    }

                    if (slider.isDragging)
                    {
                        if (!leftMouseDown && m_activeSlider == entity)
                        {
                            slider.isDragging = false;
                            endedDrag = true;
                            m_activeSlider = entt::null;
                        }
                        else if (registry.all_of<PointerUpEvent>(entity))
                        {
                            slider.isDragging = false;
                            endedDrag = true;
                            if (m_activeSlider == entity) m_activeSlider = entt::null;
                        }
                    }
                }

                slider.value = std::clamp(slider.value, slider.minValue, slider.maxValue);
                slider.normalizedValue = (range > EPSILON)
                    ? Clamp01((slider.value - slider.minValue) / range)
                    : 0.0f;

                if (slider.isDragging && m_activeSlider == entity && isRuntime)
                {
                    Vector2f local = worldToLocal(transform, worldMousePos);
                    float normalized = 0.0f;

                    if (slider.isVertical)
                    {
                        const float halfHeight = slider.rect.w * 0.5f;
                        normalized = Clamp01(1.0f - ((local.y + halfHeight) / std::max(slider.rect.w, EPSILON)));
                    }
                    else
                    {
                        const float halfWidth = slider.rect.z * 0.5f;
                        normalized = Clamp01((local.x + halfWidth) / std::max(slider.rect.z, EPSILON));
                    }

                    float newValue = slider.minValue + normalized * safeRange;
                    if (slider.step > 0.0f)
                    {
                        float steps = std::round((newValue - slider.minValue) / slider.step);
                        newValue = slider.minValue + steps * slider.step;
                    }

                    newValue = std::clamp(newValue, slider.minValue, slider.maxValue);

                    if (!almostEqual(newValue, slider.value))
                    {
                        slider.value = newValue;
                        slider.normalizedValue = (range > EPSILON)
                            ? Clamp01((slider.value - slider.minValue) / range)
                            : 0.0f;
                    }
                }

                if (startedDrag)
                {
                    invokeScriptTargets(scene, slider.onDragStartedTargets, [&](YAML::Node& args)
                    {
                        args["value"] = slider.value;
                        args["normalizedValue"] = slider.normalizedValue;
                    });
                }

                if (endedDrag)
                {
                    invokeScriptTargets(scene, slider.onDragEndedTargets, [&](YAML::Node& args)
                    {
                        args["value"] = slider.value;
                        args["normalizedValue"] = slider.normalizedValue;
                    });
                }

                if (!almostEqual(prevValue, slider.value))
                {
                    invokeScriptTargets(scene, slider.onValueChangedTargets, [&](YAML::Node& args)
                    {
                        args["value"] = slider.value;
                        args["normalizedValue"] = slider.normalizedValue;
                    });
                }
            }

            if (!registry.valid(m_activeSlider))
            {
                m_activeSlider = entt::null;
            }
        }


        {
            auto view = registry.view<ECS::TransformComponent, ECS::ComboBoxComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto [transform, combo] = view.get<ECS::TransformComponent, ECS::ComboBoxComponent>(entity);
                if (!combo.isVisible) continue;

                if (combo.items.empty())
                {
                    combo.selectedIndex = -1;
                }
                else if (combo.selectedIndex >= static_cast<int>(combo.items.size()))
                {
                    combo.selectedIndex = static_cast<int>(combo.items.size()) - 1;
                }

                const int previousIndex = combo.selectedIndex;

                updateButtonState(registry, entity, combo.Enable, combo.isInteractable, combo.currentState);

                if (!combo.Enable || !combo.isInteractable)
                {
                    combo.currentState = ButtonState::Disabled;
                    combo.hoveredIndex = -1;
                    combo.isDropdownOpen = false;
                    if (m_openCombo == entity) m_openCombo = entt::null;
                    continue;
                }

                if (combo.isDropdownOpen && isRuntime)
                {
                    Vector2f local = worldToLocal(transform, worldMousePos);
                    const float headerHeight = std::min(combo.rect.w, combo.displayText.fontSize * 1.6f + 8.0f);
                    const float itemHeight = combo.displayText.fontSize * 1.4f + 6.0f;
                    const float top = -combo.rect.w * 0.5f;
                    const float itemsTop = top + headerHeight;

                    if (local.y >= itemsTop && local.y <= (combo.rect.w * 0.5f) && itemHeight > 0.0f)
                    {
                        const float relative = local.y - itemsTop;
                        int index = static_cast<int>(std::floor(relative / itemHeight));
                        if (index >= 0 && index < static_cast<int>(combo.items.size()))
                        {
                            combo.hoveredIndex = index;
                        }
                        else
                        {
                            combo.hoveredIndex = -1;
                        }
                    }
                    else
                    {
                        combo.hoveredIndex = -1;
                    }
                }
                else
                {
                    combo.hoveredIndex = -1;
                }

                if (registry.all_of<PointerClickEvent>(entity))
                {
                    Vector2f local = worldToLocal(transform, worldMousePos);
                    const float headerHeight = std::min(combo.rect.w, combo.displayText.fontSize * 1.6f + 8.0f);
                    const float top = -combo.rect.w * 0.5f;
                    const bool headerClicked = (local.y >= top && local.y <= (top + headerHeight));

                    if (!combo.isDropdownOpen)
                    {
                        if (headerClicked)
                        {
                            combo.isDropdownOpen = true;
                            m_openCombo = entity;
                        }
                    }
                    else
                    {
                        if (headerClicked)
                        {
                            combo.isDropdownOpen = false;
                            if (m_openCombo == entity) m_openCombo = entt::null;
                        }
                        else if (combo.hoveredIndex >= 0 &&
                                 combo.hoveredIndex < static_cast<int>(combo.items.size()))
                        {
                            combo.selectedIndex = combo.hoveredIndex;
                            combo.isDropdownOpen = false;
                            if (m_openCombo == entity) m_openCombo = entt::null;
                        }
                        else
                        {
                            combo.isDropdownOpen = false;
                            if (m_openCombo == entity) m_openCombo = entt::null;
                        }
                    }
                }

                if (combo.isDropdownOpen && m_openCombo != entt::null && m_openCombo != entity)
                {
                    combo.isDropdownOpen = false;
                    combo.hoveredIndex = -1;
                }

                if (!combo.isDropdownOpen && m_openCombo == entity)
                {
                    m_openCombo = entt::null;
                }

                if (previousIndex != combo.selectedIndex)
                {
                    invokeScriptTargets(scene, combo.onSelectionChangedTargets, [&](YAML::Node& args)
                    {
                        args["selectedIndex"] = combo.selectedIndex;
                        if (combo.selectedIndex >= 0 &&
                            combo.selectedIndex < static_cast<int>(combo.items.size()))
                        {
                            args["selectedItem"] = combo.items[combo.selectedIndex];
                        }
                        else
                        {
                            args["selectedItem"] = "";
                        }
                    });
                }
            }

            if (!registry.valid(m_openCombo))
            {
                m_openCombo = entt::null;
            }
        }


        {
            auto view = registry.view<ECS::ExpanderComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto& expander = view.get<ECS::ExpanderComponent>(entity);
                if (!expander.isVisible) continue;

                const bool previous = expander.isExpanded;

                if (expander.Enable && expander.isInteractable &&
                    registry.all_of<PointerClickEvent>(entity))
                {
                    expander.isExpanded = !expander.isExpanded;
                }

                if (previous != expander.isExpanded)
                {
                    if (expander.isExpanded)
                    {
                        invokeScriptTargets(scene, expander.onExpandedTargets);
                    }
                    else
                    {
                        invokeScriptTargets(scene, expander.onCollapsedTargets);
                    }
                }
            }
        }


        {
            auto view = registry.view<ECS::ProgressBarComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto& progress = view.get<ECS::ProgressBarComponent>(entity);
                if (!progress.isVisible) continue;

                const float prevValue = progress.value;

                progress.value = std::clamp(progress.value, progress.minValue, progress.maxValue);

                if (progress.isIndeterminate)
                {
                    progress.indeterminatePhase += deltaTime * progress.indeterminateSpeed;
                    if (progress.indeterminatePhase > 1.0f)
                    {
                        progress.indeterminatePhase -= std::floor(progress.indeterminatePhase);
                    }
                }
                else
                {
                    progress.indeterminatePhase = 0.0f;
                }

                if (!almostEqual(prevValue, progress.value))
                {
                    invokeScriptTargets(scene, progress.onValueChangedTargets, [&](YAML::Node& args)
                    {
                        args["value"] = progress.value;
                        args["minValue"] = progress.minValue;
                        args["maxValue"] = progress.maxValue;
                        args["normalizedValue"] =
                            (progress.maxValue - progress.minValue) > EPSILON
                                ? (progress.value - progress.minValue) / (progress.maxValue - progress.minValue)
                                : 0.0f;
                    });
                }

                const bool nowCompleted = !progress.isIndeterminate &&
                    almostEqual(progress.value, progress.maxValue);

                if (nowCompleted && !progress.hasCompletedEventFired)
                {
                    progress.hasCompletedEventFired = true;
                    invokeScriptTargets(scene, progress.onCompletedTargets);
                }
                else if (!nowCompleted)
                {
                    progress.hasCompletedEventFired = false;
                }
            }
        }


        {
            auto view = registry.view<ECS::TransformComponent, ECS::TabControlComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto [transform, tabs] = view.get<ECS::TransformComponent, ECS::TabControlComponent>(entity);
                if (!tabs.isVisible) continue;

                const int prevHovered = tabs.hoveredTabIndex;
                const int prevActive = tabs.activeTabIndex;

                tabs.activeTabIndex = std::clamp(tabs.activeTabIndex, 0,
                    static_cast<int>(std::max<size_t>(1, tabs.tabs.size())) - 1);

                if (!tabs.Enable || !tabs.isInteractable || tabs.tabs.empty())
                {
                    tabs.hoveredTabIndex = -1;
                }
                else if (isRuntime)
                {
                    Vector2f local = worldToLocal(transform, worldMousePos);
                    const float top = -tabs.rect.w * 0.5f;
                    const float headerBottom = top + tabs.tabHeight;

                    if (local.y >= top && local.y <= headerBottom)
                    {
                        float x = local.x + tabs.rect.z * 0.5f;
                        float cursor = 0.0f;
                        int hoverIndex = -1;

                        for (size_t i = 0; i < tabs.tabs.size(); ++i)
                        {
                            const auto& tab = tabs.tabs[i];
                            if (!tab.isVisible) continue;

                            const float tabWidth = std::max(tabs.tabHeight * 2.5f,
                                tabs.tabHeight * (static_cast<float>(tab.title.size()) * 0.6f + 1.5f));
                            const float start = cursor;
                            const float end = cursor + tabWidth;

                            if (x >= start && x <= end)
                            {
                                hoverIndex = static_cast<int>(i);
                                break;
                            }
                            cursor = end + tabs.tabSpacing;
                        }

                        tabs.hoveredTabIndex = hoverIndex;
                    }
                    else if (registry.all_of<PointerExitEvent>(entity))
                    {
                        tabs.hoveredTabIndex = -1;
                    }
                }

                if (registry.all_of<PointerClickEvent>(entity) &&
                    tabs.hoveredTabIndex >= 0 &&
                    tabs.hoveredTabIndex < static_cast<int>(tabs.tabs.size()))
                {
                    const auto& hoveredTab = tabs.tabs[tabs.hoveredTabIndex];
                    if (hoveredTab.isVisible && hoveredTab.isEnabled)
                    {
                        tabs.activeTabIndex = tabs.hoveredTabIndex;
                    }
                }

                tabs.activeTabIndex = std::clamp(tabs.activeTabIndex, 0,
                    static_cast<int>(std::max<size_t>(1, tabs.tabs.size())) - 1);

                if (prevActive != tabs.activeTabIndex)
                {
                    invokeScriptTargets(scene, tabs.onTabChangedTargets, [&](YAML::Node& args)
                    {
                        args["activeTabIndex"] = tabs.activeTabIndex;
                        if (tabs.activeTabIndex >= 0 &&
                            tabs.activeTabIndex < static_cast<int>(tabs.tabs.size()))
                        {
                            const auto& activeTab = tabs.tabs[tabs.activeTabIndex];
                            args["title"] = activeTab.title;
                            args["contentGuid"] = activeTab.contentGuid.ToString();
                        }
                    });
                }

                if (prevHovered != tabs.hoveredTabIndex && tabs.hoveredTabIndex < 0)
                {
                    tabs.hoveredTabIndex = -1;
                }
            }
        }


        {
            auto view = registry.view<ECS::TransformComponent, ECS::ListBoxComponent>();
            for (auto entity : view)
            {
                if (!ensureSceneEntityActive(scene, entity)) continue;

                auto [transform, listBox] = view.get<ECS::TransformComponent, ECS::ListBoxComponent>(entity);
                if (!listBox.isVisible) continue;

                std::vector<int> previousSelection = listBox.selectedIndices;
                const int prevHovered = listBox.hoveredIndex;

                bool useContainer = listBox.itemsContainerGuid.Valid();
                std::vector<Guid> managedGuids;

                if (useContainer)
                {
                    RuntimeGameObject containerGO = scene->FindGameObjectByGuid(listBox.itemsContainerGuid);
                    if (containerGO.IsValid())
                    {
                        entt::entity containerEntity = static_cast<entt::entity>(containerGO);
                        if (registry.valid(containerEntity) && registry.any_of<ECS::ChildrenComponent>(containerEntity))
                        {
                            const auto& childrenComp = registry.get<ECS::ChildrenComponent>(containerEntity);
                            managedGuids.reserve(childrenComp.children.size());
                            for (auto childEntity : childrenComp.children)
                            {
                                if (!registry.valid(childEntity)) continue;
                                RuntimeGameObject childGO(childEntity, scene);
                                if (!childGO.IsValid() || !childGO.IsActive()) continue;
                                managedGuids.push_back(childGO.GetGuid());
                            }
                        }
                    }
                    else
                    {
                        useContainer = false;
                    }
                }

                const int itemCount = useContainer
                    ? static_cast<int>(managedGuids.size())
                    : static_cast<int>(listBox.items.size());
                const int visibleCandidate = listBox.visibleItemCount > 0
                    ? std::min(listBox.visibleItemCount, std::max(1, itemCount))
                    : std::max(1, itemCount);
                auto clampPositive = [](int value, int fallback)
                {
                    return value > 0 ? value : fallback;
                };
                int columns = 1;
                int rows = 1;
                int itemsPerPage = 0;
                switch (listBox.layout)
                {
                case ListBoxLayout::Horizontal:
                    rows = clampPositive(listBox.maxItemsPerColumn, 1);
                    if (listBox.visibleItemCount > 0)
                    {
                        columns = std::max(1, static_cast<int>(std::ceil(static_cast<float>(visibleCandidate) / static_cast<float>(rows))));
                        itemsPerPage = std::min(itemCount, columns * rows);
                    }
                    else
                    {
                        columns = std::max(1, static_cast<int>(std::ceil(static_cast<float>(std::max(1, itemCount)) / static_cast<float>(rows))));
                        itemsPerPage = itemCount;
                    }
                    break;
                case ListBoxLayout::Grid:
                    columns = clampPositive(listBox.maxItemsPerRow, 1);
                    rows = clampPositive(listBox.maxItemsPerColumn, 1);
                    if (listBox.visibleItemCount > 0)
                    {
                        int target = std::min(visibleCandidate, std::max(1, itemCount));
                        if (listBox.maxItemsPerRow <= 0 && listBox.maxItemsPerColumn <= 0)
                        {
                            columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(target)))));
                            rows = std::max(1, static_cast<int>(std::ceil(static_cast<float>(target) / static_cast<float>(columns))));
                        }
                        else if (listBox.maxItemsPerRow <= 0)
                        {
                            rows = clampPositive(listBox.maxItemsPerColumn, 1);
                            columns = std::max(1, static_cast<int>(std::ceil(static_cast<float>(target) / static_cast<float>(rows))));
                        }
                        else if (listBox.maxItemsPerColumn <= 0)
                        {
                            columns = std::max(1, std::min(listBox.maxItemsPerRow, target));
                            rows = std::max(1, static_cast<int>(std::ceil(static_cast<float>(target) / static_cast<float>(columns))));
                        }
                        itemsPerPage = std::min(itemCount, columns * rows);
                    }
                    else
                    {
                        itemsPerPage = std::min(itemCount, columns * rows);
                    }
                    break;
                case ListBoxLayout::Vertical:
                default:
                    columns = clampPositive(listBox.maxItemsPerRow, 1);
                    if (listBox.visibleItemCount > 0){
                        rows = std::max(1, static_cast<int>(std::ceil(static_cast<float>(visibleCandidate) / static_cast<float>(columns))));
                        itemsPerPage = std::min(itemCount, rows * columns);
                    }else
                    {
                        rows = std::max(1, static_cast<int>(std::ceil(static_cast<float>(std::max(1, itemCount)) / static_cast<float>(columns))));
                        itemsPerPage = itemCount;
                    }
                    break;
                }
                
                if (listBox.visibleItemCount <= 0)
                {
                    const float spacingX = std::max(0.0f, listBox.itemSpacing.x);
                    const float spacingY = std::max(0.0f, listBox.itemSpacing.y);
                    const float approxLineHeight = listBox.itemTemplate.fontSize > 0.0f
                        ? (listBox.itemTemplate.fontSize * 1.4f)
                        : 20.0f;
                    const float containerWidth = listBox.rect.Width();
                    const float containerHeight = listBox.rect.Height();

                    switch (listBox.layout)
                    {
                    case ListBoxLayout::Horizontal:
                    {
                        int maxTextLen = 1;
                        for (const auto& s : listBox.items)
                            maxTextLen = std::max<int>(maxTextLen, static_cast<int>(s.size()));
                        const float estCharWidth = std::max(1.0f, listBox.itemTemplate.fontSize * 0.6f);
                        const float paddingX = 8.0f;
                        const float estItemWidth = paddingX * 2.0f + estCharWidth * static_cast<float>(maxTextLen);

                        rows = clampPositive(listBox.maxItemsPerColumn, 1);
                        columns = std::max(1, static_cast<int>(std::floor((containerWidth + spacingX) / (estItemWidth + spacingX))));
                        itemsPerPage = std::min(itemCount, rows * columns);
                        break;
                    }
                    case ListBoxLayout::Grid:
                    case ListBoxLayout::Vertical:
                    default:
                    {
                        columns = clampPositive(listBox.maxItemsPerRow, 1);
                        const float cellH = approxLineHeight;
                        rows = std::max(1, static_cast<int>(std::floor((containerHeight + spacingY) / (cellH + spacingY))));
                        itemsPerPage = std::min(itemCount, rows * columns);
                        break;
                    }
                    }
                }
                if (itemCount == 0)
                {
                    itemsPerPage = 0;
                }else
                {
                    itemsPerPage = std::max(1, itemsPerPage);
                }
                const bool primaryIsVertical = listBox.layout != ListBoxLayout::Horizontal;
                const bool baseHorizontalScrollable = !primaryIsVertical && itemCount > itemsPerPage;

                const float spacingX = std::max(0.0f, listBox.itemSpacing.x);
                const float spacingY = std::max(0.0f, listBox.itemSpacing.y);
                const float trackSpacing = 4.0f;
                const float trackThickness = std::max(listBox.scrollbarThickness, 2.0f);

                float availableWidth = listBox.rect.Width();
                float availableHeight = listBox.rect.Height();

                bool showVertical = listBox.enableVerticalScrollbar &&
                                   (primaryIsVertical ? (itemCount > itemsPerPage) : !listBox.verticalScrollbarAutoHide);
                if (!primaryIsVertical && listBox.verticalScrollbarAutoHide)
                {
                    showVertical = false;
                }
                if (showVertical)
                {
                    availableWidth -= (trackThickness + trackSpacing);
                }

                bool showHorizontal = listBox.enableHorizontalScrollbar &&
                    (baseHorizontalScrollable || !listBox.horizontalScrollbarAutoHide);
                if (showHorizontal)
                {
                    availableHeight -= (trackThickness + trackSpacing);
                }
                availableWidth = std::max(availableWidth, 1.0f);
                availableHeight = std::max(availableHeight, 1.0f);
                columns = std::max(1, columns);
                rows = std::max(1, rows);
                float totalSpacingX = spacingX * static_cast<float>(std::max(0, columns - 1));
                float totalSpacingY = spacingY * static_cast<float>(std::max(0, rows - 1));
                float itemWidth = (availableWidth - totalSpacingX) / static_cast<float>(columns);
                float itemHeight = (availableHeight - totalSpacingY) / static_cast<float>(rows);
                itemWidth = std::max(itemWidth, 1.0f);
                itemHeight = std::max(itemHeight, 1.0f);
                const float contentLeftWorld = transform.position.x - availableWidth * 0.5f;
                const float contentTopWorld = transform.position.y - availableHeight * 0.5f;
                const int maxScroll = std::max(0, itemCount - itemsPerPage);
                listBox.scrollOffset = std::clamp(listBox.scrollOffset, 0, maxScroll);

                
                if (isRuntime && (showVertical || showHorizontal) && maxScroll > 0)
                {
                    const bool mouseDown = context.window ? context.window->GetInputState().isLeftMouseDown : context.inputState.isLeftMouseDown;
                    
                    if (showVertical)
                    {
                        const float trackLeft = contentLeftWorld + availableWidth + trackSpacing;
                        const float trackTop = contentTopWorld;
                        const float trackWidth = trackThickness;
                        const float trackHeight = availableHeight;

                        const float ratio = static_cast<float>(itemsPerPage) / static_cast<float>(itemCount);
                        const float thumbHeight = std::max(trackHeight * std::clamp(ratio, 0.0f, 1.0f), trackThickness);
                        const float scrollRange = std::max(1, itemCount - itemsPerPage);
                        float thumbOffset = 0.0f;
                        if (scrollRange > 0)
                        {
                            const float offsetRatio = static_cast<float>(std::clamp((float)listBox.scrollOffset, 0.f, scrollRange)) / static_cast<float>(scrollRange);
                            thumbOffset = (trackHeight - thumbHeight) * std::clamp(offsetRatio, 0.0f, 1.0f);
                        }
                        const float thumbTop = trackTop + thumbOffset;

                        const bool insideTrack = (worldMousePos.x >= trackLeft && worldMousePos.x <= trackLeft + trackWidth &&
                                                  worldMousePos.y >= trackTop && worldMousePos.y <= trackTop + trackHeight);
                        const bool insideThumb = (worldMousePos.x >= trackLeft && worldMousePos.x <= trackLeft + trackWidth &&
                                                  worldMousePos.y >= thumbTop && worldMousePos.y <= thumbTop + thumbHeight);

                        if (registry.all_of<PointerDownEvent>(entity) && insideTrack)
                        {
                            listBox.draggingVerticalScrollbar = true;
                            listBox.dragGrabOffset = insideThumb ? (worldMousePos.y - thumbTop) : (thumbHeight * 0.5f);
                        }
                        if (listBox.draggingVerticalScrollbar)
                        {
                            if (mouseDown)
                            {
                                float newThumbTop = std::clamp(worldMousePos.y - listBox.dragGrabOffset, trackTop, trackTop + trackHeight - thumbHeight);
                                float newOffsetRatio = (trackHeight - thumbHeight) > 0.0f
                                    ? (newThumbTop - trackTop) / (trackHeight - thumbHeight)
                                    : 0.0f;
                                int newScroll = static_cast<int>(std::round(newOffsetRatio * static_cast<float>(scrollRange)));
                                listBox.scrollOffset = std::clamp(newScroll, 0, maxScroll);
                            }
                            else
                            {
                                listBox.draggingVerticalScrollbar = false;
                                listBox.dragGrabOffset = 0.0f;
                            }
                        }
                    }

                    
                    if (showHorizontal)
                    {
                        const float trackLeft = contentLeftWorld;
                        const float trackTop = contentTopWorld + availableHeight + trackSpacing;
                        const float trackWidth = availableWidth;
                        const float trackHeight = trackThickness;

                        const float ratio = static_cast<float>(itemsPerPage) / static_cast<float>(itemCount);
                        float thumbWidth = std::max(trackWidth * std::clamp(ratio, 0.0f, 1.0f), trackThickness);
                        const float scrollRange = std::max(1, itemCount - itemsPerPage);
                        float thumbOffset = 0.0f;
                        if (scrollRange > 0)
                        {
                            const float offsetRatio = static_cast<float>(std::clamp((float)listBox.scrollOffset, 0.f, scrollRange)) / static_cast<float>(scrollRange);
                            thumbOffset = (trackWidth - thumbWidth) * std::clamp(offsetRatio, 0.0f, 1.0f);
                        }
                        const float thumbLeft = trackLeft + thumbOffset;

                        const bool insideTrack = (worldMousePos.x >= trackLeft && worldMousePos.x <= trackLeft + trackWidth &&
                                                  worldMousePos.y >= trackTop && worldMousePos.y <= trackTop + trackHeight);
                        const bool insideThumb = (worldMousePos.x >= thumbLeft && worldMousePos.x <= thumbLeft + thumbWidth &&
                                                  worldMousePos.y >= trackTop && worldMousePos.y <= trackTop + trackHeight);

                        if (registry.all_of<PointerDownEvent>(entity) && insideTrack)
                        {
                            listBox.draggingHorizontalScrollbar = true;
                            listBox.dragGrabOffset = insideThumb ? (worldMousePos.x - thumbLeft) : (thumbWidth * 0.5f);
                        }
                        if (listBox.draggingHorizontalScrollbar)
                        {
                            if (mouseDown)
                            {
                                float newThumbLeft = std::clamp(worldMousePos.x - listBox.dragGrabOffset, trackLeft, trackLeft + trackWidth - thumbWidth);
                                float newOffsetRatio = (trackWidth - thumbWidth) > 0.0f
                                    ? (newThumbLeft - trackLeft) / (trackWidth - thumbWidth)
                                    : 0.0f;
                                int newScroll = static_cast<int>(std::round(newOffsetRatio * static_cast<float>(scrollRange)));
                                listBox.scrollOffset = std::clamp(newScroll, 0, maxScroll);
                            }
                            else
                            {
                                listBox.draggingHorizontalScrollbar = false;
                                listBox.dragGrabOffset = 0.0f;
                            }
                        }
                    }
                }
                Vector2f localMouse = worldToLocal(transform, worldMousePos);
                const float leftLocal = -availableWidth * 0.5f;
                const float rightLocal = availableWidth * 0.5f;
                const float topLocal = -availableHeight * 0.5f;
                const float bottomLocal = availableHeight * 0.5f;
                bool pointerInside = isRuntime &&
                    localMouse.x >= leftLocal && localMouse.x <= rightLocal &&
                    localMouse.y >= topLocal && localMouse.y <= bottomLocal;
                if (pointerInside)
                {
                    int wheelDelta = primaryIsVertical ? scrollWheelY : (scrollWheelX != 0 ? scrollWheelX : scrollWheelY);


                    if (wheelDelta != 0)
                    {
                        int lineStride = (listBox.layout == ListBoxLayout::Horizontal)
                           ? std::max(1, rows)
                           : std::max(1, columns);
                        listBox.scrollOffset = std::clamp(listBox.scrollOffset - wheelDelta * lineStride, 0, maxScroll);
                    }
                }

                const int startIndex = listBox.scrollOffset;
                const int endIndex = (itemsPerPage > 0) ? std::min(itemCount, startIndex + itemsPerPage) : startIndex;

                if (useContainer)
                {
                    
                    const int childCount = static_cast<int>(managedGuids.size());
                    for (int i = 0; i < childCount; ++i)
                    {
                        RuntimeGameObject childGO = scene->FindGameObjectByGuid(managedGuids[i]);
                        if (!childGO.IsValid()) continue;

                        entt::entity childEntity = static_cast<entt::entity>(childGO);
                        if (!registry.valid(childEntity) || !registry.any_of<ECS::TransformComponent>(childEntity))
                        {
                            continue;
                        }

                        auto& childTransform = registry.get<ECS::TransformComponent>(childEntity);

                        if (i < startIndex || i >= endIndex)
                        {
                            
                            
                            childTransform.position.x = transform.position.x + 1.0e7f;
                            childTransform.position.y = transform.position.y + 1.0e7f;
                            continue;
                        }

                        
                        const int baseIndex = i - startIndex;
                        int rowIndex = 0;
                        int columnIndex = 0;
                        if (listBox.layout == ListBoxLayout::Horizontal)
                        {
                            rowIndex = floorMod(baseIndex, rows);
                            columnIndex = floorDiv(baseIndex, rows);
                        }
                        else
                        {
                            columnIndex = floorMod(baseIndex, columns);
                            rowIndex = floorDiv(baseIndex, columns);
                        }

                        const float cellCenterX = contentLeftWorld + static_cast<float>(columnIndex) * (itemWidth + spacingX) + itemWidth * 0.5f;
                        const float cellCenterY = contentTopWorld + static_cast<float>(rowIndex) * (itemHeight + spacingY) + itemHeight * 0.5f;

                        childTransform.position.x = cellCenterX;
                        childTransform.position.y = cellCenterY;
                    }
                }

                int hoveredCandidate = -1;
                if (listBox.Enable && listBox.isInteractable && itemCount > 0 && isRuntime)
                {
                    if (pointerInside && itemsPerPage > 0)
                    {
                        float relativeX = localMouse.x - leftLocal;
                        float relativeY = localMouse.y - topLocal;
                        float stepX = itemWidth + spacingX;
                        float stepY = itemHeight + spacingY;

                        if (listBox.layout == ListBoxLayout::Horizontal)
                        {
                            int columnIdx = std::clamp(static_cast<int>(relativeX / stepX), 0, columns - 1);
                            int rowIdx = std::clamp(static_cast<int>(relativeY / stepY), 0, rows - 1);
                            if (relativeX <= static_cast<float>(columns) * stepX &&
                                                           relativeY <= static_cast<float>(rows) * stepY)
                            {
                                float offsetX = relativeX - static_cast<float>(columnIdx) * stepX;
                                float offsetY = relativeY - static_cast<float>(rowIdx) * stepY;
                                if (offsetX <= itemWidth && offsetY <= itemHeight)

                                {
                                    int visibleIndex = columnIdx * rows + rowIdx;
                                    if (visibleIndex < (endIndex - startIndex))
                                    {
                                        hoveredCandidate = startIndex + visibleIndex;
                                    }
                                }
                            }
                        }
                        else
                        {
                            int columnIdx = std::clamp(static_cast<int>(relativeX / stepX), 0, columns - 1);
                            int rowIdx = std::clamp(static_cast<int>(relativeY / stepY), 0, rows - 1);
                            if (relativeX <= static_cast<float>(columns) * stepX &&
                                relativeY <= static_cast<float>(rows) * stepY)
                            {
                                float offsetX = relativeX - static_cast<float>(columnIdx) * stepX;
                                float offsetY = relativeY - static_cast<float>(rowIdx) * stepY;
                                if (offsetX <= itemWidth && offsetY <= itemHeight)

                                {
                                    int visibleIndex = rowIdx * columns + columnIdx;
                                    if (visibleIndex < (endIndex - startIndex))
                                    {
                                        hoveredCandidate = startIndex + visibleIndex;
                                    }
                                }
                            }
                        }
                    }
                    else if (registry.all_of<PointerExitEvent>(entity))
                    {
                        hoveredCandidate = -1;
                    }
                }
                else
                {
                    hoveredCandidate = -1;
                }

                listBox.hoveredIndex = hoveredCandidate;

                if (listBox.hoveredIndex >= itemCount)
                {
                    listBox.hoveredIndex = -1;
                }

                if (registry.all_of<PointerClickEvent>(entity) &&
                    listBox.hoveredIndex >= 0 &&
                    listBox.hoveredIndex < itemCount)
                {
                    if (listBox.allowMultiSelect)
                    {
                        auto it = std::find(listBox.selectedIndices.begin(), listBox.selectedIndices.end(),
                                            listBox.hoveredIndex);
                        if (it != listBox.selectedIndices.end())
                        {
                            listBox.selectedIndices.erase(it);
                        }
                        else
                        {
                            listBox.selectedIndices.push_back(listBox.hoveredIndex);
                        }
                    }
                    else
                    {
                        listBox.selectedIndices = {listBox.hoveredIndex};
                    }

                    invokeScriptTargets(scene, listBox.onItemActivatedTargets, [&](YAML::Node& args)
                    {
                        args["index"] = listBox.hoveredIndex;
                        if (useContainer && listBox.hoveredIndex < static_cast<int>(managedGuids.size()))
                        {
                            args["itemGuid"] = managedGuids[listBox.hoveredIndex].ToString();
                        }
                        else if (!useContainer && listBox.hoveredIndex < static_cast<int>(listBox.items.size()))
                        {
                            args["text"] = listBox.items[listBox.hoveredIndex];
                        }
                    });
                }

                listBox.selectedIndices.erase(
                    std::remove_if(listBox.selectedIndices.begin(), listBox.selectedIndices.end(),
                                   [itemCount](int idx)
                                   {
                                       return idx < 0 || idx >= itemCount;
                                   }),
                    listBox.selectedIndices.end());
                std::sort(listBox.selectedIndices.begin(), listBox.selectedIndices.end());
                listBox.selectedIndices.erase(
                    std::unique(listBox.selectedIndices.begin(), listBox.selectedIndices.end()),
                    listBox.selectedIndices.end());

                if (previousSelection != listBox.selectedIndices)
                {
                    invokeScriptTargets(scene, listBox.onSelectionChangedTargets, [&](YAML::Node& args)
                    {
                        YAML::Node indicesNode;
                        for (int index : listBox.selectedIndices)
                        {
                            indicesNode.push_back(index);
                        }
                        args["indices"] = indicesNode;

                        if (useContainer)
                        {
                            YAML::Node guidNode;
                            for (int index : listBox.selectedIndices)
                            {
                                if (index >= 0 && index < static_cast<int>(managedGuids.size()))
                                {
                                    guidNode.push_back(managedGuids[index].ToString());
                                }
                            }
                            args["itemGuids"] = guidNode;
                        }
                    });
                }

                if (prevHovered != listBox.hoveredIndex && listBox.hoveredIndex < 0)
                {
                    listBox.hoveredIndex = -1;
                }
            }
        }

    }
}


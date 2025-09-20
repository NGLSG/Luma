#include "ScrollViewSystem.h"
#include "../../Resources/RuntimeAsset/RuntimeScene.h"
#include "InteractionEvents.h"
#include "../../Components/UIComponents.h"
#include "../../Components/Transform.h"
#include <algorithm>
#include <cmath>

#include "SDL3/SDL_events.h"

namespace Systems
{
    void ScrollViewSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
    }

    void ScrollViewSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();

        auto scrollViewView = registry.view<ECS::ScrollViewComponent, ECS::TransformComponent>();

        for (auto entity : scrollViewView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            auto& scrollView = scrollViewView.get<ECS::ScrollViewComponent>(entity);

            handleScrollInteraction(scene, entity, scrollView, context);
        }
    }

    void ScrollViewSystem::handleScrollInteraction(RuntimeScene* scene, entt::entity entity,
                                                   ECS::ScrollViewComponent& scrollView, const EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        auto& transform = registry.get<ECS::TransformComponent>(entity);

        bool scrollChanged = false;
        ECS::Vector2f newScrollPosition = scrollView.scrollPosition;

        ECS::Vector2f mouseWorldPos;
        if (context.appMode == ApplicationMode::Editor)
        {
            if (context.isSceneViewFocused)
            {
                const auto& viewport = context.sceneViewRect;
                if (context.inputState.mousePosition.x >= viewport.x &&
                    context.inputState.mousePosition.x <= (viewport.x + viewport.z) &&
                    context.inputState.mousePosition.y >= viewport.y &&
                    context.inputState.mousePosition.y <= (viewport.y + viewport.w))
                {
                    mouseWorldPos = {
                        (float)context.inputState.mousePosition.x - viewport.x,
                        (float)context.inputState.mousePosition.y - viewport.y
                    };
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else
        {
            mouseWorldPos = {
                (float)context.inputState.mousePosition.x,
                (float)context.inputState.mousePosition.y
            };
        }

        if (isPointInViewport(mouseWorldPos, transform, scrollView.viewportSize))
        {
            for (const auto& event : context.frameEvents.GetView())
            {
                if (event.type == SDL_EVENT_MOUSE_WHEEL)
                {
                    if (scrollView.enableVerticalScroll)
                    {
                        newScrollPosition.y -= event.wheel.y * scrollView.scrollSensitivity;
                        scrollChanged = true;
                    }

                    if (scrollView.enableHorizontalScroll)
                    {
                        newScrollPosition.x -= event.wheel.x * scrollView.scrollSensitivity;
                        scrollChanged = true;
                    }
                }
            }
        }

        if (scrollChanged)
        {
            newScrollPosition = clampScrollPosition(newScrollPosition, scrollView.contentSize, scrollView.viewportSize);

            if (std::abs(newScrollPosition.x - scrollView.lastScrollPosition.x) > 0.01f ||
                std::abs(newScrollPosition.y - scrollView.lastScrollPosition.y) > 0.01f)
            {
                scrollView.scrollPosition = newScrollPosition;
                scrollView.lastScrollPosition = newScrollPosition;
                invokeScrollChangedEvent(scene, scrollView.onScrollChangedTargets, newScrollPosition);
            }
        }
    }

    void ScrollViewSystem::invokeScrollChangedEvent(RuntimeScene* scene,
                                                    const std::vector<ECS::SerializableEventTarget>& targets,
                                                    const ECS::Vector2f& scrollPosition)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptsComponent>())
            {
                auto& scriptsComp = targetGO.GetComponent<ECS::ScriptsComponent>();
                for (const auto& script : scriptsComp.scripts)
                {
                    if (script.metadata && script.metadata->name == target.targetComponentName)
                    {
                        YAML::Node args;
                        args["scrollPosition"]["x"] = scrollPosition.x;
                        args["scrollPosition"]["y"] = scrollPosition.y;
                        std::string argsYaml = YAML::Dump(args);

                        InteractScriptEvent scriptEvent;
                        scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                        scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                        scriptEvent.methodName = target.targetMethodName;
                        scriptEvent.methodArgs = argsYaml;

                        EventBus::GetInstance().Publish(scriptEvent);
                        break;
                    }
                }
            }
        }
    }

    bool ScrollViewSystem::isPointInViewport(const ECS::Vector2f& point, const ECS::TransformComponent& transform,
                                             const ECS::Vector2f& viewportSize)
    {
        ECS::Vector2f halfSize = {
            viewportSize.x * 0.5f * transform.scale.x,
            viewportSize.y * 0.5f * transform.scale.y
        };

        return (point.x >= transform.position.x - halfSize.x) &&
            (point.x <= transform.position.x + halfSize.x) &&
            (point.y >= transform.position.y - halfSize.y) &&
            (point.y <= transform.position.y + halfSize.y);
    }

    ECS::Vector2f ScrollViewSystem::clampScrollPosition(const ECS::Vector2f& scrollPos,
                                                        const ECS::Vector2f& contentSize,
                                                        const ECS::Vector2f& viewportSize)
    {
        ECS::Vector2f clampedPos = scrollPos;

        float maxScrollX = std::max(0.0f, contentSize.x - viewportSize.x);
        float maxScrollY = std::max(0.0f, contentSize.y - viewportSize.y);

        clampedPos.x = std::clamp(clampedPos.x, 0.0f, maxScrollX);
        clampedPos.y = std::clamp(clampedPos.y, 0.0f, maxScrollY);

        return clampedPos;
    }
}

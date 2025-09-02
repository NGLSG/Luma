#include "ButtonSystem.h"

#include "InteractionEvents.h"
#include "Sprite.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Components/UIComponents.h"
#include "Components/Transform.h"

namespace Systems
{
    void ButtonSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
    }

    void ButtonSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();

        auto buttonView = registry.view<ECS::ButtonComponent, ECS::Transform>();

        for (auto entity : buttonView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            auto& button = buttonView.get<ECS::ButtonComponent>(entity);

            if (!button.Enable || !button.isInteractable)
            {
                button.isPressed = false;
                button.isHovered = false;
                continue;
            }

            handleButtonInteraction(scene, entity, button, context);
        }
    }

    void ButtonSystem::handleButtonInteraction(RuntimeScene* scene, entt::entity entity,
                                               ECS::ButtonComponent& button, const EngineContext& context)
    {
        auto& registry = scene->GetRegistry();

        bool wasHovered = button.isHovered;
        bool wasPressed = button.isPressed;
        if (wasHovered && wasPressed)
        {
            auto& sprite = registry.get<ECS::SpriteComponent>(entity);
            sprite.color = button.PressedColor;
        }
        else if (wasHovered && !wasPressed)
        {
            auto& sprite = registry.get<ECS::SpriteComponent>(entity);
            sprite.color = button.HoverColor;
        }
        else if (!wasHovered)
        {
            auto& sprite = registry.get<ECS::SpriteComponent>(entity);
            sprite.color = button.NormalColor;
        }

        button.isHovered = registry.all_of<PointerEnterEvent>(entity) ||
            (button.isHovered && !registry.all_of<PointerExitEvent>(entity));

        if (registry.all_of<PointerDownEvent>(entity))
        {
            button.isPressed = true;
        }

        if (registry.all_of<PointerUpEvent>(entity))
        {
            button.isPressed = false;
        }

        if (registry.all_of<PointerClickEvent>(entity) && button.isHovered)
        {
            invokeButtonEvent(scene, button.onClickTargets);
        }
    }

    void ButtonSystem::invokeButtonEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptComponent>())
            {
                InteractScriptEvent scriptEvent;
                scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                scriptEvent.methodName = target.targetMethodName;
                scriptEvent.methodArgs = "";

                EventBus::GetInstance().Publish(scriptEvent);
            }
        }
    }
}

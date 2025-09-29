#include "PCH.h"
#include "Systems/ButtonSystem.h"

#include "Components/InteractionEvents.h"
#include "Components/ScriptComponent.h"
#include "Components/UIComponents.h"
#include "Event/LumaEvent.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"

namespace
{
    
    
    
    
    
    void InvokeTargets(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets)
    {
        for (const auto& target : targets)
        {
            
            if (!target.targetEntityGuid.Valid()) continue;

            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptsComponent>())
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

namespace Systems
{
    
    
    
    void ButtonSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        
    }

    
    
    
    void ButtonSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        
        auto buttonView = registry.view<ECS::ButtonComponent>();

        for (auto entity : buttonView)
        {
            
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;

            auto& button = buttonView.get<ECS::ButtonComponent>(entity);
            const auto previousState = button.currentState;

            
            
            if (!button.Enable || !button.isInteractable)
            {
                button.currentState = ECS::ButtonState::Disabled;
            }
            else
            {
                
                
                bool hasExited = registry.all_of<PointerExitEvent>(entity);
                bool hasEntered = registry.all_of<PointerEnterEvent>(entity);
                bool isPressedDown = registry.all_of<PointerDownEvent>(entity);

                
                switch (button.currentState)
                {
                case ECS::ButtonState::Normal:
                    if (hasEntered) button.currentState = ECS::ButtonState::Hovered;
                    break;

                case ECS::ButtonState::Hovered:
                    if (hasExited) button.currentState = ECS::ButtonState::Normal;
                    else if (isPressedDown) button.currentState = ECS::ButtonState::Pressed;
                    break;

                case ECS::ButtonState::Pressed:
                    if (hasExited) button.currentState = ECS::ButtonState::Normal;
                    
                    
                    if (!isPressedDown && !registry.all_of<PointerUpEvent>(entity))
                    {
                        button.currentState = ECS::ButtonState::Hovered;
                    }
                    break;

                case ECS::ButtonState::Disabled:
                    
                    button.currentState = ECS::ButtonState::Normal;
                    break;
                }
            }

            
            if (button.currentState != previousState)
            {
                
                if (button.currentState == ECS::ButtonState::Hovered &&
                    (previousState == ECS::ButtonState::Normal || previousState == ECS::ButtonState::Disabled))
                {
                    InvokeTargets(scene, button.onHoverEnterTargets);
                }
                
                else if ((previousState == ECS::ButtonState::Hovered || previousState == ECS::ButtonState::Pressed) &&
                    (button.currentState == ECS::ButtonState::Normal || button.currentState ==
                        ECS::ButtonState::Disabled))
                {
                    InvokeTargets(scene, button.onHoverExitTargets);
                }
            }

            
            
            if (button.isInteractable && registry.all_of<PointerClickEvent>(entity))
            {
                InvokeTargets(scene, button.onClickTargets);
            }
        }
    }
}

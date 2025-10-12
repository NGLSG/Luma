#include "InputTextSystem.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "InteractionEvents.h"
#include "Components/UIComponents.h"
#include "Components/Transform.h"
#include "Components/Sprite.h"
#include "Application/Window.h"
#include <cstring>

#include "RenderSystem.h"
#include "yaml-cpp/yaml.h"
#if !defined(LUMA_DISABLE_SCRIPTING)
#include "../Scripting/ManagedHost.h"
#endif

namespace Systems
{
    constexpr static float CURSOR_BLINK_RATE = 0.5f;

    static size_t GetUtf8CharByteLength(const char* str)
    {
        unsigned char c = static_cast<unsigned char>(*str);
        if (c < 0x80) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    }

    static size_t GetPreviousUtf8CharPosition(const std::string& str, size_t pos)
    {
        if (pos == 0) return 0;

        size_t newPos = pos - 1;
        while (newPos > 0 && (static_cast<unsigned char>(str[newPos]) & 0xC0) == 0x80)
        {
            newPos--;
        }
        return newPos;
    }

    static size_t GetNextUtf8CharPosition(const std::string& str, size_t pos)
    {
        if (pos >= str.length()) return str.length();

        size_t charLen = GetUtf8CharByteLength(&str[pos]);
        return std::min(pos + charLen, str.length());
    }

    void InputTextSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        ctx = &context;
        focusedEntity = entt::null;
    }

    void InputTextSystem::OnDestroy(RuntimeScene* scene)
    {
        if (scene->GetRegistry().valid(focusedEntity))
        {
            OnFocusLost(scene, focusedEntity, *ctx);
        }
        focusedEntity = entt::null;
    }

    void InputTextSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        entt::entity newlyFocused = entt::null;
        bool clickOccurredOnNothing = false;

        auto clickView = registry.view<PointerClickEvent, ECS::InputTextComponent>();
        for (auto entity : clickView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive()) continue;
            auto* inputComp = registry.try_get<ECS::InputTextComponent>(entity);
            if (inputComp && inputComp->Enable)
            {
                newlyFocused = entity;
                break;
            }
        }

        for (const auto& event : context.frameEvents.GetView())
        {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (newlyFocused == entt::null && registry.valid(focusedEntity))
                {
                    clickOccurredOnNothing = true;
                }
                break;
            }
        }

        if (newlyFocused != entt::null)
        {
            if (focusedEntity != newlyFocused)
            {
                if (registry.valid(focusedEntity))
                {
                    OnFocusLost(scene, focusedEntity, context);
                }
                OnFocusGained(scene, newlyFocused, context);
            }
        }
        else if (clickOccurredOnNothing)
        {
            if (registry.valid(focusedEntity))
            {
                OnFocusLost(scene, focusedEntity, context);
            }
        }

        
        entt::entity currentFocused = focusedEntity;

        if (registry.valid(currentFocused))
        {
            HandleActiveInput(scene, currentFocused, context);

            
            
            if (focusedEntity == currentFocused && registry.valid(focusedEntity))
            {
                auto& inputText = registry.get<ECS::InputTextComponent>(focusedEntity);
                inputText.cursorBlinkTimer += deltaTime;
                if (inputText.cursorBlinkTimer >= CURSOR_BLINK_RATE)
                {
                    inputText.cursorBlinkTimer = 0.0f;
                    inputText.isCursorVisible = !inputText.isCursorVisible;
                }
            }
        }
    }

    void InputTextSystem::OnFocusGained(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto& inputText = scene->GetRegistry().get<ECS::InputTextComponent>(entity);
        if (inputText.isFocused) return;

        focusedEntity = entity;
        inputText.isFocused = true;
        inputText.isCursorVisible = true;
        inputText.cursorBlinkTimer = 0.0f;

        
        inputText.inputBuffer = inputText.text.text;
        inputText.cursorPosition = inputText.inputBuffer.length();

        if (context.window)
        {
            context.commandsForRender.Push([window = context.window]()
            {
                window->StartTextInput();
            });
        }
    }

    void InputTextSystem::OnFocusLost(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto* inputText = scene->GetRegistry().try_get<ECS::InputTextComponent>(entity);
        if (!inputText || !inputText->isFocused) return;

        inputText->isFocused = false;
        inputText->isCursorVisible = false;

        
        if (inputText->text.text != inputText->inputBuffer)
        {
            inputText->text.text = inputText->inputBuffer;
            InvokeTextChangedEvent(scene, inputText->onTextChangedTargets, inputText->inputBuffer);
        }

        if (focusedEntity == entity)
        {
            focusedEntity = entt::null;
            if (context.window)
            {
                context.commandsForRender.Push([window = context.window]()
                {
                    window->StopTextInput();
                });
            }
        }
    }


    void Systems::InputTextSystem::HandleActiveInput(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        auto& inputText = registry.get<ECS::InputTextComponent>(entity);
        bool textChanged = false;
        bool cursorMoved = false;

        
        if (inputText.isReadOnly)
        {
            return;
        }

        
        for (const auto& event : context.frameEvents.GetView())
        {
            
            if (event.type == SDL_EVENT_TEXT_INPUT)
            {
                std::string inputTextStr = event.text.text;
                if (inputText.inputBuffer.length() + inputTextStr.length() <= static_cast<size_t>(inputText.maxLength))
                {
                    inputText.inputBuffer.insert(inputText.cursorPosition, inputTextStr);
                    inputText.cursorPosition += inputTextStr.length();
                    textChanged = true;
                }
            }
            
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                switch (event.key.key)
                {
                case SDLK_BACKSPACE:
                    if (inputText.cursorPosition > 0)
                    {
                        
                        size_t prevPos = GetPreviousUtf8CharPosition(inputText.inputBuffer, inputText.cursorPosition);
                        size_t deleteLength = inputText.cursorPosition - prevPos;

                        inputText.inputBuffer.erase(prevPos, deleteLength);
                        inputText.cursorPosition = prevPos;
                        textChanged = true;
                    }
                    break;

                case SDLK_DELETE:
                    if (inputText.cursorPosition < inputText.inputBuffer.length())
                    {
                        
                        size_t deleteLength = GetUtf8CharByteLength(&inputText.inputBuffer[inputText.cursorPosition]);
                        inputText.inputBuffer.erase(inputText.cursorPosition, deleteLength);
                        textChanged = true;
                    }
                    break;

                case SDLK_LEFT:
                    if (inputText.cursorPosition > 0)
                    {
                        
                        inputText.cursorPosition = GetPreviousUtf8CharPosition(
                            inputText.inputBuffer, inputText.cursorPosition);
                        cursorMoved = true;
                    }
                    break;

                case SDLK_RIGHT:
                    if (inputText.cursorPosition < inputText.inputBuffer.length())
                    {
                        
                        inputText.cursorPosition = GetNextUtf8CharPosition(
                            inputText.inputBuffer, inputText.cursorPosition);
                        cursorMoved = true;
                    }
                    break;

                case SDLK_HOME:
                    if (inputText.cursorPosition > 0)
                    {
                        inputText.cursorPosition = 0;
                        cursorMoved = true;
                    }
                    break;

                case SDLK_END:
                    if (inputText.cursorPosition < inputText.inputBuffer.length())
                    {
                        inputText.cursorPosition = inputText.inputBuffer.length();
                        cursorMoved = true;
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    
                    
                    InvokeSubmitEvent(scene, inputText.onSubmitTargets, inputText.inputBuffer);
                    OnFocusLost(scene, entity, context);
                    return;

                case SDLK_ESCAPE:
                    
                    
                    inputText.inputBuffer = inputText.text.text;
                    OnFocusLost(scene, entity, context);
                    return;

                default:
                    break;
                }
            }
        }

        
        if (textChanged || cursorMoved)
        {
            inputText.isCursorVisible = true;
            inputText.cursorBlinkTimer = 0.0f;
        }
    }

    void InputTextSystem::InvokeTextChangedEvent(RuntimeScene* scene,
                                                 const std::vector<ECS::SerializableEventTarget>& targets,
                                                 const std::string& newText)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptsComponent>())
            {
                YAML::Node args;
                args["text"] = newText;

                InteractScriptEvent scriptEvent;
                scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                scriptEvent.methodName = target.targetMethodName;
                scriptEvent.methodArgs = YAML::Dump(args);

                EventBus::GetInstance().Publish(scriptEvent);
            }
        }
    }

    void InputTextSystem::InvokeSubmitEvent(RuntimeScene* scene,
                                            const std::vector<ECS::SerializableEventTarget>& targets,
                                            const std::string& text)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptsComponent>())
            {
                YAML::Node args;
                args["text"] = text;

                InteractScriptEvent scriptEvent;
                scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                scriptEvent.methodName = target.targetMethodName;
                scriptEvent.methodArgs = YAML::Dump(args);

                EventBus::GetInstance().Publish(scriptEvent);
            }
        }
    }
}

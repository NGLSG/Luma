#include "InputTextSystem.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "InteractionEvents.h"
#include "Components/UIComponents.h"
#include "Components/Transform.h"
#include "Components/Sprite.h"
#include "Application/Window.h"
#include <cstring>
#include "yaml-cpp/yaml.h"
#include "../Scripting/CoreCLRHost.h"

namespace Systems
{
    static ECS::Vector2f worldToScreen(const ECS::Vector2f& worldPos, const Camera::CamProperties& cameraProps,
                                       const ECS::RectF& viewport)
    {
        const float localX = (worldPos.x - cameraProps.position.x()) * cameraProps.zoom;
        const float localY = (worldPos.y - cameraProps.position.y()) * cameraProps.zoom;

        const float screenX = localX + viewport.x;
        const float screenY = localY + viewport.y;
        return {screenX, screenY};
    }

    void InputTextSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        focusedEntity = entt::null;
    }

    void InputTextSystem::OnDestroy(RuntimeScene* scene)
    {
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
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            if (registry.get<ECS::InputTextComponent>(entity).Enable)
            {
                newlyFocused = entity;
            }
        }

        for (const auto& event : context.frameEvents)
        {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (newlyFocused == entt::null)
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

        if (registry.valid(focusedEntity))
        {
            handleActiveInput(scene, focusedEntity, context);
        }

        if (!registry.valid(focusedEntity))
        {
            if (context.window && context.window->IsTextInputActive())
            {
                context.window->StopTextInput();
            }
            focusedEntity = entt::null;
        }
    }

    void InputTextSystem::OnFocusGained(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto& inputText = scene->GetRegistry().get<ECS::InputTextComponent>(entity);
        if (inputText.isFocused) return;

        focusedEntity = entity;
        inputText.isFocused = true;
#ifdef _WIN32
        strncpy_s(inputText.inputBuffer, sizeof(inputText.inputBuffer),
                  inputText.text.text.c_str(), _TRUNCATE);
#else
        strncpy(inputText.inputBuffer, inputText.text.text.c_str(), sizeof(inputText.inputBuffer) - 1);
        inputText.inputBuffer[sizeof(inputText.inputBuffer) - 1] = '\0';
#endif

        inputText.lastText = inputText.text.text;

        if (context.window)
        {
            context.window->StartTextInput();
        }
    }

    void InputTextSystem::OnFocusLost(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto* inputText = scene->GetRegistry().try_get<ECS::InputTextComponent>(entity);
        if (!inputText || !inputText->isFocused) return;

        inputText->isFocused = false;

        std::string newText = inputText->inputBuffer;
        inputText->text.text = newText;
        if (newText != inputText->lastText)
        {
            invokeTextChangedEvent(scene, inputText->onTextChangedTargets, newText);
        }

        if (focusedEntity == entity)
        {
            focusedEntity = entt::null;
            if (context.window)
            {
                context.window->StopTextInput();
            }
        }
    }

    void InputTextSystem::handleActiveInput(RuntimeScene* scene, entt::entity entity, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        auto& inputText = registry.get<ECS::InputTextComponent>(entity);
        auto& transform = registry.get<ECS::Transform>(entity);

        if (context.window)
        {
            auto cameraProps = scene->GetCameraProperties();
            auto viewport = context.appMode == ApplicationMode::PIE
                                ? context.sceneViewRect
                                : ECS::RectF{0, 0, cameraProps.viewport.width(), cameraProps.viewport.height()};

            float width = 100.0f;
            float height = 20.0f;
            if (auto* sprite = registry.try_get<ECS::SpriteComponent>(entity))
            {
                if (sprite->image)
                {
                    width = sprite->sourceRect.Width();
                    height = sprite->sourceRect.Height();
                }
            }

            ECS::Vector2f screenPos = worldToScreen(transform.position, cameraProps, viewport);
            SDL_Rect imeRect = {
                static_cast<int>(screenPos.x),
                static_cast<int>(screenPos.y),
                static_cast<int>(width * transform.scale.x * cameraProps.zoom),
                static_cast<int>(height * transform.scale.y * cameraProps.zoom)
            };
            context.window->SetTextInputArea(imeRect, static_cast<int>(strlen(inputText.inputBuffer)));
        }

        bool textChanged = false;
        if (!inputText.isReadOnly)
        {
            for (const auto& event : context.frameEvents)
            {
                if (event.type == SDL_EVENT_TEXT_INPUT)
                {
                    size_t bufferLen = strlen(inputText.inputBuffer);
                    size_t inputLen = strlen(event.text.text);
                    if (bufferLen + inputLen < sizeof(inputText.inputBuffer) &&
                        static_cast<int>(bufferLen + inputLen) < inputText.maxLength)
                    {
#ifdef _WIN32
                        strcat_s(inputText.inputBuffer, sizeof(inputText.inputBuffer), event.text.text);
#else
                        strncat(inputText.inputBuffer, event.text.text, sizeof(inputText.inputBuffer) - bufferLen - 1);
#endif
                        textChanged = true;
                    }
                }
                else if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    size_t bufferLen = strlen(inputText.inputBuffer);
                    if (event.key.key == SDLK_BACKSPACE && bufferLen > 0)
                    {
                        inputText.inputBuffer[bufferLen - 1] = '\0';
                        textChanged = true;
                    }
                    else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
                    {
                        invokeSubmitEvent(scene, inputText.onSubmitTargets, inputText.inputBuffer);
                        OnFocusLost(scene, entity, context);
                    }
                    else if (event.key.key == SDLK_ESCAPE)
                    {
#ifdef _WIN32
                        strncpy_s(inputText.inputBuffer, sizeof(inputText.inputBuffer),
                                  inputText.lastText.c_str(), _TRUNCATE);
#else
                        strncpy(inputText.inputBuffer, inputText.lastText.c_str(), sizeof(inputText.inputBuffer) - 1);
                        inputText.inputBuffer[sizeof(inputText.inputBuffer) - 1] = '\0';
#endif
                        OnFocusLost(scene, entity, context);
                    }
                }
            }
        }

        if (textChanged)
        {
            inputText.text.text = inputText.inputBuffer;
        }
    }

    void InputTextSystem::invokeTextChangedEvent(RuntimeScene* scene,
                                                 const std::vector<ECS::SerializableEventTarget>& targets,
                                                 const std::string& newText)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptComponent>())
            {
                YAML::Node args;
                args["text"] = newText;
                std::string argsYaml = YAML::Dump(args);


                InteractScriptEvent scriptEvent;
                scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                scriptEvent.methodName = target.targetMethodName;
                scriptEvent.methodArgs = argsYaml;

                EventBus::GetInstance().Publish(scriptEvent);
            }
        }
    }

    void InputTextSystem::invokeSubmitEvent(RuntimeScene* scene,
                                            const std::vector<ECS::SerializableEventTarget>& targets,
                                            const std::string& text)
    {
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = scene->FindGameObjectByGuid(target.targetEntityGuid);
            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptComponent>())
            {
                YAML::Node args;
                args["text"] = text;
                std::string argsYaml = YAML::Dump(args);


                InteractScriptEvent scriptEvent;
                scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                scriptEvent.methodName = target.targetMethodName;
                scriptEvent.methodArgs = argsYaml;

                EventBus::GetInstance().Publish(scriptEvent);
            }
        }
    }
}

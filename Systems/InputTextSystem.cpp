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
#include "../Scripting/CoreCLRHost.h"
#include "External/skia-linux/include/include/core/SkFontTypes.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"

namespace Systems
{
    constexpr static float CURSOR_BLINK_RATE = 0.5f;

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

            auto* inputComp = registry.try_get<ECS::InputTextComponent>(entity);
            if (inputComp && inputComp->Enable)
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

            auto& inputText = registry.get<ECS::InputTextComponent>(focusedEntity);
            auto& transform = registry.get<ECS::TransformComponent>(focusedEntity);

            
            inputText.cursorBlinkTimer += deltaTime;
            if (inputText.cursorBlinkTimer >= CURSOR_BLINK_RATE)
            {
                inputText.cursorBlinkTimer = 0.0f;
                inputText.isCursorVisible = !inputText.isCursorVisible;
            }

            
            if (inputText.isFocused && inputText.isCursorVisible && context.renderSystem)
            {
                
                if (inputText.text.typeface)
                {
                    SkFont font(inputText.text.typeface, inputText.text.fontSize);
                    SkFontMetrics metrics;
                    font.getMetrics(&metrics);

                    

                    
                    std::string textBeforeCursor = std::string(inputText.inputBuffer).substr(
                        0, inputText.cursorPosition);
                    std::string fullText = std::string(inputText.inputBuffer);

                    SkRect boundsBeforeCursor;
                    font.measureText(textBeforeCursor.c_str(), textBeforeCursor.size(), SkTextEncoding::kUTF8,
                                     &boundsBeforeCursor);
                    float widthBeforeCursor = boundsBeforeCursor.width();

                    SkRect fullTextBounds;
                    font.measureText(fullText.c_str(), fullText.size(), SkTextEncoding::kUTF8, &fullTextBounds);
                    float fullTextWidth = fullTextBounds.width();

                    
                    
                    float localCursorX = widthBeforeCursor;

                    
                    float s = sinf(transform.rotation);
                    float c = cosf(transform.rotation);
                    float worldOffsetX = localCursorX * c * transform.scale.x;
                    float worldOffsetY = localCursorX * s * transform.scale.y;

                    float finalCenterX = transform.position.x + worldOffsetX;
                    float finalCenterY = transform.position.y + worldOffsetY;

                    
                    float cursorHeight = (metrics.fDescent - metrics.fAscent) * transform.scale.y;
                    SkPoint cursorTopLeft = {
                        finalCenterX,
                        finalCenterY - cursorHeight / 2.0f 
                    };

                    
                    context.renderSystem->DrawCursor(cursorTopLeft, cursorHeight, inputText.text.color);
                }
            }
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

        
        inputText.isCursorVisible = true;
        inputText.cursorBlinkTimer = 0.0f;

        
#ifdef _WIN32
        strncpy_s(inputText.inputBuffer, sizeof(inputText.inputBuffer),
                  inputText.text.text.c_str(), _TRUNCATE);
#else
        strncpy(inputText.inputBuffer, inputText.text.text.c_str(), sizeof(inputText.inputBuffer) - 1);
        inputText.inputBuffer[sizeof(inputText.inputBuffer) - 1] = '\0';
#endif

        
        inputText.cursorPosition = strlen(inputText.inputBuffer);
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
        
        inputText->isCursorVisible = false;

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
        auto& transform = registry.get<ECS::TransformComponent>(entity);

        
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
            context.window->SetTextInputArea(imeRect, static_cast<int>(inputText.cursorPosition));
        }

        bool textChanged = false;
        bool cursorMoved = false;

        if (inputText.isReadOnly)
        {
            return; 
        }

        
        std::string currentText(inputText.inputBuffer);

        for (const auto& event : context.frameEvents)
        {
            
            if (event.type == SDL_EVENT_TEXT_INPUT)
            {
                std::string inputTextStr = event.text.text;
                if (currentText.length() + inputTextStr.length() < sizeof(inputText.inputBuffer) &&
                    currentText.length() + inputTextStr.length() <= static_cast<size_t>(inputText.maxLength))
                {
                    currentText.insert(inputText.cursorPosition, inputTextStr);
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
                        currentText.erase(inputText.cursorPosition - 1, 1);
                        inputText.cursorPosition--;
                        textChanged = true;
                    }
                    break;

                
                case SDLK_DELETE:
                    if (inputText.cursorPosition < currentText.length())
                    {
                        currentText.erase(inputText.cursorPosition, 1);
                        textChanged = true;
                    }
                    break;

                
                case SDLK_LEFT:
                    if (inputText.cursorPosition > 0)
                    {
                        inputText.cursorPosition--;
                        cursorMoved = true;
                    }
                    break;

                
                case SDLK_RIGHT:
                    if (inputText.cursorPosition < currentText.length())
                    {
                        inputText.cursorPosition++;
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
                    if (inputText.cursorPosition < currentText.length())
                    {
                        inputText.cursorPosition = currentText.length();
                        cursorMoved = true;
                    }
                    break;

                
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    invokeSubmitEvent(scene, inputText.onSubmitTargets, inputText.inputBuffer);
                    OnFocusLost(scene, entity, context);
                    return; 

                
                case SDLK_ESCAPE:
#ifdef _WIN32
                    strncpy_s(inputText.inputBuffer, sizeof(inputText.inputBuffer),
                              inputText.lastText.c_str(), _TRUNCATE);
#else
                    strncpy(inputText.inputBuffer, inputText.lastText.c_str(), sizeof(inputText.inputBuffer) - 1);
                    inputText.inputBuffer[sizeof(inputText.inputBuffer) - 1] = '\0';
#endif
                    OnFocusLost(scene, entity, context);
                    return; 

                default:
                    break;
                }
            }
        }

        
        if (textChanged)
        {
            inputText.text.text = currentText;
#ifdef _WIN32
            strncpy_s(inputText.inputBuffer, sizeof(inputText.inputBuffer),
                      currentText.c_str(), _TRUNCATE);
#else
            
            strncpy(inputText.inputBuffer, currentText.c_str(), sizeof(inputText.inputBuffer) - 1);
            inputText.inputBuffer[sizeof(inputText.inputBuffer) - 1] = '\0';
#endif
        }

        
        if (textChanged || cursorMoved)
        {
            inputText.isCursorVisible = true;
            inputText.cursorBlinkTimer = 0.0f;
        }
    }

    void InputTextSystem::invokeTextChangedEvent(RuntimeScene* scene,
                                                 const std::vector<ECS::SerializableEventTarget>& targets,
                                                 const std::string& newText)
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
                        args["text"] = newText;
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

    void InputTextSystem::invokeSubmitEvent(RuntimeScene* scene,
                                            const std::vector<ECS::SerializableEventTarget>& targets,
                                            const std::string& text)
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
                        args["text"] = text;
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
}

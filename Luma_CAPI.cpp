#include "Luma_CAPI.h"

#include <AudioManager.h>
#include <Loaders/AudioLoader.h>
#include <algorithm>
#include <array>
#include <utility>
#include <cstring>

#include "AnimationControllerComponent.h"
#include "ColliderComponent.h"
#include "PathUtils.h"
#include "UIComponents.h"
#include "PhysicsSystem.h"
#include "ProjectSettings.h"
#include "SceneManager.h"
#include "SIMDWrapper.h"
#include "TagComponent.h"
#include "TextComponent.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Components/ComponentRegistry.h"
#include "Event/JobSystem.h"
#include "Input/Cursor.h"
#include "Input/Keyboards.h"
#include "RuntimeAsset/RuntimeAnimationController.h"
#include "Renderer/GraphicsBackend.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Camera.h"
#include "Renderer/Nut/NutContext.h"
#include "Renderer/Nut/Buffer.h"
#include "Renderer/Nut/TextureA.h"
#include "Renderer/Nut/ShaderRegistry.h"
#include "Resources/RuntimeAsset/RuntimeWGSLMaterial.h"
#include "Resources/RuntimeAsset/RuntimeTexture.h"
#include "Resources/Managers/RuntimeWGSLMaterialManager.h"
#include "Components/Sprite.h"
#include "Loaders/TextureLoader.h"
#include "Utils/AndroidPermissions.h"
#ifdef __ANDROID__
#include <android/native_window.h>
#endif

static std::vector<std::string> BuildPermissionList(const char** permissions, int count)
{
    std::vector<std::string> output;
    if (!permissions || count <= 0)
    {
        return output;
    }
    output.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const char* entry = permissions[i];
        if (entry && entry[0] != '\0')
        {
            output.emplace_back(entry);
        }
    }
    return output;
}

static inline RuntimeScene* AsScene(LumaSceneHandle handle)
{
    return reinterpret_cast<RuntimeScene*>(handle);
}

static inline RuntimeGameObject AsGameObject(LumaSceneHandle scene, LumaEntityHandle entity)
{
    return AsScene(scene)->FindGameObjectByEntity((entt::entity)entity);
}


static inline RuntimeAnimationController* GetController(LumaSceneHandle scene, LumaEntityHandle entity)
{
    if (!scene) return nullptr;

    auto* acc = static_cast<ECS::AnimationControllerComponent*>(Entity_GetComponent(
        scene, entity, "AnimationControllerComponent"));


    if (!acc || !acc->runtimeController)
    {
        return nullptr;
    }
    return acc->runtimeController.get();
}

static inline void PublishComponentUpdated(RuntimeScene* runtimeScene, LumaEntityHandle entity)
{
    if (!runtimeScene) return;

    EventBus::GetInstance().Publish(ComponentUpdatedEvent{
        runtimeScene->GetRegistry(), (entt::entity)entity
    });
}

static inline Guid_CAPI ToGuidCAPI(const Guid& guid)
{
    Guid_CAPI result{};
    const auto& bytes = guid.GetBytes();
    std::memcpy(&result, bytes.data(), bytes.size());
    return result;
}

static inline Guid FromGuidCAPI(const Guid_CAPI& guid)
{
    std::array<uint8_t, 16> bytes{};
    std::memcpy(bytes.data(), &guid, bytes.size());
    return Guid(bytes);
}

static inline SerializableEventTarget_CAPI ToSerializableEventTargetCAPI(const ECS::SerializableEventTarget& target)
{
    SerializableEventTarget_CAPI result{};
    result.targetEntityGuid = ToGuidCAPI(target.targetEntityGuid);
    result.targetComponentName = target.targetComponentName.c_str();
    result.targetMethodName = target.targetMethodName.c_str();
    return result;
}

LUMA_API bool Platform_HasPermissions(const char** permissions, int count)
{
#if defined(__ANDROID__)
    auto list = BuildPermissionList(permissions, count);
    if (list.empty())
    {
        return true;
    }
    return Platform::Android::HasPermissions(list);
#else
    (void)permissions;
    (void)count;
    return true;
#endif
}

LUMA_API bool Platform_RequestPermissions(const char** permissions, int count)
{
#if defined(__ANDROID__)
    auto list = BuildPermissionList(permissions, count);
    if (list.empty())
    {
        return true;
    }
    return Platform::Android::AcquirePermissions(list);
#else
    (void)permissions;
    (void)count;
    return true;
#endif
}


LUMA_API const char* InputTextComponent_GetText(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return "";
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return "";
    return comp->text.text.c_str();
}

LUMA_API void InputTextComponent_SetText(LumaSceneHandle scene, LumaEntityHandle entity, const char* text)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !text) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    comp->text.text = text;
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API const char* InputTextComponent_GetPlaceholder(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return "";
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return "";
    return comp->placeholder.text.c_str();
}

LUMA_API void InputTextComponent_SetPlaceholder(LumaSceneHandle scene, LumaEntityHandle entity, const char* text)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !text) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    comp->placeholder.text = text;
    PublishComponentUpdated(runtimeScene, entity);
}


LUMA_API int ButtonComponent_GetOnClickTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return 0;
    const auto* comp = TryGetComponent<ECS::ButtonComponent>(runtimeScene, entity);
    return comp ? static_cast<int>(comp->onClickTargets.size()) : 0;
}

LUMA_API bool ButtonComponent_GetOnClickTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                               SerializableEventTarget_CAPI* outTarget)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outTarget) return false;
    const auto* comp = TryGetComponent<ECS::ButtonComponent>(runtimeScene, entity);
    if (!comp) return false;
    if (index < 0 || index >= static_cast<int>(comp->onClickTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(comp->onClickTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ButtonComponent_ClearOnClickTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::ButtonComponent>(runtimeScene, entity);
    if (!comp) return;
    comp->onClickTargets.clear();
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API void ButtonComponent_AddOnClickTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                               const char* componentName, const char* methodName)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::ButtonComponent>(runtimeScene, entity);
    if (!comp) return;
    ECS::SerializableEventTarget target{};
    target.targetEntityGuid = FromGuidCAPI(targetGuid);
    if (componentName) target.targetComponentName = componentName;
    if (methodName) target.targetMethodName = methodName;
    comp->onClickTargets.push_back(std::move(target));
    PublishComponentUpdated(runtimeScene, entity);
}


LUMA_API int InputTextComponent_GetOnTextChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return 0;
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    return comp ? static_cast<int>(comp->onTextChangedTargets.size()) : 0;
}


LUMA_API int ToggleButtonComponent_GetOnToggleOnTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity);
    return c ? static_cast<int>(c->onToggleOnTargets.size()) : 0;
}

LUMA_API bool ToggleButtonComponent_GetOnToggleOnTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onToggleOnTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onToggleOnTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ToggleButtonComponent_ClearOnToggleOnTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity))
    {
        c->onToggleOnTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ToggleButtonComponent_AddOnToggleOnTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onToggleOnTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int ToggleButtonComponent_GetOnToggleOffTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity);
    return c ? static_cast<int>(c->onToggleOffTargets.size()) : 0;
}

LUMA_API bool ToggleButtonComponent_GetOnToggleOffTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                         SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onToggleOffTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onToggleOffTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ToggleButtonComponent_ClearOnToggleOffTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity))
    {
        c->onToggleOffTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ToggleButtonComponent_AddOnToggleOffTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                         Guid_CAPI targetGuid,
                                                         const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ToggleButtonComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onToggleOffTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int RadioButtonComponent_GetOnSelectedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity);
    return c ? static_cast<int>(c->onSelectedTargets.size()) : 0;
}

LUMA_API bool RadioButtonComponent_GetOnSelectedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                       SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onSelectedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onSelectedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void RadioButtonComponent_ClearOnSelectedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity))
    {
        c->onSelectedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void RadioButtonComponent_AddOnSelectedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                       Guid_CAPI targetGuid,
                                                       const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onSelectedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int RadioButtonComponent_GetOnDeselectedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity);
    return c ? static_cast<int>(c->onDeselectedTargets.size()) : 0;
}

LUMA_API bool RadioButtonComponent_GetOnDeselectedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                         SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onDeselectedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onDeselectedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void RadioButtonComponent_ClearOnDeselectedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity))
    {
        c->onDeselectedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void RadioButtonComponent_AddOnDeselectedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                         Guid_CAPI targetGuid,
                                                         const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::RadioButtonComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onDeselectedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int CheckBoxComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::CheckBoxComponent>(s, entity);
    return c ? static_cast<int>(c->onValueChangedTargets.size()) : 0;
}

LUMA_API bool CheckBoxComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::CheckBoxComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onValueChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onValueChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void CheckBoxComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::CheckBoxComponent>(s, entity))
    {
        c->onValueChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void CheckBoxComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::CheckBoxComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onValueChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int SliderComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    return c ? static_cast<int>(c->onValueChangedTargets.size()) : 0;
}

LUMA_API bool SliderComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                      SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onValueChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onValueChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void SliderComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        c->onValueChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void SliderComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                      Guid_CAPI targetGuid,
                                                      const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onValueChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int SliderComponent_GetOnDragStartedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    return c ? static_cast<int>(c->onDragStartedTargets.size()) : 0;
}

LUMA_API bool SliderComponent_GetOnDragStartedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                     SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onDragStartedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onDragStartedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void SliderComponent_ClearOnDragStartedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        c->onDragStartedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void SliderComponent_AddOnDragStartedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                     Guid_CAPI targetGuid,
                                                     const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onDragStartedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int SliderComponent_GetOnDragEndedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    return c ? static_cast<int>(c->onDragEndedTargets.size()) : 0;
}

LUMA_API bool SliderComponent_GetOnDragEndedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                   SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::SliderComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onDragEndedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onDragEndedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void SliderComponent_ClearOnDragEndedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        c->onDragEndedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void SliderComponent_AddOnDragEndedTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                                   const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::SliderComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onDragEndedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int ComboBoxComponent_GetOnSelectionChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ComboBoxComponent>(s, entity);
    return c ? static_cast<int>(c->onSelectionChangedTargets.size()) : 0;
}

LUMA_API bool ComboBoxComponent_GetOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                            SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ComboBoxComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onSelectionChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onSelectionChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ComboBoxComponent_ClearOnSelectionChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ComboBoxComponent>(s, entity))
    {
        c->onSelectionChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ComboBoxComponent_AddOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                            Guid_CAPI targetGuid,
                                                            const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ComboBoxComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onSelectionChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int ProgressBarComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity);
    return c ? static_cast<int>(c->onValueChangedTargets.size()) : 0;
}

LUMA_API bool ProgressBarComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                           SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onValueChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onValueChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ProgressBarComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity))
    {
        c->onValueChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ProgressBarComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                           Guid_CAPI targetGuid,
                                                           const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onValueChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int ProgressBarComponent_GetOnCompletedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity);
    return c ? static_cast<int>(c->onCompletedTargets.size()) : 0;
}

LUMA_API bool ProgressBarComponent_GetOnCompletedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onCompletedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onCompletedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ProgressBarComponent_ClearOnCompletedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity))
    {
        c->onCompletedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ProgressBarComponent_AddOnCompletedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ProgressBarComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onCompletedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int TabControlComponent_GetOnTabChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity);
    return c ? static_cast<int>(c->onTabChangedTargets.size()) : 0;
}

LUMA_API bool TabControlComponent_GetOnTabChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onTabChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onTabChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void TabControlComponent_ClearOnTabChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity))
    {
        c->onTabChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void TabControlComponent_AddOnTabChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onTabChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int TabControlComponent_GetOnTabClosedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity);
    return c ? static_cast<int>(c->onTabClosedTargets.size()) : 0;
}

LUMA_API bool TabControlComponent_GetOnTabClosedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                       SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onTabClosedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onTabClosedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void TabControlComponent_ClearOnTabClosedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity))
    {
        c->onTabClosedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void TabControlComponent_AddOnTabClosedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                       Guid_CAPI targetGuid,
                                                       const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::TabControlComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onTabClosedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}


LUMA_API int ListBoxComponent_GetOnSelectionChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity);
    return c ? static_cast<int>(c->onSelectionChangedTargets.size()) : 0;
}

LUMA_API bool ListBoxComponent_GetOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                           SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onSelectionChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onSelectionChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ListBoxComponent_ClearOnSelectionChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity))
    {
        c->onSelectionChangedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ListBoxComponent_AddOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                           Guid_CAPI targetGuid,
                                                           const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onSelectionChangedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API int ListBoxComponent_GetOnItemActivatedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    const auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity);
    return c ? static_cast<int>(c->onItemActivatedTargets.size()) : 0;
}

LUMA_API bool ListBoxComponent_GetOnItemActivatedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* s = AsScene(scene);
    if (!s || !outTarget) return false;
    const auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity);
    if (!c) return false;
    if (index < 0 || index >= static_cast<int>(c->onItemActivatedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(c->onItemActivatedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void ListBoxComponent_ClearOnItemActivatedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity))
    {
        c->onItemActivatedTargets.clear();
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API void ListBoxComponent_AddOnItemActivatedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName)
{
    auto* s = AsScene(scene);
    if (auto* c = TryGetComponent<ECS::ListBoxComponent>(s, entity))
    {
        ECS::SerializableEventTarget t{};
        t.targetEntityGuid = FromGuidCAPI(targetGuid);
        if (componentName) t.targetComponentName = componentName;
        if (methodName) t.targetMethodName = methodName;
        c->onItemActivatedTargets.push_back(std::move(t));
        PublishComponentUpdated(s, entity);
    }
}

LUMA_API bool InputTextComponent_GetOnTextChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outTarget) return false;
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return false;
    if (index < 0 || index >= static_cast<int>(comp->onTextChangedTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(comp->onTextChangedTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void InputTextComponent_ClearOnTextChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    comp->onTextChangedTargets.clear();
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API void InputTextComponent_AddOnTextChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid, const char* componentName,
                                                        const char* methodName)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    ECS::SerializableEventTarget target{};
    target.targetEntityGuid = FromGuidCAPI(targetGuid);
    if (componentName) target.targetComponentName = componentName;
    if (methodName) target.targetMethodName = methodName;
    comp->onTextChangedTargets.push_back(std::move(target));
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API int InputTextComponent_GetOnSubmitTargetCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return 0;
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    return comp ? static_cast<int>(comp->onSubmitTargets.size()) : 0;
}

LUMA_API bool InputTextComponent_GetOnSubmitTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                   SerializableEventTarget_CAPI* outTarget)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outTarget) return false;
    const auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return false;
    if (index < 0 || index >= static_cast<int>(comp->onSubmitTargets.size())) return false;
    *outTarget = ToSerializableEventTargetCAPI(comp->onSubmitTargets[static_cast<size_t>(index)]);
    return true;
}

LUMA_API void InputTextComponent_ClearOnSubmitTargets(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    comp->onSubmitTargets.clear();
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API void InputTextComponent_AddOnSubmitTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                                   const char* componentName, const char* methodName)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;
    auto* comp = TryGetComponent<ECS::InputTextComponent>(runtimeScene, entity);
    if (!comp) return;
    ECS::SerializableEventTarget target{};
    target.targetEntityGuid = FromGuidCAPI(targetGuid);
    if (componentName) target.targetComponentName = componentName;
    if (methodName) target.targetMethodName = methodName;
    comp->onSubmitTargets.push_back(std::move(target));
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API bool Entity_HasComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName)
{
    if (!scene) return false;
    auto* registration = ComponentRegistry::GetInstance().Get(componentName);
    if (!registration || !registration->has) return false;

    return registration->has(AsScene(scene)->GetRegistry(), (entt::entity)entity);
}

LUMA_API void* Entity_AddComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName)
{
    if (!scene) return nullptr;
    auto* registration = ComponentRegistry::GetInstance().Get(componentName);
    if (!registration || !registration->add || !registration->get_raw_ptr) return nullptr;


    registration->add(AsScene(scene)->GetRegistry(), (entt::entity)entity);

    return registration->get_raw_ptr(AsScene(scene)->GetRegistry(), (entt::entity)entity);
}

LUMA_API void* Entity_GetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName)
{
    if (!scene) return nullptr;
    auto* registration = ComponentRegistry::GetInstance().Get(componentName);

    if (!registration || !registration->has || !registration->get_raw_ptr)
    {
        LogError("Component '{}' not found or not registered.", componentName);
        return nullptr;
    }
    if (!registration->has(
        AsScene(scene)->GetRegistry(), (entt::entity)entity))
    {
        LogError("Entity {} does not have component '{}'.", entity, componentName);
        return nullptr;
    }

    return registration->get_raw_ptr(AsScene(scene)->GetRegistry(), (entt::entity)entity);
}

LUMA_API void Entity_RemoveComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName)
{
    if (!scene) return;
    auto* registration = ComponentRegistry::GetInstance().Get(componentName);
    if (registration && registration->remove)
    {
        registration->remove(AsScene(scene)->GetRegistry(), (entt::entity)entity);
    }
}

LUMA_API void Entity_SetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName,
                                  void* componentData, size_t dataSize)
{
    if (!scene || !componentData) return;

    auto* registration = ComponentRegistry::GetInstance().Get(componentName);
    auto* runtimeScene = AsScene(scene);


    if (!registration || !registration->has || !registration->get_raw_ptr ||
        !registration->has(runtimeScene->GetRegistry(), (entt::entity)entity))
    {
        return;
    }

    void* dest = registration->get_raw_ptr(runtimeScene->GetRegistry(), (entt::entity)entity);
    if (!dest) return;


    if (dataSize == 0) return;
    memcpy(dest, componentData, dataSize);

    EventBus::GetInstance().Publish(ComponentUpdatedEvent{
        runtimeScene->GetRegistry(), (entt::entity)entity
    });
}

LUMA_API void Logger_Log(LumaLogLevel level, const char* message)
{
    if (!message) return;

    switch (level)
    {
    case LumaLogLevel_Trace: LogTrace("[C#] {}", message);
        break;
    case LumaLogLevel_Info: LogInfo("[C#] {}", message);
        break;
    case LumaLogLevel_Warning: LogWarn("[C#] {}", message);
        break;
    case LumaLogLevel_Error: LogError("[C#] {}", message);
        break;
    case LumaLogLevel_Critical: LogCritical("[C#] {}", message);
        break;
    }
}


LUMA_API void Event_Invoke_Void(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName)
{
    if (!scene || !eventName) return;
    AsScene(scene)->InvokeEvent((entt::entity)entity, eventName);
}

LUMA_API void Event_Invoke_String(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                  const char* arg1)
{
    if (!scene || !eventName || !arg1) return;
    AsScene(scene)->InvokeEvent((entt::entity)entity, eventName, std::string(arg1));
}

LUMA_API void Event_Invoke_Int(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName, int32_t arg1)
{
    if (!scene || !eventName) return;
    AsScene(scene)->InvokeEvent((entt::entity)entity, eventName, arg1);
}

LUMA_API void Event_Invoke_Float(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName, float arg1)
{
    if (!scene || !eventName) return;
    AsScene(scene)->InvokeEvent((entt::entity)entity, eventName, arg1);
}

LUMA_API void Event_Invoke_Bool(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName, bool arg1)
{
    if (!scene || !eventName) return;
    AsScene(scene)->InvokeEvent((entt::entity)entity, eventName, arg1);
}

LUMA_API void Event_InvokeWithArgs(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                   const char* argsAsYaml)
{
    if (!scene || !eventName || !argsAsYaml) return;

    AsScene(scene)->InvokeEventFromSerializedArgs((entt::entity)entity, eventName, argsAsYaml);
}

LUMA_API LumaEntityHandle Scene_FindGameObjectByGuid(LumaSceneHandle scene, const char* guid)
{
    if (!scene)
    {
        LogError("Scene_FindGameObjectByGuid: scene is null.");
        return 0;
    }
    RuntimeGameObject go = AsScene(scene)->FindGameObjectByGuid(Guid::FromString(guid));
    int i = go.IsValid() ? (LumaEntityHandle)go.GetEntityHandle() : INT_MAX;
    return i;
}

LUMA_API LumaEntityHandle Scene_CreateGameObject(LumaSceneHandle scene, const char* name)
{
    if (!scene) return 0;
    RuntimeGameObject go = AsScene(scene)->CreateGameObject(name);
    return (LumaEntityHandle)go.GetEntityHandle();
}

LUMA_API bool SceneManager_LoadScene(const char* sceneGuidStr)
{
    if (!sceneGuidStr) return false;
    sk_sp<RuntimeScene> scene = SceneManager::GetInstance().LoadScene(Guid::FromString(sceneGuidStr));
    if (scene)
    {
        SceneManager::GetInstance().SetCurrentScene(std::move(scene));
        return true;
    }
    return false;
}


namespace
{
    std::unique_ptr<GraphicsBackend> g_backend;
    std::unique_ptr<RenderSystem> g_renderSystem;
    int g_fbWidth = 0;
    int g_fbHeight = 0;
}

LUMA_API bool Engine_InitWithANativeWindow(void* aNativeWindow, int width, int height)
{
    try
    {
        GraphicsBackendOptions opts;
        opts.width = static_cast<uint16_t>(width > 0 ? width : 1);
        opts.height = static_cast<uint16_t>(height > 0 ? height : 1);
        opts.enableVSync = true;
#if defined(_WIN32)
        opts.backendTypePriority = {BackendType::D3D12, BackendType::D3D11, BackendType::Vulkan, BackendType::OpenGL};
#elif defined(__APPLE__)
        opts.backendTypePriority = {BackendType::Metal, BackendType::OpenGL};
#elif defined(__linux__) && !defined(__ANDROID__)
        opts.backendTypePriority = {BackendType::Vulkan, BackendType::OpenGL};
#elif defined(__ANDROID__)
        opts.backendTypePriority = {BackendType::OpenGLES, BackendType::Vulkan};
#else
#error "Unsupported platform"
#endif

#if defined(__ANDROID__)
        opts.windowHandle.aNativeWindow = reinterpret_cast<ANativeWindow*>(aNativeWindow);
#else
        (void)aNativeWindow;
#endif
        g_backend = GraphicsBackend::Create(opts);
        if (!g_backend) return false;
        g_renderSystem = RenderSystem::Create(*g_backend);
        if (!g_renderSystem)
        {
            g_backend.reset();
            return false;
        }
        g_fbWidth = width;
        g_fbHeight = height;
        auto& activeCamera = CameraManager::GetInstance().GetActiveCamera();
        auto props = activeCamera.GetProperties();
        props.viewport = {0, 0, static_cast<float>(width), static_cast<float>(height)};
        activeCamera.SetProperties(props);
        return true;
    }
    catch (...)
    {
        g_renderSystem.reset();
        g_backend.reset();
        return false;
    }
}

LUMA_API void Engine_Shutdown()
{
    g_renderSystem.reset();
    g_backend.reset();
}

LUMA_API void Engine_Frame(float)
{
    if (!g_renderSystem) return;

    g_renderSystem->Flush();
}

LUMA_API void SceneManager_LoadSceneAsync(const char* sceneGuidStr)
{
    if (!sceneGuidStr) return;
    SceneManager::GetInstance().LoadSceneAsync(Guid::FromString(sceneGuidStr));
}

LUMA_API LumaSceneHandle SceneManager_GetCurrentScene()
{
    return reinterpret_cast<LumaSceneHandle>(SceneManager::GetInstance().GetCurrentScene().get());
}

LUMA_API const char* SceneManager_GetCurrentSceneGuid()
{
    thread_local static std::string guidBuffer;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        guidBuffer = scene->GetGuid().ToString();
        return guidBuffer.c_str();
    }
    return "";
}

LUMA_API const char* GameObject_GetName(LumaSceneHandle scene, LumaEntityHandle entity)
{
    thread_local static std::string nameBuffer;

    if (!scene) return "";
    RuntimeGameObject go = AsScene(scene)->FindGameObjectByEntity((entt::entity)entity);
    if (go.IsValid())
    {
        nameBuffer = go.GetName();
        return nameBuffer.c_str();
    }
    return "";
}

LUMA_API void GameObject_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name)
{
    if (!scene || !name) return;
    RuntimeGameObject go = AsScene(scene)->FindGameObjectByEntity((entt::entity)entity);
    if (go.IsValid())
    {
        go.SetName(name);
    }
}

void GameObject_SetActive(LumaSceneHandle scene, LumaEntityHandle entity, bool active)
{
    if (!scene) return;
    RuntimeGameObject go = AsScene(scene)->FindGameObjectByEntity((entt::entity)entity);
    if (go.IsValid())
    {
        go.SetActive(active);
    }
}

bool GameObject_IsActive(LumaSceneHandle scene, LumaEntityHandle entity)
{
    if (!scene || !entity) return false;
    RuntimeGameObject go = AsScene(scene)->FindGameObjectByEntity((entt::entity)entity);
    if (go.IsValid())
    {
        return go.IsActive();
    }
    LogError("GameObject_IsActive : {} failed", go.GetName());
    return false;
}

LUMA_API void AnimationController_Play(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName,
                                       float speed, float transitionDuration)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (animationName)
        {
            controller->PlayAnimation(animationName, speed, transitionDuration);
        }
    }
}

LUMA_API void AnimationController_Stop(LumaSceneHandle scene, LumaEntityHandle entity)
{
    if (auto* controller = GetController(scene, entity))
    {
        controller->StopAnimation();
    }
}

LUMA_API bool AnimationController_IsPlaying(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (animationName)
        {
            return controller->IsAnimationPlaying(animationName);
        }
    }
    return false;
}

LUMA_API void AnimationController_SetFloat(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                           float value)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (name)
        {
            controller->SetVariable(name, value);
        }
    }
}

LUMA_API void AnimationController_SetBool(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                          bool value)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (name)
        {
            controller->SetVariable(name, value);
        }
    }
}

LUMA_API void AnimationController_SetInt(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                         int value)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (name)
        {
            controller->SetVariable(name, value);
        }
    }
}

LUMA_API void AnimationController_SetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity, float frameRate)
{
    if (auto* controller = GetController(scene, entity))
    {
        controller->SetFrameRate(frameRate);
    }
}

LUMA_API float AnimationController_GetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity)
{
    if (auto* controller = GetController(scene, entity))
    {
        return controller->GetFrameRate();
    }
    return 0.0f;
}

LUMA_API Vector2i_CAPI Cursor_GetPosition()
{
    ECS::Vector2i pos = LumaCursor::GetPosition();
    return {pos.x, pos.y};
}

LUMA_API Vector2i_CAPI Cursor_GetDelta()
{
    ECS::Vector2i delta = LumaCursor::GetDelta();
    return {delta.x, delta.y};
}

LUMA_API Vector2f_CAPI Cursor_GetScrollDelta()
{
    ECS::Vector2f scroll = LumaCursor::GetScrollDelta();
    return {scroll.x, scroll.y};
}

LUMA_API bool Cursor_IsLeftButtonPressed() { return LumaCursor::Left.IsPressed(); }
LUMA_API bool Cursor_IsLeftButtonDown() { return LumaCursor::Left.IsDown(); }
LUMA_API bool Cursor_IsLeftButtonUp() { return LumaCursor::Left.IsUp(); }
LUMA_API Vector2i_CAPI Cursor_GetLeftClickPosition()
{
    const auto& pos = LumaCursor::Left.GetClickPosition();
    return {pos.x, pos.y};
}

LUMA_API bool Cursor_IsRightButtonPressed() { return LumaCursor::Right.IsPressed(); }
LUMA_API bool Cursor_IsRightButtonDown() { return LumaCursor::Right.IsDown(); }
LUMA_API bool Cursor_IsRightButtonUp() { return LumaCursor::Right.IsUp(); }
LUMA_API Vector2i_CAPI Cursor_GetRightClickPosition()
{
    const auto& pos = LumaCursor::Right.GetClickPosition();
    return {pos.x, pos.y};
}

LUMA_API bool Cursor_IsMiddleButtonPressed() { return LumaCursor::Middle.IsPressed(); }
LUMA_API bool Cursor_IsMiddleButtonDown() { return LumaCursor::Middle.IsDown(); }
LUMA_API bool Cursor_IsMiddleButtonUp() { return LumaCursor::Middle.IsUp(); }
LUMA_API Vector2i_CAPI Cursor_GetMiddleClickPosition()
{
    const auto& pos = LumaCursor::Middle.GetClickPosition();
    return {pos.x, pos.y};
}


static const Key* GetKey(int scancode)
{
    if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT)
    {
        return nullptr;
    }

    return &Keyboard::GetInstance().GetKeyState(scancode);
}

LUMA_API bool Keyboard_IsKeyPressed(int scancode)
{
    const Key* key = GetKey(scancode);
    return key ? key->IsPressed() : false;
}

LUMA_API bool Keyboard_IsKeyDown(int scancode)
{
    const Key* key = GetKey(scancode);
    return key ? key->IsDown() : false;
}

LUMA_API bool Keyboard_IsKeyUp(int scancode)
{
    const Key* key = GetKey(scancode);
    return key ? key->IsUp() : false;
}


static ManagedJobCallback s_freeGCHandleCallback = nullptr;


class ManagedJobAdapter final : public IJob
{
public:
    ManagedJobAdapter(ManagedJobCallback callback, void* context)
        : m_callback(callback), m_context(context)
    {
    }


    ManagedJobAdapter(const ManagedJobAdapter&) = delete;
    ManagedJobAdapter& operator=(const ManagedJobAdapter&) = delete;

    void Execute() override
    {
        if (m_callback)
        {
            m_callback(m_context);
        }


        if (s_freeGCHandleCallback && m_context)
        {
            s_freeGCHandleCallback(m_context);
        }


        delete this;
    }

private:
    ManagedJobCallback m_callback;
    void* m_context;
};


LUMA_API void JobSystem_RegisterGCHandleFreeCallback(ManagedJobCallback freeCallback)
{
    s_freeGCHandleCallback = freeCallback;
}

LUMA_API JobHandle_CAPI JobSystem_Schedule(ManagedJobCallback callback, void* context)
{
    if (!callback || !context)
    {
        return nullptr;
    }


    IJob* jobAdapter = new ManagedJobAdapter(callback, context);


    JobHandle cppHandle = JobSystem::GetInstance().Schedule(jobAdapter);


    auto* handlePtr = new JobHandle(std::move(cppHandle));

    return reinterpret_cast<JobHandle_CAPI>(handlePtr);
}

LUMA_API void JobSystem_Complete(JobHandle_CAPI handle)
{
    if (!handle) return;

    auto* cppHandle = reinterpret_cast<JobHandle*>(handle);
    JobSystem::Complete(*cppHandle);


    delete cppHandle;
}

LUMA_API void JobSystem_CompleteAll(JobHandle_CAPI* handles, int count)
{
    if (!handles || count <= 0) return;


    std::vector<JobHandle*> cppHandles;
    cppHandles.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        if (handles[i])
        {
            cppHandles.push_back(reinterpret_cast<JobHandle*>(handles[i]));
        }
    }


    for (JobHandle* handle : cppHandles)
    {
        JobSystem::Complete(*handle);
    }


    for (JobHandle* handle : cppHandles)
    {
        delete handle;
    }
}

LUMA_API int JobSystem_GetThreadCount()
{
    return JobSystem::GetInstance().GetThreadCount();
}

void AnimationController_SetTrigger(LumaSceneHandle scene, LumaEntityHandle entity, const char* name)
{
    if (auto* controller = GetController(scene, entity))
    {
        if (name)
        {
            controller->SetTrigger(name);
        }
    }
}

void SIMDVectorAdd(const float* a, const float* b, float* result, size_t count)
{
    SIMD::GetInstance().VectorAdd(a, b, result, count);
}

void SIMDVectorMultiply(const float* a, const float* b, float* result, size_t count)
{
    SIMD::GetInstance().VectorMultiply(a, b, result, count);
}

float SIMDVectorDotProduct(const float* a, const float* b, size_t count)
{
    return SIMD::GetInstance().VectorDotProduct(a, b, count);
}

void SIMDVectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count)
{
    SIMD::GetInstance().VectorMultiplyAdd(a, b, c, result, count);
}


void SIMDVectorSqrt(const float* input, float* result, size_t count)
{
    SIMD::GetInstance().VectorSqrt(input, result, count);
}

void SIMDVectorReciprocal(const float* input, float* result, size_t count)
{
    SIMD::GetInstance().VectorReciprocal(input, result, count);
}

float SIMDVectorMax(const float* input, size_t count)
{
    return SIMD::GetInstance().VectorMax(input, count);
}

float SIMDVectorMin(const float* input, size_t count)
{
    return SIMD::GetInstance().VectorMin(input, count);
}

void SIMDVectorAbs(const float* input, float* result, size_t count)
{
    SIMD::GetInstance().VectorAbs(input, result, count);
}

void SIMDVectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count)
{
    SIMD::GetInstance().VectorRotatePoints(points_x, points_y, sin_vals, cos_vals, result_x, result_y, count);
}

void ScriptComponent_GetAllGCHandles(LumaSceneHandle scene, uint32_t entity, intptr_t* outHandles, int count)
{
    if (!scene || !outHandles || count <= 0) return;

    auto* runtimeScene = AsScene(scene);
    auto& registry = runtimeScene->GetRegistry();

    if (!registry.valid((entt::entity)entity))
    {
        return;
    }

    if (!registry.all_of<ECS::ScriptsComponent>((entt::entity)entity))
    {
        return;
    }

    const auto& scriptsComp = registry.get<const ECS::ScriptsComponent>((entt::entity)entity);
    int index = 0;
    for (const auto& scriptComp : scriptsComp.scripts)
    {
        if (scriptComp.managedGCHandle && index < count)
        {
            outHandles[index] = *scriptComp.managedGCHandle;
            index++;
        }
    }

    for (int i = index; i < count; i++)
    {
        outHandles[i] = 0;
    }
}

void ScriptComponent_GetAllGCHandlesCount(LumaSceneHandle scene, uint32_t entity, int* outCount)
{
    if (!scene || !outCount) return;

    auto* runtimeScene = AsScene(scene);
    auto& registry = runtimeScene->GetRegistry();
    if (!registry.valid((entt::entity)entity))
    {
        *outCount = 0;
        return;
    }

    if (!registry.all_of<ECS::ScriptsComponent>((entt::entity)entity))
    {
        *outCount = 0;
        return;
    }

    const auto& scriptsComp = registry.get<const ECS::ScriptsComponent>((entt::entity)entity);


    int count = 0;
    for (const auto& scriptComp : scriptsComp.scripts)
    {
        if (scriptComp.managedGCHandle)
        {
            count++;
        }
    }

    *outCount = count;
}

intptr_t ScriptComponent_GetGCHandle(void* componentPtr, const char* typeName)
{
    if (!componentPtr || !typeName) return 0;

    auto* scriptsComp = static_cast<ECS::ScriptsComponent*>(componentPtr);
    auto& sComp = scriptsComp->GetScriptByTypeName(typeName);
    if (sComp.managedGCHandle)
    {
        return *sComp.managedGCHandle;
    }
    return 0;
}

LUMA_API void Physics_ApplyForce(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI force,
                                 ForceMode_CAPI mode)
{
    auto runtimeScene = AsScene(scene);
    if (auto* physicsSystem = runtimeScene->GetSystem<Systems::PhysicsSystem>())
    {
        ECS::Vector2f f = {force.x, force.y};
        Systems::ForceMode m = (mode == ForceMode_Force) ? Systems::ForceMode::Force : Systems::ForceMode::Impulse;
        physicsSystem->ApplyForce(static_cast<entt::entity>(entity), f, m);
    }
}

LUMA_API bool Physics_RayCast(LumaSceneHandle scene, Vector2f_CAPI start, Vector2f_CAPI end, bool penetrate,
                              RayCastResult_CAPI* outHits, int maxHits, int* outHitCount)
{
    if (!outHitCount) return false;
    *outHitCount = 0;

    auto* runtimeScene = AsScene(scene);
    auto* physicsSystem = runtimeScene->GetSystem<Systems::PhysicsSystem>();
    if (!physicsSystem) return false;

    ECS::Vector2f s = {start.x, start.y};
    ECS::Vector2f e = {end.x, end.y};

    auto results = physicsSystem->RayCast(s, e, penetrate);

    if (!results || results->results.empty())
    {
        return false;
    }

    *outHitCount = static_cast<int>(results->results.size());

    if (outHits && maxHits > 0)
    {
        int numToCopy = std::min(*outHitCount, maxHits);
        for (int i = 0; i < numToCopy; ++i)
        {
            const auto& hit = results->results[i];
            outHits[i].hitEntity = static_cast<LumaEntityHandle>(hit.entity);
            outHits[i].point = {hit.point.x, hit.point.y};
            outHits[i].normal = {hit.normal.x, hit.normal.y};
            outHits[i].fraction = hit.fraction;
        }
    }

    return true;
}

LUMA_API bool Physics_CircleCheck(LumaSceneHandle scene, Vector2f_CAPI center, float radius,
                                  const char* tags[], int tagCount, RayCastResult_CAPI* outHit)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outHit) return false;

    auto* physicsSystem = runtimeScene->GetSystem<Systems::PhysicsSystem>();
    if (!physicsSystem) return false;

    ECS::Vector2f c = {center.x, center.y};
    std::vector<std::string> tagsVec;
    if (tags && tagCount > 0)
    {
        tagsVec.assign(tags, tags + tagCount);
    }

    auto result = physicsSystem->CircleCheck(c, radius, runtimeScene->GetRegistry(), tagsVec);

    if (result)
    {
        outHit->hitEntity = static_cast<LumaEntityHandle>(result->entity);
        outHit->point = {result->point.x, result->point.y};
        outHit->normal = {result->normal.x, result->normal.y};
        outHit->fraction = result->fraction;
        return true;
    }

    return false;
}

LUMA_API bool Physics_OverlapCircle(LumaSceneHandle scene, Vector2f_CAPI center, float radius,
                                    const char* tags[], int tagCount,
                                    LumaEntityHandle* outEntities, int maxEntities, int* outEntityCount)
{
    if (!outEntityCount) return false;
    *outEntityCount = 0;

    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return false;

    auto* physicsSystem = runtimeScene->GetSystem<Systems::PhysicsSystem>();
    if (!physicsSystem) return false;

    ECS::Vector2f c = {center.x, center.y};
    std::vector<std::string> tagsVec;
    if (tags && tagCount > 0)
    {
        for (int i = 0; i < tagCount; ++i)
        {
            if (tags[i]) tagsVec.emplace_back(tags[i]);
        }
    }

    auto results = physicsSystem->OverlapCircle(c, radius, runtimeScene->GetRegistry(), tagsVec);

    if (results.empty())
    {
        return false;
    }

    *outEntityCount = static_cast<int>(results.size());

    if (outEntities && maxEntities > 0)
    {
        int numToCopy = std::min(*outEntityCount, maxEntities);
        for (int i = 0; i < numToCopy; ++i)
        {
            outEntities[i] = static_cast<LumaEntityHandle>(results[i]);
        }
    }

    return true;
}

LUMA_API uint32_t AudioManager_Play(PlayDesc_CAPI desc)
{
    if (!desc.audioHandle.Valid()) return 0;

    AudioLoader loader(AudioManager::GetInstance().GetSampleRate(), AudioManager::GetInstance().GetChannels());
    sk_sp<RuntimeAudio> audio = loader.LoadAsset(desc.audioHandle.assetGuid);
    if (!audio) return 0;

    AudioManager::PlayDesc playDesc;
    playDesc.audio = audio;
    playDesc.loop = desc.loop;
    playDesc.volume = desc.volume;
    playDesc.spatial = desc.spatial;
    playDesc.sourceX = desc.sourceX;
    playDesc.sourceY = desc.sourceY;
    playDesc.sourceZ = desc.sourceZ;
    playDesc.minDistance = desc.minDistance;
    playDesc.maxDistance = desc.maxDistance;

    return AudioManager::GetInstance().Play(playDesc);
}

LUMA_API void AudioManager_Stop(uint32_t voiceId)
{
    AudioManager::GetInstance().Stop(voiceId);
}

LUMA_API void AudioManager_StopAll()
{
    AudioManager::GetInstance().StopAll();
}

LUMA_API bool AudioManager_IsFinished(uint32_t voiceId)
{
    return AudioManager::GetInstance().IsFinished(voiceId);
}

LUMA_API void AudioManager_SetVolume(uint32_t voiceId, float volume)
{
    AudioManager::GetInstance().SetVolume(voiceId, volume);
}

LUMA_API void AudioManager_SetLoop(uint32_t voiceId, bool loop)
{
    AudioManager::GetInstance().SetLoop(voiceId, loop);
}

LUMA_API void AudioManager_SetVoicePosition(uint32_t voiceId, float x, float y, float z)
{
    AudioManager::GetInstance().SetVoicePosition(voiceId, x, y, z);
}

LUMA_API void Entity_SetComponentProperty(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName,
                                          const char* propertyName, void* valueData)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !valueData) return;

    const auto* compReg = ComponentRegistry::GetInstance().Get(componentName);
    if (!compReg) return;


    const auto& props = compReg->properties;
    auto propIt = std::find_if(props.begin(), props.end(),
                               [&](const PropertyRegistration& prop)
                               {
                                   return prop.name == propertyName;
                               });

    if (propIt != props.end() && propIt->set_from_raw_ptr)
    {
        propIt->set_from_raw_ptr(runtimeScene->GetRegistry(), (entt::entity)entity, valueData);

        EventBus::GetInstance().Publish(ComponentUpdatedEvent{
            runtimeScene->GetRegistry(), (entt::entity)entity
        });
    }
}

LUMA_API void Entity_GetComponentProperty(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName,
                                          const char* propertyName, void* valueData)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !valueData) return;

    const auto* compReg = ComponentRegistry::GetInstance().Get(componentName);
    if (!compReg) return;


    const auto& props = compReg->properties;
    auto propIt = std::find_if(props.begin(), props.end(),
                               [&](const PropertyRegistration& prop)
                               {
                                   return prop.name == propertyName;
                               });

    if (propIt != props.end() && propIt->get_to_raw_ptr)
    {
        propIt->get_to_raw_ptr(runtimeScene->GetRegistry(), (entt::entity)entity, valueData);
    }
}

LUMA_API const char* TextComponent_GetText(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return "";

    const auto& comp = runtimeScene->GetRegistry().get<ECS::TextComponent>((entt::entity)entity);
    return comp.text.c_str();
}

LUMA_API const char* TextComponent_GetName(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return "";

    const auto& comp = runtimeScene->GetRegistry().get<ECS::TextComponent>((entt::entity)entity);
    return comp.name.c_str();
}

LUMA_API void TextComponent_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !name) return;

    auto& comp = runtimeScene->GetRegistry().get<ECS::TextComponent>((entt::entity)entity);
    comp.name = name;

    EventBus::GetInstance().Publish(ComponentUpdatedEvent{
        runtimeScene->GetRegistry(), (entt::entity)entity
    });
}

LUMA_API void TextComponent_SetText(LumaSceneHandle scene, LumaEntityHandle entity, const char* text)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !text) return;

    auto& comp = runtimeScene->GetRegistry().get<ECS::TextComponent>((entt::entity)entity);
    comp.text = text;

    EventBus::GetInstance().Publish(ComponentUpdatedEvent{
        runtimeScene->GetRegistry(), (entt::entity)entity
    });
}

LUMA_API const char* TagComponent_GetName(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return "";

    const auto* comp = runtimeScene->GetRegistry().try_get<ECS::TagComponent>((entt::entity)entity);
    if (!comp) return "";

    return comp->tag.c_str();
}

LUMA_API void TagComponent_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return;

    auto* comp = runtimeScene->GetRegistry().try_get<ECS::TagComponent>((entt::entity)entity);
    if (!comp) return;

    comp->tag = name ? name : "";
    PublishComponentUpdated(runtimeScene, entity);
}

LUMA_API int PolygonCollider_GetVertexCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return 0;
    const auto& comp = runtimeScene->GetRegistry().get<ECS::PolygonColliderComponent>((entt::entity)entity);
    return static_cast<int>(comp.vertices.size());
}

LUMA_API void PolygonCollider_GetVertices(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI* outVertices)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outVertices) return;
    const auto& comp = runtimeScene->GetRegistry().get<ECS::PolygonColliderComponent>((entt::entity)entity);

    for (size_t i = 0; i < comp.vertices.size(); ++i)
    {
        outVertices[i] = {comp.vertices[i].x, comp.vertices[i].y};
    }
}

LUMA_API void PolygonCollider_SetVertices(LumaSceneHandle scene, LumaEntityHandle entity, const Vector2f_CAPI* vertices,
                                          int count)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !vertices) return;
    auto& comp = runtimeScene->GetRegistry().get<ECS::PolygonColliderComponent>((entt::entity)entity);

    comp.vertices.clear();
    comp.vertices.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        comp.vertices.emplace_back(vertices[i].x, vertices[i].y);
    }
    comp.isDirty = true;
}


LUMA_API int EdgeCollider_GetVertexCount(LumaSceneHandle scene, LumaEntityHandle entity)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene) return 0;
    const auto& comp = runtimeScene->GetRegistry().get<ECS::EdgeColliderComponent>((entt::entity)entity);
    return static_cast<int>(comp.vertices.size());
}

LUMA_API void EdgeCollider_GetVertices(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI* outVertices)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !outVertices) return;
    const auto& comp = runtimeScene->GetRegistry().get<ECS::EdgeColliderComponent>((entt::entity)entity);
    for (size_t i = 0; i < comp.vertices.size(); ++i)
    {
        outVertices[i] = {comp.vertices[i].x, comp.vertices[i].y};
    }
}

LUMA_API void EdgeCollider_SetVertices(LumaSceneHandle scene, LumaEntityHandle entity, const Vector2f_CAPI* vertices,
                                       int count)
{
    auto* runtimeScene = AsScene(scene);
    if (!runtimeScene || !vertices) return;
    auto& comp = runtimeScene->GetRegistry().get<ECS::EdgeColliderComponent>((entt::entity)entity);

    comp.vertices.clear();
    comp.vertices.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        comp.vertices.emplace_back(vertices[i].x, vertices[i].y);
    }
    comp.isDirty = true;
}

LUMA_API bool AssetManager_StartPreload()
{
    return AssetManager::GetInstance().StartPreload();
}

LUMA_API void AssetManager_StopPreload()
{
    AssetManager::GetInstance().StopPreload();
}


LUMA_API bool AssetManager_IsPreloadComplete()
{
    return AssetManager::GetInstance().IsPreloadComplete();
}

LUMA_API bool AssetManager_IsPreloadRunning()
{
    return AssetManager::GetInstance().IsPreloadRunning();
}

LUMA_API void AssetManager_GetPreloadProgress(int* outTotalCount, int* outLoadedCount)
{
    auto res = AssetManager::GetInstance().GetPreloadProgress();
    if (outLoadedCount)
    {
        *outLoadedCount = res.second;
    }
    if (outTotalCount)
    {
        *outTotalCount = res.first;
    }
}

LUMA_API Guid_CAPI LoadAsset(const char* assetPath)
{
    if (!assetPath) return Guid_CAPI{0};

    auto guid = AssetManager::GetInstance().LoadAsset(assetPath);
    if (guid.Valid())
    {
        return ToGuidCAPI(guid);
    }
    return Guid_CAPI{0};
}

LUMA_API Guid_CAPI AssetManager_GetGuidByAddress(const char* address)
{
    if (!address) return Guid_CAPI{0};

    auto guid = AssetManager::GetInstance().GetGuidByAddress(address);
    if (guid.Valid())
    {
        return ToGuidCAPI(guid);
    }
    return Guid_CAPI{0};
}

LUMA_API Guid_CAPI AssetManager_LoadAssetByAddress(const char* address)
{
    if (!address) return Guid_CAPI{0};

    auto guid = AssetManager::GetInstance().LoadAssetByAddress(address);
    if (guid.Valid())
    {
        return ToGuidCAPI(guid);
    }
    return Guid_CAPI{0};
}

LUMA_API const char* PathUtils_GetExecutableDir()
{
    static std::string path = PathUtils::GetExecutableDir().string();
    return path.c_str();
}

LUMA_API const char* PathUtils_GetPersistentDataDir()
{
    static std::string path = PathUtils::GetPersistentDataDir().string();
    return path.c_str();
}

LUMA_API const char* PathUtils_GetCacheDir()
{
    static std::string path = PathUtils::GetCacheDir().string();
    return path.c_str();
}

LUMA_API const char* PathUtils_GetContentDir()
{
    static std::string path = PathUtils::GetContentDir().string();
    return path.c_str();
}

LUMA_API const char* PathUtils_GetAndroidInternalDataDir()
{
    static std::string path = PathUtils::GetAndroidInternalDataDir().string();
    return path.c_str();
}

LUMA_API const char* PathUtils_GetAndroidExternalDataDir()
{
    static std::string path = PathUtils::GetAndroidExternalDataDir().string();
    return path.c_str();
}


LUMA_API WGSLMaterialHandle WGSLMaterial_Load(Guid_CAPI assetGuid)
{
    Guid guid = FromGuidCAPI(assetGuid);
    if (!guid.Valid()) return nullptr;

    auto material = RuntimeWGSLMaterialManager::GetInstance().TryGetAsset(guid);
    return static_cast<WGSLMaterialHandle>(material.get());
}

LUMA_API WGSLMaterialHandle WGSLMaterial_GetFromSprite(LumaSceneHandle scene, LumaEntityHandle entity)
{
    RuntimeScene* runtimeScene = AsScene(scene);
    if (!runtimeScene) return nullptr;

    auto* sprite = TryGetComponent<ECS::SpriteComponent>(runtimeScene, entity);
    if (!sprite || !sprite->wgslMaterial) return nullptr;

    return static_cast<WGSLMaterialHandle>(sprite->wgslMaterial.get());
}

LUMA_API bool WGSLMaterial_IsValid(WGSLMaterialHandle material)
{
    if (!material) return false;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    return mat->IsValid();
}

LUMA_API void WGSLMaterial_SetFloat(WGSLMaterialHandle material, const char* name, float value)
{
    if (!material || !name) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniform<float>(name, value);
}

LUMA_API void WGSLMaterial_SetInt(WGSLMaterialHandle material, const char* name, int value)
{
    if (!material || !name) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniform<int>(name, value);
}

LUMA_API void WGSLMaterial_SetVec2(WGSLMaterialHandle material, const char* name, float x, float y)
{
    if (!material || !name) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniformVec2(name, x, y);
}

LUMA_API void WGSLMaterial_SetVec3(WGSLMaterialHandle material, const char* name, float x, float y, float z)
{
    if (!material || !name) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniformVec3(name, x, y, z);
}

LUMA_API void WGSLMaterial_SetVec4(WGSLMaterialHandle material, const char* name, float r, float g, float b, float a)
{
    if (!material || !name) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniformVec4(name, r, g, b, a);
}

LUMA_API void WGSLMaterial_SetUniformStruct(WGSLMaterialHandle material, const char* name, const void* data, int size)
{
    if (!material || !name || !data || size <= 0) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->SetUniformStruct(name, data, static_cast<size_t>(size));
}

LUMA_API void WGSLMaterial_SetTexture(WGSLMaterialHandle material, const char* name, void* texture, uint32_t binding,
                                      uint32_t group)
{
    if (!material || !name || !texture) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    auto* tex = static_cast<Nut::TextureA*>(texture);

    Nut::TextureAPtr texPtr(tex, [](Nut::TextureA*)
    {
    });
    mat->SetTexture(name, texPtr, binding, group);
}

LUMA_API void WGSLMaterial_UpdateUniformBuffer(WGSLMaterialHandle material)
{
    if (!material) return;
    auto* mat = static_cast<RuntimeWGSLMaterial*>(material);
    mat->UpdateUniformBuffer();
}


LUMA_API TextureAHandle TextureA_Load(Guid_CAPI assetGuid)
{
    Guid guid = FromGuidCAPI(assetGuid);
    if (!guid.Valid()) return nullptr;

    auto loader = new TextureLoader(*GraphicsBackend::GetInstance());
    if (!loader) return nullptr;
    auto texture = loader->LoadAsset(guid);
    if (!texture) return nullptr;

    auto nutTexture = texture->getNutTexture();
    return static_cast<TextureAHandle>(nutTexture.get());
}

LUMA_API TextureAHandle TextureA_Create(uint32_t width, uint32_t height)
{
    auto context = GraphicsBackend::GetInstance()->GetNutContext();
    if (!context) return nullptr;

    auto texture = Nut::TextureBuilder()
                   .SetSize(width, height)
                   .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
                   .SetUsage(Nut::TextureUsageFlags::GetCommonTextureUsage().GetUsage())
                   .Build(context);

    if (!texture) return nullptr;


    static std::vector<Nut::TextureAPtr> s_createdTextures;
    s_createdTextures.push_back(texture);

    return static_cast<TextureAHandle>(texture.get());
}

LUMA_API bool TextureA_IsValid(TextureAHandle texture)
{
    if (!texture) return false;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return static_cast<bool>(*tex);
}

LUMA_API void TextureA_Release(TextureAHandle texture)
{
    (void)texture;
}

LUMA_API uint32_t TextureA_GetWidth(TextureAHandle texture)
{
    if (!texture) return 0;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return static_cast<uint32_t>(tex->GetWidth());
}

LUMA_API uint32_t TextureA_GetHeight(TextureAHandle texture)
{
    if (!texture) return 0;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return static_cast<uint32_t>(tex->GetHeight());
}

LUMA_API uint32_t TextureA_GetDepth(TextureAHandle texture)
{
    if (!texture) return 0;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return static_cast<uint32_t>(tex->GetDepth());
}

LUMA_API uint32_t TextureA_GetMipLevelCount(TextureAHandle texture)
{
    if (!texture) return 0;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return tex->GetMipLevelCount();
}

LUMA_API uint32_t TextureA_GetSampleCount(TextureAHandle texture)
{
    if (!texture) return 0;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return tex->GetSampleCount();
}

LUMA_API bool TextureA_Is3D(TextureAHandle texture)
{
    if (!texture) return false;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return tex->Is3D();
}

LUMA_API bool TextureA_IsArray(TextureAHandle texture)
{
    if (!texture) return false;
    auto* tex = static_cast<Nut::TextureA*>(texture);
    return tex->IsArray();
}


LUMA_API GpuBufferHandle GpuBuffer_Create(uint32_t size, uint32_t usage)
{
    auto context = GraphicsBackend::GetInstance()->GetNutContext();
    if (!context) return nullptr;

    Nut::BufferLayout layout;
    layout.size = size;
    layout.usage = static_cast<wgpu::BufferUsage>(usage);
    layout.mapped = false;

    auto buffer = std::make_unique<Nut::Buffer>(layout, context);
    if (!buffer || !(*buffer)) return nullptr;


    static std::vector<std::unique_ptr<Nut::Buffer>> s_createdBuffers;
    s_createdBuffers.push_back(std::move(buffer));

    return static_cast<GpuBufferHandle>(s_createdBuffers.back().get());
}

LUMA_API bool GpuBuffer_IsValid(GpuBufferHandle buffer)
{
    if (!buffer) return false;
    auto* buf = static_cast<Nut::Buffer*>(buffer);
    return static_cast<bool>(*buf);
}

LUMA_API void GpuBuffer_Release(GpuBufferHandle buffer)
{
    (void)buffer;
}

LUMA_API uint32_t GpuBuffer_GetSize(GpuBufferHandle buffer)
{
    if (!buffer) return 0;
    auto* buf = static_cast<Nut::Buffer*>(buffer);
    return static_cast<uint32_t>(buf->GetSize());
}

LUMA_API bool GpuBuffer_Write(GpuBufferHandle buffer, const void* data, uint32_t size, uint32_t offset)
{
    if (!buffer || !data) return false;
    auto* buf = static_cast<Nut::Buffer*>(buffer);
    return buf->WriteBuffer(data, size, offset);
}

LUMA_API bool CameraManager_CreateCamera(const char* id)
{
    if (!id) return false;
    return CameraManager::GetInstance().CreateCamera(id);
}

LUMA_API bool CameraManager_DestroyCamera(const char* id)
{
    if (!id) return false;
    return CameraManager::GetInstance().DestroyCamera(id);
}

LUMA_API bool CameraManager_SetActiveCamera(const char* id)
{
    if (!id) return false;
    return CameraManager::GetInstance().SetActiveCamera(id);
}

LUMA_API const char* CameraManager_GetActiveCameraId()
{
    return CameraManager::GetInstance().GetActiveCameraId().c_str();
}

LUMA_API bool CameraManager_HasCamera(const char* id)
{
    if (!id) return false;
    return CameraManager::GetInstance().HasCamera(id);
}

LUMA_API float Camera_GetPositionX()
{
    auto props = CameraManager::GetInstance().GetActiveCamera().GetProperties();
    return props.position.x();
}

LUMA_API float Camera_GetPositionY()
{
    auto props = CameraManager::GetInstance().GetActiveCamera().GetProperties();
    return props.position.y();
}

LUMA_API void Camera_SetPosition(float x, float y)
{
    auto& camera = CameraManager::GetInstance().GetActiveCamera();
    auto props = camera.GetProperties();
    props.position = SkPoint::Make(x, y);
    camera.SetProperties(props);
}

LUMA_API float Camera_GetZoom()
{
    return CameraManager::GetInstance().GetActiveCamera().GetProperties().zoom.x();
}

LUMA_API void Camera_SetZoom(float zoom)
{
    auto& camera = CameraManager::GetInstance().GetActiveCamera();
    auto props = camera.GetProperties();
    props.zoom = {zoom, zoom};
    camera.SetProperties(props);
}

LUMA_API float Camera_GetRotation()
{
    return CameraManager::GetInstance().GetActiveCamera().GetProperties().rotation;
}

LUMA_API void Camera_SetRotation(float rotation)
{
    auto& camera = CameraManager::GetInstance().GetActiveCamera();
    auto props = camera.GetProperties();
    props.rotation = rotation;
    camera.SetProperties(props);
}

LUMA_API float Camera_GetViewportWidth()
{
    return CameraManager::GetInstance().GetActiveCamera().GetProperties().viewport.width();
}

LUMA_API float Camera_GetViewportHeight()
{
    return CameraManager::GetInstance().GetActiveCamera().GetProperties().viewport.height();
}

LUMA_API void Camera_GetClearColor(float* r, float* g, float* b, float* a)
{
    if (!r || !g || !b || !a) return;
    auto props = CameraManager::GetInstance().GetActiveCamera().GetProperties();
    *r = props.clearColor.fR;
    *g = props.clearColor.fG;
    *b = props.clearColor.fB;
    *a = props.clearColor.fA;
}

LUMA_API void Camera_SetClearColor(float r, float g, float b, float a)
{
    auto& camera = CameraManager::GetInstance().GetActiveCamera();
    auto props = camera.GetProperties();
    props.clearColor = SkColor4f{r, g, b, a};
    camera.SetProperties(props);
}

LUMA_API void Camera_ScreenToWorld(float screenX, float screenY, float* worldX, float* worldY)
{
    if (!worldX || !worldY) return;
    SkPoint result = CameraManager::GetInstance().GetActiveCamera().ScreenToWorld(SkPoint::Make(screenX, screenY));
    *worldX = result.x();
    *worldY = result.y();
}

LUMA_API void Camera_WorldToScreen(float worldX, float worldY, float* screenX, float* screenY)
{
    if (!screenX || !screenY) return;
    SkPoint result = CameraManager::GetInstance().GetActiveCamera().WorldToScreen(SkPoint::Make(worldX, worldY));
    *screenX = result.x();
    *screenY = result.y();
}

LUMA_API float CameraById_GetPositionX(const char* id)
{
    if (!id) return 0.0f;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return 0.0f;
    return camera->GetProperties().position.x();
}

LUMA_API float CameraById_GetPositionY(const char* id)
{
    if (!id) return 0.0f;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return 0.0f;
    return camera->GetProperties().position.y();
}

LUMA_API void CameraById_SetPosition(const char* id, float x, float y)
{
    if (!id) return;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return;
    auto props = camera->GetProperties();
    props.position = SkPoint::Make(x, y);
    camera->SetProperties(props);
}

LUMA_API float CameraById_GetZoom(const char* id)
{
    if (!id) return 1.0f;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return 1.0f;
    return camera->GetProperties().zoom.x();
}

LUMA_API void CameraById_SetZoom(const char* id, float zoom)
{
    if (!id) return;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return;
    auto props = camera->GetProperties();
    props.zoom = {zoom, zoom};
    camera->SetProperties(props);
}

LUMA_API float CameraById_GetRotation(const char* id)
{
    if (!id) return 0.0f;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return 0.0f;
    return camera->GetProperties().rotation;
}

LUMA_API void CameraById_SetRotation(const char* id, float rotation)
{
    if (!id) return;
    Camera* camera = CameraManager::GetInstance().GetCamera(id);
    if (!camera) return;
    auto props = camera->GetProperties();
    props.rotation = rotation;
    camera->SetProperties(props);
}


LUMA_API void Shader_StartPreWarmingAsync()
{
    Nut::ShaderRegistry::GetInstance().StartPreWarmingAsync();
}

LUMA_API void Shader_PreWarming()
{
    Nut::ShaderRegistry::GetInstance().PreWarming();
}

LUMA_API void Shader_StopPreWarming()
{
    Nut::ShaderRegistry::GetInstance().StopPreWarming();
}

LUMA_API bool Shader_IsPreWarmingRunning()
{
    return Nut::ShaderRegistry::GetInstance().IsPreWarmingRunning();
}

LUMA_API bool Shader_IsPreWarmingComplete()
{
    return Nut::ShaderRegistry::GetInstance().IsPreWarmingComplete();
}

LUMA_API void Shader_GetPreWarmingState(int* outTotal, int* outLoaded, bool* outIsRunning, bool* outIsComplete)
{
    auto state = Nut::ShaderRegistry::GetInstance().GetPreWarmingState();
    if (outTotal) *outTotal = state.total;
    if (outLoaded) *outLoaded = state.loaded;
    if (outIsRunning) *outIsRunning = state.isRunning;
    if (outIsComplete) *outIsComplete = state.isComplete;
}





#include <imgui.h>
#include "Plugins/PluginManager.h"

LUMA_API bool ImGui_Begin(const char* name)
{
    return ImGui::Begin(name);
}

LUMA_API bool ImGui_BeginWithOpen(const char* name, bool* open)
{
    return ImGui::Begin(name, open);
}

LUMA_API void ImGui_End()
{
    ImGui::End();
}

LUMA_API void ImGui_Text(const char* text)
{
    ImGui::TextUnformatted(text);
}

LUMA_API void ImGui_TextColored(float r, float g, float b, float a, const char* text)
{
    ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
}

LUMA_API void ImGui_TextDisabled(const char* text)
{
    ImGui::TextDisabled("%s", text);
}

LUMA_API void ImGui_TextWrapped(const char* text)
{
    ImGui::TextWrapped("%s", text);
}

LUMA_API void ImGui_LabelText(const char* label, const char* text)
{
    ImGui::LabelText(label, "%s", text);
}

LUMA_API bool ImGui_Button(const char* label)
{
    return ImGui::Button(label);
}

LUMA_API bool ImGui_ButtonEx(const char* label, float width, float height)
{
    return ImGui::Button(label, ImVec2(width, height));
}

LUMA_API bool ImGui_SmallButton(const char* label)
{
    return ImGui::SmallButton(label);
}

LUMA_API bool ImGui_Checkbox(const char* label, bool* value)
{
    return ImGui::Checkbox(label, value);
}

LUMA_API bool ImGui_InputText(const char* label, char* buffer, int bufferSize)
{
    return ImGui::InputText(label, buffer, static_cast<size_t>(bufferSize));
}


LUMA_API bool ImGui_InputTextCallback(const char* label, const char* text, char* outBuffer, int outBufferSize,
                                      bool* changed)
{
    
    if (text)
    {
        strncpy(outBuffer, text, outBufferSize - 1);
        outBuffer[outBufferSize - 1] = '\0';
    }
    else
    {
        outBuffer[0] = '\0';
    }

    bool result = ImGui::InputText(label, outBuffer, static_cast<size_t>(outBufferSize));
    if (changed)
    {
        *changed = result;
    }
    return result;
}

LUMA_API bool ImGui_InputTextMultiline(const char* label, char* buffer, int bufferSize, float width, float height)
{
    return ImGui::InputTextMultiline(label, buffer, static_cast<size_t>(bufferSize), ImVec2(width, height));
}

LUMA_API bool ImGui_InputTextMultilineCallback(const char* label, const char* text, char* outBuffer, int outBufferSize,
                                               float width, float height, bool* changed)
{
    if (text)
    {
        strncpy(outBuffer, text, outBufferSize - 1);
        outBuffer[outBufferSize - 1] = '\0';
    }
    else
    {
        outBuffer[0] = '\0';
    }

    bool result = ImGui::InputTextMultiline(label, outBuffer, static_cast<size_t>(outBufferSize),
                                            ImVec2(width, height));
    if (changed)
    {
        *changed = result;
    }
    return result;
}

LUMA_API bool ImGui_InputFloat(const char* label, float* value)
{
    return ImGui::InputFloat(label, value);
}

LUMA_API bool ImGui_InputInt(const char* label, int* value)
{
    return ImGui::InputInt(label, value);
}

LUMA_API bool ImGui_SliderFloat(const char* label, float* value, float min, float max)
{
    return ImGui::SliderFloat(label, value, min, max);
}

LUMA_API bool ImGui_SliderInt(const char* label, int* value, int min, int max)
{
    return ImGui::SliderInt(label, value, min, max);
}

LUMA_API void ImGui_Separator()
{
    ImGui::Separator();
}

LUMA_API void ImGui_SameLine()
{
    ImGui::SameLine();
}

LUMA_API void ImGui_SameLineEx(float offsetFromStartX, float spacing)
{
    ImGui::SameLine(offsetFromStartX, spacing);
}

LUMA_API void ImGui_Spacing()
{
    ImGui::Spacing();
}

LUMA_API void ImGui_Dummy(float width, float height)
{
    ImGui::Dummy(ImVec2(width, height));
}

LUMA_API void ImGui_Indent()
{
    ImGui::Indent();
}

LUMA_API void ImGui_Unindent()
{
    ImGui::Unindent();
}

LUMA_API bool ImGui_TreeNode(const char* label)
{
    return ImGui::TreeNode(label);
}

LUMA_API void ImGui_TreePop()
{
    ImGui::TreePop();
}

LUMA_API bool ImGui_CollapsingHeader(const char* label)
{
    return ImGui::CollapsingHeader(label);
}

LUMA_API bool ImGui_Selectable(const char* label, bool selected)
{
    return ImGui::Selectable(label, selected);
}

LUMA_API bool ImGui_BeginMenuBar()
{
    return ImGui::BeginMenuBar();
}

LUMA_API void ImGui_EndMenuBar()
{
    ImGui::EndMenuBar();
}

LUMA_API bool ImGui_BeginMenu(const char* label)
{
    return ImGui::BeginMenu(label);
}

LUMA_API void ImGui_EndMenu()
{
    ImGui::EndMenu();
}

LUMA_API bool ImGui_MenuItem(const char* label)
{
    return ImGui::MenuItem(label);
}

LUMA_API bool ImGui_MenuItemEx(const char* label, const char* shortcut, bool selected)
{
    return ImGui::MenuItem(label, shortcut, selected);
}

LUMA_API void ImGui_OpenPopup(const char* strId)
{
    ImGui::OpenPopup(strId);
}

LUMA_API bool ImGui_BeginPopup(const char* strId)
{
    return ImGui::BeginPopup(strId);
}

LUMA_API bool ImGui_BeginPopupModal(const char* name)
{
    return ImGui::BeginPopupModal(name);
}

LUMA_API void ImGui_EndPopup()
{
    ImGui::EndPopup();
}

LUMA_API void ImGui_CloseCurrentPopup()
{
    ImGui::CloseCurrentPopup();
}

LUMA_API bool ImGui_IsItemHovered()
{
    return ImGui::IsItemHovered();
}

LUMA_API bool ImGui_IsItemClicked()
{
    return ImGui::IsItemClicked();
}

LUMA_API void ImGui_SetTooltip(const char* text)
{
    ImGui::SetTooltip("%s", text);
}

LUMA_API float ImGui_GetWindowWidth()
{
    return ImGui::GetWindowWidth();
}

LUMA_API float ImGui_GetWindowHeight()
{
    return ImGui::GetWindowHeight();
}

LUMA_API void ImGui_SetNextWindowSize(float width, float height)
{
    ImGui::SetNextWindowSize(ImVec2(width, height));
}

LUMA_API void ImGui_SetNextWindowPos(float x, float y)
{
    ImGui::SetNextWindowPos(ImVec2(x, y));
}

LUMA_API void ImGui_PushID(int id)
{
    ImGui::PushID(id);
}

LUMA_API void ImGui_PushIDStr(const char* strId)
{
    ImGui::PushID(strId);
}

LUMA_API void ImGui_PopID()
{
    ImGui::PopID();
}


LUMA_API bool ImGui_ColorEdit3(const char* label, float* r, float* g, float* b)
{
    float col[3] = {*r, *g, *b};
    bool result = ImGui::ColorEdit3(label, col);
    if (result)
    {
        *r = col[0];
        *g = col[1];
        *b = col[2];
    }
    return result;
}

LUMA_API bool ImGui_ColorEdit4(const char* label, float* r, float* g, float* b, float* a)
{
    float col[4] = {*r, *g, *b, *a};
    bool result = ImGui::ColorEdit4(label, col);
    if (result)
    {
        *r = col[0];
        *g = col[1];
        *b = col[2];
        *a = col[3];
    }
    return result;
}


LUMA_API bool ImGui_DragFloat(const char* label, float* value, float speed, float min, float max)
{
    return ImGui::DragFloat(label, value, speed, min, max);
}

LUMA_API bool ImGui_DragInt(const char* label, int* value, float speed, int min, int max)
{
    return ImGui::DragInt(label, value, speed, min, max);
}


LUMA_API void ImGui_ProgressBar(float fraction, float width, float height, const char* overlay)
{
    ImGui::ProgressBar(fraction, ImVec2(width, height), overlay);
}


LUMA_API bool ImGui_BeginChild(const char* strId, float width, float height, bool border)
{
    return ImGui::BeginChild(strId, ImVec2(width, height), border ? ImGuiChildFlags_Border : ImGuiChildFlags_None);
}

LUMA_API void ImGui_EndChild()
{
    ImGui::EndChild();
}


LUMA_API bool ImGui_BeginTabBar(const char* strId)
{
    return ImGui::BeginTabBar(strId);
}

LUMA_API void ImGui_EndTabBar()
{
    ImGui::EndTabBar();
}

LUMA_API bool ImGui_BeginTabItem(const char* label)
{
    return ImGui::BeginTabItem(label);
}

LUMA_API void ImGui_EndTabItem()
{
    ImGui::EndTabItem();
}


LUMA_API bool ImGui_BeginTable(const char* strId, int columns)
{
    return ImGui::BeginTable(strId, columns);
}

LUMA_API void ImGui_EndTable()
{
    ImGui::EndTable();
}

LUMA_API void ImGui_TableNextRow()
{
    ImGui::TableNextRow();
}

LUMA_API bool ImGui_TableNextColumn()
{
    return ImGui::TableNextColumn();
}

LUMA_API void ImGui_TableSetupColumn(const char* label)
{
    ImGui::TableSetupColumn(label);
}

LUMA_API void ImGui_TableHeadersRow()
{
    ImGui::TableHeadersRow();
}


LUMA_API void ImGui_Image(ImTextureID textureId, float width, float height)
{
    ImGui::Image(textureId, ImVec2(width, height));
}

LUMA_API bool ImGui_ImageButton(const char* strId, ImTextureID textureId, float width, float height)
{
    return ImGui::ImageButton(strId, textureId, ImVec2(width, height));
}





LUMA_API bool Plugin_Load(const char* dllPath, const char* pluginId)
{
    return PluginManager::GetInstance().LoadPlugin(pluginId);
}

LUMA_API bool Plugin_Unload(const char* pluginId)
{
    return PluginManager::GetInstance().UnloadPlugin(pluginId);
}

LUMA_API int Plugin_GetCount()
{
    return static_cast<int>(PluginManager::GetInstance().GetAllPlugins().size());
}

LUMA_API bool Plugin_GetInfo(int index, char* idBuffer, int idBufferSize,
                             char* nameBuffer, int nameBufferSize,
                             char* versionBuffer, int versionBufferSize,
                             bool* enabled, bool* loaded)
{
    const auto& plugins = PluginManager::GetInstance().GetAllPlugins();
    if (index < 0 || index >= static_cast<int>(plugins.size()))
        return false;

    const auto& plugin = plugins[index];

    if (idBuffer && idBufferSize > 0)
    {
        strncpy(idBuffer, plugin.id.c_str(), idBufferSize - 1);
        idBuffer[idBufferSize - 1] = '\0';
    }
    if (nameBuffer && nameBufferSize > 0)
    {
        strncpy(nameBuffer, plugin.name.c_str(), nameBufferSize - 1);
        nameBuffer[nameBufferSize - 1] = '\0';
    }
    if (versionBuffer && versionBufferSize > 0)
    {
        strncpy(versionBuffer, plugin.version.c_str(), versionBufferSize - 1);
        versionBuffer[versionBufferSize - 1] = '\0';
    }
    if (enabled) *enabled = plugin.enabled;
    if (loaded) *loaded = plugin.loaded;

    return true;
}





LUMA_API void Luma_LogInfo(const char* message)
{
    LogInfo("[Plugin] {}", message);
}

LUMA_API void Luma_LogWarn(const char* message)
{
    LogWarn("[Plugin] {}", message);
}

LUMA_API void Luma_LogError(const char* message)
{
    LogError("[Plugin] {}", message);
}

LUMA_API void Luma_LogDebug(const char* message)
{
    LogDebug("[Plugin] {}", message);
}





#include "Application/Editor.h"
#include "Application/Editor/EditorContext.h"

LUMA_API bool Luma_IsEditorMode()
{
    return ApplicationBase::CURRENT_MODE == ApplicationMode::Editor;
}

LUMA_API bool Luma_IsPlaying()
{
    if (!Luma_IsEditorMode()) return false;
    return Editor::GetInstance()->GetEditorContext().editorState == EditorState::Playing;
}

LUMA_API int Luma_GetSelectedEntityCount()
{
    if (!Luma_IsEditorMode()) return 0;
    auto& context = Editor::GetInstance()->GetEditorContext();
    if (context.selectionType != SelectionType::GameObject) return 0;
    return static_cast<int>(context.selectionList.size());
}

LUMA_API Guid_CAPI Luma_GetSelectedEntityGuid(int index)
{
    Guid_CAPI result = {0, 0};
    if (!Luma_IsEditorMode()) return result;

    auto& context = Editor::GetInstance()->GetEditorContext();
    if (context.selectionType != SelectionType::GameObject) return result;
    if (index < 0 || index >= static_cast<int>(context.selectionList.size())) return result;

    const Guid& guid = context.selectionList[index];
    return ToGuidCAPI(guid);
}

LUMA_API void Luma_GetSelectedEntityName(int index, char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) return;
    buffer[0] = '\0';

    if (!Luma_IsEditorMode()) return;

    auto& context = Editor::GetInstance()->GetEditorContext();
    if (context.selectionType != SelectionType::GameObject) return;
    if (index < 0 || index >= static_cast<int>(context.selectionList.size())) return;

    const Guid& guid = context.selectionList[index];
    if (!context.activeScene) return;

    auto gameObject = context.activeScene->FindGameObjectByGuid(guid);
    if (gameObject.IsValid())
    {
        strncpy(buffer, gameObject.GetName().c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}





LUMA_API bool Luma_IsProjectLoaded()
{
    return ProjectSettings::GetInstance().IsProjectLoaded();
}

LUMA_API void Luma_GetProjectName(char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) return;

    auto& settings = ProjectSettings::GetInstance();
    if (settings.IsProjectLoaded())
    {
        strncpy(buffer, settings.GetAppName().c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    else
    {
        buffer[0] = '\0';
    }
}

LUMA_API void Luma_GetProjectPath(char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) return;

    auto& settings = ProjectSettings::GetInstance();
    if (settings.IsProjectLoaded())
    {
        strncpy(buffer, settings.GetProjectRoot().string().c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    else
    {
        buffer[0] = '\0';
    }
}

LUMA_API void Luma_GetAssetsPath(char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) return;

    auto& settings = ProjectSettings::GetInstance();
    if (settings.IsProjectLoaded())
    {
        auto assetsPath = settings.GetProjectRoot() / "Assets";
        strncpy(buffer, assetsPath.string().c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    else
    {
        buffer[0] = '\0';
    }
}





#include "Resources/RuntimeAsset/RuntimeScene.h"

LUMA_API int Luma_GetEntityCount()
{
    auto currentScene = SceneManager::GetInstance().GetCurrentScene();
    if (currentScene)
        return currentScene->GetGameObjectCount();
    return 0;
}

LUMA_API void Luma_GetCurrentSceneName(char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0) return;
    buffer[0] = '\0';
    auto currentScene = SceneManager::GetInstance().GetCurrentScene();
    if (currentScene)
    {
        auto sceneName = currentScene->GetName();
        strncpy(buffer, sceneName.c_str(), bufferSize - 1);
    }
}





#include "AssetManager.h"

LUMA_API int Luma_GetAssetCount()
{
    return static_cast<int>(AssetManager::GetInstance().GetAssetDatabase().size());
}

LUMA_API bool Luma_AssetExists(const char* path)
{
    if (!path) return false;
    return AssetManager::GetInstance().GetMetadata(path) != nullptr;
}

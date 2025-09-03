#include "Luma_CAPI.h"

#include "AnimationControllerComponent.h"
#include "SceneManager.h"
#include "SIMDWrapper.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Components/ComponentRegistry.h"
#include "Event/JobSystem.h"
#include "Input/Cursor.h"
#include "Input/Keyboards.h"
#include "RuntimeAsset/RuntimeAnimationController.h"


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

LUMA_API intptr_t* ScriptComponent_GetGCHandle(void* componentPtr)
{
    if (!componentPtr)
    {
        return nullptr;
    }

    const auto* scriptComponent = static_cast<const ECS::ScriptComponent*>(componentPtr);

    if (!scriptComponent->managedGCHandle)
    {
        return nullptr;
    }

    return scriptComponent->managedGCHandle;
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
                                  void* componentData)
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

    size_t componentSize = registration->size;
    if (componentSize == 0) return;
    memcpy(dest, componentData, componentSize);
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
    return go.IsValid() ? (LumaEntityHandle)go.GetEntityHandle() : 0;
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
    SIMD::GetInstance().VectorMax(input, count);
}

float SIMDVectorMin(const float* input, size_t count)
{
    SIMD::GetInstance().VectorMin(input, count);
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

#include "Luma_CAPI.h"

#include <AudioManager.h>
#include <Loaders/AudioLoader.h>
#include <algorithm>
#include <array>
#include <utility>
#include <cstring>

#include "AnimationControllerComponent.h"
#include "ColliderComponent.h"
#include "UIComponents.h"
#include "PhysicsSystem.h"
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

template <typename TComponent>
static inline TComponent* TryGetComponent(RuntimeScene* runtimeScene, LumaEntityHandle entity)
{
    if (!runtimeScene) return nullptr;
    return runtimeScene->GetRegistry().try_get<TComponent>((entt::entity)entity);
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

    const auto& propIt = compReg->properties.find(propertyName);
    if (propIt != compReg->properties.end() && propIt->second.set_from_raw_ptr)
    {
        propIt->second.set_from_raw_ptr(runtimeScene->GetRegistry(), (entt::entity)entity, valueData);

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

    const auto& propIt = compReg->properties.find(propertyName);
    if (propIt != compReg->properties.end() && propIt->second.get_to_raw_ptr)
    {
        propIt->second.get_to_raw_ptr(runtimeScene->GetRegistry(), (entt::entity)entity, valueData);
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

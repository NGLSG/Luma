#include "SceneManager.h"
#include "AnimationSystem.h"
#include "ApplicationBase.h"
#include "AudioSystem.h"
#include "ButtonSystem.h"
#include "CommonUIControlSystem.h"
#include "HydrateResources.h"
#include "InputTextSystem.h"
#include "InteractionSystem.h"
#include "PhysicsSystem.h"
#if !defined(LUMA_DISABLE_SCRIPTING)
#include "ScriptingSystem.h"
#endif
#include "TransformSystem.h"
#include "../Systems/ParticleSystem.h"
#include "../Resources/AssetManager.h"
#include "../Resources/Managers/RuntimeSceneManager.h"
#include "../Data/SceneData.h"
#include "../Resources/Importers/SceneImporter.h"
#include "../Resources/Loaders/SceneLoader.h"
SceneManager::SceneManager()
{
}
SceneManager::~SceneManager()
{
}
void SceneManager::LoadSceneAsync(const Guid& guid, SceneLoadCallback callback)
{
    std::future<sk_sp<RuntimeScene>> future = std::async(std::launch::async, [this, guid]()
    {
        return loadSceneFromDisk(guid);
    });
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_completedLoads.push({guid, std::move(callback), std::move(future)});
}
void SceneManager::Initialize(EngineContext* context)
{
    m_context = context;
}
sk_sp<RuntimeScene> SceneManager::LoadScene(const Guid& guid)
{
    sk_sp<RuntimeScene> newScene = loadSceneFromDisk(guid);
    if (!newScene)
    {
        LogError("加载场景失败，GUID: {}", guid.ToString());
        return nullptr;
    }
    if (ApplicationBase::CURRENT_MODE == ApplicationMode::Runtime)
    {
        setupRuntimeSystems(newScene, m_context);
    }
    activateScene(newScene, guid, m_context);
    return newScene;
}
void SceneManager::Update(EngineContext& engineCtx)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_completedLoads.empty())
    {
        return;
    }
    auto& request = m_completedLoads.front();
    if (request.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        sk_sp<RuntimeScene> loadedScene = request.future.get();
        if (loadedScene)
        {
            if (ApplicationBase::CURRENT_MODE == ApplicationMode::Runtime)
            {
                setupRuntimeSystems(loadedScene, &engineCtx);
            }
            if (request.callback)
            {
                request.callback(loadedScene);
            }
            activateScene(loadedScene, request.guid, &engineCtx);
        }
        else
        {
            LogError("异步加载场景失败，GUID: {}", request.guid.ToString());
            if (request.callback)
            {
                request.callback(nullptr);
            }
        }
        m_completedLoads.pop();
    }
}
void SceneManager::SetCurrentScene(sk_sp<RuntimeScene> scene)
{
    std::unique_lock<std::shared_mutex> lock(m_currentSceneMutex);
    m_currentScene = std::move(scene);
}
sk_sp<RuntimeScene> SceneManager::GetCurrentScene() const
{
    std::shared_lock<std::shared_mutex> lock(m_currentSceneMutex);
    return m_currentScene;
}
Guid SceneManager::GetCurrentSceneGuid() const
{
    std::shared_lock<std::shared_mutex> lock(m_currentSceneMutex);
    return m_currentScene ? m_currentScene->GetGuid() : Guid::Invalid();
}
bool SceneManager::SaveScene(sk_sp<RuntimeScene> scene)
{
    m_markedAsDirty = false;
    if (!scene) return false;
    const AssetMetadata* meta = AssetManager::GetInstance().GetMetadata(scene->GetGuid());
    std::filesystem::path sceneName;
    if (!meta)
    {
        sceneName = "NewScene.scene";
        int counter = 1;
        while (std::filesystem::exists(AssetManager::GetInstance().GetAssetsRootPath() / sceneName))
        {
            sceneName = "NewScene_" + std::to_string(counter++) + ".scene";
        }
    }
    else
    {
        sceneName = meta->assetPath;
    }
    Data::SceneData sceneData = scene->SerializeToData();
    YAML::Node sceneNode = YAML::convert<Data::SceneData>::encode(sceneData);
    std::string targetPath = (AssetManager::GetInstance().GetAssetsRootPath() / sceneName).generic_string();
    std::ofstream fout(targetPath);
    fout << sceneNode;
    fout.close();
    return true;
}
bool SceneManager::SaveCurrentScene()
{
    sk_sp<RuntimeScene> scene;
    {
        std::shared_lock<std::shared_mutex> lock(m_currentSceneMutex);
        scene = m_currentScene;
    }
    return SaveScene(scene);
}
void SceneManager::PushUndoState(sk_sp<RuntimeScene> scene)
{
    MarkCurrentSceneDirty();
    if (!scene) return;
    m_redoStack.clear();
    m_undoStack.push_back(scene->SerializeToData());
    if (m_undoStack.size() > MAX_UNDO_STEPS)
    {
        m_undoStack.pop_front();
    }
}
void SceneManager::Undo()
{
    if (m_undoStack.size() <= 1) return;
    m_redoStack.push_back(m_undoStack.back());
    m_undoStack.pop_back();
    const Data::SceneData& prevState = m_undoStack.back();
    m_currentScene->LoadFromData(prevState);
}
void SceneManager::Redo()
{
    if (m_redoStack.empty()) return;
    Data::SceneData nextState = m_redoStack.back();
    m_redoStack.pop_back();
    m_currentScene->LoadFromData(nextState);
    m_undoStack.push_back(std::move(nextState));
}
bool SceneManager::CanUndo() const
{
    return m_undoStack.size() > 1;
}
bool SceneManager::CanRedo() const
{
    return !m_redoStack.empty();
}
void SceneManager::Shutdown()
{
    std::unique_lock<std::shared_mutex> lock(m_currentSceneMutex);
    if (m_currentScene)
    {
        LogInfo("关闭场景管理器，停用场景: {}", m_currentScene->GetName());
        m_currentScene->Deactivate();
    }
    m_currentScene.reset();
    m_undoStack.clear();
    m_redoStack.clear();
    std::lock_guard<std::mutex> queueLock(m_queueMutex);
    while (!m_completedLoads.empty())
    {
        m_completedLoads.pop();
    }
}
sk_sp<RuntimeScene> SceneManager::loadSceneFromDisk(const Guid& guid)
{
    SceneLoader loader;
    sk_sp<RuntimeScene> newScene = loader.LoadAsset(guid);
    if (!newScene)
    {
        LogError("从磁盘加载场景失败，GUID: {}", guid.ToString());
        return nullptr;
    }
    return newScene;
}
void SceneManager::setupRuntimeSystems(sk_sp<RuntimeScene> scene, EngineContext* context)
{
    if (!scene)
    {
        LogWarn("尝试为空场景配置运行时系统");
        return;
    }
    auto setupSystems = [scene]()
    {
        scene->AddEssentialSystem<Systems::HydrateResources>();
        scene->AddEssentialSystem<Systems::TransformSystem>();
        scene->AddSystem<Systems::PhysicsSystem>();
        scene->AddSystem<Systems::InteractionSystem>();
        scene->AddSystem<Systems::AudioSystem>();
        scene->AddSystem<Systems::ButtonSystem>();
        scene->AddSystemToMainThread<Systems::InputTextSystem>();
        scene->AddSystem<Systems::CommonUIControlSystem>();
#if !defined(LUMA_DISABLE_SCRIPTING)
        scene->AddSystem<Systems::ScriptingSystem>();
#endif
        scene->AddSystem<Systems::AnimationSystem>();
        scene->AddSystem<Systems::ParticleSystem>();
        LogInfo("运行时系统已配置完成，场景: {}", scene->GetName());
    };
    if (context)
    {
        context->commandsForSim.Push(setupSystems);
    }
    else
    {
        setupSystems();
    }
}
void SceneManager::activateScene(sk_sp<RuntimeScene> scene, const Guid& guid, EngineContext* context)
{
    if (!scene)
    {
        LogError("尝试激活空场景");
        return;
    }
    auto activateFunc = [this, scene, guid, context]()
    {
        sk_sp<RuntimeScene> oldScene = GetCurrentScene();
        if (oldScene && oldScene != scene)
        {
            LogInfo("停用旧场景: {}", oldScene->GetName());
            oldScene->Deactivate();
        }
        auto& runtimeSceneManager = RuntimeSceneManager::GetInstance();
        runtimeSceneManager.TryAddOrUpdateAsset(guid, scene);
        SetCurrentScene(scene);
        if (context)
        {
            scene->Activate(*context);
        }
        m_markedAsDirty = false;
        LogInfo("场景已激活: {} (GUID: {})", scene->GetName(), guid.ToString());
    };
    if (context)
    {
        context->commandsForSim.Push(activateFunc);
    }
    else
    {
        activateFunc();
    }
}

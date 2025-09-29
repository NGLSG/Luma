#include "SceneManager.h"

#include "AnimationSystem.h"
#include "ApplicationBase.h"
#include "AudioSystem.h"
#include "ButtonSystem.h"
#include "HydrateResources.h"
#include "InputTextSystem.h"
#include "InteractionSystem.h"
#include "PhysicsSystem.h"
#include "ScriptingSystem.h"
#include "TransformSystem.h"
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
    std::future<sk_sp<RuntimeScene>> future = std::async(std::launch::async, [guid]()
    {
        const auto& assetManager = AssetManager::GetInstance();
        const AssetMetadata* meta = assetManager.GetMetadata(guid);
        if (!meta || meta->type != AssetType::Scene)
        {
            return sk_sp<RuntimeScene>(nullptr);
        }


        Data::SceneData sceneData = meta->importerSettings.as<Data::SceneData>();


        sk_sp<RuntimeScene> newScene = sk_make_sp<RuntimeScene>();
        newScene->LoadFromData(sceneData);

        return newScene;
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
    SceneLoader loader;
    auto newScene = loader.LoadAsset(guid);;

    auto& runtimeSceneManager = RuntimeSceneManager::GetInstance();
    runtimeSceneManager.TryAddOrUpdateAsset(guid, newScene);

    if (newScene)
    {
        SetCurrentScene(newScene);
    }
    if (ApplicationBase::CURRENT_MODE == ApplicationMode::Runtime)
    {
        if (!newScene)
        {
            LogError("Failed to load scene with GUID: {}", guid.ToString());
            return nullptr;
        }

        newScene->AddEssentialSystem<Systems::HydrateResources>();
        newScene->AddEssentialSystem<Systems::TransformSystem>();
        newScene->AddSystem<Systems::PhysicsSystem>();
        newScene->AddSystem<Systems::InteractionSystem>();
        newScene->AddSystem<Systems::AudioSystem>();
        newScene->AddSystem<Systems::ButtonSystem>();
        newScene->AddSystem<Systems::InputTextSystem>();
        newScene->AddSystem<Systems::ScriptingSystem>();
        newScene->AddSystem<Systems::AnimationSystem>();
        newScene->Activate(*m_context);
    }


    markedAsDirty = false;
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
                loadedScene->AddEssentialSystem<Systems::HydrateResources>();
                loadedScene->AddEssentialSystem<Systems::TransformSystem>();
                loadedScene->AddSystem<Systems::PhysicsSystem>();
                loadedScene->AddSystem<Systems::InteractionSystem>();
                loadedScene->AddSystem<Systems::AudioSystem>();
                loadedScene->AddSystem<Systems::ButtonSystem>();
                loadedScene->AddSystem<Systems::InputTextSystem>();
                loadedScene->AddSystem<Systems::ScriptingSystem>();
                loadedScene->AddSystem<Systems::AnimationSystem>();
            }

            if (request.callback)
            {
                request.callback(loadedScene);
            }


            SetCurrentScene(loadedScene);
            loadedScene->Activate(engineCtx);

            auto& runtimeSceneManager = RuntimeSceneManager::GetInstance();
            runtimeSceneManager.TryAddOrUpdateAsset(request.guid, loadedScene);
        }
        else
        {
            LogError("异步加载场景失败，GUID: {}", request.guid.ToString());
            if (request.callback)
            {
                request.callback(nullptr);
            }
        }
        markedAsDirty = false;
        m_completedLoads.pop();
    }
}


void SceneManager::SetCurrentScene(sk_sp<RuntimeScene> scene)
{
    m_currentScene = std::move(scene);
}

sk_sp<RuntimeScene> SceneManager::GetCurrentScene() const
{
    return m_currentScene;
}

Guid SceneManager::GetCurrentSceneGuid() const
{
    return m_currentScene ? m_currentScene->GetGuid() : Guid::Invalid();
}

bool SceneManager::SaveScene(sk_sp<RuntimeScene> scene)
{
    markedAsDirty = false;
    if (!scene) return false;


    const AssetMetadata* meta = AssetManager::GetInstance().GetMetadata(scene->GetGuid());
    std::filesystem::path sceneName;
    if (!meta)
    {
        sceneName = "NewScene.scene";
        int counter = 1;
        do
        {
            sceneName = "NewScene_" + std::to_string(counter++) + ".scene";
        }
        while (std::filesystem::exists(AssetManager::GetInstance().GetAssetsRootPath() / sceneName));
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

    LogInfo("Scene '{}' saved successfully to {}", scene->GetName(), sceneName.string());
    return true;
}

bool SceneManager::SaveCurrentScene()
{
    return SaveScene(m_currentScene);
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
    m_currentScene.reset();
    m_undoStack.clear();
    m_redoStack.clear();
}

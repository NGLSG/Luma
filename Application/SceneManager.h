#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H
#pragma once
#include <functional>
#include <string>
#include <future>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include "../Utils/LazySingleton.h"
#include "../Utils/Guid.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
using SceneLoadCallback = std::function<void(sk_sp<RuntimeScene>)>;
class LUMA_API SceneManager : public LazySingleton<SceneManager>
{
public:
    friend class LazySingleton<SceneManager>;
    void LoadSceneAsync(const Guid& guid, SceneLoadCallback callback = nullptr);
    void Initialize(EngineContext* context);
    sk_sp<RuntimeScene> LoadScene(const Guid& guid);
    bool IsCurrentSceneDirty() const { return m_markedAsDirty; }
    void MarkCurrentSceneDirty() { m_markedAsDirty = true; }
    void Update(EngineContext& engineCtx);
    void SetCurrentScene(sk_sp<RuntimeScene> scene);
    sk_sp<RuntimeScene> GetCurrentScene() const;
    Guid GetCurrentSceneGuid() const;
    bool SaveCurrentScene();
    bool SaveScene(sk_sp<RuntimeScene> scene);
    void PushUndoState(sk_sp<RuntimeScene> scene);
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void Shutdown();
private:
    SceneManager();
    ~SceneManager() override;
    struct SceneLoadRequest
    {
        Guid guid; 
        SceneLoadCallback callback; 
        std::future<sk_sp<RuntimeScene>> future; 
    };
    sk_sp<RuntimeScene> loadSceneFromDisk(const Guid& guid);
    void setupRuntimeSystems(sk_sp<RuntimeScene> scene, EngineContext* context);
    void activateScene(sk_sp<RuntimeScene> scene, const Guid& guid, EngineContext* context);
    sk_sp<RuntimeScene> m_currentScene; 
    mutable std::shared_mutex m_currentSceneMutex; 
    std::queue<SceneLoadRequest> m_completedLoads; 
    std::mutex m_queueMutex; 
    std::deque<Data::SceneData> m_undoStack; 
    std::deque<Data::SceneData> m_redoStack; 
    const size_t MAX_UNDO_STEPS = 32; 
    EngineContext* m_context = nullptr; 
    bool m_markedAsDirty = false; 
};
#endif

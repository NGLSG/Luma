#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#pragma once

#include <functional>
#include <string>
#include <future>
#include <queue>
#include <mutex>

#include "../Utils/LazySingleton.h"
#include "../Utils/Guid.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"


/// 场景加载完成后的回调函数类型。
using SceneLoadCallback = std::function<void(sk_sp<RuntimeScene>)>;

/**
 * @brief 场景管理器，负责场景的加载、保存、切换和管理。
 */
class LUMA_API SceneManager : public LazySingleton<SceneManager>
{
public:
    friend class LazySingleton<SceneManager>;

    /**
     * @brief 异步加载指定GUID的场景。
     * @param guid 场景的全局唯一标识符。
     * @param callback 场景加载完成后的回调函数，可选。
     */
    void LoadSceneAsync(const Guid& guid, SceneLoadCallback callback = nullptr);
    /**
     * @brief 初始化场景管理器。
     * @param context 引擎上下文指针。
     */
    void Initialize(EngineContext* context);
    /**
     * @brief 同步加载指定GUID的场景。
     * @param guid 场景的全局唯一标识符。
     * @return 加载的运行时场景智能指针。
     */
    sk_sp<RuntimeScene> LoadScene(const Guid& guid);
    /**
     * @brief 检查当前场景是否已被标记为脏（需要保存）。
     * @return 如果当前场景已脏，则返回true；否则返回false。
     */
    bool IsCurrentSceneDirty() const { return markedAsDirty; }
    /**
     * @brief 将当前场景标记为脏，表示其已被修改，需要保存。
     */
    void MarkCurrentSceneDirty() { markedAsDirty = true; }

    /**
     * @brief 更新场景管理器状态，例如处理异步加载完成的场景。
     * @param engineCtx 引擎上下文引用。
     */
    void Update(EngineContext& engineCtx);

    /**
     * @brief 设置当前活动的场景。
     * @param scene 要设置为当前场景的运行时场景智能指针。
     */
    void SetCurrentScene(sk_sp<RuntimeScene> scene);

    /**
     * @brief 获取当前活动的场景。
     * @return 当前运行时场景的智能指针。
     */
    sk_sp<RuntimeScene> GetCurrentScene() const;
    /**
     * @brief 获取当前场景的全局唯一标识符。
     * @return 当前场景的GUID。
     */
    Guid GetCurrentSceneGuid() const;

    /**
     * @brief 保存当前活动的场景。
     * @return 如果保存成功，则返回true；否则返回false。
     */
    bool SaveCurrentScene();

    /**
     * @brief 保存指定的场景。
     * @param scene 要保存的运行时场景智能指针。
     * @return 如果保存成功，则返回true；否则返回false。
     */
    bool SaveScene(sk_sp<RuntimeScene> scene);
    /**
     * @brief 将当前场景状态推入撤销栈。
     * @param scene 要推入撤销栈的场景状态。
     */
    void PushUndoState(sk_sp<RuntimeScene> scene);
    /**
     * @brief 执行撤销操作，恢复到上一个场景状态。
     */
    void Undo();
    /**
     * @brief 执行重做操作，恢复到下一个场景状态。
     */
    void Redo();

    /**
     * @brief 检查是否可以执行撤销操作。
     * @return 如果可以撤销，则返回true；否则返回false。
     */
    bool CanUndo() const;
    /**
     * @brief 检查是否可以执行重做操作。
     * @return 如果可以重做，则返回true；否则返回false。
     */
    bool CanRedo() const;

    /**
     * @brief 关闭场景管理器，清理资源。
     */
    void Shutdown();

private:
    SceneManager();
    ~SceneManager() override;

    /**
     * @brief 场景加载请求结构体，用于管理异步场景加载任务。
     */
    struct SceneLoadRequest
    {
        Guid guid; ///< 待加载场景的GUID。
        SceneLoadCallback callback; ///< 场景加载完成后的回调函数。
        std::future<sk_sp<RuntimeScene>> future; ///< 异步加载任务的future对象。
    };

    sk_sp<RuntimeScene> m_currentScene; ///< 当前活动的运行时场景。

    std::thread m_loadingThread; ///< 用于异步加载场景的线程。
    std::queue<SceneLoadRequest> m_completedLoads; ///< 已完成的场景加载请求队列。
    std::mutex m_queueMutex; ///< 保护m_completedLoads队列的互斥锁。
    bool m_stopLoading = false; ///< 指示是否停止加载线程的标志。
    std::deque<Data::SceneData> m_undoStack; ///< 存储撤销操作的场景数据栈。
    std::deque<Data::SceneData> m_redoStack; ///< 存储重做操作的场景数据栈。
    const size_t MAX_UNDO_STEPS = 32; ///< 撤销栈的最大步数。
    EngineContext* m_context = nullptr; ///< 引擎上下文指针。
    bool markedAsDirty = false; ///< 标记当前场景是否已被修改。
};

#endif
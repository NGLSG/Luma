#ifndef RUNTIMESCENEMANAGER_H
#define RUNTIMESCENEMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../RuntimeAsset/RuntimeScene.h"

/**
 * @brief 场景性能数据结构体。
 * 继承自AssetPerformanceData，用于存储场景相关的性能指标。
 */
struct ScenePerformanceData : public AssetPerformanceData
{
    int sceneCount = 0; ///< 场景数量。
    int cacheHits = 0; ///< 缓存命中次数。
    int cacheMisses = 0; ///< 缓存未命中次数。
};

/**
 * @brief 运行时场景管理器。
 * 负责运行时场景资产的加载、管理和卸载。
 * 继承自RuntimeAssetManagerBase，并实现LazySingleton模式。
 */
class RuntimeSceneManager : public RuntimeAssetManagerBase<RuntimeScene>,
                            public LazySingleton<RuntimeSceneManager>
{
public:
    friend class LazySingleton<RuntimeSceneManager>;

    /**
     * @brief 尝试获取指定GUID的运行时场景资产。
     * @param guid 资产的全局唯一标识符。
     * @return 智能指针指向的运行时场景资产，如果未找到则为空。
     */
    sk_sp<RuntimeScene> TryGetAsset(const Guid& guid) const override;

    /**
     * @brief 尝试获取指定GUID的运行时场景资产，并通过输出参数返回。
     * @param guid 资产的全局唯一标识符。
     * @param out 用于接收运行时场景资产的智能指针引用。
     * @return 如果成功获取资产则为true，否则为false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<RuntimeScene>& out) const override;

    /**
     * @brief 尝试添加或更新指定GUID的运行时场景资产。
     * @param guid 资产的全局唯一标识符。
     * @param asset 要添加或更新的运行时场景资产。
     * @return 如果成功添加或更新资产则为true，否则为false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimeScene> asset) override;

    /**
     * @brief 尝试移除指定GUID的运行时场景资产。
     * @param guid 要移除资产的全局唯一标识符。
     * @return 如果成功移除资产则为true，否则为false。
     */
    bool TryRemoveAsset(const Guid& guid) override;

    /**
     * @brief 获取性能数据。
     * @return 指向资产性能数据的指针。
     */
    const AssetPerformanceData* GetPerformanceData() const override;

    /**
     * @brief 绘制调试用户界面。
     */
    void DrawDebugUI();

private:
    /**
     * @brief 构造函数。
     * 私有化以强制使用单例模式。
     */
    RuntimeSceneManager() = default;

    /**
     * @brief 析构函数。
     */
    ~RuntimeSceneManager() override = default;

private:
    mutable ScenePerformanceData m_performanceData; ///< 场景性能数据。
};

#endif
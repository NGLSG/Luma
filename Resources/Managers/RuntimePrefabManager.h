/**
 * @file RUNTIMEPREFABMANAGER_H
 * @brief 运行时预制体管理器头文件。
 */
#ifndef RUNTIMEPREFABMANAGER_H
#define RUNTIMEPREFABMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../RuntimeAsset/RuntimePrefab.h"

/**
 * @brief 预制体性能数据结构体。
 * 继承自AssetPerformanceData，增加了预制体特有的性能指标。
 */
struct PrefabPerformanceData : public AssetPerformanceData
{
    int prefabCount = 0; ///< 当前管理的预制体数量。
    int cacheHits = 0; ///< 缓存命中次数。
    int cacheMisses = 0; ///< 缓存未命中次数。
};

/**
 * @brief 运行时预制体管理器类。
 * 负责管理运行时预制体的加载、存储和访问。
 * 继承自RuntimeAssetManagerBase，并实现LazySingleton模式。
 */
class RuntimePrefabManager : public RuntimeAssetManagerBase<RuntimePrefab>,
                             public LazySingleton<RuntimePrefabManager>
{
public:
    friend class LazySingleton<RuntimePrefabManager>;

    /**
     * @brief 尝试根据GUID获取运行时预制体资产。
     * @param guid 资产的全局唯一标识符。
     * @return 如果找到，返回预制体的共享指针；否则返回空指针。
     */
    sk_sp<RuntimePrefab> TryGetAsset(const Guid& guid) const override;

    /**
     * @brief 尝试添加或更新运行时预制体资产。
     * @param guid 资产的全局唯一标识符。
     * @param asset 要添加或更新的预制体资产。
     * @return 如果操作成功，返回true；否则返回false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimePrefab> asset) override;

    /**
     * @brief 尝试移除运行时预制体资产。
     * @param guid 要移除资产的全局唯一标识符。
     * @return 如果移除成功，返回true；否则返回false。
     */
    bool TryRemoveAsset(const Guid& guid) override;

    /**
     * @brief 尝试根据GUID获取运行时预制体资产，并通过输出参数返回。
     * @param guid 资产的全局唯一标识符。
     * @param out 用于接收找到的预制体资产的输出参数。
     * @return 如果找到资产，返回true；否则返回false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<RuntimePrefab>& out) const override;

    /**
     * @brief 获取资产性能数据。
     * @return 指向资产性能数据的常量指针。
     */
    const AssetPerformanceData* GetPerformanceData() const override;

    /**
     * @brief 绘制调试用户界面。
     */
    void DrawDebugUI();

private:
    /**
     * @brief 默认构造函数。
     * 私有化以强制使用单例模式。
     */
    RuntimePrefabManager() = default;

    /**
     * @brief 默认析构函数。
     */
    ~RuntimePrefabManager() = default;

private:
    mutable PrefabPerformanceData m_performanceData; ///< 预制体性能数据。
};

#endif
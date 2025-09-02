#ifndef IRUNTIMETEXTUREMANAGER_H
#define IRUNTIMETEXTUREMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../../Utils/LazySingleton.h"
#include <include/core/SkImage.h>
#include <list>

#include "RuntimeAsset/RuntimeTexture.h"

/**
 * @brief 纹理性能数据结构。
 * 继承自 AssetPerformanceData，用于存储纹理相关的性能统计信息。
 */
struct TexturePerformanceData : public AssetPerformanceData
{
    size_t memoryUsageBytes = 0; ///< 当前纹理内存使用量（字节）。
    size_t memoryBudgetBytes = 0; ///< 纹理内存预算（字节）。
    int textureCount = 0; ///< 当前加载的纹理数量。
    int maxTextureCount = -1; ///< 允许的最大纹理数量。
    int cacheHits = 0; ///< 纹理缓存命中次数。
    int cacheMisses = 0; ///< 纹理缓存未命中次数。
    int evictions = 0; ///< 纹理从缓存中被逐出的次数。
};


/**
 * @brief 运行时纹理管理器。
 * 负责管理运行时纹理的加载、卸载、缓存和性能监控。
 * 继承自 RuntimeAssetManagerBase<RuntimeTexture> 提供基础资产管理功能，
 * 并通过 LazySingleton 实现单例模式。
 */
class RuntimeTextureManager : public RuntimeAssetManagerBase<RuntimeTexture>,
                              public LazySingleton<RuntimeTextureManager>
{
public:
    friend class LazySingleton<RuntimeTextureManager>;


    /**
     * @brief 尝试根据GUID获取一个运行时纹理。
     * @param guid 纹理的全局唯一标识符。
     * @return 如果找到，返回纹理的智能指针；否则返回空指针。
     */
    sk_sp<RuntimeTexture> TryGetAsset(const Guid& guid) const override;

    /**
     * @brief 尝试根据GUID获取一个运行时纹理，并通过输出参数返回。
     * @param guid 纹理的全局唯一标识符。
     * @param out 如果找到，纹理的智能指针将存储在此参数中。
     * @return 如果找到纹理，返回 true；否则返回 false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<RuntimeTexture>& out) const override;

    /**
     * @brief 尝试添加或更新一个运行时纹理。
     * @param guid 纹理的全局唯一标识符。
     * @param asset 要添加或更新的纹理智能指针。
     * @return 如果操作成功，返回 true；否则返回 false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimeTexture> asset) override;

    /**
     * @brief 尝试移除一个运行时纹理。
     * @param guid 要移除纹理的全局唯一标识符。
     * @return 如果成功移除纹理，返回 true；否则返回 false。
     */
    bool TryRemoveAsset(const Guid& guid) override;

    /**
     * @brief 获取纹理管理器的性能数据。
     * @return 指向纹理性能数据的常量指针。
     */
    const AssetPerformanceData* GetPerformanceData() const override;

    /**
     * @brief 绘制调试用户界面。
     * 用于显示纹理管理器的内部状态和性能数据。
     */
    void DrawDebugUI();

    /**
     * @brief 设置纹理内存预算。
     * @param mb 内存预算，单位为兆字节（MB）。
     */
    void SetMemoryBudget(float mb);

    /**
     * @brief 设置最大纹理数量。
     * @param count 允许加载的最大纹理数量。
     */
    void SetMaxTextureCount(int count);

    /**
     * @brief 析构函数。
     * 负责清理纹理管理器占用的资源。
     */
    ~RuntimeTextureManager() override;

private:
    /**
     * @brief 构造函数。
     * 私有构造函数，确保通过 LazySingleton 模式创建实例。
     */
    RuntimeTextureManager();

    /**
     * @brief 执行内存预算强制策略。
     * 当内存使用超出预算时，根据LRU策略逐出纹理。
     */
    void EnforceBudget();

    /**
     * @brief 资产更新事件回调函数。
     * 当资产更新时被调用，用于处理纹理相关的更新逻辑。
     * @param e 资产更新事件数据。
     */
    void OnAssetUpdated(const AssetUpdatedEvent& e);

private:
    ListenerHandle m_onAssetUpdatedHandle; ///< 资产更新事件的监听器句柄。
    mutable TexturePerformanceData m_performanceData; ///< 纹理性能数据。
    mutable std::list<Guid> m_lruTracker; ///< 用于LRU（最近最少使用）策略的纹理GUID列表。
    mutable std::unordered_map<std::string, std::list<Guid>::iterator> m_lruMap; ///< 将纹理GUID映射到LRU列表中迭代器的哈希表。
};
#endif
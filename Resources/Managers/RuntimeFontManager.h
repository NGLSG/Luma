#ifndef RUNTIMEFONTMANAGER_H
#define RUNTIMEFONTMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../../Utils/LazySingleton.h"
#include <include/core/SkTypeface.h>
#include <list>
#include <unordered_map>

/**
 * @brief 字体性能数据结构。
 * 继承自 AssetPerformanceData，用于存储字体相关的性能指标。
 */
struct FontPerformanceData : public AssetPerformanceData
{
    size_t memoryUsageBytes = 0; ///< 当前内存使用量（字节）。
    size_t memoryBudgetBytes = 0; ///< 内存预算（字节）。
    int fontCount = 0; ///< 当前加载的字体数量。
    int maxFontCount = -1; ///< 允许的最大字体数量。
    int cacheHits = 0; ///< 缓存命中次数。
    int cacheMisses = 0; ///< 缓存未命中次数。
    int evictions = 0; ///< 缓存逐出次数。
};

/**
 * @brief 运行时字体管理器。
 * 负责管理应用程序运行时的字体资产，支持资产的加载、卸载、性能跟踪以及内存预算控制。
 * 采用单例模式。
 */
class RuntimeFontManager : public RuntimeAssetManagerBase<SkTypeface>,
                           public LazySingleton<RuntimeFontManager>
{
public:
    friend class LazySingleton<RuntimeFontManager>;

    /**
     * @brief 尝试根据GUID获取字体资产。
     * @param guid 资产的全局唯一标识符。
     * @return 字体资产的智能指针，如果未找到则为空。
     */
    sk_sp<SkTypeface> TryGetAsset(const Guid& guid) const override;

    /**
     * @brief 尝试根据GUID获取字体资产，并将其存储到输出参数中。
     * @param guid 资产的全局唯一标识符。
     * @param out 用于接收字体资产的智能指针引用。
     * @return 如果成功获取资产则为 true，否则为 false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<SkTypeface>& out) const override;

    /**
     * @brief 尝试添加或更新字体资产。
     * @param guid 资产的全局唯一标识符。
     * @param asset 要添加或更新的字体资产。
     * @return 如果成功添加或更新资产则为 true，否则为 false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<SkTypeface> asset) override;

    /**
     * @brief 尝试移除字体资产。
     * @param guid 要移除资产的全局唯一标识符。
     * @return 如果成功移除资产则为 true，否则为 false。
     */
    bool TryRemoveAsset(const Guid& guid) override;

    /**
     * @brief 获取字体管理器的性能数据。
     * @return 指向资产性能数据的指针。
     */
    const AssetPerformanceData* GetPerformanceData() const override;

    /**
     * @brief 绘制调试用户界面。
     */
    void DrawDebugUI();

    /**
     * @brief 设置字体内存预算。
     * @param mb 内存预算大小（以兆字节为单位）。
     */
    void SetMemoryBudget(float mb);

    /**
     * @brief 设置最大字体数量。
     * @param count 允许的最大字体数量。
     */
    void SetMaxFontCount(int count);

private:
    /**
     * @brief 默认构造函数。
     */
    RuntimeFontManager() = default;

    /**
     * @brief 执行内存预算限制。
     */
    void EnforceBudget();

private:
    mutable FontPerformanceData m_performanceData; ///< 字体性能数据。
    mutable std::list<Guid> m_lruTracker; ///< 用于跟踪最近最少使用（LRU）顺序的列表。
    mutable std::unordered_map<Guid, std::list<Guid>::iterator> m_lruMap; ///< 将资产标识符映射到LRU跟踪器迭代器的哈希表。
};

#endif
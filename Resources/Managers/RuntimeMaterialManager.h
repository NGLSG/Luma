#ifndef RUNTIMEMATERIALMANAGER_H
#define RUNTIMEMATERIALMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../../Renderer/RenderComponent.h"
#include "../../Utils/LazySingleton.h"

/**
 * @brief 材质性能数据结构。
 * 继承自AssetPerformanceData，用于存储材质相关的性能统计信息。
 */
struct MaterialPerformanceData : public AssetPerformanceData
{
    int materialCount = 0; ///< 材质数量。
    int cacheHits = 0;     ///< 缓存命中次数。
    int cacheMisses = 0;   ///< 缓存未命中次数。
};

/**
 * @brief 运行时材质管理器。
 * 负责在运行时加载、管理和提供材质资源。
 * 采用单例模式，确保全局只有一个实例。
 */
class RuntimeMaterialManager : public RuntimeAssetManagerBase<Material>,
                               public LazySingleton<RuntimeMaterialManager>
{
public:
    friend class LazySingleton<RuntimeMaterialManager>;

    /**
     * @brief 尝试根据GUID获取材质资产。
     * @param guid 材质的全局唯一标识符。
     * @return 如果找到，返回材质的智能指针；否则返回空指针。
     */
    sk_sp<Material> TryGetAsset(const Guid& guid) const override;

    /**
     * @brief 尝试根据GUID获取材质资产，并通过输出参数返回。
     * @param guid 材质的全局唯一标识符。
     * @param out 输出参数，用于接收找到的材质智能指针。
     * @return 如果找到材质，返回true；否则返回false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<Material>& out) const override;

    /**
     * @brief 尝试添加或更新一个材质资产。
     * @param guid 材质的全局唯一标识符。
     * @param asset 要添加或更新的材质智能指针。
     * @return 如果操作成功，返回true；否则返回false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<Material> asset) override;

    /**
     * @brief 尝试移除一个材质资产。
     * @param guid 要移除材质的全局唯一标识符。
     * @return 如果移除成功，返回true；否则返回false。
     */
    bool TryRemoveAsset(const Guid& guid) override;

    /**
     * @brief 获取资产性能数据。
     * @return 指向资产性能数据的常量指针。
     */
    const AssetPerformanceData* GetPerformanceData() const override;

    /**
     * @brief 绘制调试用户界面。
     * @return 无。
     */
    void DrawDebugUI();

    /**
     * @brief 析构函数。
     */
    ~RuntimeMaterialManager();

private:
    RuntimeMaterialManager();

    /**
     * @brief 资产更新事件回调函数。
     * 当资产更新时被调用，用于处理材质相关的更新逻辑。
     * @param e 资产更新事件数据。
     */
    void OnAssetUpdated(const AssetUpdatedEvent& e);

private:
    ListenerHandle m_onAssetUpdatedHandle; ///< 资产更新事件的监听器句柄。
    mutable MaterialPerformanceData m_performanceData; ///< 材质性能数据。
};

#endif
#ifndef LUMAENGINE_RUNTIMETILEMANAGER_H
#define LUMAENGINE_RUNTIMETILEMANAGER_H
#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeTile.h"
#include "Utils/LazySingleton.h"

/**
 * @brief 运行时瓦片管理器。
 *
 * 负责管理运行时瓦片资源，并作为单例提供访问。
 * 它继承自 RuntimeAssetManagerBase，用于处理 RuntimeTile 类型的资产，
 * 并通过 LazySingleton 模式确保全局只有一个实例。
 */
class RuntimeTileManager : public RuntimeAssetManagerBase<RuntimeTile>, public LazySingleton<RuntimeTileManager>
{
    friend class LazySingleton<RuntimeTileManager>;

private:
    /**
     * @brief 默认构造函数。
     *
     * 私有构造函数，确保 RuntimeTileManager 只能通过其单例接口获取实例。
     */
    RuntimeTileManager() = default;
};
#endif
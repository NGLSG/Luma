#ifndef LUMAENGINE_RUNTIMERULETILEMANAGER_H
#define LUMAENGINE_RUNTIMERULETILEMANAGER_H

#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeRuleTile.h"
#include "Utils/LazySingleton.h"

/**
 * @brief 运行时规则瓦片管理器。
 *
 * 该类是一个懒加载单例，负责管理和提供运行时规则瓦片（RuntimeRuleTile）资产。
 * 它继承自 RuntimeAssetManagerBase，提供了对 RuntimeRuleTile 类型的资产管理功能。
 */
class RuntimeRuleTileManager : public RuntimeAssetManagerBase<RuntimeRuleTile>,
                               public LazySingleton<RuntimeRuleTileManager>
{
    friend class LazySingleton<RuntimeRuleTileManager>;

private:
    /**
     * @brief 默认构造函数。
     *
     * 私有构造函数，确保该类只能通过其单例接口进行实例化。
     */
    RuntimeRuleTileManager() = default;
};
#endif
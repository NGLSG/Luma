#ifndef LUMAENGINE_RUNTIMETILESETMANAGER_H
#define LUMAENGINE_RUNTIMETILESETMANAGER_H
#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeTileset.h"
#include "Utils/LazySingleton.h"

/**
 * @brief 运行时图块集管理器。
 *
 * 负责在运行时管理和提供图块集资源。
 * 它继承自RuntimeAssetManagerBase以处理资源管理逻辑，并作为LazySingleton实现单例模式。
 */
class RuntimeTilesetManager : public RuntimeAssetManagerBase<RuntimeTileset>,
                              public LazySingleton<RuntimeTilesetManager>
{
    friend class LazySingleton<RuntimeTilesetManager>;

private:
    /**
     * @brief 默认构造函数。
     *
     * 作为单例模式的一部分，此构造函数是私有的，通常由LazySingleton友元类调用以创建实例。
     */
    RuntimeTilesetManager() = default;
};
#endif
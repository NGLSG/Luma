#ifndef TILESETLOADER_H
#define TILESETLOADER_H
#include "IAssetLoader.h"
#include "RuntimeAsset/RuntimeTileset.h"

/**
 * @brief 瓦片集加载器。
 *
 * 负责从不同的源加载运行时瓦片集（RuntimeTileset）资产。
 */
class TilesetLoader : public IAssetLoader<RuntimeTileset>
{
public:
    /**
     * @brief 根据资产元数据加载瓦片集。
     *
     * @param metadata 资产的元数据信息。
     * @return 智能指针，指向加载的运行时瓦片集对象。
     */
    sk_sp<RuntimeTileset> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符（GUID）加载瓦片集。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return 智能指针，指向加载的运行时瓦片集对象。
     */
    sk_sp<RuntimeTileset> LoadAsset(const Guid& Guid) override;
};


#endif
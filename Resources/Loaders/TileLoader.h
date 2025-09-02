/**
 * @brief 负责加载运行时瓦片资源的加载器。
 *
 * 该类继承自 IAssetLoader<RuntimeTile>，提供了根据资产元数据或 GUID 加载 RuntimeTile 资源的功能。
 */
#ifndef LUMAENGINE_TILELOADER_H
#define LUMAENGINE_TILELOADER_H


#pragma once
#include "IAssetLoader.h"
#include "../RuntimeAsset/RuntimeTile.h"

class TileLoader : public IAssetLoader<RuntimeTile>
{
public:
    /**
     * @brief 根据资产元数据加载运行时瓦片资源。
     *
     * 此方法根据提供的资产元数据加载并返回一个 RuntimeTile 实例。
     * @param metadata 包含资产信息的元数据。
     * @return 指向加载的运行时瓦片资源的智能指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeTile> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载运行时瓦片资源。
     *
     * 此方法根据提供的 GUID 查找、加载并返回一个 RuntimeTile 实例。
     * @param Guid 资产的全局唯一标识符。
     * @return 指向加载的运行时瓦片资源的智能指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeTile> LoadAsset(const Guid& Guid) override;
};


#endif
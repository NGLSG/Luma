#include "TileLoader.h"

#include "AssetManager.h"
#include "Logger.h"
#include "Managers/RuntimeTileManager.h"

sk_sp<RuntimeTile> TileLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Tile || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }

    try
    {
        TileAssetData data = metadata.importerSettings.as<TileAssetData>();
        return sk_make_sp<RuntimeTile>(metadata.guid, std::move(data));
    }
    catch (const std::exception& e)
    {
        LogError("加载运行时Tile资产失败: {}，错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeTile> TileLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimeTileManager::GetInstance();
    sk_sp<RuntimeTile> asset = manager.TryGetAsset(guid);
    if (asset)
    {
        return asset;
    }

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata)
    {
        return nullptr;
    }

    asset = LoadAsset(*metadata);
    if (asset)
    {
        manager.TryAddOrUpdateAsset(guid, asset);
    }
    return asset;
}

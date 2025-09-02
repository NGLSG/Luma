#include "TilesetLoader.h"
#include "AssetManager.h"
#include "Logger.h"
#include "Data/RuleTile.h"
#include "Managers/RuntimeTilesetManager.h"

sk_sp<RuntimeTileset> TilesetLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Tileset || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }

    try
    {
        auto data = metadata.importerSettings.as<TilesetData>();
        return sk_make_sp<RuntimeTileset>(metadata.guid, std::move(data));
    }
    catch (const std::exception& e)
    {
        LogError("加载运行时Tile资产失败: {}，错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeTileset> TilesetLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimeTilesetManager::GetInstance();
    sk_sp<RuntimeTileset> asset = manager.TryGetAsset(guid);
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

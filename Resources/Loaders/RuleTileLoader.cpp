#include "RuleTileLoader.h"
#include "AssetManager.h"
#include "Logger.h"
#include "Data/RuleTile.h"
#include "Managers/RuntimeRuleTileManager.h"

sk_sp<RuntimeRuleTile> RuleTileLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::RuleTile || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }

    try
    {
        RuleTileAssetData data = metadata.importerSettings.as<RuleTileAssetData>();
        return sk_make_sp<RuntimeRuleTile>(metadata.guid, std::move(data));
    }
    catch (const std::exception& e)
    {
        LogError("加载运行时Tile资产失败: {}，错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeRuleTile> RuleTileLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimeRuleTileManager::GetInstance();
    sk_sp<RuntimeRuleTile> asset = manager.TryGetAsset(guid);
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

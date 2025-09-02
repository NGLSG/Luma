#include "PrefabLoader.h"

#include <iostream>

#include "../../Data/PrefabData.h"
#include "../AssetManager.h"
#include "../Managers/RuntimePrefabManager.h"

sk_sp<RuntimePrefab> PrefabLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Prefab)
    {
        return nullptr;
    }

    try
    {
        Data::PrefabData prefabData = metadata.importerSettings.as<Data::PrefabData>();


        return sk_make_sp<RuntimePrefab>(metadata.guid, std::move(prefabData));
    }
    catch (const YAML::Exception& e)
    {
        return nullptr;
    }
}

sk_sp<RuntimePrefab> PrefabLoader::LoadAsset(const Guid& Guid)
{
    sk_sp<RuntimePrefab> prefab;
    if (RuntimePrefabManager::GetInstance().TryGetAsset(Guid, prefab))
    {
        return prefab;
    }
    const auto& assetManager = AssetManager::GetInstance();


    const AssetMetadata* metadata = assetManager.GetMetadata(Guid);
    if (!metadata || metadata->type != AssetType::Prefab)
    {
        return nullptr;
    }


    prefab = LoadAsset(*metadata);
    if (!prefab)
    {
        return nullptr;
    }

    if (!RuntimePrefabManager::GetInstance().TryAddOrUpdateAsset(Guid, prefab))
    {
        std::cerr << "Failed to add or update Prefab in RuntimePrefabManager for Guid: " << Guid.ToString() <<
            std::endl;
    }
    return prefab;
}

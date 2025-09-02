#include "SceneLoader.h"
#include "../../Data/SceneData.h"
#include "../AssetManager.h"
#include "../../Utils/Path.h"

sk_sp<RuntimeScene> SceneLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Scene)
    {
        return nullptr;
    }
    try
    {
        Data::SceneData sceneData = metadata.importerSettings.as<Data::SceneData>();
        sceneData.name = Path::GetFileNameWithoutExtension(metadata.assetPath.string());


        sk_sp<RuntimeScene> newScene = sk_make_sp<RuntimeScene>(metadata.guid);


        newScene->LoadFromData(sceneData);

        return newScene;
    }
    catch (const YAML::Exception& e)
    {
        return nullptr;
    }
}

sk_sp<RuntimeScene> SceneLoader::LoadAsset(const Guid& guid)
{
    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata)
    {
        return nullptr;
    }


    return LoadAsset(*metadata);
}

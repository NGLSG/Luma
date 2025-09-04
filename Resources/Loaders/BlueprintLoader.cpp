#include "BlueprintLoader.h"
#include "Resources/AssetManager.h"
#include "Utils/Logger.h"

sk_sp<RuntimeBlueprint> BlueprintLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Blueprint || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }
    try
    {
        Blueprint data = metadata.importerSettings.as<Blueprint>();
        return sk_make_sp<RuntimeBlueprint>(data, metadata.guid);
    }
    catch (const std::exception& e)
    {
        LogError("加载蓝图失败: {}, 错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeBlueprint> BlueprintLoader::LoadAsset(const Guid& Guid)
{
    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(Guid);
    if (!metadata)
    {
        LogError("找不到GUID为 {} 的蓝图元数据", Guid.ToString());
        return nullptr;
    }

    return LoadAsset(*metadata);
}

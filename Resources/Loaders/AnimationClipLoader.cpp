#include "AnimationClipLoader.h"

#include "AssetManager.h"
#include "Logger.h"
#include "Managers/RuntimeAnimationClipManager.h"

sk_sp<RuntimeAnimationClip> AnimationClipLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::AnimationClip || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }
    try
    {
        AnimationClip data = metadata.importerSettings.as<AnimationClip>();
        return sk_make_sp<RuntimeAnimationClip>(metadata.guid, data);
    }
    catch (const std::exception& e)
    {
        LogError("加载动画切片失败: {}，错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeAnimationClip> AnimationClipLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimeAnimationClipManager::GetInstance();
    sk_sp<RuntimeAnimationClip> material = manager.TryGetAsset(guid);
    if (material) return material;

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata) return nullptr;

    material = LoadAsset(*metadata);
    if (material)
    {
        manager.TryAddOrUpdateAsset(guid, material);
    }
    return material;
}

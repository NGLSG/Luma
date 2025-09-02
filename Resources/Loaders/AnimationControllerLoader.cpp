#include "AnimationControllerLoader.h"

#include "AssetManager.h"
#include "Logger.h"
#include "Managers/RuntimeAnimationControllerManager.h"

sk_sp<RuntimeAnimationController> AnimationControllerLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::AnimationController || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }
    try
    {
        AnimationControllerData data = metadata.importerSettings.as<AnimationControllerData>();
        return sk_make_sp<RuntimeAnimationController>(std::move(data));
    }
    catch (const std::exception& e)
    {
        LogError("加载动画控制器失败: {}，错误: {}", metadata.assetPath.string(), e.what());
        return nullptr;
    }
}

sk_sp<RuntimeAnimationController> AnimationControllerLoader::LoadAsset(const Guid& Guid)
{
    sk_sp<RuntimeAnimationController> controller;

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(Guid);
    if (!metadata) return nullptr;

    controller = LoadAsset(*metadata);
    if (!controller)
    {
        LogError("加载动画控制器失败: {}", metadata->assetPath.string());
        return nullptr;
    }
    return controller;
}

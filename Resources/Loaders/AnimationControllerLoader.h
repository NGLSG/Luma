#ifndef LUMAENGINE_ANIMATIONCONTROLLERLOADER_H
#define LUMAENGINE_ANIMATIONCONTROLLERLOADER_H
#include "IAssetLoader.h"
#include "RuntimeAsset/RuntimeAnimationController.h"

/**
 * @brief 动画控制器加载器。
 *
 * 负责从各种源加载运行时动画控制器资产。
 */
class AnimationControllerLoader : public IAssetLoader<RuntimeAnimationController>
{
public:
    /**
     * @brief 根据资产元数据加载动画控制器资产。
     *
     * @param metadata 资产的元数据，包含加载所需的信息。
     * @return 一个指向加载的运行时动画控制器的共享指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeAnimationController> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载动画控制器资产。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return 一个指向加载的运行时动画控制器的共享指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeAnimationController> LoadAsset(const Guid& Guid) override;
};


#endif
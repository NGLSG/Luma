#ifndef ANIMATIONCLIPLOADER_H
#define ANIMATIONCLIPLOADER_H
#include "IAssetLoader.h"
#include "RuntimeAsset/RuntimeAnimationClip.h"

/**
 * @brief 动画剪辑资源加载器。
 *
 * 负责从不同的源加载并创建 RuntimeAnimationClip 实例。
 */
class AnimationClipLoader : public IAssetLoader<RuntimeAnimationClip>
{
public:
    /**
     * @brief 根据资产元数据加载动画剪辑资源。
     * @param metadata 资产的元数据信息，包含加载所需的所有数据。
     * @return 加载的动画剪辑资源的智能指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeAnimationClip> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符(GUID)加载动画剪辑资源。
     * @param Guid 资产的全局唯一标识符。
     * @return 加载的动画剪辑资源的智能指针。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimeAnimationClip> LoadAsset(const Guid& Guid) override;
};


#endif
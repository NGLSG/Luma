#ifndef SCENELOADER_H
#define SCENELOADER_H
#include "IAssetLoader.h"
#include "../RuntimeAsset/RuntimeScene.h"

/**
 * @brief 场景加载器，负责从各种源加载运行时场景资产。
 *
 * 该类实现了IAssetLoader接口，专门用于加载RuntimeScene类型的资产。
 */
class SceneLoader : IAssetLoader<RuntimeScene>
{
public:
    /**
     * @brief 根据资产元数据加载运行时场景。
     *
     * @param metadata 包含资产信息的元数据对象。
     * @return 一个指向加载的运行时场景的智能指针。
     */
    sk_sp<RuntimeScene> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载运行时场景。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return 一个指向加载的运行时场景的智能指针。
     */
    sk_sp<RuntimeScene> LoadAsset(const Guid& Guid) override;
};


#endif
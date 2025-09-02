#ifndef PREFABLOADER_H
#define PREFABLOADER_H
#include "IAssetLoader.h"
#include "../RuntimeAsset/RuntimePrefab.h"

/**
 * @brief 预制体资源加载器。
 *
 * 负责从存储中加载和管理运行时预制体 (RuntimePrefab) 资产。
 */
class PrefabLoader : public IAssetLoader<RuntimePrefab>
{
public:
    /**
     * @brief 根据资产元数据加载预制体资产。
     *
     * @param metadata 资产的元数据信息，包含加载所需的所有细节。
     * @return 智能指针指向加载的运行时预制体对象。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimePrefab> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载预制体资产。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return 智能指针指向加载的运行时预制体对象。如果加载失败，可能返回空指针。
     */
    sk_sp<RuntimePrefab> LoadAsset(const Guid& Guid) override;
};


#endif
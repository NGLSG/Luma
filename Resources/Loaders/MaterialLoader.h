#ifndef MATERIALLOADER_H
#define MATERIALLOADER_H

#include "IAssetLoader.h"
#include "../../Renderer/RenderComponent.h"

/**
 * @brief 材质加载器，用于加载材质资源。
 *
 * 继承自 IAssetLoader 接口，专门处理 Material 类型的资产加载。
 */
class MaterialLoader : public IAssetLoader<Material>
{
public:
    /**
     * @brief 根据资产元数据加载材质资源。
     * @param metadata 资产的元数据信息。
     * @return 指向加载的材质资源的共享指针。
     */
    sk_sp<Material> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符（GUID）加载材质资源。
     * @param Guid 资产的全局唯一标识符。
     * @return 指向加载的材质资源的共享指针。
     */
    sk_sp<Material> LoadAsset(const Guid& Guid) override;
};


#endif
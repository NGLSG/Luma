#ifndef PHYSICSMATERIALLOADER_H
#define PHYSICSMATERIALLOADER_H
#include "IAssetLoader.h"

class RuntimePhysicsMaterial;

/**
 * @brief 物理材质加载器。
 *
 * 负责加载运行时物理材质资产。
 */
class PhysicsMaterialLoader : public IAssetLoader<RuntimePhysicsMaterial>
{
public:
    /**
     * @brief 根据资产元数据加载运行时物理材质。
     * @param metadata 资产的元数据。
     * @return 加载的运行时物理材质的智能指针。
     */
    sk_sp<RuntimePhysicsMaterial> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符(GUID)加载运行时物理材质。
     * @param Guid 资产的全局唯一标识符。
     * @return 加载的运行时物理材质的智能指针。
     */
    sk_sp<RuntimePhysicsMaterial> LoadAsset(const Guid& Guid) override;
};


#endif
#include "PhysicsMaterialLoader.h"
#include "../../Data/PhysicsMaterial.h"
#include "../AssetManager.h"
#include "../Managers/RuntimePhysicsMaterialManager.h"

sk_sp<RuntimePhysicsMaterial> PhysicsMaterialLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::PhysicsMaterial) return nullptr;
    try
    {
        Data::PhysicsMaterialData data = metadata.importerSettings.as<Data::PhysicsMaterialData>();
        return sk_make_sp<RuntimePhysicsMaterial>(metadata.guid, data.friction, data.restitution,
                                                  data.rollingResistance, data.tangentSpeed);
    }
    catch (const YAML::Exception&) { return nullptr; }
}

sk_sp<RuntimePhysicsMaterial> PhysicsMaterialLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimePhysicsMaterialManager::GetInstance();
    sk_sp<RuntimePhysicsMaterial> material = manager.TryGetAsset(guid);
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

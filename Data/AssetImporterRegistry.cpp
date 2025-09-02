#include "AssetImporterRegistry.h"


void AssetImporterRegistry::Register(AssetType type, AssetImporterRegistration registration)
{
    m_registry[type] = std::move(registration);
}


const AssetImporterRegistration* AssetImporterRegistry::Get(AssetType type) const
{
    if (m_registry.contains(type))
    {
        return &m_registry.at(type);
    }

    return nullptr;
}

#include "PhysicsMaterialImporter.h"

#include "AssetManager.h"
#include "Path.h"
#include "../../Utils/Utils.h"

const std::vector<std::string>& PhysicsMaterialImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".physmat"};
    return extensions;
}

AssetMetadata PhysicsMaterialImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.type = AssetType::PhysicsMaterial;
    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.guid = Guid::NewGuid();

    metadata.importerSettings = YAML::LoadFile(assetPath.string());

    return metadata;
}

AssetMetadata PhysicsMaterialImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata newMetadata = metadata;
    newMetadata.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());


    newMetadata.importerSettings = YAML::LoadFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());

    return newMetadata;
}

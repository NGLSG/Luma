#include "RuleTileImporter.h"

#include "AssetManager.h"
#include "Path.h"
#include "Data/RuleTile.h"
#include "Utils/Utils.h"

const std::vector<std::string>& RuleTileImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".ruletile"};
    return extensions;
}

AssetMetadata RuleTileImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::RuleTile;
    metadata.importerSettings = YAML::LoadFile(assetPath.string());
    return metadata;
}

AssetMetadata RuleTileImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    updatedMeta.importerSettings = YAML::LoadFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    return updatedMeta;
}

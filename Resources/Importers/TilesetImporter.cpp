#include "TilesetImporter.h"

#include "AssetManager.h"
#include "Path.h"
#include "Utils/Utils.h"
#include "Data/Tileset.h"

const std::vector<std::string>& TilesetImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".tileset"};
    return extensions;
}

AssetMetadata TilesetImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());
    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Tileset;


    metadata.importerSettings = YAML::LoadFile(assetPath.string());

    return metadata;
}

AssetMetadata TilesetImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    updatedMeta.importerSettings = YAML::LoadFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    return updatedMeta;
}

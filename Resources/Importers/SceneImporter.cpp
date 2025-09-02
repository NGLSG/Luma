#include "SceneImporter.h"
#include "../../Data/PrefabData.h"
#include "../../Utils/Utils.h"
#include <fstream>

#include "AssetManager.h"
#include "Path.h"

const std::vector<std::string>& SceneImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".scene"};
    return extensions;
}

AssetMetadata SceneImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Scene;


    metadata.importerSettings = YAML::LoadFile(assetPath.string());

    return metadata;
}

AssetMetadata SceneImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    updatedMeta.importerSettings = YAML::LoadFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    return updatedMeta;
}

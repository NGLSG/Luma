#include "AnimationControllerImporter.h"

#include "AssetManager.h"
#include "Path.h"
#include "Utils.h"

const std::vector<std::string>& AnimationControllerImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".animctrl"};
    return extensions;
}

AssetMetadata AnimationControllerImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::AnimationController;
    metadata.importerSettings = YAML::LoadFile(assetPath.string());
    return metadata;
}

AssetMetadata AnimationControllerImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    YAML::Node animCtrlData = YAML::LoadFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    updatedMeta.importerSettings = animCtrlData;
    return updatedMeta;
}

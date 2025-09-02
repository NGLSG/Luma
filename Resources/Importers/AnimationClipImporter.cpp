#include "AnimationClipImporter.h"

#include "AnimationClip.h"
#include "AssetManager.h"
#include "Path.h"
#include "Utils.h"

const std::vector<std::string>& AnimationClipImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".anim"};
    return extensions;
}


AssetMetadata AnimationClipImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());
    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::AnimationClip;
    metadata.importerSettings = YAML::LoadFile(assetPath.string());
    return metadata;
}

AssetMetadata AnimationClipImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    YAML::Node matData =
        YAML::LoadFile((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    AnimationClip definition = matData.as<AnimationClip>();
    updatedMeta.importerSettings = definition;
    return updatedMeta;
}

#include "ShaderImporter.h"

#include "AssetManager.h"
#include "Path.h"
#include "ShaderData.h"
#include "Utils.h"

const std::vector<std::string>& ShaderImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".shader"};
    return extensions;
}

AssetMetadata ShaderImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());
    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Shader;
    metadata.importerSettings = YAML::LoadFile(assetPath.string());
    return metadata;
}

AssetMetadata ShaderImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    YAML::Node matData =
        YAML::LoadFile((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    Data::ShaderData definition = matData.as<Data::ShaderData>();
    updatedMeta.importerSettings = definition;
    return updatedMeta;
}

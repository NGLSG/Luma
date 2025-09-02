#include "IAssetImporter.h"

#include "AssetManager.h"
IAssetImporter::~IAssetImporter() = default;

void IAssetImporter::WriteMetadata(const AssetMetadata& metadata)
{
    std::filesystem::path metaFilePath = AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath;
    metaFilePath += ".meta";


    YAML::Node node = YAML::convert<AssetMetadata>::encode(metadata);

    std::ofstream fout(metaFilePath);
    fout << node;
}

AssetMetadata IAssetImporter::LoadMetadata(const std::filesystem::path& metaFilePath)
{
    try
    {
        YAML::Node data = YAML::LoadFile(metaFilePath.string());


        AssetMetadata metadata = data.as<AssetMetadata>();


        metadata.assetPath = metaFilePath;
        metadata.assetPath.replace_extension("");

        return metadata;
    }
    catch (const std::exception& e)
    {
        return {};
    }
}

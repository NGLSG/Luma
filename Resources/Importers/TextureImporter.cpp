#include "TextureImporter.h"
#include "../../Utils/Utils.h"
#include "TextureImporterSettings.h"
#include <fstream>
#include <vector>
#include <iterator>

#include "AssetManager.h"
#include "Path.h"


const std::vector<std::string>& TextureImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {
        ".png", ".jpg", ".jpeg", ".webp", ".svg"
    };
    return extensions;
}


static std::vector<uint8_t> ReadFileBinary(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return {};
    }


    file.unsetf(std::ios::skipws);
    return std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}


AssetMetadata TextureImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Texture;

    TextureImporterSettings settings;


    const std::vector<uint8_t> buffer = ReadFileBinary(assetPath);
    if (!buffer.empty())
    {
        settings.rawData = YAML::Binary(buffer.data(), buffer.size());
    }


    metadata.importerSettings = settings;

    return metadata;
}


AssetMetadata TextureImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());

    TextureImporterSettings settings;


    if (metadata.importerSettings)
    {
        settings = metadata.importerSettings.as<TextureImporterSettings>();
    }


    const std::vector<uint8_t> buffer = ReadFileBinary(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    if (!buffer.empty())
    {
        settings.rawData = YAML::Binary(buffer.data(), buffer.size());
    }
    else
    {
        settings.rawData = YAML::Binary(nullptr, 0);
    }


    updatedMeta.importerSettings = settings;

    return updatedMeta;
}

#include "FontImporter.h"
#include "../../Utils/Utils.h"
#include <fstream>
#include <vector>
#include <iterator>

#include "AssetManager.h"
#include "Path.h"

const std::vector<std::string>& FontImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {
        ".ttf", ".otf", ".woff", ".woff2"
    };
    return extensions;
}

AssetMetadata FontImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Font;

    ExtractFontInfo(assetPath, metadata.importerSettings);

    return metadata;
}

AssetMetadata FontImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());

    ExtractFontInfo((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string(),
                    updatedMeta.importerSettings);

    return updatedMeta;
}

void FontImporter::ExtractFontInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode)
{
    std::ifstream file(assetPath, std::ios::binary);
    if (!file.is_open())
    {
        settingsNode = YAML::Node();
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        settingsNode["encodedData"] = YAML::Binary(buffer.data(), buffer.size());
    }
}

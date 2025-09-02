#include "AudioImporter.h"
#include "../../Utils/Utils.h"
#include <fstream>

#include "AssetManager.h"
#include "Path.h"

const std::vector<std::string>& AudioImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> exts = {
        ".mp3", ".ogg", ".flac", ".wav", ".aac", ".m4a", ".wma"
    };
    return exts;
}

AssetMetadata AudioImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata meta;
    meta.guid = Guid::NewGuid();
    meta.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    meta.fileHash = Utils::GetHashFromFile(assetPath.string());
    meta.type = AssetType::Audio;

    ReadFileBytes(assetPath, meta.importerSettings);
    return meta;
}

AssetMetadata AudioImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata out = metadata;
    out.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    ReadFileBytes((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string(),
                  out.importerSettings);
    return out;
}

void AudioImporter::ReadFileBytes(const std::filesystem::path& assetPath, YAML::Node& settingsNode)
{
    std::ifstream f(assetPath, std::ios::binary);
    if (!f.is_open())
    {
        settingsNode = YAML::Node();
        return;
    }
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    if (size <= 0)
    {
        settingsNode = YAML::Node();
        return;
    }
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    if (f.read(reinterpret_cast<char*>(buf.data()), size))
    {
        settingsNode["encodedData"] = YAML::Binary(buf.data(), buf.size());
    }
}

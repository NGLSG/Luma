#include "RuntimeAssetManager.h"
#include "AssetPacker.h"
#include "../Utils/Logger.h"

RuntimeAssetManager::RuntimeAssetManager(const std::filesystem::path& packageManifestPath)
{
    LogInfo("RuntimeAssetManager: Initializing from package '{}'...", packageManifestPath.string());
    try
    {
        m_assetDatabase = AssetPacker::Unpack(packageManifestPath);
        LogInfo("RuntimeAssetManager: Loaded {} assets from package.", m_assetDatabase.size());
    }
    catch (const std::exception& e)
    {
        m_assetDatabase.clear();
        LogError("RuntimeAssetManager: Failed to initialize. Reason: {}", e.what());
    }
}

std::string RuntimeAssetManager::GetAssetName(const Guid& guid) const
{
    const AssetMetadata* meta = GetMetadata(guid);
    if (meta)
    {
        return meta->assetPath.stem().string();
    }
    return "[Unknown Asset]";
}

const AssetMetadata* RuntimeAssetManager::GetMetadata(const Guid& guid) const
{
    auto it = m_assetDatabase.find(guid.ToString());
    if (it != m_assetDatabase.end())
    {
        return &it->second;
    }
    return nullptr;
}

const AssetMetadata* RuntimeAssetManager::GetMetadata(const std::filesystem::path& assetPath) const
{
    LogWarn("RuntimeAssetManager: GetMetadata by path is not supported in Runtime mode.");
    return nullptr;
}

const std::unordered_map<std::string, AssetMetadata>& RuntimeAssetManager::GetAssetDatabase() const
{
    return m_assetDatabase;
}

const std::filesystem::path& RuntimeAssetManager::GetAssetsRootPath() const
{
    return m_dummyPath;
}

#include "AssetManager.h"
#include "EditorAssetManager.h"
#include "RuntimeAssetManager.h"
#include "../Utils/Logger.h"

AssetManager::~AssetManager() = default;

void AssetManager::Initialize(ApplicationMode mode, const std::filesystem::path& path)
{
    if (mode == ApplicationMode::Editor)
    {
        m_implementation = std::make_unique<EditorAssetManager>(path);
    }
    else
    {
        m_implementation = std::make_unique<RuntimeAssetManager>(path);
    }
}

std::string AssetManager::GetAssetName(const Guid& guid) const
{
    if (m_implementation) return m_implementation->GetAssetName(guid);

    return "[Not Initialized]";
}

const AssetMetadata* AssetManager::GetMetadata(const Guid& guid) const
{
    if (m_implementation) return m_implementation->GetMetadata(guid);

    return nullptr;
}

const AssetMetadata* AssetManager::GetMetadata(const std::filesystem::path& assetPath) const
{
    if (m_implementation) return m_implementation->GetMetadata(assetPath);

    return nullptr;
}

const std::unordered_map<std::string, AssetMetadata>& AssetManager::GetAssetDatabase() const
{
    if (m_implementation) return m_implementation->GetAssetDatabase();

    static const std::unordered_map<std::string, AssetMetadata> emptyDb;

    return emptyDb;
}

const std::filesystem::path& AssetManager::GetAssetsRootPath() const
{
    if (m_implementation) return m_implementation->GetAssetsRootPath();
    static const std::filesystem::path emptyPath;

    return emptyPath;
}

const void AssetManager::ReImport(const AssetMetadata& metadata)
{
    if (m_implementation) m_implementation->ReImport(metadata);
}

void AssetManager::Update(float deltaTime)
{
    if (m_implementation)
    {
        m_implementation->Update(deltaTime);
    }
    else
    {
    }
}

void AssetManager::Shutdown()
{
    if (m_implementation)
    {
        m_implementation.reset();
        LogInfo("AssetManager: Shutdown complete.");
    }
}

bool AssetManager::StartPreload()
{
    if (m_implementation)
    {
        return m_implementation->StartPreload();
    }
    return false;
}

void AssetManager::StopPreload()
{
    if (m_implementation)
    {
        m_implementation->StopPreload();
    }
}

std::pair<int, int> AssetManager::GetPreloadProgress() const
{
    if (m_implementation)
    {
        return m_implementation->GetPreloadProgress();
    }
    return std::make_pair(0, 0);
}

bool AssetManager::IsPreloadComplete() const
{
    if (m_implementation)
    {
        return m_implementation->IsPreloadComplete();
    }
    return false;
}

bool AssetManager::IsPreloadRunning() const
{
    if (m_implementation)
    {
        return m_implementation->IsPreloadRunning();
    }
    return false;
}

Guid AssetManager::LoadAsset(const std::filesystem::path& assetPath)
{
    if (m_implementation)
    {
        return m_implementation->LoadAsset(assetPath);
    }
    return Guid::Invalid();
}

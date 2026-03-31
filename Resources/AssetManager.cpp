#include "AssetManager.h"
#include "EditorAssetManager.h"
#include "RuntimeAssetManager.h"
#include "../Utils/Logger.h"

AssetManager::~AssetManager()
{
    if (m_fileWatcher)
        m_fileWatcher->Stop();
}

void AssetManager::Initialize(ApplicationMode mode, const std::filesystem::path& path)
{
    m_appMode = mode;

    if (mode == ApplicationMode::Editor)
    {
        m_implementation = std::make_unique<EditorAssetManager>(path);

        m_fileWatcher = std::make_unique<FileWatcher>(path);
        m_fileWatcher->SetCallback([this](const FileChangeEvent& event) { OnFileChanged(event); });
        m_fileWatcher->Start();
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

Guid AssetManager::GetGuidByAddress(const std::string& address) const
{
    if (m_implementation)
    {
        return m_implementation->GetGuidByAddress(address);
    }
    return Guid::Invalid();
}

std::vector<Guid> AssetManager::GetGuidsByGroup(const std::string& group) const
{
    if (m_implementation)
    {
        return m_implementation->GetGuidsByGroup(group);
    }
    return {};
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
    if (m_fileWatcher)
    {
        m_fileWatcher->Stop();
        m_fileWatcher.reset();
    }

    if (m_implementation)
    {
        m_implementation.reset();
        LogInfo("AssetManager: Shutdown complete.");
    }
}

void AssetManager::OnFileChanged(const FileChangeEvent& event)
{
    if (!m_implementation)
        return;

    if (event.path.extension() == ".meta")
        return;

    auto ext = event.path.extension().string();
    if (event.type == FileChangeType::Deleted)
    {
        LogInfo("AssetManager: File deleted: {}", event.path.string());
        return;
    }

    const auto* metadata = m_implementation->GetMetadata(event.path);
    if (metadata)
    {
        LogInfo("AssetManager: Hot-reloading {}", event.path.string());
        m_implementation->ReImport(*metadata);
    }
    else if (event.type == FileChangeType::Created)
    {
        LogInfo("AssetManager: New file detected: {}", event.path.string());
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

Guid AssetManager::LoadAssetByAddress(const std::string& address)
{
    if (m_implementation)
    {
        return m_implementation->LoadAssetByAddress(address);
    }
    return Guid::Invalid();
}

bool AssetManager::StartPreWarmingShader()
{
    if (m_implementation)
    {
        return m_implementation->StartPreWarmingShader();
    }
    return false;
}

void AssetManager::StopPreWarmingShader()
{
    if (m_implementation)
    {
        m_implementation->StopPreWarmingShader();
    }
}

std::pair<int, int> AssetManager::GetPreWarmingProgress() const
{
    if (m_implementation)
    {
        return m_implementation->GetPreWarmingProgress();
    }
    return std::make_pair(0, 0);
}

bool AssetManager::IsPreWarmingComplete() const
{
    if (m_implementation)
    {
        return m_implementation->IsPreWarmingComplete();
    }
    return false;
}

bool AssetManager::IsPreWarmingRunning() const
{
    if (m_implementation)
    {
        return m_implementation->IsPreWarmingRunning();
    }
    return false;
}

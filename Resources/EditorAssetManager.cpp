#include "EditorAssetManager.h"
#include "../Utils/Logger.h"
#include "../Utils/Utils.h"
#include <algorithm>
#include <iostream>

#include "Path.h"
#include "Importers/TextureImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/PhysicsMaterialImporter.h"
#include "Importers/SceneImporter.h"
#include "Importers/PrefabImporter.h"
#include "Importers/CSharpScriptImporter.h"
#include "Importers/FontImporter.h"
#include "Importers/AudioImporter.h"
#include "Importers/AnimationClipImporter.h"
#include "Importers/AnimationControllerImporter.h"
#include "Importers/TileImporter.h"
#include "Importers/TilesetImporter.h"
#include "Importers/RuleTileImporter.h"

EditorAssetManager::EditorAssetManager(const std::filesystem::path& projectRootPath)
{
    RegisterImporters();
    m_assetsRoot = projectRootPath / "Assets";
    if (!std::filesystem::exists(m_assetsRoot))
    {
        std::filesystem::create_directories(m_assetsRoot);
    }

    unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency() / 2);
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        m_workerThreads.emplace_back(&EditorAssetManager::WorkerLoop, this);
    }

    InitialScan();
}

EditorAssetManager::~EditorAssetManager()
{
    {
        std::unique_lock<std::mutex> lock(m_taskQueueMutex);
        m_stopThreads = true;
    }
    m_taskCondition.notify_all();
}

void EditorAssetManager::WorkerLoop()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);
            m_taskCondition.wait(lock, [this]
            {
                return m_stopThreads || !m_taskQueue.empty();
            });
            if (m_stopThreads && m_taskQueue.empty()) return;
            task = std::move(m_taskQueue.front());
            m_taskQueue.pop();
        }
        task();
    }
}

void EditorAssetManager::Update(float deltaTime)
{
    std::unique_ptr<ScanResult> newResult = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_scanResultMutex);
        if (m_scanResult)
        {
            newResult = std::move(m_scanResult);
        }
    }

    if (newResult)
    {
        {
            std::lock_guard<std::mutex> lock(m_dbMutex);
            m_guidToMeta = std::move(newResult->guidToMeta);
            m_pathToGuid = std::move(newResult->pathToGuid);
        }
        m_isScanning = false;
        m_rescanTimer = RESCAN_INTERVAL;
    }

    if (!m_isScanning)
    {
        m_rescanTimer -= deltaTime;
        if (m_rescanTimer <= 0.0f)
        {
            m_isScanning = true;
            std::thread([this]() { this->ScanDirectoryTask(); }).detach();
        }
    }
}

void EditorAssetManager::ScanDirectoryTask()
{
    auto result = std::make_unique<ScanResult>();
    std::mutex resultMutex;
    if (!std::filesystem::exists(m_assetsRoot) || !std::filesystem::is_directory(m_assetsRoot))
    {
        std::lock_guard<std::mutex> lock(m_scanResultMutex);
        m_scanResult = std::move(result);
        return;
    }

    std::vector<std::filesystem::path> pathsToProcess;
    try
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetsRoot))
        {
            if (entry.is_regular_file() && entry.path().extension() != ".meta")
            {
                pathsToProcess.push_back(entry.path());
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("AssetManager: Error while iterating asset directory: {}", e.what());
    }

    std::atomic<int> tasksRemaining = pathsToProcess.size();
    if (tasksRemaining == 0)
    {
        std::lock_guard<std::mutex> lock(m_scanResultMutex);
        m_scanResult = std::move(result);
        return;
    }

    for (const auto& path : pathsToProcess)
    {
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);
            m_taskQueue.emplace([this, path, &result, &resultMutex, &tasksRemaining]()
            {
                this->ProcessAssetFile(path, *result, resultMutex);
                tasksRemaining--;
            });
        }
    }
    m_taskCondition.notify_all();

    while (tasksRemaining > 0)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }

    {
        std::lock_guard<std::mutex> lock(m_scanResultMutex);
        m_scanResult = std::move(result);
    }
}

void EditorAssetManager::ProcessAssetFile(const std::filesystem::path& assetPath, ScanResult& result,
                                          std::mutex& resultMutex)
{
    if (!Path::Exists(assetPath.string()))
    {
        return;
    }
    IAssetImporter* importer = FindImporterFor(assetPath);
    if (!importer) return;

    std::filesystem::path metaPath = assetPath;
    metaPath += ".meta";

    AssetMetadata metadata;
    bool needsMetaWrite = false;

    if (std::filesystem::exists(metaPath))
    {
        metadata = importer->LoadMetadata(metaPath);

        try
        {
            auto assetWriteTime = std::filesystem::last_write_time(assetPath);
            auto metaWriteTime = std::filesystem::last_write_time(metaPath);

            if (assetWriteTime > metaWriteTime)
            {
                std::string currentHash = Utils::GetHashFromFile(assetPath.string());
                if (currentHash != metadata.fileHash)
                {
                    LogInfo("AssetManager: Re-importing modified asset: {}", assetPath.filename().string());
                    metadata = importer->Reimport(metadata);
                    needsMetaWrite = true;
                }
            }
        }
        catch (const std::exception& e)
        {
            LogError("AssetManager: Failed to check timestamp/hash for {}: {}", assetPath.string(), e.what());
            return;
        }
    }
    else
    {
        metadata = importer->Import(assetPath);
        needsMetaWrite = true;
    }

    if (needsMetaWrite)
    {
        importer->WriteMetadata(metadata);
    }

    std::filesystem::path relativePath = std::filesystem::relative(assetPath, m_assetsRoot);
    metadata.assetPath = relativePath;

    std::lock_guard<std::mutex> lock(resultMutex);
    result.guidToMeta[metadata.guid.ToString()] = metadata;
    result.pathToGuid[relativePath.generic_string()] = metadata.guid;
}

std::string EditorAssetManager::GetAssetName(const Guid& guid) const
{
    std::lock_guard<std::mutex> lock(m_dbMutex);
    auto it = m_guidToMeta.find(guid.ToString());
    if (it != m_guidToMeta.end())
    {
        return it->second.assetPath.stem().string();
    }
    return "[Unknown Asset]";
}

const AssetMetadata* EditorAssetManager::GetMetadata(const Guid& guid) const
{
    std::lock_guard<std::mutex> lock(m_dbMutex);
    auto it = m_guidToMeta.find(guid.ToString());
    if (it != m_guidToMeta.end())
    {
        return &it->second;
    }
    return nullptr;
}

void EditorAssetManager::InitialScan()
{
    ScanResult result;
    std::mutex resultMutex;

    std::vector<std::filesystem::path> pathsToProcess;
    try
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetsRoot))
        {
            if (entry.is_regular_file() && entry.path().extension() != ".meta")
            {
                pathsToProcess.push_back(entry.path());
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("AssetManager: 在初始目录扫描期间发生错误: {}", e.what());
        return;
    }

    if (pathsToProcess.empty())
    {
        LogInfo("AssetManager: 首次扫描完成，未发现资产。");
        return;
    }


    std::atomic<int> tasksRemaining = pathsToProcess.size();


    for (const auto& path : pathsToProcess)
    {
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);
            m_taskQueue.emplace([this, path, &result, &resultMutex, &tasksRemaining]()
            {
                this->ProcessAssetFile(path, result, resultMutex);
                tasksRemaining--;
            });
        }
    }
    m_taskCondition.notify_all();


    while (tasksRemaining > 0)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }


    m_guidToMeta = std::move(result.guidToMeta);
    m_pathToGuid = std::move(result.pathToGuid);

    LogInfo("AssetManager: 首次扫描结束，加载了 {} 个资产。", m_guidToMeta.size());
}

const AssetMetadata* EditorAssetManager::GetMetadata(const std::filesystem::path& assetPath) const
{
    std::lock_guard<std::mutex> lock(m_dbMutex);
    auto it = m_pathToGuid.find(assetPath.generic_string());
    if (it != m_pathToGuid.end())
    {
        auto metaIt = m_guidToMeta.find(it->second.ToString());
        if (metaIt != m_guidToMeta.end())
        {
            return &metaIt->second;
        }
    }
    return nullptr;
}

const std::unordered_map<std::string, AssetMetadata>& EditorAssetManager::GetAssetDatabase() const
{
    return m_guidToMeta;
}

const std::filesystem::path& EditorAssetManager::GetAssetsRootPath() const
{
    return m_assetsRoot;
}

void EditorAssetManager::ReImport(const AssetMetadata& metadata)
{
    IAssetImporter* importer = FindImporterFor(metadata.assetPath);
    if (!importer)
    {
        LogError("AssetManager: 无法重新导入 {}，没有找到合适的导入器。", metadata.assetPath.string());
        return;
    }

    std::filesystem::path fullAssetPath = m_assetsRoot / metadata.assetPath;
    if (!std::filesystem::exists(fullAssetPath))
    {
        LogError("AssetManager: 无法重新导入 {}，文件不存在。", fullAssetPath.string());
        return;
    }

    AssetMetadata newMetadata = importer->Reimport(metadata);
    importer->WriteMetadata(newMetadata);

    std::lock_guard<std::mutex> lock(m_dbMutex);
    m_guidToMeta[newMetadata.guid.ToString()] = newMetadata;
    m_pathToGuid[newMetadata.assetPath.generic_string()] = newMetadata.guid;
}

IAssetImporter* EditorAssetManager::FindImporterFor(const std::filesystem::path& filePath)
{
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    for (const auto& importer : m_importers)
    {
        const auto& supportedExtensions = importer->GetSupportedExtensions();
        if (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end())
        {
            return importer.get();
        }
    }
    return nullptr;
}

void EditorAssetManager::RegisterImporters()
{
    m_importers.clear();
    m_importers.push_back(std::make_unique<TextureImporter>());
    m_importers.push_back(std::make_unique<MaterialImporter>());
    m_importers.push_back(std::make_unique<SceneImporter>());
    m_importers.push_back(std::make_unique<PrefabImporter>());
    m_importers.push_back(std::make_unique<CSharpScriptImporter>());
    m_importers.push_back(std::make_unique<PhysicsMaterialImporter>());
    m_importers.push_back(std::make_unique<FontImporter>());
    m_importers.push_back(std::make_unique<AudioImporter>());
    m_importers.push_back(std::make_unique<AnimationClipImporter>());
    m_importers.push_back(std::make_unique<AnimationControllerImporter>());
    m_importers.push_back(std::make_unique<TileImporter>());
    m_importers.push_back(std::make_unique<TilesetImporter>());
    m_importers.push_back(std::make_unique<RuleTileImporter>());
}

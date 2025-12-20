#include "RuntimeAssetManager.h"
#include "AssetPacker.h"
#include "../Utils/Logger.h"
#include "../Renderer/Nut/ShaderRegistry.h"
#include "../Utils/PathUtils.h"
#include <chrono>
#include <thread>
#include <algorithm>

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
#include "Importers/BlueprintImporter.h"
#include "Importers/TileImporter.h"
#include "Importers/TilesetImporter.h"
#include "Importers/RuleTileImporter.h"
#include "Importers/ShaderImporter.h"

namespace
{
    std::string NormalizeAddress(const std::string& address)
    {
        std::string normalized = address;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    }
}

RuntimeAssetManager::RuntimeAssetManager(const std::filesystem::path& packageManifestPath)
    : m_packageManifestPath(packageManifestPath)
      , m_preloadRunning(false)
      , m_preloadComplete(false)
      , m_preloadedCount(0)
      , m_preWarmingRunning(false)
      , m_preWarmingComplete(false)
      , m_preWarmingTotal(0)
      , m_preWarmingLoaded(0)
{
    LogInfo("RuntimeAssetManager: 从包初始化 '{}'...", packageManifestPath.string());


    registerImporters();

    try
    {
        m_assetIndex = AssetPacker::LoadIndex(packageManifestPath);


        m_indexEntries.reserve(m_assetIndex.size());
        for (const auto& [guid, indexEntry] : m_assetIndex)
        {
            m_indexEntries.emplace_back(guid, indexEntry);
        }

        LogInfo("RuntimeAssetManager: 已加载 {} 个资产的索引", m_assetIndex.size());

        AddressablesIndex addressables{};
        if (AssetPacker::TryLoadAddressablesIndex(packageManifestPath, addressables))
        {
            m_addressToGuid = std::move(addressables.addressToGuid);
            m_groupToGuids = std::move(addressables.groupToGuids);
            m_hasAddressables = true;
        }
        else
        {
            LogWarn("RuntimeAssetManager: 未找到 Addressables 索引");
        }
    }
    catch (const std::exception& e)
    {
        m_assetIndex.clear();
        m_indexEntries.clear();
        LogError("RuntimeAssetManager: 初始化失败，原因: {}", e.what());
    }
}

RuntimeAssetManager::~RuntimeAssetManager()
{
    StopPreload();
    StopPreWarmingShader();
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
    std::string guidStr = guid.ToString();


    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto cacheIt = m_assetCache.find(guidStr);
        if (cacheIt != m_assetCache.end())
        {
            return &cacheIt->second;
        }
    }


    return lazyLoadMetadata(guidStr);
}

const AssetMetadata* RuntimeAssetManager::GetMetadata(const std::filesystem::path& assetPath) const
{
    std::string pathKey = assetPath.generic_string();

    std::lock_guard<std::mutex> lock(m_cacheMutex);


    auto pathIt = m_pathToGuid.find(pathKey);
    if (pathIt != m_pathToGuid.end())
    {
        auto cacheIt = m_assetCache.find(pathIt->second.ToString());
        if (cacheIt != m_assetCache.end())
        {
            return &cacheIt->second;
        }
    }

    LogWarn("RuntimeAssetManager: 运行时模式通过路径获取元数据需先调用LoadAsset或等待预加载完成");
    return nullptr;
}

const std::unordered_map<std::string, AssetMetadata>& RuntimeAssetManager::GetAssetDatabase() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);


    if (m_assetCache.size() < m_assetIndex.size())
    {
        LogInfo("RuntimeAssetManager: 按需加载完整资产数据库...");

        for (const auto& [guid, indexEntry] : m_assetIndex)
        {
            if (m_assetCache.find(guid) == m_assetCache.end())
            {
                try
                {
                    auto metadata = AssetPacker::LoadSingleAsset(m_packageManifestPath, indexEntry);
                    std::string pathKey = metadata.assetPath.generic_string();
                    m_pathToGuid[pathKey] = metadata.guid;
                    m_assetCache[guid] = std::move(metadata);
                }
                catch (const std::exception& e)
                {
                    LogError("RuntimeAssetManager: 加载资产失败 {}: {}", guid, e.what());
                }
            }
        }

        LogInfo("RuntimeAssetManager: 完整数据库加载完成，共 {} 个资产", m_assetCache.size());
    }

    return m_assetCache;
}

const std::filesystem::path& RuntimeAssetManager::GetAssetsRootPath() const
{
    return m_dummyPath;
}

Guid RuntimeAssetManager::GetGuidByAddress(const std::string& address) const
{
    if (!m_hasAddressables || address.empty())
    {
        return Guid::Invalid();
    }

    std::string key = NormalizeAddress(address);
    auto it = m_addressToGuid.find(key);
    if (it != m_addressToGuid.end())
    {
        return it->second;
    }
    return Guid::Invalid();
}

std::vector<Guid> RuntimeAssetManager::GetGuidsByGroup(const std::string& group) const
{
    if (!m_hasAddressables || group.empty())
    {
        return {};
    }

    auto it = m_groupToGuids.find(group);
    if (it != m_groupToGuids.end())
    {
        return it->second;
    }
    return {};
}

bool RuntimeAssetManager::StartPreload()
{
    if (m_preloadRunning.load() || m_preloadComplete.load())
    {
        LogWarn("RuntimeAssetManager: 预加载已在运行或已完成，无法重复启动");
        return false;
    }


    if (m_indexEntries.empty())
    {
        LogWarn("RuntimeAssetManager: 没有资产需要预加载");
        m_preloadComplete.store(true);
        return false;
    }


    m_preloadedCount.store(0);
    m_preloadComplete.store(false);
    m_preloadRunning.store(true);


    size_t hardwareConcurrency = std::thread::hardware_concurrency();
    size_t numThreads = hardwareConcurrency > 0 ? hardwareConcurrency : 4;


    numThreads = std::min(numThreads, m_indexEntries.size());

    LogInfo("RuntimeAssetManager: 启动后台预加载，使用 {} 个线程，共 {} 个资产",
            numThreads, m_indexEntries.size());


    m_preloadThreads.clear();
    m_preloadThreads.reserve(numThreads);

    for (size_t i = 0; i < numThreads; ++i)
    {
        m_preloadThreads.emplace_back(&RuntimeAssetManager::preloadWorker, this, i, numThreads);
    }

    return true;
}

void RuntimeAssetManager::StopPreload()
{
    m_preloadRunning.store(false);


    for (auto& thread : m_preloadThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    if (!m_preloadThreads.empty())
    {
        LogInfo("RuntimeAssetManager: 所有预加载线程已停止");
        m_preloadThreads.clear();
    }
}

std::pair<int, int> RuntimeAssetManager::GetPreloadProgress() const
{
    int total = static_cast<int>(m_indexEntries.size());
    int processed = m_preloadedCount.load();
    return std::make_pair(total, processed);
}

bool RuntimeAssetManager::IsPreloadComplete() const
{
    return m_preloadComplete.load();
}

bool RuntimeAssetManager::IsPreloadRunning() const
{
    return m_preloadRunning.load();
}

Guid RuntimeAssetManager::LoadAsset(const std::filesystem::path& assetPath)
{
    std::string pathKey = assetPath.generic_string();


    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto pathIt = m_pathToGuid.find(pathKey);
        if (pathIt != m_pathToGuid.end())
        {
            LogInfo("RuntimeAssetManager: 资产已在缓存中，路径: {}", assetPath.string());
            return pathIt->second;
        }
    }


    IAssetImporter* importer = findImporterFor(assetPath);
    if (!importer)
    {
        LogError("RuntimeAssetManager: 无法加载资产 {}，没有找到合适的导入器", assetPath.string());
        return Guid::Invalid();
    }


    std::filesystem::path fullAssetPath = assetPath;
    if (!std::filesystem::exists(fullAssetPath))
    {
        LogError("RuntimeAssetManager: 无法加载资产 {}，文件不存在", fullAssetPath.string());
        return Guid::Invalid();
    }


    try
    {
        AssetMetadata metadata = importer->Import(fullAssetPath);
        Guid resultGuid = metadata.guid;


        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_assetCache[metadata.guid.ToString()] = metadata;
            m_pathToGuid[pathKey] = metadata.guid;
        }

        LogInfo("RuntimeAssetManager: 成功加载资产，路径: {}, GUID: {}",
                assetPath.string(), resultGuid.ToString());

        return resultGuid;
    }
    catch (const std::exception& e)
    {
        LogError("RuntimeAssetManager: 加载资产失败 {}: {}", assetPath.string(), e.what());
        return Guid::Invalid();
    }
}

Guid RuntimeAssetManager::LoadAssetByAddress(const std::string& address)
{
    Guid guid = GetGuidByAddress(address);
    if (guid.Valid())
    {
        GetMetadata(guid);
    }
    return guid;
}

const AssetMetadata* RuntimeAssetManager::lazyLoadMetadata(const std::string& guid) const
{
    auto indexIt = m_assetIndex.find(guid);
    if (indexIt == m_assetIndex.end())
    {
        LogWarn("RuntimeAssetManager: 索引中未找到资产 {}", guid);
        return nullptr;
    }


    try
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);


        auto cacheIt = m_assetCache.find(guid);
        if (cacheIt != m_assetCache.end())
        {
            return &cacheIt->second;
        }

        LogInfo("RuntimeAssetManager: 懒加载资产 {}", guid);
        auto metadata = AssetPacker::LoadSingleAsset(m_packageManifestPath, indexIt->second);


        std::string pathKey = metadata.assetPath.generic_string();
        m_pathToGuid[pathKey] = metadata.guid;

        auto result = m_assetCache.emplace(guid, std::move(metadata));
        LogInfo("RuntimeAssetManager: 懒加载资产 {} 完成", guid);

        return &result.first->second;
    }
    catch (const std::exception& e)
    {
        LogError("RuntimeAssetManager: 懒加载资产失败 {}: {}", guid, e.what());
        return nullptr;
    }
}

void RuntimeAssetManager::preloadWorker(size_t threadIndex, size_t totalThreads)
{
    LogInfo("RuntimeAssetManager: 预加载线程 {} 启动", threadIndex);


    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int totalCount = static_cast<int>(m_indexEntries.size());
    int threadLoadedCount = 0;


    for (size_t i = threadIndex; i < m_indexEntries.size(); i += totalThreads)
    {
        if (!m_preloadRunning.load())
        {
            LogInfo("RuntimeAssetManager: 预加载线程 {} 被中断", threadIndex);
            return;
        }

        const auto& [guid, indexEntry] = m_indexEntries[i];


        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            if (m_assetCache.find(guid) != m_assetCache.end())
            {
                threadLoadedCount++;
                m_preloadedCount.fetch_add(1);
                continue;
            }
        }


        try
        {
            auto metadata = AssetPacker::LoadSingleAsset(m_packageManifestPath, indexEntry);

            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);


                std::string pathKey = metadata.assetPath.generic_string();
                m_pathToGuid[pathKey] = metadata.guid;

                m_assetCache[guid] = std::move(metadata);
            }

            threadLoadedCount++;
            int currentTotal = m_preloadedCount.fetch_add(1) + 1;


            if (threadIndex == 0 && currentTotal % std::max(1, totalCount / 10) == 0)
            {
                float progress = (static_cast<float>(currentTotal) / static_cast<float>(totalCount)) * 100.0f;
                LogInfo("RuntimeAssetManager: 预加载进度 {:.1f}% ({}/{})",
                        progress, currentTotal, totalCount);
            }


            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        catch (const std::exception& e)
        {
            LogError("RuntimeAssetManager: 线程 {} 预加载资产失败 {}: {}",
                     threadIndex, guid, e.what());
        }
    }

    LogInfo("RuntimeAssetManager: 预加载线程 {} 完成，共处理 {} 个资产",
            threadIndex, threadLoadedCount);


    int currentCount = m_preloadedCount.load();
    if (currentCount >= totalCount)
    {
        m_preloadComplete.store(true);
        m_preloadRunning.store(false);
        LogInfo("RuntimeAssetManager: 后台预加载完成，共加载 {} 个资产", currentCount);
    }
}

void RuntimeAssetManager::registerImporters()
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
    m_importers.push_back(std::make_unique<BlueprintImporter>());
    m_importers.push_back(std::make_unique<ShaderImporter>());

    LogInfo("RuntimeAssetManager: 已注册 {} 个资产导入器", m_importers.size());
}

IAssetImporter* RuntimeAssetManager::findImporterFor(const std::filesystem::path& filePath)
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

bool RuntimeAssetManager::StartPreWarmingShader()
{
    if (m_preWarmingRunning.load() || m_preWarmingComplete.load())
    {
        return false;
    }

    m_preWarmingRunning.store(true);
    m_preWarmingComplete.store(false);
    m_preWarmingTotal.store(0);
    m_preWarmingLoaded.store(0);

    m_preWarmingThread = std::thread(&RuntimeAssetManager::preWarmingWorker, this);

    LogInfo("RuntimeAssetManager: Shader pre-warming started");
    return true;
}

void RuntimeAssetManager::StopPreWarmingShader()
{
    if (m_preWarmingThread.joinable())
    {
        m_preWarmingRunning.store(false);
        m_preWarmingThread.join();
        LogInfo("RuntimeAssetManager: Shader pre-warming stopped");
    }
}

std::pair<int, int> RuntimeAssetManager::GetPreWarmingProgress() const
{
    int total = m_preWarmingTotal.load();
    int loaded = m_preWarmingLoaded.load();
    return std::make_pair(total, loaded);
}

bool RuntimeAssetManager::IsPreWarmingComplete() const
{
    return m_preWarmingComplete.load();
}

bool RuntimeAssetManager::IsPreWarmingRunning() const
{
    return m_preWarmingRunning.load();
}

void RuntimeAssetManager::preWarmingWorker()
{
    LogInfo("RuntimeAssetManager: Shader pre-warming worker started");
    
    auto& shaderRegistry = Nut::ShaderRegistry::GetInstance();
    
    
    std::filesystem::path shaderRegistryPath;
    
    
    std::vector<std::filesystem::path> possiblePaths = {
        PathUtils::GetExecutableDir() / "Resources" / "ShaderRegistry.yaml",
        PathUtils::GetExecutableDir() / ".." / "Resources" / "ShaderRegistry.yaml",
        "Resources/ShaderRegistry.yaml",
        "ShaderRegistry.yaml"
    };
    
    bool found = false;
    for (const auto& path : possiblePaths)
    {
        if (std::filesystem::exists(path))
        {
            shaderRegistryPath = path;
            found = true;
            LogInfo("RuntimeAssetManager: Found ShaderRegistry at: {}", path.string());
            break;
        }
    }
    
    if (!found)
    {
        LogWarn("RuntimeAssetManager: ShaderRegistry.yaml not found, skipping shader pre-warming");
        m_preWarmingRunning.store(false);
        m_preWarmingComplete.store(true);
        return;
    }
    
    
    if (!shaderRegistry.LoadFromFile(shaderRegistryPath.string()))
    {
        LogError("RuntimeAssetManager: Failed to load ShaderRegistry from: {}", shaderRegistryPath.string());
        m_preWarmingRunning.store(false);
        m_preWarmingComplete.store(true);
        return;
    }
    
    int total = static_cast<int>(shaderRegistry.GetShaderCount());
    m_preWarmingTotal.store(total);
    
    LogInfo("RuntimeAssetManager: Loaded {} shaders from registry", total);
    
    
    shaderRegistry.PreWarming();
    
    
    auto state = shaderRegistry.GetPreWarmingState();
    m_preWarmingTotal.store(state.total);
    m_preWarmingLoaded.store(state.loaded);
    
    m_preWarmingRunning.store(false);
    m_preWarmingComplete.store(true);
    
    LogInfo("RuntimeAssetManager: Shader pre-warming complete");
}

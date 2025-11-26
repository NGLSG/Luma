#include "ShaderCache.h"
#include "SHA256.h"
#include "Logger.h"
#include "Utils/PathUtils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Nut
{
    std::filesystem::path ShaderCache::s_cacheDirectory;
    ShaderCache::CacheStats ShaderCache::s_stats;
    bool ShaderCache::s_initialized = false;

    void ShaderCache::Initialize(const std::filesystem::path& cacheDir)
    {
        if (s_initialized)
        {
            LogWarn("ShaderCache::Initialize - Already initialized");
            return;
        }

        if (cacheDir.empty())
        {
            s_cacheDirectory = PathUtils::GetPersistentDataDir() / "ShadersCache";
        }
        else
        {
            s_cacheDirectory = cacheDir;
        }

        
        try
        {
            if (!std::filesystem::exists(s_cacheDirectory))
            {
                std::filesystem::create_directories(s_cacheDirectory);
                LogInfo("ShaderCache: Created cache directory: {}", s_cacheDirectory.string());
            }
            else
            {
                LogInfo("ShaderCache: Using cache directory: {}", s_cacheDirectory.string());
            }
        }
        catch (const std::exception& e)
        {
            LogError("ShaderCache::Initialize - Failed to create cache directory: {}", e.what());
            return;
        }

        s_initialized = true;
        s_stats = CacheStats();

        LogInfo("ShaderCache: Initialized successfully");
    }

    void ShaderCache::Shutdown()
    {
        if (!s_initialized)
        {
            return;
        }

        LogInfo("ShaderCache: Shutdown - Hit: {}, Miss: {}, Total files: {}, Total size: {} bytes",
                s_stats.hitCount, s_stats.missCount, s_stats.totalFiles, s_stats.totalSize);

        s_initialized = false;
    }

    std::optional<std::vector<uint8_t>> ShaderCache::LoadFromCache(const std::string& shaderCode)
    {
        if (!s_initialized)
        {
            LogWarn("ShaderCache::LoadFromCache - Cache not initialized");
            return std::nullopt;
        }

        std::string cacheKey = GenerateCacheKey(shaderCode);
        std::filesystem::path cachePath = GetCacheFilePath(cacheKey);

        if (!std::filesystem::exists(cachePath))
        {
            s_stats.missCount++;
            return std::nullopt;
        }

        try
        {
            std::ifstream file(cachePath, std::ios::binary);
            if (!file.is_open())
            {
                LogError("ShaderCache::LoadFromCache - Failed to open cache file: {}", cachePath.string());
                s_stats.missCount++;
                return std::nullopt;
            }

            
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            
            std::vector<uint8_t> blob(fileSize);
            file.read(reinterpret_cast<char*>(blob.data()), fileSize);

            if (!file)
            {
                LogError("ShaderCache::LoadFromCache - Failed to read cache file: {}", cachePath.string());
                s_stats.missCount++;
                return std::nullopt;
            }

            s_stats.hitCount++;
            LogInfo("ShaderCache: Cache hit - Key: {}, Size: {} bytes", cacheKey, fileSize);
            return blob;
        }
        catch (const std::exception& e)
        {
            LogError("ShaderCache::LoadFromCache - Exception: {}", e.what());
            s_stats.missCount++;
            return std::nullopt;
        }
    }

    void ShaderCache::SaveToCache(const std::string& shaderCode, const std::vector<uint8_t>& blob)
    {
        if (!s_initialized)
        {
            LogWarn("ShaderCache::SaveToCache - Cache not initialized");
            return;
        }

        if (blob.empty())
        {
            LogWarn("ShaderCache::SaveToCache - Empty blob, skipping");
            return;
        }

        std::string cacheKey = GenerateCacheKey(shaderCode);
        std::filesystem::path cachePath = GetCacheFilePath(cacheKey);

        try
        {
            std::ofstream file(cachePath, std::ios::binary);
            if (!file.is_open())
            {
                LogError("ShaderCache::SaveToCache - Failed to create cache file: {}", cachePath.string());
                return;
            }

            file.write(reinterpret_cast<const char*>(blob.data()), blob.size());

            if (!file)
            {
                LogError("ShaderCache::SaveToCache - Failed to write cache file: {}", cachePath.string());
                return;
            }

            s_stats.totalFiles++;
            s_stats.totalSize += blob.size();

            LogInfo("ShaderCache: Saved to cache - Key: {}, Size: {} bytes", cacheKey, blob.size());
        }
        catch (const std::exception& e)
        {
            LogError("ShaderCache::SaveToCache - Exception: {}", e.what());
        }
    }

    void ShaderCache::ClearCache()
    {
        if (!s_initialized)
        {
            LogWarn("ShaderCache::ClearCache - Cache not initialized");
            return;
        }

        try
        {
            size_t removedCount = 0;
            for (const auto& entry : std::filesystem::directory_iterator(s_cacheDirectory))
            {
                if (entry.is_regular_file())
                {
                    std::filesystem::remove(entry.path());
                    removedCount++;
                }
            }

            s_stats.totalFiles = 0;
            s_stats.totalSize = 0;

            LogInfo("ShaderCache: Cleared cache - Removed {} files", removedCount);
        }
        catch (const std::exception& e)
        {
            LogError("ShaderCache::ClearCache - Exception: {}", e.what());
        }
    }

    std::filesystem::path ShaderCache::GetCacheDirectory()
    {
        return s_cacheDirectory;
    }

    ShaderCache::CacheStats ShaderCache::GetStats()
    {
        return s_stats;
    }

    std::string ShaderCache::GenerateCacheKey(const std::string& shaderCode)
    {
        
        return SHA256::Hash(shaderCode);
    }

    std::string ShaderCache::GenerateCacheKeyFromRaw(const void* key, size_t keySize)
    {
        
        return SHA256::Hash(static_cast<const uint8_t*>(key), keySize);
    }

    std::filesystem::path ShaderCache::GetCacheFilePath(const std::string& cacheKey)
    {
        return s_cacheDirectory / (cacheKey + ".cache");
    }

    std::optional<std::vector<uint8_t>> ShaderCache::LoadFromCacheRaw(const void* key, size_t keySize)
    {
        if (!s_initialized)
        {
            return std::nullopt;
        }

        std::string cacheKey = GenerateCacheKeyFromRaw(key, keySize);
        std::filesystem::path cachePath = GetCacheFilePath(cacheKey);

        if (!std::filesystem::exists(cachePath))
        {
            return std::nullopt;
        }

        try
        {
            std::ifstream file(cachePath, std::ios::binary);
            if (!file.is_open())
            {
                return std::nullopt;
            }

            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<uint8_t> blob(fileSize);
            file.read(reinterpret_cast<char*>(blob.data()), fileSize);

            if (!file)
            {
                return std::nullopt;
            }

            s_stats.hitCount++;
            return blob;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    void ShaderCache::SaveToCacheRaw(const void* key, size_t keySize, const std::vector<uint8_t>& blob)
    {
        if (!s_initialized || blob.empty())
        {
            return;
        }

        std::string cacheKey = GenerateCacheKeyFromRaw(key, keySize);
        std::filesystem::path cachePath = GetCacheFilePath(cacheKey);

        try
        {
            std::ofstream file(cachePath, std::ios::binary);
            if (!file.is_open())
            {
                return;
            }

            file.write(reinterpret_cast<const char*>(blob.data()), blob.size());

            if (file)
            {
                s_stats.totalFiles++;
                s_stats.totalSize += blob.size();
            }
        }
        catch (const std::exception& e)
        {
            LogError("ShaderCache::SaveToCacheRaw - Exception: {}", e.what());
        }
    }

    ShaderCache::LoadDataFunction ShaderCache::GetLoadDataFunction()
    {
        return [](const void* key, size_t keySize, void* value, size_t valueSize) -> size_t
        {
            auto blob = LoadFromCacheRaw(key, keySize);
            if (!blob.has_value())
            {
                return 0;
            }

            if (blob->size() > valueSize)
            {
                LogWarn("ShaderCache: Cached blob size ({}) exceeds buffer size ({})", 
                        blob->size(), valueSize);
                return 0;
            }

            std::memcpy(value, blob->data(), blob->size());
            LogInfo("ShaderCache: Dawn blob cache hit - Size: {} bytes", blob->size());
            return blob->size();
        };
    }

    ShaderCache::StoreDataFunction ShaderCache::GetStoreDataFunction()
    {
        return [](const void* key, size_t keySize, const void* value, size_t valueSize)
        {
            std::vector<uint8_t> blob(static_cast<const uint8_t*>(value),
                                      static_cast<const uint8_t*>(value) + valueSize);
            SaveToCacheRaw(key, keySize, blob);
        };
    }
}

#ifndef SHADER_CACHE_H
#define SHADER_CACHE_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <functional>
#include "dawn/webgpu_cpp.h"

namespace Nut
{
    /**
     * @brief Shader 缓存管理器，使用 Dawn Blob Cache 功能
     * 
     * 将编译后的 shader 缓存到本地磁盘，减少重复编译开销
     * 使用 SHA256 生成缓存 key，支持完整的 Dawn Blob Cache 集成
     */
    class ShaderCache
    {
    public:
        /**
         * @brief Dawn Blob Cache 回调函数类型
         */
        using LoadDataFunction = std::function<size_t(const void* key, size_t keySize, 
                                                       void* value, size_t valueSize)>;
        using StoreDataFunction = std::function<void(const void* key, size_t keySize,
                                                      const void* value, size_t valueSize)>;
    
    public:
        /**
         * @brief 初始化 shader 缓存系统
         * @param cacheDir 缓存目录路径，默认使用 GetPersistentDataDir()/ShadersCache
         */
        static void Initialize(const std::filesystem::path& cacheDir = {});

        /**
         * @brief 清理缓存系统
         */
        static void Shutdown();

        /**
         * @brief 从缓存加载 shader blob（使用 shader 代码作为 key）
         * @param shaderCode shader 源代码（用于生成唯一 key）
         * @return 如果缓存存在，返回 blob 数据；否则返回 nullopt
         */
        static std::optional<std::vector<uint8_t>> LoadFromCache(const std::string& shaderCode);

        /**
         * @brief 保存 shader blob 到缓存（使用 shader 代码作为 key）
         * @param shaderCode shader 源代码（用于生成唯一 key）
         * @param blob shader 编译后的 blob 数据
         */
        static void SaveToCache(const std::string& shaderCode, const std::vector<uint8_t>& blob);

        /**
         * @brief Dawn Blob Cache 加载回调（使用原始 key）
         * @param key 缓存 key（通常是 shader 代码的 hash）
         * @param keySize key 的大小
         * @return 如果缓存存在，返回 blob 数据；否则返回 nullopt
         */
        static std::optional<std::vector<uint8_t>> LoadFromCacheRaw(const void* key, size_t keySize);

        /**
         * @brief Dawn Blob Cache 保存回调（使用原始 key）
         * @param key 缓存 key（通常是 shader 代码的 hash）
         * @param keySize key 的大小
         * @param blob shader 编译后的 blob 数据
         */
        static void SaveToCacheRaw(const void* key, size_t keySize, const std::vector<uint8_t>& blob);

        /**
         * @brief 获取 Dawn Blob Cache 的加载回调函数
         */
        static LoadDataFunction GetLoadDataFunction();

        /**
         * @brief 获取 Dawn Blob Cache 的保存回调函数
         */
        static StoreDataFunction GetStoreDataFunction();

        /**
         * @brief 清空所有缓存
         */
        static void ClearCache();

        /**
         * @brief 获取缓存目录路径
         */
        static std::filesystem::path GetCacheDirectory();

        /**
         * @brief 获取缓存统计信息
         */
        struct CacheStats
        {
            size_t totalFiles = 0;
            size_t totalSize = 0;
            size_t hitCount = 0;
            size_t missCount = 0;
        };
        static CacheStats GetStats();

    private:
        /**
         * @brief 根据 shader 代码生成唯一的缓存 key（使用 SHA256）
         */
        static std::string GenerateCacheKey(const std::string& shaderCode);

        /**
         * @brief 根据原始 key 生成缓存 key（使用 SHA256）
         */
        static std::string GenerateCacheKeyFromRaw(const void* key, size_t keySize);

        /**
         * @brief 获取缓存文件路径
         */
        static std::filesystem::path GetCacheFilePath(const std::string& cacheKey);

        static std::filesystem::path s_cacheDirectory;
        static CacheStats s_stats;
        static bool s_initialized;
    };
}

#endif // SHADER_CACHE_H

#ifndef PATHUTILS_H
#define PATHUTILS_H

#include <filesystem>
#include <string>

/**
 * @brief 在跨平台环境下用于解析运行时目录的实用工具类。
 */
class PathUtils
{
public:
    /**
     * @brief 为当前进程初始化全局路径状态。
     *
     * @param appName 应用程序名称，用于创建用户数据文件夹。
     */
    static void Initialize(const std::string& appName);

#if defined(__ANDROID__)
    /**
     * @brief 注入 Android 包名，以便在本地代码中派生路径。
     *
     * @param packageName Android 应用包名。
     */
    static void InjectAndroidPackageName(const std::string& packageName);
#endif

    /**
     * @brief 获取 Android 内部数据目录。
     *
     * @return std::filesystem::path 返回 Android 内部数据目录路径。
     *         非 Android 平台则返回持久化数据目录。
     */
    static std::filesystem::path GetAndroidInternalDataDir()
    {
#if defined(__ANDROID__)
        return s_AndroidInternalDataDir;
#else
        return s_PersistentDataDir;
#endif
    }

    /**
     * @brief 获取 Android 外部数据目录。
     *
     * @return std::filesystem::path 返回 Android 外部数据目录路径。
     *         非 Android 平台则返回持久化数据目录。
     */
    static std::filesystem::path GetAndroidExternalDataDir()
    {
#if defined(__ANDROID__)
        return s_AndroidExternalDataDir;
#else
        return s_PersistentDataDir;
#endif
    }

    /**
     * @brief 获取可执行文件所在目录（在 Android 上为应用基目录）。
     *
     * @return std::filesystem::path 返回可执行文件或应用基目录路径。
     */
    static std::filesystem::path GetExecutableDir();

    /**
     * @brief 获取持久化用户数据目录（用于存档、配置等）。
     *        在 Android 上优先使用外部存储（若可用）。
     *
     * @return std::filesystem::path 返回持久化数据目录路径。
     */
    static std::filesystem::path GetPersistentDataDir();

    /**
     * @brief 获取缓存或临时数据目录。
     *
     * @return std::filesystem::path 返回缓存目录路径。
     */
    static std::filesystem::path GetCacheDir();

    /**
     * @brief 获取包含已发布内容的基础目录（如 Resources/GameData）。
     *
     * @return std::filesystem::path 返回内容资源目录路径。
     */
    static std::filesystem::path GetContentDir();

private:
    /**
     * @brief 解析可执行文件目录。
     *
     * @return std::filesystem::path 返回可执行文件所在目录路径。
     */
    static std::filesystem::path ResolveExecutableDir();

    /**
     * @brief 解析持久化数据目录。
     *
     * @return std::filesystem::path 返回持久化数据目录路径。
     */
    static std::filesystem::path ResolvePersistentDataDir();

    /**
     * @brief 解析缓存数据目录。
     *
     * @return std::filesystem::path 返回缓存目录路径。
     */
    static std::filesystem::path ResolveCacheDir();

    /**
     * @brief 确保指定目录存在，如不存在则创建。
     *
     * @param path 要检查或创建的目录路径。
     * @return std::filesystem::path 返回已确保存在的目录路径。
     */
    static std::filesystem::path EnsureDirectory(const std::filesystem::path& path);

private:
    static inline bool s_IsInitialized = false;                ///< 是否已初始化标志。
    static inline std::string s_AppName = "Luma Game";         ///< 应用名称。
    static inline std::filesystem::path s_ExecutableDir;       ///< 可执行文件目录。
    static inline std::filesystem::path s_PersistentDataDir;   ///< 持久化数据目录。

#if defined(__ANDROID__)
    static inline std::string s_AndroidPackageName;            ///< Android 包名。
    static inline std::filesystem::path s_AndroidInternalDataDir; ///< Android 内部数据目录。
    static inline std::filesystem::path s_AndroidExternalDataDir; ///< Android 外部数据目录。
#endif
};

#endif

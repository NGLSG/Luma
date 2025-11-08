#ifndef  PATHUTILS_H
#define  PATHUTILS_H

#include <filesystem>
#include <string>

/**
 * @brief Utility helpers for resolving runtime directories in a platform aware way.
 */
class PathUtils
{
public:
    /**
     * @brief Initialize global path state for the current process.
     * @param appName Application name used when creating user data folders.
     */
    static void Initialize(const std::string& appName);

#if defined(__ANDROID__)
    /**
     * @brief Inject Android package name so paths can be derived in native code.
     */
    static void InjectAndroidPackageName(const std::string& packageName);

    static std::filesystem::path GetAndroidInternalDataDir()
    {
        return s_AndroidInternalDataDir;
    }
    static std::filesystem::path GetAndroidExternalDataDir()
    {
        return s_AndroidExternalDataDir;
    }
#endif

    /**
     * @brief Executable directory (or base app directory on Android).
     */
    static std::filesystem::path GetExecutableDir();

    /**
     * @brief Directory for persistent user data (saves, configs, etc).
     *        Android prefers external storage when available.
     */
    static std::filesystem::path GetPersistentDataDir();

    /**
     * @brief Directory for cache/temp data.
     */
    static std::filesystem::path GetCacheDir();

    /**
     * @brief Base directory containing shipped content (Resources/GameData).
     */
    static std::filesystem::path GetContentDir();

private:
    static std::filesystem::path ResolveExecutableDir();
    static std::filesystem::path ResolvePersistentDataDir();
    static std::filesystem::path ResolveCacheDir();
    static std::filesystem::path EnsureDirectory(const std::filesystem::path& path);

private:
    static inline bool s_IsInitialized = false;
    static inline std::string s_AppName = "Luma Game";
    static inline std::filesystem::path s_ExecutableDir;
    static inline std::filesystem::path s_PersistentDataDir;

#if defined(__ANDROID__)
    static inline std::string s_AndroidPackageName;
    static inline std::filesystem::path s_AndroidInternalDataDir;
    static inline std::filesystem::path s_AndroidExternalDataDir;
#endif
};
#endif

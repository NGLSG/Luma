#include "PathUtils.h"

#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

namespace
{
    std::filesystem::path GetCurrentDirectory()
    {
        std::error_code ec;
        auto dir = std::filesystem::current_path(ec);
        if (ec)
        {
            return {};
        }
        return std::filesystem::weakly_canonical(dir, ec);
    }

#if defined(_WIN32)
    std::filesystem::path GetWindowsLocalAppData()
    {
        PWSTR widePath = nullptr;
        std::filesystem::path localAppData;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &widePath)))
        {
            localAppData = widePath;
            CoTaskMemFree(widePath);
        }

        if (!localAppData.empty())
        {
            return localAppData;
        }

        if (const char* envPath = std::getenv("LOCALAPPDATA"))
        {
            return std::filesystem::path(envPath);
        }

        if (const char* home = std::getenv("USERPROFILE"))
        {
            return std::filesystem::path(home) / "AppData" / "Local";
        }

        return {};
    }
#endif

#if defined(__ANDROID__)
    std::filesystem::path BuildAndroidInternalDir(const std::string& packageName)
    {
        if (packageName.empty())
        {
            return {};
        }
        return std::filesystem::path("/data/data") / packageName / "files";
    }

    std::filesystem::path BuildAndroidExternalDir(const std::string& packageName)
    {
        if (packageName.empty())
        {
            return {};
        }

        const char* externalEnv = std::getenv("EXTERNAL_STORAGE");
        std::filesystem::path base = (externalEnv && externalEnv[0] != '\0')
                                       ? std::filesystem::path(externalEnv)
                                       : std::filesystem::path("/storage/emulated/0");
        return base / "Android" / "data" / packageName / "files";
    }
#endif
}

void PathUtils::Initialize(const std::string& appName)
{
    if (!appName.empty())
    {
        s_AppName = appName;
    }

    if (!s_IsInitialized)
    {
        s_ExecutableDir = ResolveExecutableDir();
        s_IsInitialized = true;
    }

    s_PersistentDataDir = EnsureDirectory(ResolvePersistentDataDir());
}

#if defined(__ANDROID__)
void PathUtils::InjectAndroidPackageName(const std::string& packageName)
{
    s_AndroidPackageName = packageName;

    if (!packageName.empty())
    {
        s_AndroidInternalDataDir = BuildAndroidInternalDir(packageName);
        s_AndroidExternalDataDir = BuildAndroidExternalDir(packageName);
    }
    else
    {
        s_AndroidInternalDataDir.clear();
        s_AndroidExternalDataDir.clear();
    }
}
#endif

std::filesystem::path PathUtils::GetExecutableDir()
{
    if (s_ExecutableDir.empty())
    {
        s_ExecutableDir = ResolveExecutableDir();
    }
    return s_ExecutableDir;
}

std::filesystem::path PathUtils::GetPersistentDataDir()
{
    if (s_PersistentDataDir.empty())
    {
        s_PersistentDataDir = EnsureDirectory(ResolvePersistentDataDir());
    }
    return s_PersistentDataDir;
}

std::filesystem::path PathUtils::GetCacheDir()
{
    return EnsureDirectory(ResolveCacheDir());
}

std::filesystem::path PathUtils::GetContentDir()
{
    if (s_ExecutableDir.empty())
    {
        s_ExecutableDir = ResolveExecutableDir();
    }
    return s_ExecutableDir;
}

std::filesystem::path PathUtils::ResolveExecutableDir()
{
#if defined(__ANDROID__)
    if (!s_AndroidInternalDataDir.empty())
    {
        return s_AndroidInternalDataDir;
    }
#endif

    auto dir = GetCurrentDirectory();
    if (!dir.empty())
    {
        return dir;
    }

    return std::filesystem::path(".");
}

std::filesystem::path PathUtils::ResolvePersistentDataDir()
{
#if defined(__ANDROID__)
    if (!s_AndroidExternalDataDir.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(s_AndroidExternalDataDir, ec);
        if (!ec)
        {
            return s_AndroidExternalDataDir;
        }
    }
    if (!s_AndroidInternalDataDir.empty())
    {
        return s_AndroidInternalDataDir;
    }
#endif

#if defined(_WIN32)
    auto base = GetWindowsLocalAppData();
    if (base.empty())
    {
        base = GetExecutableDir();
    }
    return base / s_AppName;
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home ? std::filesystem::path(home) / "Library" / "Application Support" : GetExecutableDir();
    return base / s_AppName;
#elif defined(__linux__)
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && *xdgData)
    {
        return std::filesystem::path(xdgData) / s_AppName;
    }
    if (const char* home = std::getenv("HOME"))
    {
        return std::filesystem::path(home) / ".local" / "share" / s_AppName;
    }
    return GetExecutableDir() / "PersistentData";
#else
    return GetExecutableDir() / "PersistentData";
#endif
}

std::filesystem::path PathUtils::ResolveCacheDir()
{
#if defined(__ANDROID__)
    if (!s_AndroidInternalDataDir.empty())
    {
        return s_AndroidInternalDataDir / "cache";
    }
#endif
    return GetPersistentDataDir() / "Cache";
}

std::filesystem::path PathUtils::EnsureDirectory(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return path;
    }
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return path;
}

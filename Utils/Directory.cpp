#include "Directory.h"

namespace Directory
{
    bool Exists(const std::string& path)
    {
        return std::filesystem::exists(path);
    }

    bool Create(const std::string& path, bool recursive)
    {
        if (Exists(path)) return true;
        if (recursive)
        {
            return std::filesystem::create_directories(path);
        }
        else
        {
            return std::filesystem::create_directory(path);
        }
    }

    bool Remove(const std::string& path, bool recursive)
    {
        if (!Exists(path)) return true;
        if (recursive)
        {
            return std::filesystem::remove_all(path) > 0;
        }
        else
        {
            return std::filesystem::remove(path);
        }
    }

    bool Rename(const std::string& oldPath, const std::string& newPath)
    {
        if (!Exists(oldPath)) return false;
        if (Exists(newPath)) return false;
        std::filesystem::rename(oldPath, newPath);
        return true;
    }

    bool Copy(const std::string& source, const std::string& destination, bool overwrite)
    {
        if (!Exists(source)) return false;
        if (Exists(destination) && !overwrite) return false;
        try
        {
            std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive |
                                  (overwrite
                                       ? std::filesystem::copy_options::overwrite_existing
                                       : std::filesystem::copy_options::none));
            return true;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            return false;
        }
    }

    bool IsDirectory(const std::string& path)
    {
        return std::filesystem::is_directory(path);
    }

    std::string GetCurrentPath()
    {
        return std::filesystem::current_path().string();
    }

    std::string GetCurrentExecutablePath()
    {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path().string();
#else
        char buffer[1024];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (count != -1)
        {
            buffer[count] = '\0';
            return std::filesystem::path(buffer).parent_path().string();
        }
        return {};
#endif
    }

    std::wstring GetCurrentExecutablePathW()
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH] = {0};

        GetModuleFileNameW(nullptr, buffer, MAX_PATH);


        return std::wstring(buffer);
#else
        char buffer[1024];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (count != -1)
        {
            buffer[count] = '\0';

            return std::filesystem::path(buffer).wstring();
        }
        return {};
#endif
    }

    std::string GetAbsolutePath(const std::string& relativePath)
    {
        if (relativePath.empty())
        {
            return GetCurrentPath();
        }
        if (std::filesystem::path(relativePath).is_absolute())
        {
            return relativePath;
        }
        return std::filesystem::absolute(relativePath).string();
    }
}

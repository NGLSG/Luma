#include "Path.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>


namespace fs = std::filesystem;

namespace Path
{
    std::string Combine(const std::string& path1, const std::string& path2)
    {
        fs::path p1(path1);
        fs::path p2(path2);
        return (p1 / p2).string();
    }

    std::string GetFileNameWithoutExtension(const std::string_view& filePath)
    {
        return fs::path(filePath).stem().string();
    }

    std::string GetFileExtension(const std::string_view& filePath)
    {
        return fs::path(filePath).extension().string();
    }

    std::string GetDirectoryName(const std::string_view& filePath)
    {
        fs::path p(filePath);

        if (p.has_parent_path())
        {
            return p.parent_path().string();
        }
        return "";
    }

    std::string GetFullPath(const std::string_view& relativePath)
    {
        try
        {
            return fs::absolute(fs::path(relativePath)).string();
        }
        catch (const fs::filesystem_error&)
        {
            return "";
        }
    }

    std::string GetRelativePath(const std::string_view& fullPath, const std::string_view& basePath)
    {
        try
        {
            return fs::relative(fs::path(fullPath), fs::path(basePath)).string();
        }
        catch (const fs::filesystem_error&)
        {
            return "";
        }
    }

    std::string GetRelativePath(const std::filesystem::path& fullPath, const std::filesystem::path& basePath)
    {
        try
        {
            return fs::relative(fullPath, basePath).string();
        }
        catch (const fs::filesystem_error&)
        {
            return "";
        }
    }

    bool Exists(const std::string_view& path)
    {
        try
        {
            return fs::exists(fs::path(path));
        }
        catch (const fs::filesystem_error&)
        {
            return false;
        }
    }

    std::vector<unsigned char> ReadAllBytes(const std::string& filePath)
    {
        if (!fs::exists(filePath))
        {
            throw std::runtime_error("File does not exist: " + filePath);
        }


        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filePath);
        }


        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);


        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        {
            throw std::runtime_error("Failed to read file: " + filePath);
        }

        return buffer;
    }

    bool WriteAllBytes(const std::string& filePath, const std::vector<unsigned char>& data)
    {
        fs::path path(filePath);
        if (path.has_parent_path() && !fs::exists(path.parent_path()))
        {
            std::error_code ec;
            if (!fs::create_directories(path.parent_path(), ec))
            {
                return false;
            }
        }


        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }


        if (!file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size())))
        {
            return false;
        }

        return true;
    }

    bool WriteFile(const std::string& filePath, const std::string& content, bool append)
    {
        fs::path path(filePath);
        if (path.has_parent_path() && !fs::exists(path.parent_path()))
        {
            std::error_code ec;
            if (!fs::create_directories(path.parent_path(), ec))
            {
                return false;
            }
        }


        std::ofstream file(filePath, append ? std::ios::binary | std::ios::app : std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }


        file << content;
        return true;
    }
}

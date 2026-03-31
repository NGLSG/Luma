#ifndef ASSET_BUNDLE_H
#define ASSET_BUNDLE_H

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_map>

constexpr uint32_t BUNDLE_MAGIC = 0x42414D4C; // "LMAB"
constexpr uint32_t BUNDLE_VERSION = 1;

struct BundleEntryHeader
{
    char path[256]{};
    uint64_t offset = 0;
    uint64_t compressedSize = 0;
    uint64_t originalSize = 0;
    uint32_t crc32 = 0;
};

struct BundleFileHeader
{
    uint32_t magic = BUNDLE_MAGIC;
    uint32_t version = BUNDLE_VERSION;
    uint32_t entryCount = 0;
};

class AssetBundleWriter
{
public:
    void AddFile(const std::filesystem::path& relativePath, const std::vector<uint8_t>& data);
    void AddDirectory(const std::filesystem::path& rootDir);
    bool Write(const std::filesystem::path& outputPath);

private:
    struct PendingEntry
    {
        std::string path;
        std::vector<uint8_t> data;
    };
    std::vector<PendingEntry> m_entries;

    static uint32_t ComputeCRC32(const uint8_t* data, size_t len);
};

class AssetBundleReader
{
public:
    bool Open(const std::filesystem::path& bundlePath);
    void Close();
    bool Contains(const std::string& path) const;
    std::vector<uint8_t> ReadFile(const std::string& path) const;
    std::vector<std::string> ListFiles() const;

private:
    mutable std::ifstream m_file;
    std::unordered_map<std::string, BundleEntryHeader> m_entries;
    std::filesystem::path m_path;
};

#endif

/**
 * @file UNI_UTILS_H
 * @brief 提供通用工具函数，包括文件操作、目录操作和 YAML 数据处理。
 */
#ifndef UNI_UTILS_H
#define UNI_UTILS_H
#include <fstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <optional>
#include "Logger.h"

/**
 * @brief 文件工具类，提供静态方法用于文件相关的操作。
 */
class UFile
{
public:
    /**
     * @brief 检查文件是否存在。
     * @param filename 要检查的文件名。
     * @return 如果文件存在则返回 true，否则返回 false。
     */
    static bool Exists(const std::string& filename);

    /**
     * @brief 获取指定目录下的所有文件。
     * @param folder 要扫描的目录路径。
     * @return 包含所有文件名的字符串向量。
     */
    static std::vector<std::string> GetFilesInDirectory(const std::string& folder);

    /**
     * @brief 将路径转换为当前平台的标准路径格式。
     * @param path 原始路径字符串。
     * @return 转换后的平台特定路径字符串。
     */
    static std::string PlatformPath(std::string path);

    /**
     * @brief 检查字符串是否以指定的后缀结束。
     * @param str 要检查的字符串。
     * @param suffix 要匹配的后缀。
     * @return 如果字符串以后缀结束则返回 true，否则返回 false。
     */
    static bool EndsWith(const std::string& str, const std::string& suffix);

    /**
     * @brief 复制文件。
     * @param src 源文件路径。
     * @param dst 目标文件路径。
     * @return 如果复制成功则返回 true，否则返回 false。
     */
    static bool UCopyFile(const std::string& src, const std::string& dst);
};

/**
 * @brief 目录工具类，提供静态方法用于目录相关的操作。
 */
class UDirectory
{
public:
    /**
     * @brief 创建目录。
     * @param dirname 要创建的目录名。
     * @return 如果创建成功则返回 true，否则返回 false。
     */
    static bool Create(const std::string& dirname);

    /**
     * @brief 如果目录不存在则创建它。
     * @param dir 要检查并创建的目录路径。
     * @return 如果目录存在或创建成功则返回 true，否则返回 false。
     */
    static bool CreateDirIfNotExists(const std::string& dir);

    /**
     * @brief 检查目录是否存在。
     * @param dirname 要检查的目录名。
     * @return 如果目录存在则返回 true，否则返回 false。
     */
    static bool Exists(const std::string& dirname);

    /**
     * @brief 删除目录。
     * @param dir 要删除的目录路径。
     * @return 如果删除成功则返回 true，否则返回 false。
     */
    static bool Remove(const std::string& dir);

    /**
     * @brief 获取指定目录下的所有子文件。
     * @param dirPath 要扫描的目录路径。
     * @return 包含所有子文件名的字符串向量。
     */
    static std::vector<std::string> GetSubFiles(const std::string& dirPath);

    /**
     * @brief 获取指定目录下的所有子目录。
     * @param dirPath 要扫描的目录路径。
     * @return 包含所有子目录名的字符串向量。
     */
    static std::vector<std::string> GetSubDirectories(const std::string& dirPath);
};

/**
 * @brief 获取当前系统时间戳。
 * @return 当前时间戳，通常是自纪元以来的纳秒数或类似单位。
 */
static size_t GetCurrentTimestamp()
{
    return static_cast<size_t>(std::chrono::system_clock::now().time_since_epoch().count());
}

/**
 * @brief 将 YAML 节点保存到文件。
 * @param filename 要保存到的文件名。
 * @param node 要保存的 YAML 节点。
 */
static void SaveYaml(const std::string& filename, const YAML::Node& node)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("LogError: Failed to open file " + filename);
        return;
    }
    file << node;
    file.close();
}

/**
 * @brief 从 YAML 文件加载数据并转换为指定类型。
 * @tparam T 要转换的目标类型。
 * @param file 要加载的 YAML 文件路径。
 * @return 一个包含加载数据的 std::optional 对象，如果加载失败则为空。
 */
template <typename T>
static std::optional<T> LoadYaml(const std::string& file)
{
    try
    {
        YAML::Node node = YAML::LoadFile(file);

        return node.as<T>();
    }
    catch (const std::exception& e)
    {
        LogError("{0}", e.what());
        return std::optional<T>();
    }
}

/**
 * @brief 将给定值转换为 YAML 节点。
 * @tparam T 要转换的值的类型。
 * @param value 要转换的值。
 * @return 表示给定值的 YAML 节点。
 */
template <typename T>
static YAML::Node toYaml(const T& value)
{
    YAML::Node node;
    node = value;
    return node;
}

#endif
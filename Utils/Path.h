#pragma once
#include <string>

/**
 * @brief Path 命名空间，提供常用的文件路径操作工具函数。
 */
namespace Path
{
    /**
     * @brief 合并两个路径，生成新的路径。
     * @param path1 第一个路径
     * @param path2 第二个路径
     * @return 合并后的路径字符串
     */
    std::string Combine(const std::string& path1, const std::string& path2);

    /**
     * @brief 获取文件名（不含扩展名）。
     * @param filePath 文件路径
     * @return 文件名（无扩展名）
     */
    std::string GetFileNameWithoutExtension(const std::string_view& filePath);

    /**
     * @brief 获取文件扩展名。
     * @param filePath 文件路径
     * @return 文件扩展名（包含点号）
     */
    std::string GetFileExtension(const std::string_view& filePath);

    /**
     * @brief 获取文件所在目录路径。
     * @param filePath 文件路径
     * @return 目录路径
     */
    std::string GetDirectoryName(const std::string_view& filePath);

    /**
     * @brief 获取绝对路径。
     * @param relativePath 相对路径
     * @return 绝对路径字符串
     */
    std::string GetFullPath(const std::string_view& relativePath);

    /**
     * @brief 获取相对于基路径的相对路径。
     * @param fullPath 绝对路径
     * @param basePath 基准路径
     * @return 相对路径字符串
     */
    std::string GetRelativePath(const std::string_view& fullPath, const std::string_view& basePath);

    /**
     * @brief 获取相对于基路径的相对路径（重载，支持 std::filesystem::path）。
     * @param fullPath 绝对路径
     * @param basePath 基准路径
     * @return 相对路径字符串
     */
    std::string GetRelativePath(const std::filesystem::path& fullPath, const std::filesystem::path& basePath);

    /**
     * @brief 判断指定路径是否存在。
     * @param path 路径
     * @return 存在返回 true，否则返回 false
     */
    bool Exists(const std::string_view& path);

    /**
     * @brief 读取文件所有字节内容。
     * @param string 文件路径
     * @return 文件内容字节数组
     */
    std::vector<unsigned char> ReadAllBytes(const std::string& string);

    /**
     * @brief 将字节数据写入文件。
     * @param string 文件路径
     * @param data 要写入的数据
     * @return 写入成功返回 true，否则返回 false
     */
    bool WriteAllBytes(const std::string& string, const std::vector<unsigned char>& data);

    /**
     * @brief 写入字符串内容到文件，可选追加模式。
     * @param filePath 文件路径
     * @param content 要写入的内容
     * @param append 是否追加到文件末尾，默认为 false
     * @return 写入成功返回 true，否则返回 false
     */
    bool WriteFile(const std::string& filePath, const std::string& content, bool append = false);
}
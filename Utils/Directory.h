#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <filesystem>

namespace Directory
{
    /**
     * @brief 检查指定路径是否存在。
     * @param path 要检查的路径
     * @return 如果路径存在返回 true，否则返回 false
     */
    bool Exists(const std::string& path);

    /**
     * @brief 创建指定路径的目录。
     * @param path 要创建的目录路径
     * @param recursive 是否递归创建父目录，默认为 false
     * @return 如果创建成功返回 true，否则返回 false
     */
    bool Create(const std::string& path, bool recursive = false);

    /**
     * @brief 删除指定路径的目录或文件。
     * @param path 要删除的路径
     * @param recursive 是否递归删除子目录，默认为 false
     * @return 如果删除成功返回 true，否则返回 false
     */
    bool Remove(const std::string& path, bool recursive = false);

    /**
     * @brief 重命名指定路径。
     * @param oldPath 原路径
     * @param newPath 新路径
     * @return 如果重命名成功返回 true，否则返回 false
     */
    bool Rename(const std::string& oldPath, const std::string& newPath);

    /**
     * @brief 复制文件或目录到目标路径。
     * @param source 源路径
     * @param destination 目标路径
     * @param overwrite 是否覆盖目标文件，默认为 false
     * @return 如果复制成功返回 true，否则返回 false
     */
    bool Copy(const std::string& source, const std::string& destination, bool overwrite = false);

    /**
     * @brief 检查指定路径是否为目录。
     * @param path 要检查的路径
     * @return 如果是目录返回 true，否则返回 false
     */
    bool IsDirectory(const std::string& path);

    /**
     * @brief 获取当前工作目录的路径。
     * @return 当前工作目录的路径字符串
     */
    std::string GetCurrentPath();

    /**
     * @brief 获取当前可执行文件的路径。
     * @return 当前可执行文件的路径字符串
     */
    std::string GetCurrentExecutablePath();

    /**
     * @brief 获取当前可执行文件的宽字符路径。
     * @return 当前可执行文件的宽字符路径字符串
     */
    std::wstring GetCurrentExecutablePathW();

    /**
     * @brief 将相对路径转换为绝对路径。
     * @param relativePath 相对路径
     * @return 转换后的绝对路径字符串
     */
    std::string GetAbsolutePath(const std::string& relativePath);
}

#endif

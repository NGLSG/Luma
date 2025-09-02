#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <openssl/sha.h>

/**
 * @brief Utils 工具类，提供文件、字符串哈希及系统相关操作的静态方法。
 *
 * 该类为纯静态工具类，禁止实例化和拷贝。
 */
class Utils
{
private:
public:
    Utils() = delete; ///< 禁止实例化
    Utils(const Utils&) = delete; ///< 禁止拷贝构造
    Utils& operator=(const Utils&) = delete; ///< 禁止赋值

    /**
     * @brief 在文件资源管理器中打开指定路径。
     * @param path 需要打开的文件或文件夹路径
     */
    static void OpenFileExplorerAt(std::filesystem::path::iterator::reference path);

    /**
     * @brief 在默认浏览器中打开指定 URL。
     * @param url 需要打开的网址
     */
    static void OpenBrowserAt(const std::string& url);

    /**
     * @brief 获取指定文件内容的 SHA 哈希值。
     * @param filePath 文件路径
     * @return 文件内容的哈希字符串
     */
    static std::string GetHashFromFile(const std::string& filePath);

    /**
     * @brief 获取输入字符串的 SHA 哈希值。
     * @param input 输入字符串
     * @return 字符串的哈希值
     */
    static std::string GetHashFromString(const std::string& input);
};

#endif
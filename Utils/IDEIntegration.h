#ifndef IDEINTEGRATION_H
#define IDEINTEGRATION_H

#include <filesystem>
#include <string>

/**
 * @brief 枚举 IDE，表示支持的集成开发环境类型。
 */
enum class IDE
{
    Unknown,        ///< 未知 IDE
    VisualStudio,   ///< Microsoft Visual Studio
    Rider,          ///< JetBrains Rider
    VSCode          ///< Visual Studio Code
};

/**
 * @brief IDEIntegration 类，提供与不同 IDE 集成的相关功能。
 */
class IDEIntegration
{
public:
    /**
     * @brief 检测当前系统已安装的 IDE。
     * @return 检测到的 IDE 类型（IDE 枚举值）
     */
    static IDE DetectInstalledIDE();

    /**
     * @brief 在指定 IDE 中打开解决方案和文件。
     * @param ide 目标 IDE 类型
     * @param solutionPath 解决方案路径
     * @param filePath 要打开的文件路径
     * @return 打开成功返回 true，否则返回 false
     */
    static bool Open(IDE ide, const std::filesystem::path& solutionPath, const std::filesystem::path& filePath);

private:
#ifdef _WIN32

    /**
     * @brief 查找 VSCode 的安装路径（仅限 Windows）。
     * @return VSCode 的安装路径（宽字符串）
     */
    static std::wstring FindVSCodePath();

    /**
     * @brief 查找 Visual Studio 的安装路径（仅限 Windows）。
     * @return Visual Studio 的安装路径（宽字符串）
     */
    static std::wstring FindVisualStudioPath();

    /**
     * @brief 查找 Rider 的安装路径（仅限 Windows）。
     * @return Rider 的安装路径（宽字符串）
     */
    static std::wstring FindRiderPath();

    /**
     * @brief 从注册表获取指定键值的路径（仅限 Windows）。
     * @param keyPath 注册表键路径
     * @param valueName 注册表值名称
     * @return 注册表中对应的路径（宽字符串）
     */
    static std::wstring GetPathFromRegistry(const std::wstring& keyPath, const std::wstring& valueName);
#endif
};

#endif
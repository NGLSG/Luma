#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>
#include <unordered_set>

#include <include/effects/SkRuntimeEffect.h>


/**
 * @brief 用于计算 std::filesystem::path 哈希值的结构体。
 *
 * 允许 std::filesystem::path 在基于哈希的容器中使用。
 */
struct PathHash
{
    /**
     * @brief 计算给定文件路径的哈希值。
     * @param p 要计算哈希值的文件路径。
     * @return 文件路径的哈希值。
     */
    std::size_t operator()(const std::filesystem::path& p) const
    {
        return std::filesystem::hash_value(p);
    }
};


/**
 * @brief 表示一个着色器对象，封装了 SkRuntimeEffect。
 *
 * 提供从代码字符串或文件路径创建着色器的方法。
 */
class LUMA_API Shader
{
public:
    /**
     * @brief 默认构造函数，创建一个无效的着色器对象。
     */
    Shader();

    /**
     * @brief 从顶点着色器和片段着色器代码创建着色器。
     * @param vertexSksl 顶点着色器代码字符串。
     * @param fragmentSksl 片段着色器代码字符串。
     * @return 创建的 Shader 对象。
     */
    static Shader FromCode(const std::string& vertexSksl, const std::string& fragmentSksl);

    /**
     * @brief 从顶点着色器和片段着色器文件创建着色器。
     * @param vertexPath 顶点着色器文件路径。
     * @param fragmentPath 片段着色器文件路径。
     * @return 创建的 Shader 对象。
     */
    static Shader FromFile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);

    /**
     * @brief 从单个 SKSL 代码字符串创建着色器（通常是片段着色器）。
     * @deprecated 请使用 Shader::FromFragmentCode 或管道版本 Shader::FromCode(vs, fs) 代替。
     * @param skslCode SKSL 代码字符串。
     * @return 创建的 Shader 对象。
     */
    [[deprecated("Use Shader::FromFragmentCode or the pipeline version Shader::FromCode(vs, fs) instead.")]]
    static Shader FromCode(const std::string& skslCode);

    /**
     * @brief 从单个 SKSL 文件创建着色器（通常是片段着色器）。
     * @deprecated 请使用 Shader::FromFragmentFile 或管道版本 Shader::FromFile(vs, fs) 代替。
     * @param filePath SKSL 文件路径。
     * @return 创建的 Shader 对象。
     */
    [[deprecated("Use Shader::FromFragmentFile or the pipeline version Shader::FromFile(vs, fs) instead.")]]
    static Shader FromFile(const std::filesystem::path& filePath);

    /**
     * @brief 从片段着色器代码字符串创建着色器。
     * @param fragmentSksl 片段着色器代码字符串。
     * @return 创建的 Shader 对象。
     */
    static Shader FromFragmentCode(const std::string& fragmentSksl);

    /**
     * @brief 从片段着色器文件创建着色器。
     * @param fragmentPath 片段着色器文件路径。
     * @return 创建的 Shader 对象。
     */
    static Shader FromFragmentFile(const std::filesystem::path& fragmentPath);

    /**
     * @brief 检查着色器对象是否有效（是否包含一个 SkRuntimeEffect）。
     * @return 如果着色器有效则返回 true，否则返回 false。
     */
    bool IsValid() const;

    /**
     * @brief 隐式转换为 SkRuntimeEffect 智能指针。
     * @return 封装的 SkRuntimeEffect 智能指针。
     */
    operator sk_sp<SkRuntimeEffect>() const;

private:
    /**
     * @brief 私有构造函数，用于从 SkRuntimeEffect 智能指针创建 Shader 对象。
     * @param runtimeEffect 要封装的 SkRuntimeEffect 智能指针。
     */
    explicit Shader(sk_sp<SkRuntimeEffect> runtimeEffect);

    /**
     * @brief 预处理 SKSL 文件，处理包含指令。
     * @param filePath 要预处理的 SKSL 文件路径。
     * @param visited 用于跟踪已访问文件的集合，防止循环包含。
     * @return 预处理后的 SKSL 代码字符串。
     */
    static std::string PreprocessSksl(const std::filesystem::path& filePath,
                                      std::unordered_set<std::filesystem::path, PathHash>& visited);

    sk_sp<SkRuntimeEffect> effect = nullptr; ///< 封装的 SkRuntimeEffect 智能指针。
};
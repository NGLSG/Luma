#ifndef NOAI_SHADER_H
#define NOAI_SHADER_H
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#include "dawn/webgpu_cpp.h"
#include "ShaderModuleRegistry.h"

namespace Nut
{
    class NutContext;

    enum class BindingType:uint32_t
    {
        UniformBuffer = 0,
        StorageBuffer = 1,
        Texture = 2,
        Sampler = 3,
    };

    struct ShaderBindingInfo
    {
        size_t groupIndex;
        size_t location;
        BindingType type;
        std::string name;
    };

    class LUMA_API ShaderModule
    {
        wgpu::ShaderModule shaderModule;
        std::unordered_map<std::string, ShaderBindingInfo> bindings;

    public:
        ShaderModule(const std::string& shaderCode, const std::shared_ptr<NutContext>& ctx);

        ShaderModule();

        wgpu::ShaderModule& Get();
        operator bool() const;

        ShaderBindingInfo GetBindingInfo(const std::string& name);
        bool GetBindingInfo(const std::string& name, ShaderBindingInfo& info);

        void ForeachBinding(const std::function<void(const ShaderBindingInfo&)>& callback) const;
    };

    class ShaderManager
    {
        inline static std::unordered_map<std::string, std::shared_ptr<ShaderModule>> shaderModules;

    public:
        /**
         * @brief 从文件加载shader（已弃用，保留兼容性）
         */
        static ShaderModule& GetFromFile(const std::string& file, const std::shared_ptr<NutContext>& ctx);

        /**
         * @brief 从字符串创建shader，自动展开模块引用
         * @param code 原始shader代码（可包含extern语句）
         * @param ctx Nut上下文
         * @return Shader模块引用
         */
        static ShaderModule& GetFromString(const std::string& code, const std::shared_ptr<NutContext>& ctx);

        /**
         * @brief 注册shader模块到全局注册表
         * @param moduleName 模块名称（支持点号分隔的层级）
         * @param sourceCode 模块源代码
         */
        static void RegisterShaderModule(const std::string& moduleName, const std::string& sourceCode);

        /**
         * @brief 从文件加载并注册shader模块
         * @param moduleName 模块名称
         * @param filePath 文件路径
         * @return 成功返回true
         */
        static bool RegisterShaderModuleFromFile(const std::string& moduleName, const std::filesystem::path& filePath);
    };
} // namespace Nut

#endif //NOAI_SHADER_H

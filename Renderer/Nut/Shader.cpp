#include "Shader.h"

#include <regex>

#include "NutContext.h"

namespace Nut {

ShaderModule::ShaderModule(const std::string& shaderCode, const std::shared_ptr<NutContext>& ctx)
{
    wgpu::ShaderModuleWGSLDescriptor wgslModuleDesc;
    wgslModuleDesc.code = shaderCode.c_str();
    wgpu::ShaderModuleDescriptor shaderModuleDesc;
    shaderModuleDesc.nextInChain = &wgslModuleDesc;

    shaderModule = ctx->GetWGPUDevice().CreateShaderModule(&shaderModuleDesc); // 编译着色器

    // 改进的正则表达式，能够匹配 var<storage, read_write> 的形式
    std::regex re(R"(@group\((\d+)\)\s*@binding\((\d+)\)\s*var(?:<([^>]+)>)?\s+(\w+)\s*:\s*([\w_<>,\s]+))");
    std::smatch match;
    std::string::const_iterator searchStart(shaderCode.cbegin());

    while (std::regex_search(searchStart, shaderCode.cend(), match, re))
    {
        ShaderBindingInfo bindingInfo;
        bindingInfo.groupIndex = std::stoul(match[1]);
        bindingInfo.location = std::stoul(match[2]);

        // 解析 var<...> 中的内容
        std::string varModifierFull = match[3]; // 完整的修饰符，如 "uniform" 或 "storage, read_write"
        std::string name = match[4];
        std::string typeStr = match[5];

        bindingInfo.name = name;

        // 判断绑定类型
        if (typeStr.find("sampler") != std::string::npos)
        {
            bindingInfo.type = BindingType::Sampler;
        }
        else if (typeStr.find("texture") != std::string::npos)
        {
            bindingInfo.type = BindingType::Texture;
        }
        else if (varModifierFull.find("uniform") != std::string::npos)
        {
            bindingInfo.type = BindingType::UniformBuffer;
        }
        else if (varModifierFull.find("storage") != std::string::npos)
        {
            bindingInfo.type = BindingType::StorageBuffer;
        }

        bindings[name] = bindingInfo;

        std::cout << "Group: " << bindingInfo.groupIndex
            << ", Binding: " << bindingInfo.location
            << ", Name: " << bindingInfo.name
            << ", Type: " << typeStr
            << " (" << varModifierFull << ")" << std::endl;

        searchStart = match.suffix().first;
    }
}

ShaderModule::ShaderModule()
{
    shaderModule = nullptr;
}

wgpu::ShaderModule& ShaderModule::Get()
{
    return shaderModule;
}

ShaderModule::operator bool() const
{
    return shaderModule != nullptr;
}

ShaderBindingInfo ShaderModule::GetBindingInfo(const std::string& name)
{
    auto it = bindings.find(name);
    if (it != bindings.end())
    {
        return it->second;
    }
    return {};
}

bool ShaderModule::GetBindingInfo(const std::string& name, ShaderBindingInfo& info)
{
    auto it = bindings.find(name);
    if (it != bindings.end())
    {
        info = it->second;
        return true;
    }
    return false;
}

void ShaderModule::ForeachBinding(const std::function<void(const ShaderBindingInfo&)>& callback) const
{
    if (!shaderModule)
    {
        return;
    }
    for (auto& binding : bindings)
    {
        callback(binding.second);
    }
}

ShaderModule& ShaderManager::GetFromFile(const std::string& file, const std::shared_ptr<NutContext>& ctx)
{
    std::fstream fs(file);
    if (!fs.is_open())
    {
        std::cerr << "Failed to open file " << file << std::endl;
    }
    std::ostringstream buffer;
    buffer << fs.rdbuf();
    std::string code = buffer.str();
    return GetFromString(code, ctx);
}

ShaderModule& ShaderManager::GetFromString(const std::string& code, const std::shared_ptr<NutContext>& ctx)
{
    if (shaderModules.contains(code) && shaderModules[code].get() != nullptr)
    {
        return *shaderModules[code].get();
    }
    shaderModules[code] = std::make_shared<ShaderModule>(code, ctx);
    return *shaderModules[code].get();
}

} // namespace Nut

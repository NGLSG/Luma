#include "Shader.h"

#include <regex>
#include <unordered_set>
#include <filesystem>

#include "NutContext.h"
#include "ShaderModuleRegistry.h"
#include "ShaderModuleInitializer.h"

namespace Nut {

namespace {

std::string LoadFileToString(const std::filesystem::path& path)
{
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open())
    {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return {};
    }
    std::ostringstream buffer;
    buffer << fs.rdbuf();
    return buffer.str();
}

} 

ShaderModule::ShaderModule(const std::string& shaderCode, const std::shared_ptr<NutContext>& ctx)
{
    wgpu::ShaderModuleWGSLDescriptor wgslModuleDesc;
    wgslModuleDesc.code = shaderCode.c_str();
    wgpu::ShaderModuleDescriptor shaderModuleDesc;
    shaderModuleDesc.nextInChain = &wgslModuleDesc;

    shaderModule = ctx->GetWGPUDevice().CreateShaderModule(&shaderModuleDesc); 

    
    std::regex re(R"(@group\((\d+)\)\s*@binding\((\d+)\)\s*var(?:<([^>]+)>)?\s+(\w+)\s*:\s*([\w_<>,\s]+))");
    std::smatch match;
    std::string::const_iterator searchStart(shaderCode.cbegin());

    while (std::regex_search(searchStart, shaderCode.cend(), match, re))
    {
        ShaderBindingInfo bindingInfo;
        bindingInfo.groupIndex = std::stoul(match[1]);
        bindingInfo.location = std::stoul(match[2]);

        
        std::string varModifierFull = match[3]; 
        std::string name = match[4];
        std::string typeStr = match[5];

        bindingInfo.name = name;

        
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
    std::string code = LoadFileToString(file);
    if (code.empty())
    {
        static ShaderModule emptyFallback{};
        return emptyFallback;
    }
    return GetFromString(code, ctx);
}

ShaderModule& ShaderManager::GetFromString(const std::string& code, const std::shared_ptr<NutContext>& ctx)
{
    
    std::string errorMessage;
    std::string exportedModuleName;
    std::string expandedCode = ShaderModuleExpander::ExpandModules(code, errorMessage, exportedModuleName);
    
    if (expandedCode.empty())
    {
        std::cerr << "Failed to expand shader modules: " << errorMessage << std::endl;
        static ShaderModule emptyFallback{};
        return emptyFallback;
    }
    
    
    if (!exportedModuleName.empty())
    {
        std::string cleanCode = ShaderModuleInitializer::RemoveExportStatement(code);
        RegisterShaderModule(exportedModuleName, cleanCode);
        std::cout << "[ShaderManager] Auto-registered module: " << exportedModuleName << std::endl;
    }
    
    
    if (shaderModules.contains(expandedCode) && shaderModules[expandedCode].get() != nullptr)
    {
        return *shaderModules[expandedCode].get();
    }
    
    shaderModules[expandedCode] = std::make_shared<ShaderModule>(expandedCode, ctx);
    return *shaderModules[expandedCode].get();
}

void ShaderManager::RegisterShaderModule(const std::string& moduleName, const std::string& sourceCode)
{
    ShaderModuleRegistry::GetInstance().RegisterModule(moduleName, sourceCode);
}

bool ShaderManager::RegisterShaderModuleFromFile(const std::string& moduleName, const std::filesystem::path& filePath)
{
    std::string code = LoadFileToString(filePath);
    if (code.empty())
    {
        std::cerr << "Failed to load shader module from file: " << filePath << std::endl;
        return false;
    }
    
    RegisterShaderModule(moduleName, code);
    return true;
}

} 

#include "Shader.h"

#include <regex>
#include <unordered_set>
#include <filesystem>

#include "NutContext.h"

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

std::string PreprocessWGSL(const std::filesystem::path& path,
                           const std::filesystem::path& systemIncludeDir,
                           std::unordered_set<std::string>& includeStack)
{
    if (includeStack.contains(path.string()))
    {
        std::cerr << "Circular include detected for WGSL: " << path << std::endl;
        return {};
    }
    includeStack.insert(path.string());

    std::string source = LoadFileToString(path);
    if (source.empty())
    {
        return {};
    }

    std::stringstream output;
    std::string line;
    std::istringstream input(source);
    std::regex includeRegex(R"(^\s*#include\s*[<\"]([^>\"]+)[>\"]\s*)");

    while (std::getline(input, line))
    {
        std::smatch match;
        if (std::regex_match(line, match, includeRegex))
        {
            std::string includePath = match[1];
            std::filesystem::path resolved;
            if (line.find('<') != std::string::npos)
            {
                resolved = systemIncludeDir / includePath;
            }
            else
            {
                resolved = path.parent_path() / includePath;
            }

            std::string included = PreprocessWGSL(resolved, systemIncludeDir, includeStack);
            if (!included.empty())
            {
                output << included << "\n";
            }
            continue;
        }

        output << line << "\n";
    }

    includeStack.erase(path.string());
    return output.str();
}

std::string PreprocessWGSLFromString(const std::string& code, const std::filesystem::path& systemIncludeDir)
{
    std::stringstream output;
    std::string line;
    std::istringstream input(code);
    std::regex includeRegex(R"(^\s*#include\s*[<\"]([^>\"]+)[>\"]\s*)");
    std::unordered_set<std::string> includeStack;

    while (std::getline(input, line))
    {
        std::smatch match;
        if (std::regex_match(line, match, includeRegex))
        {
            std::string includePath = match[1];
            std::filesystem::path resolved;
            
            
            if (line.find('<') != std::string::npos)
            {
                resolved = systemIncludeDir / includePath;
            }
            else
            {
                
                std::cerr << "Warning: Relative include \"" << includePath 
                          << "\" not supported in string source, skipping." << std::endl;
                continue;
            }

            std::string included = PreprocessWGSL(resolved, systemIncludeDir, includeStack);
            if (!included.empty())
            {
                output << included << "\n";
            }
            continue;
        }

        output << line << "\n";
    }

    return output.str();
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
    if (systemIncludeDir.empty())
    {
        systemIncludeDir = std::filesystem::current_path() / "Shaders";
    }

    std::unordered_set<std::string> includeStack;
    std::string code = PreprocessWGSL(file, systemIncludeDir, includeStack);
    if (code.empty())
    {
        static ShaderModule emptyFallback{};
        return emptyFallback;
    }
    return GetFromString(code, ctx);
}

ShaderModule& ShaderManager::GetFromString(const std::string& code, const std::shared_ptr<NutContext>& ctx)
{
    
    if (systemIncludeDir.empty())
    {
        systemIncludeDir = std::filesystem::current_path() / "Shaders";
    }
    
    std::string processedCode = PreprocessWGSLFromString(code, systemIncludeDir);
    
    if (shaderModules.contains(processedCode) && shaderModules[processedCode].get() != nullptr)
    {
        return *shaderModules[processedCode].get();
    }
    shaderModules[processedCode] = std::make_shared<ShaderModule>(processedCode, ctx);
    return *shaderModules[processedCode].get();
}

void ShaderManager::SetSystemIncludeDir(const std::filesystem::path& dir)
{
    systemIncludeDir = dir;
}

} 

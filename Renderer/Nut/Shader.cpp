#include "Shader.h"

#include <regex>
#include <unordered_set>
#include <filesystem>
#include <algorithm>

#include "NutContext.h"
#include "ShaderModuleRegistry.h"
#include "ShaderModuleInitializer.h"
#include "ShaderCache.h"

namespace Nut
{
    namespace
    {
        std::string LoadFileToString(const std::filesystem::path& path)
        {
            std::ifstream fs(path, std::ios::binary);
            if (!fs.is_open())
            {
                LogError("Failed to open file: {}", path.string());
                return {};
            }
            std::ostringstream buffer;
            buffer << fs.rdbuf();
            return buffer.str();
        }

        
        struct LayoutInfo
        {
            uint32_t size;
            uint32_t align;
        };

        
        uint32_t AlignUp(uint32_t offset, uint32_t alignment)
        {
            return (offset + alignment - 1) & ~(alignment - 1);
        }

        
        LayoutInfo GetPrimitiveLayout(std::string typeStr)
        {
            
            typeStr.erase(std::remove(typeStr.begin(), typeStr.end(), ' '), typeStr.end());

            
            if (typeStr == "f32" || typeStr == "i32" || typeStr == "u32") return {4, 4};
            if (typeStr == "f16") return {2, 2}; 

            
            
            if (typeStr.find("vec2") == 0) return {8, 8};
            
            if (typeStr.find("vec3") == 0) return {12, 16};
            
            if (typeStr.find("vec4") == 0) return {16, 16};

            
            
            if (typeStr.find("mat2x2") == 0) return {16, 8};
            
            if (typeStr.find("mat3x3") == 0) return {48, 16};
            
            if (typeStr.find("mat4x4") == 0) return {64, 16};

            
            return {0, 0};
        }

        
        LayoutInfo CalculateStructLayout(const std::string& structName, const std::string& shaderCode);

        
        LayoutInfo GetTypeLayout(const std::string& typeName, const std::string& shaderCode)
        {
            
            LayoutInfo info = GetPrimitiveLayout(typeName);
            if (info.size > 0) return info;

            
            return CalculateStructLayout(typeName, shaderCode);
        }

        
        LayoutInfo CalculateStructLayout(const std::string& structName, const std::string& shaderCode)
        {
            
            
            std::string regexStr = "struct\\s+" + structName + "\\s*\\{([^}]+)\\}";
            std::regex structRe(regexStr);
            std::smatch match;

            if (!std::regex_search(shaderCode, match, structRe))
            {
                
                LogWarn("Cannot find struct definition for type: {}", structName);
                return {0, 16}; 
            }

            std::string body = match[1];
            uint32_t currentOffset = 0;
            uint32_t maxAlign = 16; 

            
            
            std::regex memberRe(R"(\s*(\w+)\s*:\s*([\w_<>]+)(?:,|;)?\s*)");

            std::string::const_iterator searchStart(body.cbegin());
            while (std::regex_search(searchStart, body.cend(), match, memberRe))
            {
                std::string memberType = match[2];

                LayoutInfo memberLayout = GetTypeLayout(memberType, shaderCode);

                if (memberLayout.size == 0)
                {
                    
                    searchStart = match.suffix().first;
                    continue;
                }

                
                
                currentOffset = AlignUp(currentOffset, memberLayout.align);
                
                currentOffset += memberLayout.size;

                
                

                searchStart = match.suffix().first;
            }

            
            currentOffset = AlignUp(currentOffset, 16);

            return {currentOffset, 16};
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
            bindingInfo.size = 0; 

            std::string varModifierFull = match[3];
            std::string name = match[4];
            std::string typeStr = match[5];

            
            if (!typeStr.empty() && typeStr.back() == ';')
            {
                typeStr.pop_back();
            }

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
                CalculateBindingSize(typeStr, shaderCode, bindingInfo);
            }
            else if (varModifierFull.find("storage") != std::string::npos)
            {
                bindingInfo.type = BindingType::StorageBuffer;
                CalculateBindingSize(typeStr, shaderCode, bindingInfo);
            }

            bindings[name] = bindingInfo;

            searchStart = match.suffix().first;
        }
    }

    ShaderModule::ShaderModule()
    {
        shaderModule = nullptr;
    }

    void ShaderModule::CalculateBindingSize(const std::string& typeName, const std::string& fullShaderCode,
                                            ShaderBindingInfo& info)
    {
        
        std::string cleanType = typeName;
        
        cleanType.erase(std::remove(cleanType.begin(), cleanType.end(), ' '), cleanType.end());
        cleanType.erase(std::remove(cleanType.begin(), cleanType.end(), ';'), cleanType.end());

        
        if (cleanType.find("array") == 0)
        {
            
            info.size = 0;
            return;
        }

        LayoutInfo layout = GetTypeLayout(cleanType, fullShaderCode);
        info.size = layout.size;
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
            LogError("Failed to expand shader modules: {}", errorMessage);
            static ShaderModule emptyFallback{};
            return emptyFallback;
        }

        if (!exportedModuleName.empty())
        {
            std::string cleanCode = ShaderModuleInitializer::RemoveExportStatement(code);
            RegisterShaderModule(exportedModuleName, cleanCode);
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

    bool ShaderManager::RegisterShaderModuleFromFile(const std::string& moduleName,
                                                     const std::filesystem::path& filePath)
    {
        std::string code = LoadFileToString(filePath);
        if (code.empty())
        {
            LogError("Failed to load shader module from file: {}", filePath.string());
            return false;
        }

        RegisterShaderModule(moduleName, code);
        return true;
    }
} 

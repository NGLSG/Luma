#include "ShaderModuleInitializer.h"
#include "ShaderModuleRegistry.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

#ifdef __ANDROID__
#include "PathUtils.h"
#endif

namespace Nut
{

std::filesystem::path ShaderModuleInitializer::GetDefaultShadersPath()
{
#ifdef __ANDROID__
    return std::filesystem::path(PathUtils::GetAndroidInternalDataDir()) / "Shaders";
#else
    return std::filesystem::current_path() / "Shaders";
#endif
}

bool ShaderModuleInitializer::ExtractModuleName(const std::string& shaderCode, std::string& moduleName)
{
    
    std::regex exportRegex(R"(^\s*export\s+([\w\.]+)\s*;)", std::regex_constants::multiline);
    std::smatch match;
    
    if (std::regex_search(shaderCode, match, exportRegex))
    {
        moduleName = match[1].str();
        return true;
    }
    
    return false;
}

std::string ShaderModuleInitializer::RemoveExportStatement(const std::string& shaderCode)
{
    
    std::regex exportRegex(R"(^\s*export\s+[\w\.]+\s*;\s*$)", std::regex_constants::multiline);
    return std::regex_replace(shaderCode, exportRegex, "");
}

void ShaderModuleInitializer::ScanAndRegisterShaders(const std::filesystem::path& directory, int& registeredCount)
{
    auto& registry = ShaderModuleRegistry::GetInstance();
    ScanAndRegisterShaders(directory, registeredCount, registry);
}

void ShaderModuleInitializer::ScanAndRegisterShaders(const std::filesystem::path& directory, int& registeredCount, ShaderModuleRegistry& registry)
{
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "[ShaderModuleInitializer] Directory does not exist: " << directory.string() << std::endl;
        return;
    }

    if (!std::filesystem::is_directory(directory))
    {
        std::cerr << "[ShaderModuleInitializer] Path is not a directory: " << directory.string() << std::endl;
        return;
    }

    try
    {
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            
            if (!entry.is_regular_file())
                continue;

            const auto& path = entry.path();
            if (path.extension() != ".wgsl")
                continue;

            
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "[ShaderModuleInitializer] Failed to open file: " << path.string() << std::endl;
                continue;
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();
            std::string shaderCode = buffer.str();
            file.close();

            
            std::string moduleName;
            if (ExtractModuleName(shaderCode, moduleName))
            {
                
                std::string cleanCode = RemoveExportStatement(shaderCode);
                
                
                registry.RegisterModule(moduleName, cleanCode);
                registeredCount++;
                
                std::cout << "[ShaderModuleInitializer] Registered module '" << moduleName 
                         << "' from file: " << path.filename().string() << std::endl;
            }
            else
            {
                
                std::cout << "[ShaderModuleInitializer] Skipped file without export statement: " 
                         << path.filename().string() << std::endl;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "[ShaderModuleInitializer] Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ShaderModuleInitializer] Error scanning directory: " << e.what() << std::endl;
    }
}

int ShaderModuleInitializer::InitializeEngineShaderModules(const std::filesystem::path& shadersPath)
{
    std::filesystem::path targetPath = shadersPath.empty() ? GetDefaultShadersPath() : shadersPath;
    
    std::cout << "[ShaderModuleInitializer] Initializing engine shader modules from: " << targetPath.string() << std::endl;
    
    int registeredCount = 0;
    ScanAndRegisterShaders(targetPath, registeredCount);
    
    if (registeredCount > 0)
    {
        std::cout << "[ShaderModuleInitializer] Successfully registered " << registeredCount << " shader modules" << std::endl;
    }
    else
    {
        std::cerr << "[ShaderModuleInitializer] No shader modules were registered" << std::endl;
    }
    
    return registeredCount;
}

} 

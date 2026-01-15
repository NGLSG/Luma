#include "ShaderModuleInitializer.h"
#include "ShaderModuleRegistry.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

#include "Logger.h"

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

    void ShaderModuleInitializer::ScanAndRegisterShaders(const std::filesystem::path& directory, int& registeredCount,
                                                         ShaderModuleRegistry& registry)
    {
        if (!std::filesystem::exists(directory))
        {
            LogError("Directory does not exist: {}", directory.string());
            return;
        }

        if (!std::filesystem::is_directory(directory))
        {
            LogError("Path is not a directory: {}", directory.string());
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
                    LogError("Failed to open shader file: {}", path.string());
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

                    LogDebug("Registered shader module '{}' from file: {}",
                             moduleName, path.filename().string());
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LogError("Filesystem error while scanning directory: {}: {}", directory.string(), e.what());
        }
        catch (const std::exception& e)
        {
            LogError("Error while scanning directory: {}: {}", directory.string(), e.what());
        }
    }

    int ShaderModuleInitializer::InitializeEngineShaderModules(const std::filesystem::path& shadersPath)
    {
        std::filesystem::path targetPath = shadersPath.empty() ? GetDefaultShadersPath() : shadersPath;

        int registeredCount = 0;
        ScanAndRegisterShaders(targetPath, registeredCount);

        if (registeredCount > 0)
        {
            LogInfo("Registered {} shader modules from {}",
                    registeredCount, targetPath.string());
        }
        else
        {
            LogWarn(" No shader modules were registered from {}",
                    targetPath.string());
        }

        return registeredCount;
    }

    std::string ShaderModuleInitializer::RemoveImportAndExportStatements(const std::string& shaderCode)
    {
        std::istringstream iss(shaderCode);
        std::ostringstream oss;
        std::string line;
        
        while (std::getline(iss, line))
        {
            
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            
            
            bool isImportOrExport = false;
            
            if (trimmed.starts_with("import ") || trimmed.starts_with("export "))
            {
                isImportOrExport = true;
            }
            
            
            if (!isImportOrExport)
            {
                oss << line << '\n';
            }
        }
        
        return oss.str();
    }
}

#include "ShaderModuleRegistry.h"
#include "ShaderModuleInitializer.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>

#include "Logger.h"

namespace Nut
{
    ShaderModuleRegistry::ShaderModuleRegistry()
    {
        InitializeEngineModules();
    }

    void ShaderModuleRegistry::InitializeEngineModules()
    {
        std::filesystem::path shadersPath = ShaderModuleInitializer::GetDefaultShadersPath();
        int registeredCount = 0;

        ShaderModuleInitializer::ScanAndRegisterShaders(shadersPath, registeredCount, *this);
    }

    void ShaderModuleRegistry::RegisterModule(const std::string& moduleName, const std::string& sourceCode)
    {
        ShaderModuleInfo info(moduleName, sourceCode);
        m_modules[moduleName] = info;

        LogDebug("[ShaderModuleRegistry] Registered module: {}", moduleName);
    }

    const ShaderModuleInfo* ShaderModuleRegistry::GetModule(const std::string& moduleName) const
    {
        auto it = m_modules.find(moduleName);
        if (it != m_modules.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool ShaderModuleRegistry::HasModule(const std::string& moduleName) const
    {
        return m_modules.find(moduleName) != m_modules.end();
    }

    std::vector<std::string> ShaderModuleRegistry::GetChildModules(const std::string& parentModuleName) const
    {
        std::vector<std::string> children;
        std::string prefix = parentModuleName + ".";

        for (const auto& [name, info] : m_modules)
        {
            if (name.size() > prefix.size() && name.substr(0, prefix.size()) == prefix)
            {
                std::string remainder = name.substr(prefix.size());
                if (remainder.find('.') == std::string::npos)
                {
                    children.push_back(name);
                }
            }
        }

        return children;
    }

    void ShaderModuleRegistry::Clear()
    {
        m_modules.clear();
    }

    std::vector<std::string> ShaderModuleRegistry::GetAllModuleNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_modules.size());

        for (const auto& [name, info] : m_modules)
        {
            names.push_back(name);
        }

        return names;
    }


    bool ShaderModuleExpander::ParseImportStatement(const std::string& line, std::string& moduleName)
    {
        std::regex importRegex(R"(^\s*import\s+(::)?[\w\.]+\s*;)");
        std::smatch match;

        if (std::regex_search(line, match, importRegex))
        {
            std::string fullMatch = match[0].str();
            size_t importPos = fullMatch.find("import");
            size_t semicolonPos = fullMatch.find(';');

            std::string namepart = fullMatch.substr(importPos + 6, semicolonPos - importPos - 6);

            namepart.erase(0, namepart.find_first_not_of(" \t"));
            namepart.erase(namepart.find_last_not_of(" \t;") + 1);

            moduleName = namepart;
            return true;
        }

        return false;
    }

    bool ShaderModuleExpander::ParseExportStatement(const std::string& line, std::string& moduleName)
    {
        std::regex exportRegex(R"(^\s*export\s+([\w\.]+)\s*;)");
        std::smatch match;

        if (std::regex_match(line, match, exportRegex))
        {
            moduleName = match[1].str();
            return true;
        }

        return false;
    }

    std::string ShaderModuleExpander::ResolveModuleName(const std::string& moduleName,
                                                        const std::string& currentContext)
    {
        if (moduleName.length() >= 2 && moduleName.substr(0, 2) == "::")
        {
            if (currentContext.empty())
            {
                return moduleName.substr(2);
            }


            size_t lastDot = currentContext.find_last_of('.');
            if (lastDot != std::string::npos)
            {
                std::string parentModule = currentContext.substr(0, lastDot);
                return parentModule + "." + moduleName.substr(2);
            }
            else
            {
                return moduleName.substr(2);
            }
        }


        return moduleName;
    }

    bool ShaderModuleExpander::CollectAllDependencies(
        const std::string& moduleName,
        std::set<std::string>& allModules,
        std::vector<std::string>& moduleStack,
        std::string& errorMessage,
        const std::string& currentContext)
    {
        if (std::find(moduleStack.begin(), moduleStack.end(), moduleName) != moduleStack.end())
        {
            errorMessage = "Circular module dependency detected: ";
            for (const auto& mod : moduleStack)
            {
                errorMessage += mod + " -> ";
            }
            errorMessage += moduleName;
            return false;
        }


        if (allModules.count(moduleName) > 0)
        {
            return true;
        }

        auto& registry = ShaderModuleRegistry::GetInstance();


        if (registry.HasModule(moduleName))
        {
            allModules.insert(moduleName);


            const ShaderModuleInfo* moduleInfo = registry.GetModule(moduleName);
            if (moduleInfo)
            {
                moduleStack.push_back(moduleName);

                std::istringstream input(moduleInfo->sourceCode);
                std::string line;

                while (std::getline(input, line))
                {
                    std::string depModuleName;
                    if (ParseImportStatement(line, depModuleName))
                    {
                        std::string resolvedName = ResolveModuleName(depModuleName, moduleName);


                        if (!CollectAllDependencies(resolvedName, allModules, moduleStack, errorMessage,
                                                    currentContext))
                        {
                            return false;
                        }
                    }
                }

                moduleStack.pop_back();
            }
        }


        std::vector<std::string> children = registry.GetChildModules(moduleName);
        for (const auto& child : children)
        {
            if (!CollectAllDependencies(child, allModules, moduleStack, errorMessage, currentContext))
            {
                return false;
            }
        }


        std::function<void(const std::string&)> collectAllChildren = [&](const std::string& parent)
        {
            std::vector<std::string> directChildren = registry.GetChildModules(parent);
            for (const auto& child : directChildren)
            {
                if (allModules.count(child) == 0)
                {
                    if (registry.HasModule(child))
                    {
                        allModules.insert(child);
                    }
                    collectAllChildren(child);
                }
            }
        };

        collectAllChildren(moduleName);

        return true;
    }

    std::vector<std::string> ShaderModuleExpander::TopologicalSort(
        const std::set<std::string>& modules,
        std::string& errorMessage)
    {
        auto& registry = ShaderModuleRegistry::GetInstance();


        std::unordered_map<std::string, std::set<std::string>> dependencies;
        std::unordered_map<std::string, int> inDegree;


        for (const auto& moduleName : modules)
        {
            dependencies[moduleName] = std::set<std::string>();
            inDegree[moduleName] = 0;
        }


        for (const auto& moduleName : modules)
        {
            const ShaderModuleInfo* moduleInfo = registry.GetModule(moduleName);
            if (!moduleInfo) continue;

            std::istringstream input(moduleInfo->sourceCode);
            std::string line;

            while (std::getline(input, line))
            {
                std::string depModuleName;
                if (ParseImportStatement(line, depModuleName))
                {
                    std::string resolvedName = ResolveModuleName(depModuleName, moduleName);


                    if (modules.count(resolvedName) > 0)
                    {
                        dependencies[moduleName].insert(resolvedName);
                    }
                }
            }
        }


        for (const auto& [moduleName, deps] : dependencies)
        {
            inDegree[moduleName] += static_cast<int>(deps.size());
        }


        std::vector<std::string> result;
        std::queue<std::string> zeroInDegreeQueue;

        for (const auto& moduleName : modules)
        {
            if (inDegree[moduleName] == 0)
            {
                zeroInDegreeQueue.push(moduleName);
            }
        }

        while (!zeroInDegreeQueue.empty())
        {
            std::string current = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            result.push_back(current);

            for (const auto& [moduleName, deps] : dependencies)
            {
                if (deps.count(current) > 0)
                {
                    inDegree[moduleName]--;
                    if (inDegree[moduleName] == 0)
                    {
                        zeroInDegreeQueue.push(moduleName);
                    }
                }
            }
        }


        if (result.size() != modules.size())
        {
            errorMessage = "Circular dependency detected in module graph";
            return {};
        }

        return result;
    }

    bool ShaderModuleExpander::ExpandModuleRecursive(
        const std::string& moduleName,
        std::set<std::string>& expandedModules,
        std::vector<std::string>& moduleStack,
        std::ostringstream& output,
        std::string& errorMessage)
    {
        if (std::find(moduleStack.begin(), moduleStack.end(), moduleName) != moduleStack.end())
        {
            errorMessage = "Circular module dependency detected: ";
            for (const auto& mod : moduleStack)
            {
                errorMessage += mod + " -> ";
            }
            errorMessage += moduleName;
            return false;
        }


        if (expandedModules.count(moduleName) > 0)
        {
            return true;
        }


        auto& registry = ShaderModuleRegistry::GetInstance();
        const ShaderModuleInfo* moduleInfo = registry.GetModule(moduleName);

        if (!moduleInfo)
        {
            errorMessage = "Module not found: " + moduleName;
            return false;
        }


        moduleStack.push_back(moduleName);


        std::istringstream input(moduleInfo->sourceCode);
        std::string line;
        std::ostringstream moduleOutput;

        while (std::getline(input, line))
        {
            std::string dependencyModule;
            std::string exportName;


            if (ParseImportStatement(line, dependencyModule))
            {
                if (!ExpandModuleWithChildren(dependencyModule, expandedModules, moduleStack, output, errorMessage))
                {
                    return false;
                }

                continue;
            }


            if (ParseExportStatement(line, exportName))
            {
                continue;
            }


            moduleOutput << line << "\n";
        }


        expandedModules.insert(moduleName);


        output << "// ========== Module: " << moduleName << " ==========\n";
        output << moduleOutput.str();
        output << "// ========== End Module: " << moduleName << " ==========\n\n";


        moduleStack.pop_back();

        return true;
    }

    bool ShaderModuleExpander::ExpandModuleWithChildren(
        const std::string& moduleName,
        std::set<std::string>& expandedModules,
        std::vector<std::string>& moduleStack,
        std::ostringstream& output,
        std::string& errorMessage)
    {
        if (!ExpandModuleRecursive(moduleName, expandedModules, moduleStack, output, errorMessage))
        {
            return false;
        }


        auto& registry = ShaderModuleRegistry::GetInstance();
        std::vector<std::string> allChildren;


        std::function<void(const std::string&)> collectChildren = [&](const std::string& parent)
        {
            auto children = registry.GetChildModules(parent);
            for (const auto& child : children)
            {
                allChildren.push_back(child);
                collectChildren(child);
            }
        };

        collectChildren(moduleName);


        for (const auto& childModule : allChildren)
        {
            if (!ExpandModuleRecursive(childModule, expandedModules, moduleStack, output, errorMessage))
            {
                return false;
            }
        }

        return true;
    }

    std::string ShaderModuleExpander::ExpandModules(const std::string& sourceCode, std::string& errorMessage,
                                                    std::string& exportedModuleName)
    {
        std::istringstream input(sourceCode);
        std::string line;
        std::ostringstream mainCode;
        bool hasExport = false;
        std::set<std::string> directImports;
        std::string currentModuleContext = "";

        while (std::getline(input, line))
        {
            std::string moduleName;


            if (ParseImportStatement(line, moduleName))
            {
                std::string resolvedName = ResolveModuleName(moduleName, currentModuleContext);
                directImports.insert(resolvedName);
                continue;
            }


            if (ParseExportStatement(line, moduleName))
            {
                if (hasExport)
                {
                    errorMessage = "Multiple export statements found. Only one export per file is allowed.";
                    LogError("Error: {}", errorMessage);
                    return "";
                }
                exportedModuleName = moduleName;
                currentModuleContext = moduleName;
                hasExport = true;
                continue;
            }


            mainCode << line << "\n";
        }


        std::set<std::string> allModules;
        std::vector<std::string> moduleStack;

        for (const auto& importName : directImports)
        {
            if (!CollectAllDependencies(importName, allModules, moduleStack, errorMessage, currentModuleContext))
            {
                LogError("{}", errorMessage);
                return "";
            }
        }


        std::vector<std::string> sortedModules = TopologicalSort(allModules, errorMessage);
        if (sortedModules.empty() && !allModules.empty())
        {
            LogError("{}", errorMessage);
            return "";
        }


        std::ostringstream output;
        std::set<std::string> alreadyExpanded;
        
        for (const auto& moduleName : sortedModules)
        {
            auto& registry = ShaderModuleRegistry::GetInstance();
            const ShaderModuleInfo* moduleInfo = registry.GetModule(moduleName);

            if (!moduleInfo)
            {
                errorMessage = "Module not found: " + moduleName;
                return "";
            }

            
            output << "// ========== Module: " << moduleName << " ==========\n";
            
            std::istringstream moduleInput(moduleInfo->sourceCode);
            std::string line;
            
            while (std::getline(moduleInput, line))
            {
                std::string importedModule;
                
                
                if (ParseImportStatement(line, importedModule))
                {
                    continue;
                }
                
                
                std::string exportName;
                if (ParseExportStatement(line, exportName))
                {
                    continue;
                }
                
                
                output << line << "\n";
            }
            
            output << "// ========== End Module: " << moduleName << " ==========\n\n";
            alreadyExpanded.insert(moduleName);
        }


        if (hasExport)
        {
            output << "// ========== Module: " << exportedModuleName << " ==========\n";
        }
        else
        {
            output << "// ========== Main Shader Code ==========\n";
        }
        output << mainCode.str();
        if (hasExport)
        {
            output << "// ========== End Module: " << exportedModuleName << " ==========\n";
        }

        std::string result = output.str();
        if (hasExport)
        {
            LogInfo("Exported module: {}", exportedModuleName);
        }

        return result;
    }
}

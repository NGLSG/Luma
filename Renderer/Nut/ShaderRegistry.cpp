#include "ShaderRegistry.h"
#include "ShaderModuleRegistry.h"
#include "Shader.h"
#include "NutContext.h"
#include "Logger.h"
#include "../../Resources/AssetManager.h"
#include "../../Resources/Loaders/ShaderLoader.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <thread>

#include "ShaderModuleInitializer.h"

namespace Nut
{
    ShaderRegistry::ShaderRegistry()
    {
        LogInfo("ShaderRegistry: Initialized");
    }

    void ShaderRegistry::RegisterShader(const AssetHandle& assetHandle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string guidStr = assetHandle.assetGuid.ToString();

        if (m_shaders.find(guidStr) != m_shaders.end())
        {
            LogWarn("ShaderRegistry: Shader already registered - GUID: {}", guidStr);
            return;
        }

        m_shaders[guidStr] = ShaderAssetInfo(assetHandle);
        LogInfo("ShaderRegistry: Registered shader - GUID: {}", guidStr);
    }

    void ShaderRegistry::UnregisterShader(const AssetHandle& assetHandle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string guidStr = assetHandle.assetGuid.ToString();
        auto it = m_shaders.find(guidStr);

        if (it != m_shaders.end())
        {
            LogInfo("ShaderRegistry: Unregistered shader - GUID: {}", guidStr);
            m_shaders.erase(it);
        }
    }

    void ShaderRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shaders.clear();
        LogInfo("ShaderRegistry: Cleared all shaders");
    }

    std::vector<ShaderAssetInfo> ShaderRegistry::GetAllShaders() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<ShaderAssetInfo> result;
        result.reserve(m_shaders.size());

        for (const auto& [guid, info] : m_shaders)
        {
            result.push_back(info);
        }

        return result;
    }

    size_t ShaderRegistry::GetShaderCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_shaders.size();
    }

    bool ShaderRegistry::IsRegistered(const AssetHandle& assetHandle) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string guidStr = assetHandle.assetGuid.ToString();
        return m_shaders.find(guidStr) != m_shaders.end();
    }

    const ShaderAssetInfo* ShaderRegistry::GetShaderInfo(const AssetHandle& assetHandle) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string guidStr = assetHandle.assetGuid.ToString();
        auto it = m_shaders.find(guidStr);

        if (it != m_shaders.end())
        {
            return &it->second;
        }

        return nullptr;
    }

    void ShaderRegistry::MarkAsLoaded(const AssetHandle& assetHandle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string guidStr = assetHandle.assetGuid.ToString();
        auto it = m_shaders.find(guidStr);

        if (it != m_shaders.end())
        {
            it->second.isLoaded = true;
        }
    }

    bool ShaderRegistry::SaveToFile(const std::string& filePath) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        try
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "ShaderRegistry" << YAML::Value;
            out << YAML::BeginSeq;

            for (const auto& [guid, info] : m_shaders)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "GUID" << YAML::Value << guid;
                out << YAML::Key << "AssetType" << YAML::Value << static_cast<int>(info.assetHandle.assetType);
                out << YAML::EndMap;
            }

            out << YAML::EndSeq;
            out << YAML::EndMap;

            std::ofstream file(filePath);
            if (!file.is_open())
            {
                LogError("ShaderRegistry: Failed to open file for writing - Path: {}", filePath);
                return false;
            }

            file << out.c_str();
            file.close();

            LogInfo("ShaderRegistry: Saved {} shaders to file - Path: {}", m_shaders.size(), filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            LogError("ShaderRegistry: Failed to save to file - Error: {}", e.what());
            return false;
        }
    }

    bool ShaderRegistry::LoadFromFile(const std::string& filePath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        try
        {
            YAML::Node data = YAML::LoadFile(filePath);

            if (!data["ShaderRegistry"])
            {
                LogError("ShaderRegistry: Invalid file format - Path: {}", filePath);
                return false;
            }

            m_shaders.clear();

            for (const auto& node : data["ShaderRegistry"])
            {
                std::string guidStr = node["GUID"].as<std::string>();
                int assetType = node["AssetType"].as<int>();

                Guid guid = Guid::FromString(guidStr);
                AssetHandle handle(guid, static_cast<AssetType>(assetType));

                m_shaders[guidStr] = ShaderAssetInfo(handle);
            }

            LogInfo("ShaderRegistry: Loaded {} shaders from file - Path: {}", m_shaders.size(), filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            LogError("ShaderRegistry: Failed to load from file - Error: {}", e.what());
            return false;
        }
    }

    void ShaderRegistry::StartPreWarmingAsync()
    {
        if (m_preWarmingRunning.load())
        {
            LogWarn("ShaderRegistry: Pre-warming already in progress");
            return;
        }
        
        std::thread([this]()
        {
            PreWarmingImpl();
        }).detach();
    }

    void ShaderRegistry::PreWarming()
    {
        PreWarmingImpl();
    }

    PreWarmingState ShaderRegistry::GetPreWarmingState() const
    {
        PreWarmingState state;
        state.total = m_preWarmingTotal.load();
        state.loaded = m_preWarmingLoaded.load();
        state.isRunning = m_preWarmingRunning.load();
        state.isComplete = m_preWarmingComplete.load();
        return state;
    }

    bool ShaderRegistry::IsPreWarmingRunning() const
    {
        return m_preWarmingRunning.load();
    }

    bool ShaderRegistry::IsPreWarmingComplete() const
    {
        return m_preWarmingComplete.load();
    }

    void ShaderRegistry::StopPreWarming()
    {
        if (m_preWarmingRunning.load())
        {
            LogInfo("ShaderRegistry: Stop pre-warming requested");
            m_preWarmingStopRequested.store(true);
        }
    }

    void ShaderRegistry::PreWarmingImpl()
    {
        m_preWarmingRunning.store(true);
        m_preWarmingComplete.store(false);
        m_preWarmingLoaded.store(0);
        m_preWarmingStopRequested.store(false);

        std::vector<ShaderAssetInfo> shaders;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            shaders.reserve(m_shaders.size());
            for (const auto& [guid, info] : m_shaders)
            {
                shaders.push_back(info);
            }
        }

        int total = static_cast<int>(shaders.size());
        m_preWarmingTotal.store(total);

        LogInfo("ShaderRegistry: Starting pre-warming - Total shaders: {}", total);
        LogInfo("ShaderRegistry: Phase 1 - Parsing shader source code and registering modules");

        auto& assetManager = AssetManager::GetInstance();
        auto& moduleRegistry = ShaderModuleRegistry::GetInstance();

        std::vector<std::pair<AssetHandle, std::string>> shaderSources;
        shaderSources.reserve(shaders.size());
        auto shaderLoader = NutContext::GetShaderLoader();
        if (!shaderLoader)
        {
            LogError("ShaderRegistry: No ShaderLoader available in NutContext");
            m_preWarmingRunning.store(false);
            m_preWarmingComplete.store(true);
            return;
        }

        for (const auto& shaderInfo : shaders)
        {
            
            if (m_preWarmingStopRequested.load())
            {
                LogInfo("ShaderRegistry: Pre-warming stopped by request");
                m_preWarmingRunning.store(false);
                return;
            }

            try
            {
                const auto* metadata = assetManager.GetMetadata(shaderInfo.assetHandle.assetGuid);
                if (!metadata)
                {
                    LogWarn("ShaderRegistry: Metadata not found for shader - GUID: {}",
                            shaderInfo.assetHandle.assetGuid.ToString());
                    continue;
                }

                auto shaderData = shaderLoader->LoadShaderDataFromGuid(shaderInfo.assetHandle.assetGuid);

                if (!shaderData.source.empty())
                {
                    std::string exportedModuleName;
                    ShaderModuleInitializer::ExtractModuleName(shaderData.source, exportedModuleName);

                    if (!exportedModuleName.empty())
                    {
                        std::string cleanCode = ShaderModuleInitializer::RemoveExportStatement(shaderData.source);
                        moduleRegistry.RegisterModule(exportedModuleName, cleanCode);
                        LogInfo("ShaderRegistry: Registered module - Name: {}, GUID: {}",
                                exportedModuleName, shaderInfo.assetHandle.assetGuid.ToString());
                    }
                    else
                    {
                        shaderSources.push_back({shaderInfo.assetHandle, shaderData.source});
                    }

                    std::string shaderName = metadata->assetPath.stem().string();
                    LogInfo("ShaderRegistry: Loaded shader source - Name: {}, GUID: {}",
                            shaderName, shaderInfo.assetHandle.assetGuid.ToString());

                    m_preWarmingLoaded.fetch_add(1);
                }
                else
                {
                    std::string shaderName = metadata->assetPath.stem().string();
                    LogWarn("ShaderRegistry: Failed to load shader - Name: {}, Path: {}",
                            shaderName, metadata->assetPath.string());
                }
            }
            catch (const std::exception& e)
            {
                LogError("ShaderRegistry: Exception loading shader - GUID: {}, Error: {}",
                         shaderInfo.assetHandle.assetGuid.ToString(), e.what());
            }
        }

        
        m_preWarmingLoaded.store(0);
        total = static_cast<int>(shaderSources.size());
        m_preWarmingTotal.store(total);

        for (const auto& [handle, sourceCode] : shaderSources)
        {
            
            if (m_preWarmingStopRequested.load())
            {
                LogInfo("ShaderRegistry: Pre-warming stopped by request");
                m_preWarmingRunning.store(false);
                return;
            }

            try
            {
                auto shader = shaderLoader->LoadAsset(handle.assetGuid);

                if (shader)
                {
                    shader->EnsureCompiled();
                    MarkAsLoaded(handle);
                    m_preWarmingLoaded.fetch_add(1);

                    LogInfo("ShaderRegistry: Compiled shader to GPU - Progress: {}/{}",
                            m_preWarmingLoaded.load(), total);
                }
            }
            catch (const std::exception& e)
            {
                LogError("ShaderRegistry: Exception compiling shader - GUID: {}, Error: {}",
                         handle.assetGuid.ToString(), e.what());
            }
        }

        m_preWarmingRunning.store(false);
        m_preWarmingComplete.store(true);

        LogInfo("ShaderRegistry: Pre-warming complete - Successfully warmed {}/{} shaders",
                m_preWarmingLoaded.load(), total);
    }
}

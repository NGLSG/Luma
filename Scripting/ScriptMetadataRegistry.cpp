#include "ScriptMetadataRegistry.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

void ScriptMetadataRegistry::Initialize(const std::filesystem::path& metadataFilePath)
{
    m_metadataFilePath = metadataFilePath;
    Refresh();
}

bool ScriptMetadataRegistry::Refresh()
{
    m_classMetadata.clear();
    m_availableTypes.clear();

    if (!std::filesystem::exists(m_metadataFilePath))
    {
        LogWarn("ScriptMetadataRegistry: Metadata file not found at '{}'. No script information will be available.",
                m_metadataFilePath.string());
        return false;
    }

    try
    {
        YAML::Node root = YAML::LoadFile(m_metadataFilePath.string());
        ScriptMetadata metadata = root.as<ScriptMetadata>();

        for (auto&& scriptMeta : metadata.scripts)
        {
            if (scriptMeta.Valid())
            {
                m_classMetadata[scriptMeta.fullName] = std::move(scriptMeta);
            }
        }

        m_availableTypes = std::move(metadata.availableTypes);

        LogInfo("ScriptMetadataRegistry: Successfully loaded {} script(s) and {} available type(s).",
                m_classMetadata.size(), m_availableTypes.size());
    }
    catch (const YAML::Exception& e)
    {
        LogError("ScriptMetadataRegistry: Failed to parse metadata file '{}'. Reason: {}",
                 m_metadataFilePath.string(), e.what());
        m_classMetadata.clear();
        m_availableTypes.clear();
        return false;
    }

    return true;
}

ScriptClassMetadata ScriptMetadataRegistry::GetMetadata(const std::string& fullClassName) const
{
    auto it = m_classMetadata.find(fullClassName);
    if (it != m_classMetadata.end())
    {
        return it->second;
    }
    return ScriptClassMetadata();
}

const std::unordered_map<std::string, ScriptClassMetadata>& ScriptMetadataRegistry::GetAllMetadata() const
{
    return m_classMetadata;
}

const std::vector<std::string>& ScriptMetadataRegistry::GetAvailableTypes() const
{
    return m_availableTypes;
}
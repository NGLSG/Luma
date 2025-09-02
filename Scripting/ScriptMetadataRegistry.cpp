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

    if (!std::filesystem::exists(m_metadataFilePath))
    {
        LogWarn("ScriptMetadataRegistry: Metadata file not found at '{}'. No script information will be available.",
                m_metadataFilePath.string());
        return false;
    }


    try
    {
        YAML::Node root = YAML::LoadFile(m_metadataFilePath.string());

        if (!root["Scripts"] || !root["Scripts"].IsSequence())
        {
            LogError("ScriptMetadataRegistry: Metadata file is invalid. Missing 'Scripts' sequence root node.");
            return false;
        }

        const YAML::Node& scriptsNode = root["Scripts"];
        for (const auto& scriptNode : scriptsNode)
        {
            ScriptClassMetadata metadata = scriptNode.as<ScriptClassMetadata>();


            if (!metadata.fullName.empty())
            {
                m_classMetadata[metadata.fullName] = std::move(metadata);
            }
        }
    }
    catch (const YAML::Exception& e)
    {
        LogError("ScriptMetadataRegistry: Failed to parse metadata file. Reason: {}", e.what());
        return false;
    }

    return true;
}

ScriptClassMetadata ScriptMetadataRegistry::GetMetadata(const std::string& fullClassName) const
{
    if (m_classMetadata.contains(fullClassName))
    {
        return m_classMetadata.at(fullClassName);
    }
    return ScriptClassMetadata();
}

const std::unordered_map<std::string, ScriptClassMetadata>& ScriptMetadataRegistry::GetAllMetadata() const
{
    return m_classMetadata;
}

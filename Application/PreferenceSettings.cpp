#include "PreferenceSettings.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

static std::string IDEToString(IDE ide)
{
    switch (ide)
    {
    case IDE::VisualStudio: return "VisualStudio";
    case IDE::Rider: return "Rider";
    case IDE::VSCode: return "VSCode";
    default: return "AutoDetect";
    }
}

static IDE StringToIDE(const std::string& str)
{
    if (str == "VisualStudio") return IDE::VisualStudio;
    if (str == "Rider") return IDE::Rider;
    if (str == "VSCode") return IDE::VSCode;
    return IDE::Unknown;
}


void PreferenceSettings::Initialize(const std::filesystem::path& configPath)
{
    m_configPath = configPath;
    Load();
}

void PreferenceSettings::Load()
{
    if (!std::filesystem::exists(m_configPath))
    {
        LogInfo("Preference file not found at '{}'. Using default settings.", m_configPath.string());
        return;
    }

    try
    {
        YAML::Node config = YAML::LoadFile(m_configPath.string());

        if (config["PreferredIDE"])
        {
            m_preferredIDE = StringToIDE(config["PreferredIDE"].as<std::string>());
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed to load preferences from '{}'. Error: {}", m_configPath.string(), e.what());
    }
}

void PreferenceSettings::Save()
{
    LogInfo("Saving preferences to '{}'...", m_configPath.string());
    try
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "PreferredIDE" << YAML::Value << IDEToString(m_preferredIDE);
        out << YAML::EndMap;

        std::filesystem::path parentPath = m_configPath.parent_path();
        if (!std::filesystem::exists(parentPath))
        {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream fout(m_configPath);
        fout << out.c_str();
    }
    catch (const std::exception& e)
    {
        LogError("Failed to save preferences to '{}'. Error: {}", m_configPath.string(), e.what());
    }
}

IDE PreferenceSettings::GetPreferredIDE() const
{
    return m_preferredIDE;
}

void PreferenceSettings::SetPreferredIDE(IDE ide)
{
    if (m_preferredIDE != ide)
    {
        m_preferredIDE = ide;
        Save();
    }
}

#include "ProjectSettings.h"
#include "Utils/PCH.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "ApplicationBase.h"
#include "EngineCrypto.h"


namespace YAML
{
    template <>
    struct convert<ProjectSettings>
    {
        static Node encode(const ProjectSettings& rhs)
        {
            Node node;
            node["AppName"] = rhs.GetAppName();
            node["StartScene"] = rhs.GetStartScene().ToString();
            node["AppIconPath"] = rhs.GetAppIconPath().string();
            node["IsFullscreen"] = rhs.IsFullscreen();

            node["TargetWidth"] = rhs.GetTargetWidth();
            node["TargetHeight"] = rhs.GetTargetHeight();
            node["IsBorderless"] = rhs.IsBorderless();
            node["EnableConsole"] = rhs.IsConsoleEnabled();
            
            if (!rhs.GetTags().empty())
            {
                Node tagsNode;
                for (const auto& t : rhs.GetTags()) tagsNode.push_back(t);
                node["Tags"] = tagsNode;
            }
            return node;
        }


        static bool decode(const Node& node, ProjectSettings& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            rhs.SetAppName(node["AppName"].as<std::string>("Luma Game"));
            rhs.SetStartScene(Guid::FromString(node["StartScene"].as<std::string>("")));
            rhs.SetAppIconPath(node["AppIconPath"].as<std::string>(""));
            rhs.SetFullscreen(node["IsFullscreen"].as<bool>(false));

            rhs.SetTargetWidth(node["TargetWidth"].as<int>(1280));
            rhs.SetTargetHeight(node["TargetHeight"].as<int>(720));
            rhs.SetBorderless(node["IsBorderless"].as<bool>(false));
            rhs.SetConsoleEnabled(node["EnableConsole"].as<bool>(false));

            
            std::vector<std::string> tags;
            if (node["Tags"] && node["Tags"].IsSequence())
            {
                tags = node["Tags"].as<std::vector<std::string>>();
            }
            rhs.SetTags(tags);

            return true;
        }
    };
}

void ProjectSettings::Load()
{
    {
        if (IsProjectLoaded())
        {
            Load(m_projectFilePath);
        }
    }
}

void ProjectSettings::Save()
{
    if (IsProjectLoaded())
    {
        Save(m_projectFilePath);
    }
}

void ProjectSettings::LoadInRuntime()
{
    LoadWithCrypto("ProjectSettings.lproj");
    EnsureDefaultTags();
}

void ProjectSettings::Load(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath))
    {
        return;
    }

    m_projectFilePath = filePath;

    try
    {
        YAML::Node data = YAML::LoadFile(filePath.string());
        YAML::convert<ProjectSettings>::decode(data, *this);
        EnsureDefaultTags();
    }
    catch (const YAML::Exception& e)
    {
        LogError("加载项目设置失败 '{}': {}", filePath.string(), e.what());
    }
}

void ProjectSettings::Save(const std::filesystem::path& filePath)
{
    m_projectFilePath = filePath;

    YAML::Emitter emitter;
    emitter.SetIndent(2);
    emitter << YAML::convert<ProjectSettings>::encode(*this);

    std::ofstream fout(filePath);
    fout << emitter.c_str();
    fout.close();
}

void ProjectSettings::AddTag(const std::string& tag)
{
    if (tag.empty()) return;
    if (std::find(m_tags.begin(), m_tags.end(), tag) == m_tags.end())
    {
        m_tags.push_back(tag);
    }
}

void ProjectSettings::RemoveTag(const std::string& tag)
{
    std::erase(m_tags, tag);
}

void ProjectSettings::EnsureDefaultTags()
{
    auto ensure = [&](const char* t)
    {
        if (std::find(m_tags.begin(), m_tags.end(), std::string(t)) == m_tags.end())
        {
            m_tags.emplace_back(t);
        }
    };
    ensure("Unknown");
    ensure("Player");
    ensure("Ground");
}

std::filesystem::path ProjectSettings::GetProjectRoot() const
{
    if (m_projectFilePath.empty())
    {
        return "";
    }
    return m_projectFilePath.parent_path();
}

std::filesystem::path ProjectSettings::GetAssetsDirectory() const
{
    if (!IsProjectLoaded())
    {
        return "";
    }
    return GetProjectRoot() / "Assets";
}

TargetPlatform ProjectSettings::GetCurrentHostPlatform()
{
#ifdef _WIN32
    return TargetPlatform::Windows;
#elif __linux__
    return TargetPlatform::Linux;
#elif __ANDROID__
    return TargetPlatform::Android;
#else
    return TargetPlatform::Unknown;
#endif
}

std::string ProjectSettings::PlatformToString(TargetPlatform platform)
{
    switch (platform)
    {
    case TargetPlatform::Windows:
        return "Windows";
    case TargetPlatform::Linux:
        return "Linux";
    case TargetPlatform::Android:
        return "Android";
    default:
        return "Unknown";
    }
}

TargetPlatform ProjectSettings::StringToPlatform(const std::string& platformStr)
{
    if (platformStr == "Windows")
        return TargetPlatform::Windows;
    else if (platformStr == "Linux")
        return TargetPlatform::Linux;
    else if (platformStr == "Android")
        return TargetPlatform::Android;
    else
        return TargetPlatform::Unknown;
}

void ProjectSettings::LoadWithCrypto(const std::filesystem::path& filePath)
{
    std::vector<unsigned char> encryptedData;
    if (std::filesystem::exists(filePath))
    {
        std::ifstream file(filePath, std::ios::binary);
        if (file)
        {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            encryptedData.resize(size);
            file.read(reinterpret_cast<char*>(encryptedData.data()), size);
            file.close();
        }
    }
    std::vector<unsigned char> data = EngineCrypto::GetInstance().Decrypt(encryptedData);
    if (data.empty())
    {
        return;
    }
    try
    {
        YAML::Node dataNode = YAML::Load(std::string(data.begin(), data.end()));
        if (YAML::convert<ProjectSettings>::decode(dataNode, *this))
        {
            m_projectFilePath = filePath;
        }
        else
        {
            LogError("解析项目设置文件失败 '{}'", filePath.string());
        }
    }
    catch (const std::exception& e)
    {
        throw;
    }
}

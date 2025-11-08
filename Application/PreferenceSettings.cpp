#include "PreferenceSettings.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <cstdlib>

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

        if (config["Android"])
        {
            const auto& androidNode = config["Android"];
            if (androidNode["SDK"])
            {
                m_androidSdkPath = androidNode["SDK"].as<std::string>("");
            }
            if (androidNode["NDK"])
            {
                m_androidNdkPath = androidNode["NDK"].as<std::string>("");
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed to load preferences from '{}'. Error: {}", m_configPath.string(), e.what());
    }

    if (m_androidSdkPath.empty())
    {
        if (const char* sdk = std::getenv("ANDROID_SDK_ROOT"))
        {
            m_androidSdkPath = sdk;
        }
        else if (const char* sdkHome = std::getenv("ANDROID_HOME"))
        {
            m_androidSdkPath = sdkHome;
        }
    }
    if (m_androidNdkPath.empty())
    {
        if (const char* ndk = std::getenv("ANDROID_NDK_HOME"))
        {
            m_androidNdkPath = ndk;
        }
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
        out << YAML::Key << "Android" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "SDK" << YAML::Value << m_androidSdkPath.string();
        out << YAML::Key << "NDK" << YAML::Value << m_androidNdkPath.string();
        out << YAML::EndMap;
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

void PreferenceSettings::SetAndroidSdkPath(const std::filesystem::path& path)
{
    if (m_androidSdkPath != path)
    {
        m_androidSdkPath = path;
        Save();
    }
}

void PreferenceSettings::SetAndroidNdkPath(const std::filesystem::path& path)
{
    if (m_androidNdkPath != path)
    {
        m_androidNdkPath = path;
        Save();
    }
}

std::filesystem::path PreferenceSettings::GetLibcxxSharedPath(const std::string& abi) const
{
    if (m_androidNdkPath.empty()) return {};

    const std::filesystem::path ndkRoot = m_androidNdkPath;

#if defined(_WIN32)
    constexpr const char* kHostTag = "windows-x86_64";
#elif defined(__APPLE__)
    constexpr const char* kHostTag = "darwin-x86_64";
#else
    constexpr const char* kHostTag = "linux-x86_64";
#endif

    auto toTriple = [](const std::string& abiName) -> std::string
    {
        if (abiName == "arm64-v8a") return "aarch64-linux-android";
        if (abiName == "armeabi-v7a") return "arm-linux-androideabi";
        if (abiName == "x86_64") return "x86_64-linux-android";
        if (abiName == "x86") return "i686-linux-android";
        return {};
    };

    const std::string triple = toTriple(abi);
    if (triple.empty()) return {};

    const std::filesystem::path llvmPrebuilt = ndkRoot / "toolchains" / "llvm" / "prebuilt" / kHostTag / "sysroot" / "usr" / "lib";
    const std::filesystem::path sourcesLegacy = ndkRoot / "sources" / "cxx-stl" / "llvm-libc++" / "libs";

    std::vector<std::filesystem::path> candidates;
    candidates.emplace_back(llvmPrebuilt / triple / "libc++_shared.so");
    candidates.emplace_back(llvmPrebuilt / triple / "31" / "libc++_shared.so");
    candidates.emplace_back(llvmPrebuilt / triple / "30" / "libc++_shared.so");
    candidates.emplace_back(sourcesLegacy / abi / "libc++_shared.so");

    for (const auto& candidate : candidates)
    {
        if (!candidate.empty() && std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(ndkRoot, std::filesystem::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; ++it)
    {
        if (!it->is_regular_file()) continue;
        if (it->path().filename() == "libc++_shared.so")
        {
            return it->path();
        }
    }

    return {};
}

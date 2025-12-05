#ifndef PREFERENCESETTINGS_H
#define PREFERENCESETTINGS_H
#include "../Utils/LazySingleton.h"
#include "../Utils/IDEIntegration.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>
class PreferenceSettings : public LazySingleton<PreferenceSettings>
{
public:
    friend class LazySingleton<PreferenceSettings>;
    void Initialize(const std::filesystem::path& configPath);
    void Save();
    IDE GetPreferredIDE() const;
    void SetPreferredIDE(IDE ide);
    const std::filesystem::path& GetAndroidSdkPath() const { return m_androidSdkPath; }
    const std::filesystem::path& GetAndroidNdkPath() const { return m_androidNdkPath; }
    void SetAndroidSdkPath(const std::filesystem::path& path);
    void SetAndroidNdkPath(const std::filesystem::path& path);
    std::filesystem::path GetLibcxxSharedPath(const std::string& abi) const;
    PreferenceSettings() = default;
private:
    void Load();
    IDE m_preferredIDE = IDE::Unknown;
    std::filesystem::path m_androidSdkPath;
    std::filesystem::path m_androidNdkPath;
    std::filesystem::path m_configPath;
};
namespace YAML
{
    template <>
    struct convert<PreferenceSettings>
    {
        static Node encode(const PreferenceSettings& rhs);
        static bool decode(const Node& node, PreferenceSettings& rhs);
    };
}
#endif

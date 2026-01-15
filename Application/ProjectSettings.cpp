#include "ProjectSettings.h"
#include "Utils/PCH.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include "ApplicationBase.h"
#include "EngineCrypto.h"
namespace
{
    std::string OrientationToString(AndroidScreenOrientation orientation)
    {
        switch (orientation)
        {
        case AndroidScreenOrientation::LandscapeLeft: return "LandscapeLeft";
        case AndroidScreenOrientation::LandscapeRight: return "LandscapeRight";
        default: return "Portrait";
        }
    }
    AndroidScreenOrientation StringToOrientation(const std::string& value)
    {
        if (value == "LandscapeLeft") return AndroidScreenOrientation::LandscapeLeft;
        if (value == "LandscapeRight") return AndroidScreenOrientation::LandscapeRight;
        return AndroidScreenOrientation::Portrait;
    }
    std::string ViewportScaleModeToString(ViewportScaleMode mode)
    {
        switch (mode)
        {
        case ViewportScaleMode::FixedAspect: return "FixedAspect";
        case ViewportScaleMode::FixedWidth: return "FixedWidth";
        case ViewportScaleMode::FixedHeight: return "FixedHeight";
        case ViewportScaleMode::Expand: return "Expand";
        default: return "None";
        }
    }
    ViewportScaleMode StringToViewportScaleMode(const std::string& value)
    {
        if (value == "FixedAspect") return ViewportScaleMode::FixedAspect;
        if (value == "FixedWidth") return ViewportScaleMode::FixedWidth;
        if (value == "FixedHeight") return ViewportScaleMode::FixedHeight;
        if (value == "Expand") return ViewportScaleMode::Expand;
        return ViewportScaleMode::None;
    }
    std::string OrientationToManifestValue(AndroidScreenOrientation orientation)
    {
        switch (orientation)
        {
        case AndroidScreenOrientation::LandscapeLeft: return "landscape";
        case AndroidScreenOrientation::LandscapeRight: return "reverseLandscape";
        default: return "portrait";
        }
    }
    std::string OrientationToMetaValue(AndroidScreenOrientation orientation)
    {
        switch (orientation)
        {
        case AndroidScreenOrientation::LandscapeLeft: return "landscape_left";
        case AndroidScreenOrientation::LandscapeRight: return "landscape_right";
        default: return "portrait";
        }
    }
    std::string BuildAndroidManifestTemplate(const ProjectSettings& settings)
    {
        std::ostringstream oss;
        const std::string orientationValue = OrientationToManifestValue(settings.GetAndroidScreenOrientation());
        const std::string orientationMeta = OrientationToMetaValue(settings.GetAndroidScreenOrientation());
        oss << R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    android:versionCode="1"
    android:versionName="1.0"
    android:installLocation="auto">
    <uses-feature android:glEsVersion="0x00020000" />
    <uses-feature android:name="android.hardware.touchscreen" android:required="false" />
    <uses-feature android:name="android.hardware.bluetooth" android:required="false" />
    <uses-feature android:name="android.hardware.gamepad" android:required="false" />
    <uses-feature android:name="android.hardware.usb.host" android:required="false" />
    <uses-feature android:name="android.hardware.type.pc" android:required="false" />
)";
        if (settings.GetAndroidPermissions().empty())
        {
            oss << R"(    <uses-permission android:name="android.permission.VIBRATE" />
)";
        }
        else
        {
            for (const auto& permission : settings.GetAndroidPermissions())
            {
                oss << "    <uses-permission android:name=\"" << permission << "\" />\n";
            }
        }
        oss << R"(
    <application
        android:label="@string/app_name"
        android:icon="@mipmap/ic_launcher"
        android:allowBackup="true"
        android:theme="@style/AppTheme"
        android:hardwareAccelerated="true">
        <activity
            android:name="com.lumaengine.lumaandroid.LumaSDLActivity"
            android:label="@string/app_name"
            android:alwaysRetainTaskState="true"
            android:launchMode="singleInstance"
            android:configChanges="layoutDirection|locale|grammaticalGender|fontScale|fontWeightAdjustment|orientation|uiMode|screenLayout|screenSize|smallestScreenSize|keyboard|keyboardHidden|navigation"
            android:preferMinimalPostProcessing="true"
            android:exported="true"
            android:screenOrientation=")" << orientationValue << R"(">
            <meta-data
                android:name="com.lumaengine.orientation"
                android:value=")" << orientationMeta << R"(" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.hardware.usb.action.USB_DEVICE_ATTACHED" />
            </intent-filter>
        </activity>
    </application>
</manifest>
)";
        return oss.str();
    }
}
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
            node["ViewportScaleMode"] = ViewportScaleModeToString(rhs.GetViewportScaleMode());
            node["DesignWidth"] = rhs.GetDesignWidth();
            node["DesignHeight"] = rhs.GetDesignHeight();
            node["IsBorderless"] = rhs.IsBorderless();
            node["EnableConsole"] = rhs.IsConsoleEnabled();
            {
                Node dbg;
                dbg["Enabled"] = rhs.GetScriptDebugEnabled();
                dbg["WaitForAttach"] = rhs.GetScriptDebugWaitForAttach();
                dbg["Address"] = rhs.GetScriptDebugAddress();
                dbg["Port"] = rhs.GetScriptDebugPort();
                node["ScriptDebug"] = dbg;
            }
            if (!rhs.GetTags().empty())
            {
                Node tagsNode;
                for (const auto& t : rhs.GetTags()) tagsNode.push_back(t);
                node["Tags"] = tagsNode;
            }
            if (!rhs.GetLayers().empty())
            {
                Node layersNode;
                for (const auto& [idx, name] : rhs.GetLayers())
                {
                    layersNode[std::to_string(idx)] = name;
                }
                node["Layers"] = layersNode;
            }
            Node androidNode;
            androidNode["PackageName"] = rhs.GetAndroidPackageName();
            androidNode["Orientation"] = OrientationToString(rhs.GetAndroidScreenOrientation());
            androidNode["CompileSdk"] = rhs.GetAndroidCompileSdk();
            androidNode["TargetSdk"] = rhs.GetAndroidTargetSdk();
            androidNode["MinSdk"] = rhs.GetAndroidMinSdk();
            androidNode["MaxVersion"] = rhs.GetAndroidMaxVersion();
            androidNode["MinVersion"] = rhs.GetAndroidMinVersion();
            androidNode["VersionCode"] = rhs.GetAndroidVersionCode();
            androidNode["VersionName"] = rhs.GetAndroidVersionName();
            androidNode["KeystorePath"] = rhs.GetAndroidKeystorePath().string();
            androidNode["KeystorePassword"] = rhs.GetAndroidKeystorePassword();
            androidNode["KeyAlias"] = rhs.GetAndroidKeyAlias();
            androidNode["KeyPassword"] = rhs.GetAndroidKeyPassword();
            androidNode["UseCustomManifest"] = rhs.IsCustomAndroidManifestEnabled();
            androidNode["UseCustomGradleProperties"] = rhs.IsCustomGradlePropertiesEnabled();
            androidNode["ApkName"] = rhs.GetAndroidApkName();
            if (!rhs.GetAndroidIconMap().empty())
            {
                Node iconNode;
                for (const auto& [size, path] : rhs.GetAndroidIconMap())
                {
                    if (!path.empty())
                    {
                        iconNode[std::to_string(size)] = path.string();
                    }
                }
                if (iconNode.IsMap())
                {
                    androidNode["Icons"] = iconNode;
                }
            }
            if (!rhs.GetAndroidAliasEntries().empty())
            {
                Node aliasArray;
                for (const auto& entry : rhs.GetAndroidAliasEntries())
                {
                    Node aliasNode;
                    aliasNode["Alias"] = entry.alias;
                    aliasNode["Password"] = entry.password;
                    aliasArray.push_back(aliasNode);
                }
                androidNode["AliasEntries"] = aliasArray;
            }
            node["Android"] = androidNode;
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
            rhs.SetViewportScaleMode(StringToViewportScaleMode(node["ViewportScaleMode"].as<std::string>("None")));
            rhs.SetDesignWidth(node["DesignWidth"].as<int>(1920));
            rhs.SetDesignHeight(node["DesignHeight"].as<int>(1080));
            rhs.SetBorderless(node["IsBorderless"].as<bool>(false));
            rhs.SetConsoleEnabled(node["EnableConsole"].as<bool>(false));
            std::vector<std::string> tags;
            if (node["Tags"] && node["Tags"].IsSequence())
            {
                tags = node["Tags"].as<std::vector<std::string>>();
            }
            rhs.SetTags(tags);
            
            std::map<int, std::string> layers;
            if (node["Layers"] && node["Layers"].IsMap())
            {
                for (auto it = node["Layers"].begin(); it != node["Layers"].end(); ++it)
                {
                    try
                    {
                        int idx = std::stoi(it->first.as<std::string>());
                        if (idx >= 0 && idx <= 31)
                        {
                            layers[idx] = it->second.as<std::string>();
                        }
                    }
                    catch (...) { continue; }
                }
            }
            rhs.SetLayers(layers);
            if (node["ScriptDebug"]) {
                const Node& dbg = node["ScriptDebug"];
                rhs.SetScriptDebugEnabled(dbg["Enabled"].as<bool>(false));
                rhs.SetScriptDebugWaitForAttach(dbg["WaitForAttach"].as<bool>(false));
                rhs.SetScriptDebugAddress(dbg["Address"].as<std::string>("127.0.0.1"));
                rhs.SetScriptDebugPort(dbg["Port"].as<int>(56000));
            } else {
                rhs.SetScriptDebugEnabled(false);
                rhs.SetScriptDebugWaitForAttach(false);
                rhs.SetScriptDebugAddress("127.0.0.1");
                rhs.SetScriptDebugPort(56000);
            }
            if (node["Android"])
            {
                const Node& android = node["Android"];
                rhs.SetAndroidPackageName(android["PackageName"].as<std::string>(rhs.GetAndroidPackageName()));
                std::string orientationStr = android["Orientation"].as<std::string>("Portrait");
                rhs.SetAndroidScreenOrientation(StringToOrientation(orientationStr));
                if (android["CompileSdk"]) rhs.SetAndroidCompileSdk(android["CompileSdk"].as<int>(rhs.GetAndroidCompileSdk()));
                if (android["TargetSdk"]) rhs.SetAndroidTargetSdk(android["TargetSdk"].as<int>(rhs.GetAndroidTargetSdk()));
                if (android["MinSdk"]) rhs.SetAndroidMinSdk(android["MinSdk"].as<int>(rhs.GetAndroidMinSdk()));
                if (android["MaxVersion"]) rhs.SetAndroidMaxVersion(android["MaxVersion"].as<int>(rhs.GetAndroidMaxVersion()));
                if (android["MinVersion"]) rhs.SetAndroidMinVersion(android["MinVersion"].as<int>(rhs.GetAndroidMinVersion()));
                if (android["VersionCode"]) rhs.SetAndroidVersionCode(android["VersionCode"].as<int>(rhs.GetAndroidVersionCode()));
                if (android["VersionName"]) rhs.SetAndroidVersionName(android["VersionName"].as<std::string>(rhs.GetAndroidVersionName()));
                if (android["ApkName"]) rhs.SetAndroidApkName(android["ApkName"].as<std::string>(rhs.GetAndroidApkName()));
                if (android["KeystorePath"])
                    rhs.SetAndroidKeystorePath(android["KeystorePath"].as<std::string>());
                if (android["Aliases"])
                {
                    std::vector<AndroidAliasEntry> entries;
                    for (const auto& aliasNode : android["Aliases"])
                    {
                        AndroidAliasEntry entry;
                        entry.alias = aliasNode["Alias"].as<std::string>("");
                        entry.password = aliasNode["Password"].as<std::string>("");
                        if (!entry.alias.empty())
                        {
                            entries.push_back(entry);
                        }
                    }
                    rhs.SetAndroidAliasEntries(entries);
                }
                if (android["AliasEntries"])
                {
                    std::vector<AndroidAliasEntry> entries;
                    for (const auto& aliasNode : android["AliasEntries"])
                    {
                        AndroidAliasEntry entry;
                        entry.alias = aliasNode["Alias"].as<std::string>("");
                        entry.password = aliasNode["Password"].as<std::string>("");
                        if (!entry.alias.empty())
                        {
                            entries.push_back(entry);
                        }
                    }
                    rhs.SetAndroidAliasEntries(entries);
                }
                rhs.SetAndroidKeystorePassword(android["KeystorePassword"].as<std::string>(""));
                rhs.SetAndroidKeyAlias(android["KeyAlias"].as<std::string>(rhs.GetAndroidKeyAlias()));
                rhs.SetAndroidKeyPassword(android["KeyPassword"].as<std::string>(""));
                if (android["UseCustomManifest"])
                {
                    rhs.SetCustomAndroidManifestEnabled(android["UseCustomManifest"].as<bool>(false), false);
                }
                if (android["Icons"])
                {
                    for (auto it = android["Icons"].begin(); it != android["Icons"].end(); ++it)
                    {
                        try
                        {
                            int size = std::stoi(it->first.as<std::string>());
                            rhs.SetAndroidIconPath(size, it->second.as<std::string>());
                        }
                        catch (...)
                        {
                            continue;
                        }
                    }
                }
                if (android["Permissions"])
                {
                    std::vector<std::string> perms;
                    for (const auto& perm : android["Permissions"])
                    {
                        perms.push_back(perm.as<std::string>());
                    }
                    rhs.SetAndroidPermissions(perms);
                }
                if (android["UseCustomGradleProperties"])
                {
                    rhs.SetCustomGradlePropertiesEnabled(android["UseCustomGradleProperties"].as<bool>(false));
                }
            }
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
    EnsureDefaultLayers();
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
        EnsureDefaultLayers();
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

void ProjectSettings::SetLayerName(int layer, const std::string& name)
{
    if (layer < 0 || layer > 31) return;
    if (name.empty())
    {
        m_layers.erase(layer);
    }
    else
    {
        m_layers[layer] = name;
    }
}

const std::string& ProjectSettings::GetLayerName(int layer) const
{
    static const std::string empty;
    auto it = m_layers.find(layer);
    if (it != m_layers.end())
    {
        return it->second;
    }
    return empty;
}

void ProjectSettings::EnsureDefaultLayers()
{
    // 确保默认层存在（类似 Unity）
    auto ensure = [&](int idx, const char* name)
    {
        if (m_layers.find(idx) == m_layers.end())
        {
            m_layers[idx] = name;
        }
    };
    ensure(0, "Default");
    ensure(1, "TransparentFX");
    ensure(2, "IgnoreRaycast");
    ensure(4, "Water");
    ensure(5, "UI");
}
const std::filesystem::path& ProjectSettings::GetAndroidIconPath(int size) const
{
    auto it = m_androidIconPaths.find(size);
    if (it != m_androidIconPaths.end())
    {
        return it->second;
    }
    static const std::filesystem::path emptyPath;
    return emptyPath;
}
void ProjectSettings::SetAndroidIconPath(int size, const std::filesystem::path& path)
{
    if (path.empty())
    {
        m_androidIconPaths.erase(size);
    }
    else
    {
        m_androidIconPaths[size] = path;
    }
}
void ProjectSettings::ClearAndroidIconPath(int size)
{
    m_androidIconPaths.erase(size);
}
void ProjectSettings::SetAndroidPermissions(const std::vector<std::string>& permissions)
{
    m_androidPermissions.clear();
    for (const auto& perm : permissions)
    {
        AddAndroidPermission(perm);
    }
}
void ProjectSettings::AddAndroidPermission(const std::string& permission)
{
    if (permission.empty()) return;
    if (!HasAndroidPermission(permission))
    {
        m_androidPermissions.push_back(permission);
    }
}
void ProjectSettings::RemoveAndroidPermission(const std::string& permission)
{
    if (permission.empty()) return;
    m_androidPermissions.erase(std::remove(m_androidPermissions.begin(), m_androidPermissions.end(), permission),
                               m_androidPermissions.end());
}
bool ProjectSettings::HasAndroidPermission(const std::string& permission) const
{
    return std::find(m_androidPermissions.begin(), m_androidPermissions.end(), permission) != m_androidPermissions.end();
}
std::string ProjectSettings::GenerateAndroidManifest() const
{
    return BuildAndroidManifestTemplate(*this);
}
void ProjectSettings::SetAndroidKeyAlias(const std::string& alias)
{
    m_androidKeyAlias = alias;
    if (alias.empty())
    {
        m_activeAndroidAliasIndex = -1;
        return;
    }
    for (size_t i = 0; i < m_androidAliasEntries.size(); ++i)
    {
        if (m_androidAliasEntries[i].alias == alias)
        {
            m_activeAndroidAliasIndex = static_cast<int>(i);
            return;
        }
    }
    m_activeAndroidAliasIndex = -1;
}
void ProjectSettings::SetAndroidKeyPassword(const std::string& password)
{
    m_androidKeyPassword = password;
    if (m_activeAndroidAliasIndex >= 0 &&
        m_activeAndroidAliasIndex < static_cast<int>(m_androidAliasEntries.size()))
    {
        m_androidAliasEntries[static_cast<size_t>(m_activeAndroidAliasIndex)].password = password;
    }
    else
    {
        for (auto& entry : m_androidAliasEntries)
        {
            if (entry.alias == m_androidKeyAlias)
            {
                entry.password = password;
                break;
            }
        }
    }
}
void ProjectSettings::SetAndroidAliasEntries(const std::vector<AndroidAliasEntry>& entries)
{
    m_androidAliasEntries = entries;
    if (m_androidAliasEntries.empty())
    {
        m_activeAndroidAliasIndex = -1;
    }
    else if (m_activeAndroidAliasIndex >= static_cast<int>(m_androidAliasEntries.size()))
    {
        m_activeAndroidAliasIndex = -1;
    }
    if (!m_androidKeyAlias.empty())
    {
        SetAndroidKeyAlias(m_androidKeyAlias);
    }
}
void ProjectSettings::AddAndroidAliasEntry(const std::string& alias, const std::string& password)
{
    if (alias.empty())
    {
        return;
    }
    auto it = std::find_if(m_androidAliasEntries.begin(), m_androidAliasEntries.end(),
                           [&](const AndroidAliasEntry& entry) { return entry.alias == alias; });
    if (it == m_androidAliasEntries.end())
    {
        m_androidAliasEntries.push_back(AndroidAliasEntry{alias, password});
        m_activeAndroidAliasIndex = static_cast<int>(m_androidAliasEntries.size()) - 1;
    }
    else
    {
        it->password = password;
        m_activeAndroidAliasIndex = static_cast<int>(std::distance(m_androidAliasEntries.begin(), it));
    }
    m_androidKeyAlias = alias;
    m_androidKeyPassword = password;
}
void ProjectSettings::RemoveAndroidAliasEntry(size_t index)
{
    if (index >= m_androidAliasEntries.size())
    {
        return;
    }
    m_androidAliasEntries.erase(m_androidAliasEntries.begin() + static_cast<long long>(index));
    if (m_androidAliasEntries.empty())
    {
        m_activeAndroidAliasIndex = -1;
        m_androidKeyAlias.clear();
        m_androidKeyPassword.clear();
        return;
    }
    if (m_activeAndroidAliasIndex == static_cast<int>(index))
    {
        m_activeAndroidAliasIndex = 0;
        m_androidKeyAlias = m_androidAliasEntries.front().alias;
        m_androidKeyPassword = m_androidAliasEntries.front().password;
    }
    else if (m_activeAndroidAliasIndex > static_cast<int>(index))
    {
        --m_activeAndroidAliasIndex;
        const auto& entry = m_androidAliasEntries[static_cast<size_t>(m_activeAndroidAliasIndex)];
        m_androidKeyAlias = entry.alias;
        m_androidKeyPassword = entry.password;
    }
}
void ProjectSettings::SetActiveAndroidAliasIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(m_androidAliasEntries.size()))
    {
        m_activeAndroidAliasIndex = -1;
        return;
    }
    m_activeAndroidAliasIndex = index;
    const auto& entry = m_androidAliasEntries[static_cast<size_t>(index)];
    m_androidKeyAlias = entry.alias;
    m_androidKeyPassword = entry.password;
}
std::filesystem::path ProjectSettings::GetProjectAndroidDirectory() const
{
    if (m_projectFilePath.empty())
    {
        return {};
    }
    return m_projectFilePath.parent_path() / "Android";
}
std::filesystem::path ProjectSettings::GetCustomAndroidManifestPath() const
{
    auto dir = GetProjectAndroidDirectory();
    if (dir.empty())
    {
        return {};
    }
    return dir / "AndroidManifest.xml";
}
void ProjectSettings::SetCustomAndroidManifestEnabled(bool enabled, bool ensureTemplate)
{
    if (m_useCustomAndroidManifest == enabled)
    {
        if (enabled && ensureTemplate)
        {
            auto manifestPath = GetCustomAndroidManifestPath();
            if (!manifestPath.empty() && !std::filesystem::exists(manifestPath))
            {
                std::error_code ec;
                std::filesystem::create_directories(manifestPath.parent_path(), ec);
                std::ofstream out(manifestPath);
                out << BuildAndroidManifestTemplate(*this);
            }
        }
        return;
    }
    m_useCustomAndroidManifest = enabled;
    if (enabled && ensureTemplate)
    {
        auto manifestPath = GetCustomAndroidManifestPath();
        if (manifestPath.empty()) return;
        std::error_code ec;
        if (!manifestPath.parent_path().empty())
        {
            std::filesystem::create_directories(manifestPath.parent_path(), ec);
        }
        if (!std::filesystem::exists(manifestPath))
        {
            std::ofstream out(manifestPath);
            out << BuildAndroidManifestTemplate(*this);
        }
    }
}
std::filesystem::path ProjectSettings::GetCustomGradlePropertiesPath() const
{
    auto dir = GetProjectAndroidDirectory();
    if (dir.empty())
    {
        return {};
    }
    return dir / "gradle.properties";
}
void ProjectSettings::SetCustomGradlePropertiesEnabled(bool enabled)
{
    if (m_useCustomGradleProperties == enabled)
    {
        if (enabled)
        {
            auto propsPath = GetCustomGradlePropertiesPath();
            if (!propsPath.empty() && !std::filesystem::exists(propsPath))
            {
                std::error_code ec;
                if (!propsPath.parent_path().empty())
                {
                    std::filesystem::create_directories(propsPath.parent_path(), ec);
                }
                std::ofstream out(propsPath);
                out << "# Custom gradle.properties\n";
            }
        }
        return;
    }
    m_useCustomGradleProperties = enabled;
    if (enabled)
    {
        auto propsPath = GetCustomGradlePropertiesPath();
        if (!propsPath.empty() && !std::filesystem::exists(propsPath))
        {
            std::error_code ec;
            if (!propsPath.parent_path().empty())
            {
                std::filesystem::create_directories(propsPath.parent_path(), ec);
            }
            std::ofstream out(propsPath);
            out << "# Custom gradle.properties\n";
        }
    }
}
void ProjectSettings::SetAndroidApkName(const std::string& value)
{
    if (value.empty())
    {
        m_androidApkName = "LumaAndroid";
    }
    else
    {
        m_androidApkName = value;
    }
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

#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H
#include "Utils/PCH.h"
#include "Utils/Guid.h"
#include <filesystem>
#include <map>
#include <vector>
#include <vector>
#include <map>
enum class TargetPlatform
{
    Unknown,
    Windows,
    Linux,
    Android,
    Current 
};
enum class AndroidScreenOrientation
{
    Portrait,
    LandscapeLeft,
    LandscapeRight
};
enum class ViewportScaleMode
{
    None,           
    FixedAspect,    
    FixedWidth,     
    FixedHeight,    
    Expand          
};
enum class ProjectType
{
    Game,           
    Plugin          
};
struct AndroidAliasEntry
{
    std::string alias;
    std::string password;
};
class LUMA_API ProjectSettings : public LazySingleton<ProjectSettings>
{
public:
    friend class LazySingleton<ProjectSettings>;
    void Load();
    void Save();
    void LoadInRuntime();
    void Load(const std::filesystem::path& filePath);
    void Save(const std::filesystem::path& filePath);
    std::filesystem::path GetProjectFilePath() const { return m_projectFilePath; }
    std::filesystem::path GetProjectRoot() const;
    std::filesystem::path GetAssetsDirectory() const;
    bool IsProjectLoaded() const { return !m_projectFilePath.empty(); }
    std::string GetAppName() const { return m_appName; }
    void SetAppName(const std::string& name) { m_appName = name; }
    ProjectType GetProjectType() const { return m_projectType; }
    void SetProjectType(ProjectType type) { m_projectType = type; }
    bool IsPluginProject() const { return m_projectType == ProjectType::Plugin; }
    Guid GetStartScene() const { return m_startScene; }
    void SetStartScene(const Guid& sceneGuid) { m_startScene = sceneGuid; }
    std::filesystem::path GetAppIconPath() const { return m_appIconPath; }
    void SetAppIconPath(const std::filesystem::path& path) { m_appIconPath = path; }
    bool IsFullscreen() const { return m_isFullscreen; }
    void SetFullscreen(bool fullscreen) { m_isFullscreen = fullscreen; }
    int GetTargetWidth() const { return m_targetWidth; }
    void SetTargetWidth(int width) { m_targetWidth = width; }
    int GetTargetHeight() const { return m_targetHeight; }
    void SetTargetHeight(int height) { m_targetHeight = height; }
    ViewportScaleMode GetViewportScaleMode() const { return m_viewportScaleMode; }
    void SetViewportScaleMode(ViewportScaleMode mode) { m_viewportScaleMode = mode; }
    int GetDesignWidth() const { return m_designWidth; }
    void SetDesignWidth(int width) { m_designWidth = width; }
    int GetDesignHeight() const { return m_designHeight; }
    void SetDesignHeight(int height) { m_designHeight = height; }
    bool IsBorderless() const { return m_isBorderless; }
    void SetBorderless(bool borderless) { m_isBorderless = borderless; }
    bool IsConsoleEnabled() const { return m_enableConsole; }
    void SetConsoleEnabled(bool enabled) { m_enableConsole = enabled; }
    TargetPlatform GetTargetPlatform() const { return m_targetPlatform; }
    void SetTargetPlatform(TargetPlatform platform) { m_targetPlatform = platform; }
    static TargetPlatform GetCurrentHostPlatform();
    static std::string PlatformToString(TargetPlatform platform);
    static TargetPlatform StringToPlatform(const std::string& platformStr);
    const std::string& GetAndroidPackageName() const { return m_androidPackageName; }
    void SetAndroidPackageName(const std::string& name) { m_androidPackageName = name; }
    AndroidScreenOrientation GetAndroidScreenOrientation() const { return m_androidScreenOrientation; }
    void SetAndroidScreenOrientation(AndroidScreenOrientation orientation) { m_androidScreenOrientation = orientation; }
    const std::filesystem::path& GetAndroidKeystorePath() const { return m_androidKeystorePath; }
    void SetAndroidKeystorePath(const std::filesystem::path& path) { m_androidKeystorePath = path; }
    const std::string& GetAndroidKeystorePassword() const { return m_androidKeystorePassword; }
    void SetAndroidKeystorePassword(const std::string& password) { m_androidKeystorePassword = password; }
    const std::string& GetAndroidKeyAlias() const { return m_androidKeyAlias; }
    void SetAndroidKeyAlias(const std::string& alias);
    const std::string& GetAndroidKeyPassword() const { return m_androidKeyPassword; }
    void SetAndroidKeyPassword(const std::string& password);
    bool IsCustomAndroidManifestEnabled() const { return m_useCustomAndroidManifest; }
    void SetCustomAndroidManifestEnabled(bool enabled, bool ensureTemplate = true);
    std::filesystem::path GetProjectAndroidDirectory() const;
    std::filesystem::path GetCustomAndroidManifestPath() const;
    const std::filesystem::path& GetAndroidIconPath(int size) const;
    void SetAndroidIconPath(int size, const std::filesystem::path& path);
    void ClearAndroidIconPath(int size);
    const std::map<int, std::filesystem::path>& GetAndroidIconMap() const { return m_androidIconPaths; }
    const std::vector<std::string>& GetAndroidPermissions() const { return m_androidPermissions; }
    void SetAndroidPermissions(const std::vector<std::string>& permissions);
    void AddAndroidPermission(const std::string& permission);
    void RemoveAndroidPermission(const std::string& permission);
    bool HasAndroidPermission(const std::string& permission) const;
    std::string GenerateAndroidManifest() const;
    const std::vector<AndroidAliasEntry>& GetAndroidAliasEntries() const { return m_androidAliasEntries; }
    void SetAndroidAliasEntries(const std::vector<AndroidAliasEntry>& entries);
    void AddAndroidAliasEntry(const std::string& alias, const std::string& password);
    void RemoveAndroidAliasEntry(size_t index);
    int GetActiveAndroidAliasIndex() const { return m_activeAndroidAliasIndex; }
    void SetActiveAndroidAliasIndex(int index);
    int GetAndroidCompileSdk() const { return m_androidCompileSdk; }
    void SetAndroidCompileSdk(int value) { m_androidCompileSdk = std::max(1, value); }
    int GetAndroidTargetSdk() const { return m_androidTargetSdk; }
    void SetAndroidTargetSdk(int value) { m_androidTargetSdk = std::max(1, value); }
    int GetAndroidMinSdk() const { return m_androidMinSdk; }
    void SetAndroidMinSdk(int value) { m_androidMinSdk = std::max(1, value); }
    int GetAndroidMaxVersion() const { return m_androidMaxVersion; }
    void SetAndroidMaxVersion(int value) { m_androidMaxVersion = std::max(1, value); }
    int GetAndroidMinVersion() const { return m_androidMinVersion; }
    void SetAndroidMinVersion(int value) { m_androidMinVersion = std::max(1, value); }
    int GetAndroidVersionCode() const { return m_androidVersionCode; }
    void SetAndroidVersionCode(int value) { m_androidVersionCode = std::max(1, value); }
    const std::string& GetAndroidVersionName() const { return m_androidVersionName; }
    void SetAndroidVersionName(const std::string& value) { m_androidVersionName = value.empty() ? std::string("1.0") : value; }
    bool IsCustomGradlePropertiesEnabled() const { return m_useCustomGradleProperties; }
    void SetCustomGradlePropertiesEnabled(bool enabled);
    std::filesystem::path GetCustomGradlePropertiesPath() const;
    const std::string& GetAndroidApkName() const { return m_androidApkName; }
    void SetAndroidApkName(const std::string& value);
public:
    bool GetScriptDebugEnabled() const { return m_scriptDebugEnabled; }
    void SetScriptDebugEnabled(bool enabled) { m_scriptDebugEnabled = enabled; }
    bool GetScriptDebugWaitForAttach() const { return m_scriptDebugWaitForAttach; }
    void SetScriptDebugWaitForAttach(bool wait) { m_scriptDebugWaitForAttach = wait; }
    const std::string& GetScriptDebugAddress() const { return m_scriptDebugAddress; }
    void SetScriptDebugAddress(const std::string& address) { m_scriptDebugAddress = address; }
    int GetScriptDebugPort() const { return m_scriptDebugPort; }
    void SetScriptDebugPort(int port) { m_scriptDebugPort = port; }
public:
    const std::vector<std::string>& GetTags() const { return m_tags; }
    void SetTags(const std::vector<std::string>& tags) { m_tags = tags; EnsureDefaultTags(); }
    void AddTag(const std::string& tag);
    void RemoveTag(const std::string& tag);
    void EnsureDefaultTags();
private:
    ProjectSettings() = default;
    ~ProjectSettings() override = default;
    void LoadWithCrypto(const std::filesystem::path& filePath);
    std::string m_appName = "Luma Game"; 
    ProjectType m_projectType = ProjectType::Game; 
    Guid m_startScene; 
    std::filesystem::path m_appIconPath; 
    bool m_isFullscreen = false; 
    int m_targetWidth = 1280; 
    int m_targetHeight = 720; 
    ViewportScaleMode m_viewportScaleMode = ViewportScaleMode::None; 
    int m_designWidth = 1920; 
    int m_designHeight = 1080; 
    bool m_isBorderless = false; 
    bool m_enableConsole = false; 
    TargetPlatform m_targetPlatform = TargetPlatform::Current; 
    std::filesystem::path m_projectFilePath; 
    bool m_scriptDebugEnabled = false;
    bool m_scriptDebugWaitForAttach = false;
    std::string m_scriptDebugAddress = "127.0.0.1";
    int m_scriptDebugPort = 56000;
    std::vector<std::string> m_tags;
    std::string m_androidPackageName = "com.lumaengine.game";
    AndroidScreenOrientation m_androidScreenOrientation = AndroidScreenOrientation::Portrait;
    std::filesystem::path m_androidKeystorePath;
    std::string m_androidKeystorePassword;
    std::string m_androidKeyAlias = "luma_key";
    std::string m_androidKeyPassword;
    bool m_useCustomAndroidManifest = false;
    std::map<int, std::filesystem::path> m_androidIconPaths;
    std::vector<std::string> m_androidPermissions = {"android.permission.VIBRATE"};
    std::vector<AndroidAliasEntry> m_androidAliasEntries;
    int m_activeAndroidAliasIndex = -1;
    int m_androidCompileSdk = 36;
    int m_androidTargetSdk = 36;
    int m_androidMinSdk = 28;
    int m_androidMaxVersion = 36;
    int m_androidMinVersion = 28;
    int m_androidVersionCode = 1;
    std::string m_androidVersionName = "1.0";
    bool m_useCustomGradleProperties = false;
    std::string m_androidApkName = "LumaAndroid";
};
#endif

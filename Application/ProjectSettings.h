#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include "Utils/PCH.h"
#include "Utils/Guid.h"
#include <filesystem>
#include <map>
#include <vector>
#include <vector>
#include <map>

// 目标平台枚举
enum class TargetPlatform
{
    Unknown,
    Windows,
    Linux,
    Android,
    Current // 当前宿主平台
};

enum class AndroidScreenOrientation
{
    Portrait,
    LandscapeLeft,
    LandscapeRight
};

/**
 * @brief 视口缩放模式枚举。
 * 
 * 定义渲染时如何处理设计分辨率与实际窗口分辨率的差异。
 */
enum class ViewportScaleMode
{
    None,           ///< 无缩放，直接使用窗口分辨率。
    FixedAspect,    ///< 固定宽高比，保持设计分辨率的宽高比，可能出现黑边。
    FixedWidth,     ///< 固定宽度，高度根据窗口比例调整。
    FixedHeight,    ///< 固定高度，宽度根据窗口比例调整。
    Expand          ///< 扩展模式，填充整个窗口，可能拉伸。
};

struct AndroidAliasEntry
{
    std::string alias;
    std::string password;
};

/**
 * @brief 项目设置类，用于管理和存储应用程序及项目的各种配置。
 *
 * 这是一个单例类，确保在整个应用程序中只有一个项目设置实例。
 */
class LUMA_API ProjectSettings : public LazySingleton<ProjectSettings>
{
public:
    friend class LazySingleton<ProjectSettings>;

    /**
     * @brief 加载项目设置。
     *
     * 从默认的项目文件路径加载设置。
     */
    void Load();

    /**
     * @brief 保存项目设置。
     *
     * 将当前设置保存到默认的项目文件路径。
     */
    void Save();

    /**
     * @brief 在运行时加载项目设置。
     *
     * 专门用于运行时环境加载项目设置。
     */
    void LoadInRuntime();

    /**
     * @brief 从指定文件路径加载项目设置。
     * @param filePath 项目设置文件的路径。
     */
    void Load(const std::filesystem::path& filePath);

    /**
     * @brief 将项目设置保存到指定文件路径。
     * @param filePath 项目设置文件的路径。
     */
    void Save(const std::filesystem::path& filePath);

    /**
     * @brief 获取当前项目文件的路径。
     * @return 项目文件的路径。
     */
    std::filesystem::path GetProjectFilePath() const { return m_projectFilePath; }

    /**
     * @brief 获取项目根目录的路径。
     * @return 项目根目录的路径。
     */
    std::filesystem::path GetProjectRoot() const;

    /**
     * @brief 获取资产目录的路径。
     * @return 资产目录的路径。
     */
    std::filesystem::path GetAssetsDirectory() const;

    /**
     * @brief 检查项目是否已加载。
     * @return 如果项目已加载则返回 true，否则返回 false。
     */
    bool IsProjectLoaded() const { return !m_projectFilePath.empty(); }

    /**
     * @brief 获取应用程序名称。
     * @return 应用程序的名称。
     */
    std::string GetAppName() const { return m_appName; }
    /**
     * @brief 设置应用程序名称。
     * @param name 要设置的应用程序名称。
     */
    void SetAppName(const std::string& name) { m_appName = name; }

    /**
     * @brief 获取启动场景的全局唯一标识符 (GUID)。
     * @return 启动场景的 GUID。
     */
    Guid GetStartScene() const { return m_startScene; }
    /**
     * @brief 设置启动场景的全局唯一标识符 (GUID)。
     * @param sceneGuid 要设置的启动场景 GUID。
     */
    void SetStartScene(const Guid& sceneGuid) { m_startScene = sceneGuid; }

    /**
     * @brief 获取应用程序图标的路径。
     * @return 应用程序图标的路径。
     */
    std::filesystem::path GetAppIconPath() const { return m_appIconPath; }
    /**
     * @brief 设置应用程序图标的路径。
     * @param path 要设置的应用程序图标路径。
     */
    void SetAppIconPath(const std::filesystem::path& path) { m_appIconPath = path; }

    /**
     * @brief 检查应用程序是否处于全屏模式。
     * @return 如果处于全屏模式则返回 true，否则返回 false。
     */
    bool IsFullscreen() const { return m_isFullscreen; }
    /**
     * @brief 设置应用程序是否处于全屏模式。
     * @param fullscreen 如果为 true 则设置为全屏，否则设置为窗口模式。
     */
    void SetFullscreen(bool fullscreen) { m_isFullscreen = fullscreen; }

    /**
     * @brief 获取目标窗口宽度。
     * @return 目标窗口宽度。
     */
    int GetTargetWidth() const { return m_targetWidth; }
    /**
     * @brief 设置目标窗口宽度。
     * @param width 要设置的目标窗口宽度。
     */
    void SetTargetWidth(int width) { m_targetWidth = width; }

    /**
     * @brief 获取目标窗口高度。
     * @return 目标窗口高度。
     */
    int GetTargetHeight() const { return m_targetHeight; }
    /**
     * @brief 设置目标窗口高度。
     * @param height 要设置的目标窗口高度。
     */
    void SetTargetHeight(int height) { m_targetHeight = height; }

    /**
     * @brief 获取视口缩放模式。
     * @return 当前的视口缩放模式。
     */
    ViewportScaleMode GetViewportScaleMode() const { return m_viewportScaleMode; }
    /**
     * @brief 设置视口缩放模式。
     * @param mode 要设置的视口缩放模式。
     */
    void SetViewportScaleMode(ViewportScaleMode mode) { m_viewportScaleMode = mode; }

    /**
     * @brief 获取设计画布宽度（逻辑分辨率）。
     * @return 设计画布宽度。
     */
    int GetDesignWidth() const { return m_designWidth; }
    /**
     * @brief 设置设计画布宽度（逻辑分辨率）。
     * @param width 要设置的设计画布宽度。
     */
    void SetDesignWidth(int width) { m_designWidth = width; }

    /**
     * @brief 获取设计画布高度（逻辑分辨率）。
     * @return 设计画布高度。
     */
    int GetDesignHeight() const { return m_designHeight; }
    /**
     * @brief 设置设计画布高度（逻辑分辨率）。
     * @param height 要设置的设计画布高度。
     */
    void SetDesignHeight(int height) { m_designHeight = height; }

    /**
     * @brief 检查窗口是否无边框。
     * @return 如果窗口无边框则返回 true，否则返回 false。
     */
    bool IsBorderless() const { return m_isBorderless; }
    /**
     * @brief 设置窗口是否无边框。
     * @param borderless 如果为 true 则设置为无边框，否则有边框。
     */
    void SetBorderless(bool borderless) { m_isBorderless = borderless; }

    /**
     * @brief 检查控制台是否启用。
     * @return 如果控制台启用则返回 true，否则返回 false。
     */
    bool IsConsoleEnabled() const { return m_enableConsole; }
    /**
     * @brief 设置控制台是否启用。
     * @param enabled 如果为 true 则启用控制台，否则禁用。
     */
    void SetConsoleEnabled(bool enabled) { m_enableConsole = enabled; }

    /**
     * @brief 获取目标构建平台。
     * @return 当前设置的目标构建平台。
     */
    TargetPlatform GetTargetPlatform() const { return m_targetPlatform; }

    /**
     * @brief 设置目标构建平台。
     * @param platform 要设置的目标构建平台。
     */
    void SetTargetPlatform(TargetPlatform platform) { m_targetPlatform = platform; }

    /**
     * @brief 获取当前宿主平台。
     * @return 当前运行的宿主平台。
     */
    static TargetPlatform GetCurrentHostPlatform();

    /**
     * @brief 将目标平台枚举转换为字符串。
     * @param platform 要转换的目标平台枚举。
     * @return 对应的平台字符串名称。
     */
    static std::string PlatformToString(TargetPlatform platform);

    /**
     * @brief 将字符串转换为目标平台枚举。
     * @param platformStr 平台字符串名称。
     * @return 对应的目标平台枚举。
     */
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

    // --------------- Scripting Debugging ---------------
public:
    bool GetScriptDebugEnabled() const { return m_scriptDebugEnabled; }
    void SetScriptDebugEnabled(bool enabled) { m_scriptDebugEnabled = enabled; }

    bool GetScriptDebugWaitForAttach() const { return m_scriptDebugWaitForAttach; }
    void SetScriptDebugWaitForAttach(bool wait) { m_scriptDebugWaitForAttach = wait; }

    const std::string& GetScriptDebugAddress() const { return m_scriptDebugAddress; }
    void SetScriptDebugAddress(const std::string& address) { m_scriptDebugAddress = address; }

    int GetScriptDebugPort() const { return m_scriptDebugPort; }
    void SetScriptDebugPort(int port) { m_scriptDebugPort = port; }

    // ---------------- Tags ----------------
public:
    const std::vector<std::string>& GetTags() const { return m_tags; }
    void SetTags(const std::vector<std::string>& tags) { m_tags = tags; EnsureDefaultTags(); }
    void AddTag(const std::string& tag);
    void RemoveTag(const std::string& tag);
    void EnsureDefaultTags();

private:
    ProjectSettings() = default;
    ~ProjectSettings() override = default;
    // 私有方法，不进行Doxygen注释，因为它不是公共接口的一部分。
    void LoadWithCrypto(const std::filesystem::path& filePath);

    std::string m_appName = "Luma Game"; ///< 应用程序名称。
    Guid m_startScene; ///< 启动场景的全局唯一标识符。
    std::filesystem::path m_appIconPath; ///< 应用程序图标的路径。
    bool m_isFullscreen = false; ///< 指示应用程序是否处于全屏模式。

    int m_targetWidth = 1280; ///< 目标窗口宽度。
    int m_targetHeight = 720; ///< 目标窗口高度。
    ViewportScaleMode m_viewportScaleMode = ViewportScaleMode::None; ///< 视口缩放模式。
    int m_designWidth = 1920; ///< 设计画布宽度（逻辑分辨率）。
    int m_designHeight = 1080; ///< 设计画布高度（逻辑分辨率）。
    bool m_isBorderless = false; ///< 指示窗口是否无边框。
    bool m_enableConsole = false; ///< 指示控制台是否启用。

    TargetPlatform m_targetPlatform = TargetPlatform::Current; ///< 目标构建平台。

    std::filesystem::path m_projectFilePath; ///< 当前项目文件的路径。

    // Scripting debugging options
    bool m_scriptDebugEnabled = false;
    bool m_scriptDebugWaitForAttach = false;
    std::string m_scriptDebugAddress = "127.0.0.1";
    int m_scriptDebugPort = 56000;

    // Project-wide tags used by TagComponent dropdown in Inspector
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

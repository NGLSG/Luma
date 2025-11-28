#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include "Utils/LazySingleton.h"

/**
 * @brief 插件信息结构体
 */
struct PluginInfo
{
    std::string id;                    ///< 插件唯一标识
    std::string name;                  ///< 插件显示名称
    std::string version;               ///< 版本号
    std::string author;                ///< 作者
    std::string description;           ///< 描述
    std::filesystem::path dllPath;     ///< DLL 路径
    std::filesystem::path pluginRoot;  ///< 插件根目录
    bool enabled = true;               ///< 是否启用
    bool loaded = false;               ///< 是否已加载
    void* runtimeHandle = nullptr;     ///< 运行时句柄 (C# AssemblyLoadContext)
};

/**
 * @brief 插件配置结构体
 */
struct PluginConfig
{
    std::string pluginId;
    bool enabled = true;
};

/**
 * @brief 插件管理器，负责加载、卸载和管理所有插件
 */
class PluginManager : public LazySingleton<PluginManager>
{
public:
    friend class LazySingleton<PluginManager>;

    /**
     * @brief 初始化插件管理器
     * @param pluginsRoot 插件根目录路径
     */
    void Initialize(const std::filesystem::path& pluginsRoot);

    /**
     * @brief 关闭插件管理器，卸载所有插件
     */
    void Shutdown();

    /**
     * @brief 扫描并发现所有插件
     */
    void ScanPlugins();

    /**
     * @brief 加载指定插件
     * @param pluginId 插件ID
     * @return 是否加载成功
     */
    bool LoadPlugin(const std::string& pluginId);

    /**
     * @brief 卸载指定插件
     * @param pluginId 插件ID
     * @return 是否卸载成功
     */
    bool UnloadPlugin(const std::string& pluginId);

    /**
     * @brief 启用/禁用插件
     * @param pluginId 插件ID
     * @param enabled 是否启用
     */
    void SetPluginEnabled(const std::string& pluginId, bool enabled);

    /**
     * @brief 加载所有已启用的插件
     */
    void LoadEnabledPlugins();

    /**
     * @brief 导入插件包 (.lplug)
     * @param pluginPackagePath 插件包路径
     * @return 是否导入成功
     */
    bool ImportPlugin(const std::filesystem::path& pluginPackagePath);

    /**
     * @brief 删除插件
     * @param pluginId 插件ID
     * @return 是否删除成功
     */
    bool RemovePlugin(const std::string& pluginId);

    /**
     * @brief 获取所有已发现的插件信息
     */
    const std::vector<PluginInfo>& GetAllPlugins() const { return m_plugins; }

    /**
     * @brief 获取指定插件信息
     * @param pluginId 插件ID
     * @return 插件信息指针，不存在则返回nullptr
     */
    PluginInfo* GetPlugin(const std::string& pluginId);

    /**
     * @brief 获取插件根目录
     */
    const std::filesystem::path& GetPluginsRoot() const { return m_pluginsRoot; }

    /**
     * @brief 保存插件配置
     */
    void SaveConfig();

    /**
     * @brief 加载插件配置
     */
    void LoadConfig();

    /**
     * @brief 调用所有已加载插件的 OnEditorUpdate
     */
    void UpdateEditorPlugins(float deltaTime);

    /**
     * @brief 调用所有已加载插件的 OnEditorGUI (用于绘制自定义面板)
     */
    void DrawEditorPluginPanels();
    void DrawEditorPluginMenuBar();

    /**
     * @brief 绘制指定菜单的插件扩展项
     * @param menuName 菜单名称 (如 "文件", "项目", "窗口" 等)
     */
    void DrawPluginMenuItems(const std::string& menuName);

private:
    PluginManager() = default;
    ~PluginManager() override = default;

    /**
     * @brief 解析插件目录中的 plugin.json
     * @param pluginDir 插件目录
     * @return 解析的插件信息，失败返回空
     */
    std::optional<PluginInfo> ParsePluginManifest(const std::filesystem::path& pluginDir);

    std::filesystem::path m_pluginsRoot;                      ///< 插件根目录
    std::vector<PluginInfo> m_plugins;                        ///< 所有已发现的插件
    std::unordered_map<std::string, PluginConfig> m_configs;  ///< 插件配置映射
    bool m_initialized = false;
};

#endif // PLUGINMANAGER_H

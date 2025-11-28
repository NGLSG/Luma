#include "PluginManager.h"
#include "Utils/Logger.h"
#ifdef __ANDROID__
#include "Scripting/MonoHost.h"
#else
#include "Scripting/CoreCLRHost.h"
#endif
#include <fstream>
#include <cstdlib>
#include <format>
#include <vector>
#include <yaml-cpp/yaml.h>

void PluginManager::Initialize(const std::filesystem::path& pluginsRoot)
{
    if (m_initialized)
    {
        LogWarn("PluginManager 已经初始化");
        return;
    }

    m_pluginsRoot = pluginsRoot;

    if (!std::filesystem::exists(m_pluginsRoot))
    {
        std::filesystem::create_directories(m_pluginsRoot);
        LogInfo("创建插件目录: {}", m_pluginsRoot.string());
    }

    
    std::filesystem::path engineRoot = std::filesystem::current_path();
    std::filesystem::path sdkDir = m_pluginsRoot / "SDK";

    if (!std::filesystem::exists(sdkDir / "Luma.SDK.dll"))
    {
#ifdef _WIN32
        std::filesystem::path toolsDir = engineRoot / "Tools" / "Windows";
#elif defined(__ANDROID__)
        std::filesystem::path toolsDir = engineRoot / "Tools" / "Android";
#elif defined(__linux__)
        std::filesystem::path toolsDir = engineRoot / "Tools" / "Linux";
#else
        std::filesystem::path toolsDir = engineRoot / "Tools" / "Linux";
#endif

        if (std::filesystem::exists(toolsDir / "Luma.SDK.dll"))
        {
            std::filesystem::create_directories(sdkDir);

            const std::vector<std::string> sdkFiles = {
                "Luma.SDK.dll",
                "Luma.SDK.deps.json",
                "Luma.SDK.runtimeconfig.json",
                "YamlDotNet.dll"
            };

            for (const auto& fileName : sdkFiles)
            {
                std::filesystem::path srcFile = toolsDir / fileName;
                if (std::filesystem::exists(srcFile))
                {
                    try
                    {
                        std::filesystem::copy_file(srcFile, sdkDir / fileName,
                                                   std::filesystem::copy_options::overwrite_existing);
                    }
                    catch (const std::exception& e)
                    {
                        LogError("复制 SDK 文件失败: {} - {}", fileName, e.what());
                    }
                }
            }
            LogInfo("已自动复制 SDK 到 Plugins/SDK");
        }
        else
        {
            LogWarn("Tools 目录中未找到 SDK 文件");
        }
    }

    LoadConfig();
    ScanPlugins();

#ifdef __ANDROID__
    MonoHost::CreateNewPluginInstance();
    auto* pluginHost = MonoHost::GetPluginInstance();
#else
    CoreCLRHost::CreateNewPluginInstance();
    auto* pluginHost = CoreCLRHost::GetPluginInstance();
#endif
    if (pluginHost)
    {
        auto sdkPath = sdkDir / "Luma.SDK.dll";

        if (std::filesystem::exists(sdkPath))
        {
            if (pluginHost->Initialize(sdkPath, true))
            {
                LogInfo("插件宿主初始化成功");
            }
            else
            {
                LogError("插件宿主初始化失败");
            }
        }
        else
        {
            LogError("找不到 SDK 程序集: {}", sdkPath.string());
        }
    }

    m_initialized = true;

    LogInfo("PluginManager 初始化完成，发现 {} 个插件", m_plugins.size());


    LoadEnabledPlugins();
}

void PluginManager::Shutdown()
{
    if (!m_initialized) return;


    for (auto& plugin : m_plugins)
    {
        if (plugin.loaded)
        {
            UnloadPlugin(plugin.id);
        }
    }

#ifdef __ANDROID__
    MonoHost::DestroyPluginInstance();
#else
    CoreCLRHost::DestroyPluginInstance();
#endif

    SaveConfig();
    m_plugins.clear();
    m_configs.clear();
    m_initialized = false;

    LogInfo("PluginManager 已关闭");
}

void PluginManager::ScanPlugins()
{
    m_plugins.clear();

    if (!std::filesystem::exists(m_pluginsRoot))
    {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_pluginsRoot))
    {
        if (!entry.is_directory()) continue;

        auto pluginInfo = ParsePluginManifest(entry.path());
        if (pluginInfo.has_value())
        {

            auto configIt = m_configs.find(pluginInfo->id);
            if (configIt != m_configs.end())
            {
                pluginInfo->enabled = configIt->second.enabled;
            }

            m_plugins.push_back(std::move(pluginInfo.value()));
            LogInfo("发现插件: {} v{}", m_plugins.back().name, m_plugins.back().version);
        }
    }
}

std::optional<PluginInfo> PluginManager::ParsePluginManifest(const std::filesystem::path& pluginDir)
{
    std::filesystem::path manifestPath = pluginDir / "plugin.json";
    if (!std::filesystem::exists(manifestPath))
    {

        manifestPath = pluginDir / "plugin.yaml";
        if (!std::filesystem::exists(manifestPath))
        {
            return std::nullopt;
        }
    }

    try
    {
        YAML::Node manifest = YAML::LoadFile(manifestPath.string());

        PluginInfo info;
        info.id = manifest["id"].as<std::string>(pluginDir.filename().string());
        info.name = manifest["name"].as<std::string>(info.id);
        info.version = manifest["version"].as<std::string>("1.0.0");
        info.author = manifest["author"].as<std::string>("Unknown");
        info.description = manifest["description"].as<std::string>("");
        info.pluginRoot = pluginDir;


        std::string dllName = manifest["dll"].as<std::string>(info.id + ".dll");
        info.dllPath = pluginDir / dllName;

        if (!std::filesystem::exists(info.dllPath))
        {
            LogWarn("插件 {} 的 DLL 不存在: {}", info.id, info.dllPath.string());
            return std::nullopt;
        }

        return info;
    }
    catch (const std::exception& e)
    {
        LogError("解析插件清单失败 {}: {}", manifestPath.string(), e.what());
        return std::nullopt;
    }
}

bool PluginManager::LoadPlugin(const std::string& pluginId)
{
    auto* plugin = GetPlugin(pluginId);
    if (!plugin)
    {
        LogError("插件不存在: {}", pluginId);
        return false;
    }

    if (plugin->loaded)
    {
        LogWarn("插件已加载: {}", pluginId);
        return true;
    }

    if (!plugin->enabled)
    {
        LogWarn("插件未启用: {}", pluginId);
        return false;
    }

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (!host)
    {
        LogError("插件宿主未初始化");
        return false;
    }

    auto loadFn = host->GetPluginLoadFn();
    if (!loadFn)
    {
        LogError("插件加载函数不可用");
        return false;
    }

    std::string dllPath = plugin->dllPath.string();
    uint8_t result = loadFn(dllPath.c_str(), pluginId.c_str());
    if (result == 0)
    {
        LogError("加载插件失败: {}", pluginId);
        return false;
    }

    LogInfo("加载插件: {} ({})", plugin->name, plugin->dllPath.string());
    plugin->loaded = true;
    return true;
}

bool PluginManager::UnloadPlugin(const std::string& pluginId)
{
    auto* plugin = GetPlugin(pluginId);
    if (!plugin)
    {
        LogError("插件不存在: {}", pluginId);
        return false;
    }

    if (!plugin->loaded)
    {
        return true;
    }

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (host)
    {
        auto unloadFn = host->GetPluginUnloadFn();
        if (unloadFn)
        {
            unloadFn(pluginId.c_str());
        }
    }

    LogInfo("卸载插件: {}", plugin->name);
    plugin->loaded = false;
    plugin->runtimeHandle = nullptr;
    return true;
}

void PluginManager::SetPluginEnabled(const std::string& pluginId, bool enabled)
{
    auto* plugin = GetPlugin(pluginId);
    if (!plugin) return;

    plugin->enabled = enabled;
    m_configs[pluginId] = {pluginId, enabled};

    if (!enabled && plugin->loaded)
    {
        UnloadPlugin(pluginId);
    }

    SaveConfig();
}

void PluginManager::LoadEnabledPlugins()
{
    for (const auto& plugin : m_plugins)
    {
        if (plugin.enabled && !plugin.loaded)
        {
            LoadPlugin(plugin.id);
        }
    }
}

bool PluginManager::ImportPlugin(const std::filesystem::path& pluginPackagePath)
{
    if (!std::filesystem::exists(pluginPackagePath))
    {
        LogError("插件包不存在: {}", pluginPackagePath.string());
        return false;
    }

    if (pluginPackagePath.extension() != ".lplug")
    {
        LogError("无效的插件包格式，需要 .lplug 文件");
        return false;
    }



    std::string pluginDirName = pluginPackagePath.stem().string();
    std::filesystem::path targetDir = m_pluginsRoot / pluginDirName;


    if (std::filesystem::exists(targetDir))
    {
        LogWarn("插件目录已存在，将覆盖: {}", targetDir.string());
        std::filesystem::remove_all(targetDir);
    }

    std::filesystem::create_directories(targetDir);

#ifdef _WIN32

    std::filesystem::path tempZipPath = targetDir.parent_path() / (pluginDirName + "_temp.zip");
    try
    {
        std::filesystem::copy_file(pluginPackagePath, tempZipPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::exception& e)
    {
        LogError("复制插件包失败: {}", e.what());
        return false;
    }

    std::string cmd = std::format(
        "powershell -Command \"Expand-Archive -Path '{}' -DestinationPath '{}' -Force\"",
        tempZipPath.string(),
        targetDir.string()
    );
    int result = std::system(cmd.c_str());


    std::filesystem::remove(tempZipPath);

    if (result != 0)
    {
        LogError("解压插件包失败: {}", pluginPackagePath.string());
        std::filesystem::remove_all(targetDir);
        return false;
    }
#else

    std::string cmd = std::format(
        "unzip -o '{}' -d '{}'",
        pluginPackagePath.string(),
        targetDir.string()
    );
    int result = std::system(cmd.c_str());
    if (result != 0)
    {
        LogError("解压插件包失败: {}", pluginPackagePath.string());
        std::filesystem::remove_all(targetDir);
        return false;
    }
#endif

    LogInfo("导入插件包成功: {} -> {}", pluginPackagePath.string(), targetDir.string());


    ScanPlugins();
    return true;
}

bool PluginManager::RemovePlugin(const std::string& pluginId)
{
    auto* plugin = GetPlugin(pluginId);
    if (!plugin)
    {
        LogError("插件不存在: {}", pluginId);
        return false;
    }


    if (plugin->loaded)
    {
        UnloadPlugin(pluginId);
    }


    try
    {
        std::filesystem::remove_all(plugin->pluginRoot);
        LogInfo("已删除插件: {}", pluginId);
    }
    catch (const std::exception& e)
    {
        LogError("删除插件失败: {}", e.what());
        return false;
    }


    m_configs.erase(pluginId);
    SaveConfig();


    ScanPlugins();
    return true;
}

PluginInfo* PluginManager::GetPlugin(const std::string& pluginId)
{
    for (auto& plugin : m_plugins)
    {
        if (plugin.id == pluginId)
        {
            return &plugin;
        }
    }
    return nullptr;
}

void PluginManager::SaveConfig()
{
    std::filesystem::path configPath = m_pluginsRoot / "plugins.yaml";

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "plugins" << YAML::Value << YAML::BeginSeq;

    for (const auto& [id, config] : m_configs)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << config.pluginId;
        out << YAML::Key << "enabled" << YAML::Value << config.enabled;
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream file(configPath);
    if (file.is_open())
    {
        file << out.c_str();
        file.close();
    }
}

void PluginManager::LoadConfig()
{
    std::filesystem::path configPath = m_pluginsRoot / "plugins.yaml";
    if (!std::filesystem::exists(configPath))
    {
        return;
    }

    try
    {
        YAML::Node config = YAML::LoadFile(configPath.string());
        if (config["plugins"])
        {
            for (const auto& pluginNode : config["plugins"])
            {
                PluginConfig cfg;
                cfg.pluginId = pluginNode["id"].as<std::string>();
                cfg.enabled = pluginNode["enabled"].as<bool>(true);
                m_configs[cfg.pluginId] = cfg;
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("加载插件配置失败: {}", e.what());
    }
}

void PluginManager::UpdateEditorPlugins(float deltaTime)
{
    if (!m_initialized) return;

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (!host) return;

    auto updateFn = host->GetPluginUpdateEditorFn();
    if (updateFn)
    {
        updateFn(deltaTime);
    }
}

void PluginManager::DrawEditorPluginPanels()
{
    if (!m_initialized) return;

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (!host) return;

    auto drawFn = host->GetPluginDrawPanelsFn();
    if (drawFn)
    {
        drawFn();
    }
}

void PluginManager::DrawEditorPluginMenuBar()
{
    if (!m_initialized) return;

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (!host) return;

    auto drawFn = host->GetPluginDrawMenuBarFn();
    if (drawFn)
    {
        drawFn();
    }
}

void PluginManager::DrawPluginMenuItems(const std::string& menuName)
{
    if (!m_initialized) return;

#ifdef __ANDROID__
    auto* host = MonoHost::GetPluginInstance();
#else
    auto* host = CoreCLRHost::GetPluginInstance();
#endif
    if (!host) return;

    auto drawFn = host->GetPluginDrawMenuItemsFn();
    if (drawFn)
    {
        drawFn(menuName.c_str());
    }
}

#include "PluginManagerPanel.h"
#include "Plugins/PluginManager.h"
#include "Utils/Logger.h"
#include "Utils/PopupManager.h"
#include "EditorContext.h"
#include "Application/Editor.h"
#include "Application/Window.h"
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <fstream>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <format>
#include <chrono>
#include <thread>

static PluginManagerPanel* s_currentPanel = nullptr;

static void SDLCALL OnImportFileDialogCallback(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0] && s_currentPanel)
    {
        s_currentPanel->OnImportFileSelected(std::filesystem::path(filelist[0]));
    }
}

static void SDLCALL OnPublishDirDialogCallback(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0] && s_currentPanel)
    {
        s_currentPanel->OnPublishDirSelected(std::filesystem::path(filelist[0]));
    }
}

static void SDLCALL OnPluginProjectDirDialogCallback(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0] && s_currentPanel)
    {
        s_currentPanel->OnPluginProjectDirSelected(std::filesystem::path(filelist[0]));
    }
}

void PluginManagerPanel::Initialize(EditorContext* context)
{
    m_context = context;
    s_currentPanel = this;
    m_isVisible = false; 

    PopupManager::GetInstance().Register("ImportPluginPopup", [this]()
    {
        ImGui::Text("导入插件");
        ImGui::TextDisabled("支持 .lplug 文件或插件文件夹");
        ImGui::Separator();
        ImGui::InputText("路径", m_importPathBuffer, sizeof(m_importPathBuffer));
        ImGui::SameLine();
        if (ImGui::Button("选择文件..."))
        {
            if (SDL_Window* window = getSDLWindow())
            {
                const SDL_DialogFileFilter filters[] = {{"Luma Plugin", "lplug"}};
                SDL_ShowOpenFileDialog(OnImportFileDialogCallback, this, window, filters, 1, nullptr, false);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("选择文件夹..."))
        {
            if (SDL_Window* window = getSDLWindow())
            {
                SDL_ShowOpenFolderDialog(OnImportFileDialogCallback, this, window, nullptr, false);
            }
        }
        ImGui::Separator();

        if (ImGui::Button("导入", ImVec2(120, 0)))
        {
            if (strlen(m_importPathBuffer) > 0)
            {
                std::filesystem::path path(m_importPathBuffer);
                if (std::filesystem::is_directory(path))
                {
                    
                    std::filesystem::path destPath = PluginManager::GetInstance().GetPluginsRoot() / path.filename();
                    try
                    {
                        std::filesystem::copy(path, destPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                        PluginManager::GetInstance().ScanPlugins();
                        LogInfo("从文件夹导入插件成功: {}", path.string());
                    }
                    catch (const std::exception& e)
                    {
                        LogError("导入插件失败: {}", e.what());
                    }
                }
                else
                {
                    PluginManager::GetInstance().ImportPlugin(path);
                }
                memset(m_importPathBuffer, 0, sizeof(m_importPathBuffer));
            }
            PopupManager::GetInstance().Close("ImportPluginPopup");
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0)))
        {
            PopupManager::GetInstance().Close("ImportPluginPopup");
        }
    }, true, ImGuiWindowFlags_AlwaysAutoResize);

    
    PopupManager::GetInstance().Register("PublishPluginPopup", [this]()
    {
        drawPublishPopup();
    }, true, ImGuiWindowFlags_AlwaysAutoResize);

    PopupManager::GetInstance().Register("ConfirmRemovePlugin", [this]()
    {
        const auto& plugins = PluginManager::GetInstance().GetAllPlugins();
        if (m_selectedPluginIndex >= 0 && m_selectedPluginIndex < static_cast<int>(plugins.size()))
        {
            const auto& plugin = plugins[m_selectedPluginIndex];
            ImGui::Text("确定要删除插件 \"%s\" 吗？", plugin.name.c_str());
            ImGui::Text("此操作将删除插件目录及所有相关文件。");
            ImGui::Separator();

            if (ImGui::Button("删除", ImVec2(120, 0)))
            {
                PluginManager::GetInstance().RemovePlugin(plugin.id);
                m_selectedPluginIndex = -1;
                PopupManager::GetInstance().Close("ConfirmRemovePlugin");
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120, 0)))
            {
                PopupManager::GetInstance().Close("ConfirmRemovePlugin");
            }
        }
        else
        {
            ImGui::Text("没有选中的插件");
            if (ImGui::Button("关闭", ImVec2(120, 0)))
            {
                PopupManager::GetInstance().Close("ConfirmRemovePlugin");
            }
        }
    }, true, ImGuiWindowFlags_AlwaysAutoResize);
}

void PluginManagerPanel::Update(float deltaTime)
{
    
}

void PluginManagerPanel::Draw()
{
    if (!m_isVisible) return;

    ImGui::Begin(GetPanelName(), &m_isVisible);

    
    if (ImGui::Button("扫描插件"))
    {
        PluginManager::GetInstance().ScanPlugins();
    }
    ImGui::SameLine();
    if (ImGui::Button("导入插件"))
    {
        PopupManager::GetInstance().Open("ImportPluginPopup");
    }
    ImGui::SameLine();
    if (ImGui::Button("发布插件"))
    {
        
        memset(m_publishPluginDir, 0, sizeof(m_publishPluginDir));
        memset(m_publishOutputDir, 0, sizeof(m_publishOutputDir));
        m_publishPluginId.clear();
        PopupManager::GetInstance().Open("PublishPluginPopup");
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("选择插件项目目录，打包为 .lplug 文件");
    }
    ImGui::SameLine();
    if (ImGui::Button("刷新"))
    {
        PluginManager::GetInstance().ScanPlugins();
    }

    ImGui::Separator();

    
    float availWidth = ImGui::GetContentRegionAvail().x;
    float listWidth = availWidth * 0.4f;

    
    ImGui::BeginChild("PluginList", ImVec2(listWidth, 0), true);
    drawPluginList();
    ImGui::EndChild();

    ImGui::SameLine();

    
    ImGui::BeginChild("PluginDetails", ImVec2(0, 0), true);
    drawPluginDetails(m_selectedPluginIndex);
    ImGui::EndChild();

    ImGui::End();
}

void PluginManagerPanel::Shutdown()
{
    
}

void PluginManagerPanel::drawPluginList()
{
    const auto& plugins = PluginManager::GetInstance().GetAllPlugins();

    if (plugins.empty())
    {
        ImGui::TextDisabled("没有发现插件");
        ImGui::TextDisabled("将插件放入 Plugins 目录");
        return;
    }

    for (int i = 0; i < static_cast<int>(plugins.size()); ++i)
    {
        const auto& plugin = plugins[i];

        ImGui::PushID(i);

        
        if (plugin.loaded)
        {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "●");
        }
        else if (plugin.enabled)
        {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "●");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "●");
        }
        ImGui::SameLine();

        
        bool isSelected = (m_selectedPluginIndex == i);
        if (ImGui::Selectable(plugin.name.c_str(), isSelected))
        {
            m_selectedPluginIndex = i;
        }

        ImGui::PopID();
    }
}

void PluginManagerPanel::drawPluginDetails(int selectedIndex)
{
    const auto& plugins = PluginManager::GetInstance().GetAllPlugins();

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(plugins.size()))
    {
        ImGui::TextDisabled("选择一个插件查看详情");
        return;
    }

    const auto& plugin = plugins[selectedIndex];

    
    ImGui::Text("插件名称: %s", plugin.name.c_str());
    ImGui::Text("插件 ID: %s", plugin.id.c_str());
    ImGui::Text("版本: %s", plugin.version.c_str());
    ImGui::Text("作者: %s", plugin.author.c_str());

    ImGui::Separator();

    if (!plugin.description.empty())
    {
        ImGui::TextWrapped("描述: %s", plugin.description.c_str());
        ImGui::Separator();
    }

    ImGui::Text("DLL 路径: %s", plugin.dllPath.string().c_str());
    ImGui::Text("插件目录: %s", plugin.pluginRoot.string().c_str());

    ImGui::Separator();

    
    ImGui::Text("状态: ");
    ImGui::SameLine();
    if (plugin.loaded)
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "已加载");
    }
    else if (plugin.enabled)
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "已启用 (未加载)");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "已禁用");
    }

    ImGui::Separator();

    
    bool enabled = plugin.enabled;
    if (ImGui::Checkbox("启用", &enabled))
    {
        PluginManager::GetInstance().SetPluginEnabled(plugin.id, enabled);
    }

    ImGui::Spacing();

    if (plugin.enabled && !plugin.loaded)
    {
        if (ImGui::Button("加载插件", ImVec2(120, 0)))
        {
            PluginManager::GetInstance().LoadPlugin(plugin.id);
        }
    }
    else if (plugin.loaded)
    {
        if (ImGui::Button("卸载插件", ImVec2(120, 0)))
        {
            PluginManager::GetInstance().UnloadPlugin(plugin.id);
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("删除插件", ImVec2(120, 0)))
    {
        PopupManager::GetInstance().Open("ConfirmRemovePlugin");
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("删除此插件及其所有文件");
    }
}

void PluginManagerPanel::drawImportSection()
{
    
}

void PluginManagerPanel::drawPublishPopup()
{
    ImGui::Text("发布插件");
    ImGui::TextDisabled("选择插件项目目录，打包为 .lplug 文件");
    ImGui::Separator();

    
    ImGui::Text("插件项目目录:");
    ImGui::InputText("##PluginDir", m_publishPluginDir, sizeof(m_publishPluginDir));
    ImGui::SameLine();
    if (ImGui::Button("选择...##PluginDir"))
    {
        if (SDL_Window* window = getSDLWindow())
        {
            SDL_ShowOpenFolderDialog(OnPluginProjectDirDialogCallback, this, window, nullptr, false);
        }
    }

    
    std::filesystem::path pluginDir(m_publishPluginDir);
    bool hasValidManifest = false;
    std::string pluginId, pluginName, pluginVersion;

    if (strlen(m_publishPluginDir) > 0 && std::filesystem::exists(pluginDir))
    {
        
        std::filesystem::path yamlPath = pluginDir / "plugin.yaml";
        std::filesystem::path jsonPath = pluginDir / "plugin.json";

        if (std::filesystem::exists(yamlPath))
        {
            try
            {
                YAML::Node manifest = YAML::LoadFile(yamlPath.string());
                if (manifest["id"]) pluginId = manifest["id"].as<std::string>();
                if (manifest["name"]) pluginName = manifest["name"].as<std::string>();
                if (manifest["version"]) pluginVersion = manifest["version"].as<std::string>("1.0.0");
                hasValidManifest = !pluginId.empty();
            }
            catch (...) {}
        }
        else if (std::filesystem::exists(jsonPath))
        {
            
            hasValidManifest = false; 
        }
    }

    ImGui::Separator();

    if (hasValidManifest)
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "✓ 找到有效的插件清单");
        ImGui::Text("插件名称: %s", pluginName.c_str());
        ImGui::Text("插件 ID: %s", pluginId.c_str());
        ImGui::Text("版本: %s", pluginVersion.c_str());
    }
    else if (strlen(m_publishPluginDir) > 0)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "✗ 未找到 plugin.yaml 或无效的清单");
    }
    else
    {
        ImGui::TextDisabled("请选择插件项目目录");
    }

    ImGui::Separator();

    
    ImGui::Text("输出目录:");
    ImGui::InputText("##OutputDir", m_publishOutputDir, sizeof(m_publishOutputDir));
    ImGui::SameLine();
    if (ImGui::Button("选择...##OutputDir"))
    {
        if (SDL_Window* window = getSDLWindow())
        {
            SDL_ShowOpenFolderDialog(OnPublishDirDialogCallback, this, window, nullptr, false);
        }
    }

    ImGui::Separator();

    
    if (m_isPublishing)
    {
        
        static float spinner = 0.0f;
        spinner += ImGui::GetIO().DeltaTime * 5.0f;
        const char* spinChars = "|/-\\";
        char spinChar = spinChars[static_cast<int>(spinner) % 4];
        
        ImGui::Text("正在打包插件... %c", spinChar);
        ImGui::TextDisabled("%s", m_publishStatusMessage.c_str());
        
        
        if (m_publishFuture.valid() && 
            m_publishFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            bool success = m_publishFuture.get();
            m_isPublishing = false;
            if (success)
            {
                LogInfo("{}", m_publishStatusMessage);
            }
            else
            {
                LogError("{}", m_publishStatusMessage);
            }
            PopupManager::GetInstance().Close("PublishPluginPopup");
        }
        return;
    }

    
    bool canPublish = hasValidManifest && strlen(m_publishOutputDir) > 0;
    if (!canPublish) ImGui::BeginDisabled();

    if (ImGui::Button("打包发布", ImVec2(120, 0)))
    {
        std::filesystem::path outputDir(m_publishOutputDir);
        std::string outputFileName = pluginId + ".lplug";
        std::filesystem::path outputFile = outputDir / outputFileName;
        
        
        doPublishAsync(pluginDir, outputFile);
    }

    if (!canPublish) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("取消", ImVec2(120, 0)))
    {
        PopupManager::GetInstance().Close("PublishPluginPopup");
    }
}

void PluginManagerPanel::OnImportFileSelected(const std::filesystem::path& path)
{
    strncpy(m_importPathBuffer, path.string().c_str(), sizeof(m_importPathBuffer) - 1);
}

void PluginManagerPanel::OnPublishDirSelected(const std::filesystem::path& path)
{
    strncpy(m_publishOutputDir, path.string().c_str(), sizeof(m_publishOutputDir) - 1);
}

void PluginManagerPanel::OnPluginProjectDirSelected(const std::filesystem::path& path)
{
    strncpy(m_publishPluginDir, path.string().c_str(), sizeof(m_publishPluginDir) - 1);
    
    if (strlen(m_publishOutputDir) == 0)
    {
        std::filesystem::path publishDir = path / "Publish";
        strncpy(m_publishOutputDir, publishDir.string().c_str(), sizeof(m_publishOutputDir) - 1);
    }
}

void PluginManagerPanel::doPublishAsync(const std::filesystem::path& pluginDir, const std::filesystem::path& outputFile)
{
    m_isPublishing = true;
    m_publishStatusMessage = "正在编译插件...";

    m_publishFuture = std::async(std::launch::async, [this, pluginDir, outputFile]() -> bool
    {
        std::filesystem::path outputDir = outputFile.parent_path();
        
        
        if (!std::filesystem::exists(outputDir))
        {
            try
            {
                std::filesystem::create_directories(outputDir);
            }
            catch (...)
            {
                m_publishStatusMessage = "创建输出目录失败";
                return false;
            }
        }

        
        
        std::filesystem::path csprojPath;
        for (const auto& entry : std::filesystem::directory_iterator(pluginDir))
        {
            if (entry.path().extension() == ".csproj")
            {
                csprojPath = entry.path();
                break;
            }
        }

        if (csprojPath.empty())
        {
            m_publishStatusMessage = "未找到 .csproj 文件";
            return false;
        }

        
        std::filesystem::path buildOutputDir = pluginDir / "bin" / "Release" / "net9.0" / "publish";
        
#ifdef _WIN32
        std::string dotnetRid = "win-x64";
#else
        std::string dotnetRid = "linux-x64";
#endif

        m_publishStatusMessage = "正在编译: " + csprojPath.filename().string();
        
        std::string publishCmd = std::format(
            "dotnet publish \"{}\" -c Release -r {} --self-contained false -o \"{}\"",
            csprojPath.string(),
            dotnetRid,
            buildOutputDir.string()
        );

        int buildResult = std::system(publishCmd.c_str());
        if (buildResult != 0)
        {
            m_publishStatusMessage = "编译失败，错误码: " + std::to_string(buildResult);
            return false;
        }

        
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(buildOutputDir))
            {
                if (entry.path().extension() == ".pdb")
                {
                    std::filesystem::remove(entry.path());
                }
            }
        }
        catch (...) {}

        
        std::filesystem::path manifestSrc = pluginDir / "plugin.yaml";
        std::filesystem::path manifestDst = buildOutputDir / "plugin.yaml";
        if (std::filesystem::exists(manifestSrc))
        {
            try
            {
                std::filesystem::copy_file(manifestSrc, manifestDst, 
                    std::filesystem::copy_options::overwrite_existing);
            }
            catch (...) {}
        }

        
        const std::vector<std::string> sdkFilesToRemove = {
            "Luma.SDK.dll",
            "Luma.SDK.deps.json",
            "Luma.SDK.runtimeconfig.json",
            "Luma.SDK.pdb",
            "YamlDotNet.dll"
        };
        for (const auto& fileName : sdkFilesToRemove)
        {
            std::filesystem::path filePath = buildOutputDir / fileName;
            if (std::filesystem::exists(filePath))
            {
                try { std::filesystem::remove(filePath); } catch (...) {}
            }
        }

        m_publishStatusMessage = "正在打包...";

        
#ifdef _WIN32
        std::filesystem::path tempZipFile = outputDir / (outputFile.stem().string() + ".zip");
        
        
        try
        {
            if (std::filesystem::exists(tempZipFile))
                std::filesystem::remove(tempZipFile);
            if (std::filesystem::exists(outputFile))
                std::filesystem::remove(outputFile);
        }
        catch (...) {}
        
        std::string cmd = std::format(
            "powershell -Command \"Compress-Archive -Path '{}\\*' -DestinationPath '{}' -Force; Move-Item -Path '{}' -Destination '{}' -Force\"",
            buildOutputDir.string(),
            tempZipFile.string(),
            tempZipFile.string(),
            outputFile.string()
        );
#else
        std::string cmd = std::format(
            "cd '{}' && zip -r '{}' .",
            buildOutputDir.string(),
            outputFile.string()
        );
#endif
        int result = std::system(cmd.c_str());
        
        if (result == 0)
        {
            m_publishStatusMessage = "发布成功: " + outputFile.string();
            return true;
        }
        else
        {
            m_publishStatusMessage = "打包失败，错误码: " + std::to_string(result);
            return false;
        }
    });
}

SDL_Window* PluginManagerPanel::getSDLWindow() const
{
    if (!m_context || !m_context->editor)
    {
        return nullptr;
    }
    auto* window = m_context->editor->GetPlatWindow();
    return window ? window->GetSdlWindow() : nullptr;
}

#include "ToolBarPanel.h"
#include "PopupManager.h"
#include "AnimationSystem.h"
#include "AssetManager.h"
#include "AssetPacker.h"
#include "AudioSystem.h"
#include "ButtonSystem.h"
#include "CommonUIControlSystem.h"
#include "Debug.Test.h"
#include "Editor.h"
#include "EngineCrypto.h"
#include "InputTextSystem.h"
#include "PreferenceSettings.h"
#include "Profiler.h"
#include "ProjectSettings.h"
#include "ScriptMetadataRegistry.h"
#include "../Renderer/Nut/ShaderRegistry.h"
#include "../Utils/PCH.h"
#include "../SceneManager.h"
#include "../Resources/Managers/RuntimeTextureManager.h"
#include "../Resources/Managers/RuntimeMaterialManager.h"
#include "../Resources/Managers/RuntimePrefabManager.h"
#include "../Resources/Managers/RuntimeSceneManager.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Systems/HydrateResources.h"
#include "../Systems/TransformSystem.h"
#include "../Systems/PhysicsSystem.h"
#include "../Systems/InteractionSystem.h"
#include "../Systems/ScriptingSystem.h"
#include "../Systems/ParticleSystem.h"
#include "../Utils/Logger.h"
#include "../Data/SceneData.h"
#include "../../Systems/HydrateResources.h"
#include "Plugins/PluginManager.h"
#include "Input/Keyboards.h"
#include <sstream>
#include <cstring>
#include <array>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <cctype>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <functional>
#include <SDL3/SDL_dialog.h>
#include "../Window.h"
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#define IM_PI 3.14159265358979323846f
namespace
{
    void SDLCALL OnAndroidSdkFolderSelected(void*, const char* const* filelist, int)
    {
        if (filelist && filelist[0])
        {
            PreferenceSettings::GetInstance().SetAndroidSdkPath(std::filesystem::path(filelist[0]));
        }
    }
    void SDLCALL OnAndroidNdkFolderSelected(void*, const char* const* filelist, int)
    {
        if (filelist && filelist[0])
        {
            PreferenceSettings::GetInstance().SetAndroidNdkPath(std::filesystem::path(filelist[0]));
        }
    }
    void SDLCALL OnKeystoreFileSelected(void*, const char* const* filelist, int)
    {
        if (filelist && filelist[0])
        {
            ProjectSettings::GetInstance().SetAndroidKeystorePath(std::filesystem::path(filelist[0]));
        }
    }
    void SDLCALL OnKeystoreSavePathSelected(void* userdata, const char* const* filelist, int)
    {
        if (filelist && filelist[0])
        {
            auto* panel = static_cast<ToolbarPanel*>(userdata);
            if (panel)
            {
                panel->OnKeystoreSavePathChosen(std::filesystem::path(filelist[0]));
            }
        }
    }
    void CopyStringToFixedBuffer(char* buffer, size_t size, const std::string& value)
    {
        if (!buffer || size == 0)
        {
            return;
        }
#ifdef _WIN32
        strcpy_s(buffer, size, value.c_str());
#else
        std::strncpy(buffer, value.c_str(), size);
        buffer[size - 1] = '\0';
#endif
    }
    std::string QuoteCommandArg(const std::string& value)
    {
        std::string result;
        result.reserve(value.size() + 2);
        result.push_back('"');
#if defined(_WIN32)
        for (char ch : value)
        {
            if (ch == '"')
            {
                result.push_back('"');
            }
            result.push_back(ch);
        }
#else
        for (char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                result.push_back('\\');
            }
            result.push_back(ch);
        }
#endif
        result.push_back('"');
        return result;
    }
#if defined(_WIN32)
    std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty())
        {
            return {};
        }
        int needed = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
        if (needed <= 0)
        {
            return {};
        }
        std::wstring wide(static_cast<size_t>(needed), L'\0');
        int converted = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wide.data(), needed);
        if (converted <= 0)
        {
            return {};
        }
        if (!wide.empty() && wide.back() == L'\0')
        {
            wide.pop_back();
        }
        return wide;
    }
#endif
    std::string ResolveKeytoolExecutable()
    {
#if defined(_WIN32)
        const char* keytoolName = "keytool.exe";
#else
        const char* keytoolName = "keytool";
#endif
        const char* javaHome = std::getenv("JAVA_HOME");
        if (javaHome && javaHome[0] != '\0')
        {
            std::filesystem::path candidate = std::filesystem::path(javaHome) / "bin" / keytoolName;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec))
            {
                return candidate.string();
            }
        }
        return keytoolName;
    }
}
namespace platform_native
{
    static void OpenDirectoryInExplorer(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
        {
            LogError("无法打开目录，路径无效或不存在: {}", path.string());
            return;
        }
#ifdef _WIN32
        ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
        std::string command =
#ifdef __linux__
        "xdg-open \"" + path.string() + "\"";
#else
        "open \"" + path.string() + "\"";
#endif
        int res = system(command.c_str());
#endif
    }
}
struct AndroidPermissionOption
{
    const char* label;
    const char* permission;
    const char* description;
};
static constexpr AndroidPermissionOption kAndroidPermissionOptions[] = {
    {"振动 (VIBRATE)", "android.permission.VIBRATE", "允许设备震动反馈"},
    {"网络访问 (INTERNET)", "android.permission.INTERNET", "联网、HTTP 请求等"},
    {"网络状态 (ACCESS_NETWORK_STATE)", "android.permission.ACCESS_NETWORK_STATE", "检测网络状态"},
    {"Wi-Fi 状态 (ACCESS_WIFI_STATE)", "android.permission.ACCESS_WIFI_STATE", "读取 Wi-Fi 信息"},
    {"蓝牙 (BLUETOOTH)", "android.permission.BLUETOOTH", "经典蓝牙访问"},
    {"蓝牙管理 (BLUETOOTH_ADMIN)", "android.permission.BLUETOOTH_ADMIN", "蓝牙扫描、配对"},
    {"麦克风 (RECORD_AUDIO)", "android.permission.RECORD_AUDIO", "语音输入、录音"},
    {"摄像头 (CAMERA)", "android.permission.CAMERA", "访问摄像头"},
    {"读存储 (READ_EXTERNAL_STORAGE)", "android.permission.READ_EXTERNAL_STORAGE", "读取共享存储"},
    {"写存储 (WRITE_EXTERNAL_STORAGE)", "android.permission.WRITE_EXTERNAL_STORAGE", "写入共享存储"},
    {"通知 (POST_NOTIFICATIONS)", "android.permission.POST_NOTIFICATIONS", "发送通知 (Android 13+)"},
};
static void UpdateAndroidStringsXml(const std::filesystem::path& resDir, const std::string& appName)
{
    const std::filesystem::path valuesDir = resDir / "values";
    std::error_code ec;
    std::filesystem::create_directories(valuesDir, ec);
    const std::filesystem::path stringsXmlPath = valuesDir / "strings.xml";
    std::ofstream oss(stringsXmlPath, std::ios::trunc);
    if (!oss.is_open())
    {
        LogError("无法写入 strings.xml: {}", stringsXmlPath.string());
        return;
    }
    oss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    oss << "<resources>\n";
    std::string escapedName = appName;
    size_t pos = 0;
    while ((pos = escapedName.find("&", pos)) != std::string::npos)
    {
        escapedName.replace(pos, 1, "&amp;");
        pos += 5;
    }
    pos = 0;
    while ((pos = escapedName.find("<", pos)) != std::string::npos)
    {
        escapedName.replace(pos, 1, "&lt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = escapedName.find(">", pos)) != std::string::npos)
    {
        escapedName.replace(pos, 1, "&gt;");
        pos += 4;
    }
    oss << "    <string name=\"app_name\">" << escapedName << "</string>\n";
    oss << "</resources>\n";
    LogInfo("已更新 Android 应用名称为: {}", appName);
}
static bool ExecuteCommand(const std::string& command, const std::string& logPrefix)
{
    LogInfo("[{}] 执行命令: {}", logPrefix, command);
    std::array<char, 128> buffer;
    std::string result;
#if defined(_WIN32)
    std::wstring wideCommand = Utf8ToWide(command);
    if (wideCommand.empty() && !command.empty())
    {
        LogError("[{}] 无法将命令转换为 Unicode。", logPrefix);
        return false;
    }
    FILE* pipe = _wpopen(wideCommand.c_str(), L"r");
#else
    FILE* pipe = POPEN(command.c_str(), "r");
#endif
    if (!pipe)
    {
        LogError("[{}] 无法执行命令。", logPrefix);
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }
#if defined(_WIN32)
    int exitCode = _pclose(pipe);
#else
    int exitCode = PCLOSE(pipe);
#endif
    if (exitCode != 0)
    {
        LogError("[{}] 命令执行失败，退出码: {}. 输出:\n{}", logPrefix, exitCode, result);
        return false;
    }
    LogInfo("[{}] 命令执行成功。", logPrefix);
    return true;
}
void ToolbarPanel::Initialize(EditorContext* context)
{
    m_context = context;
    ProjectSettings::GetInstance().Load();
    m_CSharpScriptUpdated = EventBus::GetInstance().Subscribe<CSharpScriptUpdateEvent>(
        [this](const CSharpScriptUpdateEvent&) { this->rebuildScripts(); });
    PopupManager::GetInstance().Register("PreferencesPopup", [this]() { this->drawPreferencesPopup(); }, true,
                                         ImGuiWindowFlags_AlwaysAutoResize);
    PopupManager::GetInstance().Register("SaveScene", [this]() { this->drawSaveBeforePackagingPopup(); }, true,
                                         ImGuiWindowFlags_AlwaysAutoResize);
}
void ToolbarPanel::Update(float deltaTime)
{
}
void ToolbarPanel::Draw()
{
    PROFILE_FUNCTION();
    handleShortcuts();
    drawMainMenuBar();
    drawSettingsWindow();
    drawScriptCompilationPopup();
    drawPackagingPopup();
}
void ToolbarPanel::drawPackagingPopup()
{
    if (m_isPackaging)
    {
        ImGui::OpenPopup("打包游戏");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowSize(ImVec2(480, 170), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("打包游戏", nullptr,
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
        {
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            if (m_packagingProgress < 1.0f)
            {
                ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x + 10.0f);
                ImGui::TextUnformatted(m_packagingStatus.c_str());
                ImGui::SameLine();
                const float spinnerRadius = 12.0f;
                const float spinnerThickness = 4.0f;
                float spinnerPosX = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - spinnerRadius * 2.0f -
                    10.0f;
                ImGui::SetCursorPosX(spinnerPosX);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                pos.x += spinnerRadius;
                pos.y += spinnerRadius;
                float start_angle = (float)ImGui::GetTime() * 8.0f;
                const int num_segments = 12;
                for (int i = 0; i < num_segments; ++i)
                {
                    float a = start_angle + (float)i * (2.0f * IM_PI) / (float)num_segments;
                    ImU32 col = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, (float)i / (float)num_segments));
                    draw_list->AddLine(
                        ImVec2(pos.x + cosf(a) * spinnerRadius, pos.y + sinf(a) * spinnerRadius),
                        ImVec2(pos.x + cosf(a - IM_PI / (num_segments / 2)) * spinnerRadius,
                               pos.y + sinf(a - IM_PI / (num_segments / 2)) * spinnerRadius),
                        col, spinnerThickness);
                }
                ImGui::Dummy(ImVec2(spinnerRadius * 2, spinnerRadius * 2));
            }
            else
            {
                ImVec4 color = m_packagingSuccess ? ImVec4(0.2f, 0.9f, 0.2f, 1) : ImVec4(0.9f, 0.2f, 0.2f, 1);
                const char* icon = m_packagingSuccess ? "✔" : "✖";
                std::string text = std::string(icon) + " " + m_packagingStatus;
                float textWidth = ImGui::CalcTextSize(text.c_str()).x;
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) / 2.0f);
                ImGui::TextColored(color, "%s", text.c_str());
            }
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2.0f);
            ImGui::ProgressBar(m_packagingProgress, ImVec2(-1, 0.0f));
            ImGui::PopItemWidth();
            if (m_packagingProgress >= 1.0f)
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                const float openDirBtnWidth = 150.0f;
                const float closeBtnWidth = 120.0f;
                const float buttonSpacing = ImGui::GetStyle().ItemSpacing.x;
                float totalButtonsWidth = m_packagingSuccess
                                              ? (openDirBtnWidth + buttonSpacing + closeBtnWidth)
                                              : closeBtnWidth;
                float buttonsPosX = (ImGui::GetWindowWidth() - totalButtonsWidth) / 2.0f;
                ImGui::SetCursorPosX(buttonsPosX);
                if (m_packagingSuccess)
                {
                    if (ImGui::Button("打开输出目录", ImVec2(openDirBtnWidth, 0)))
                    {
                        platform_native::OpenDirectoryInExplorer(m_lastBuildDirectory);
                    }
                    ImGui::SameLine();
                }
                if (ImGui::Button("关闭", ImVec2(closeBtnWidth, 0)))
                {
                    m_isPackaging = false;
                    ImGui::CloseCurrentPopup();
                }
                if (!m_packagingSuccess) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndPopup();
        }
    }
}
void ToolbarPanel::rebuildScripts()
{
    if (m_isCompilingScripts)
    {
        LogWarn("脚本已在编译中。");
        return;
    }
    m_isCompilingScripts = true;
    m_compilationFinished = false;
    m_compilationSuccess = false;
    m_compilationStatus = "正在准备编译环境...";
    m_compilationFuture = std::async(std::launch::async, [this]()
    {
        m_compilationSuccess = runScriptCompilationLogic(m_compilationStatus);
        m_compilationFinished = true;
        if (m_compilationSuccess) { EventBus::GetInstance().Publish(CSharpScriptRebuiltEvent()); }
    });
}
bool ToolbarPanel::runScriptCompilationLogic(std::string& statusMessage, const std::filesystem::path& outPath)
{
    const std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
    const std::filesystem::path editorRoot = ".";
    const std::filesystem::path libraryDir = outPath.empty() ? (projectRoot / "Library") : outPath;
    TargetPlatform hostPlatform = ProjectSettings::GetCurrentHostPlatform();
    std::string platformSubDir = ProjectSettings::PlatformToString(hostPlatform);
    const std::filesystem::path toolsDir = editorRoot / "Tools" / platformSubDir;
    try
    {
        statusMessage = "检查并准备 C# 依赖项...";
        std::filesystem::create_directories(libraryDir);
        const std::vector<std::string> requiredFiles = {
            "Luma.SDK.dll", "Luma.SDK.deps.json", "Luma.SDK.runtimeconfig.json", "YamlDotNet.dll"
        };
        for (const auto& filename : requiredFiles)
        {
            const std::filesystem::path destFile = libraryDir / filename;
            if (!std::filesystem::exists(destFile))
            {
                const std::filesystem::path srcFile = toolsDir / filename;
                if (!std::filesystem::exists(srcFile))
                {
                    throw std::runtime_error("关键依赖文件在 Tools/" + platformSubDir + " 目录中未找到: " + srcFile.string());
                }
                statusMessage = "正在拷贝依赖: " + filename;
                std::filesystem::copy(srcFile, destFile);
            }
        }
        statusMessage = "正在发布 C# 项目...";
        std::string dotnetRid = (hostPlatform == TargetPlatform::Windows) ? "win-x64" : "linux-x64";
        std::string projectRootStr = projectRoot.string();
        std::string libraryDirStr = libraryDir.string();
#ifdef _WIN32
        char shortProjectPath[MAX_PATH];
        char shortLibraryPath[MAX_PATH];
        if (GetShortPathNameA(projectRootStr.c_str(), shortProjectPath, MAX_PATH) > 0)
        {
            projectRootStr = shortProjectPath;
        }
        if (GetShortPathNameA(libraryDirStr.c_str(), shortLibraryPath, MAX_PATH) > 0)
        {
            libraryDirStr = shortLibraryPath;
        }
#endif
        std::string slnPathStr = (projectRoot / "LumaScripting.sln").string();
        const std::string publishCmd = "dotnet publish -c Release -r " + dotnetRid + " \"" +
            slnPathStr + "\" -o \"" + libraryDirStr + "\"";
        if (!ExecuteCommand(publishCmd, "Publish"))
        {
            throw std::runtime_error("dotnet publish 命令执行失败。请检查控制台输出获取详细错误信息。");
        }
        statusMessage = "正在提取脚本元数据...";
        std::string toolExecutableName = (hostPlatform == TargetPlatform::Windows)
                                             ? "YamlExtractor.exe"
                                             : "YamlExtractor";
        const std::filesystem::path toolsExe = toolsDir / toolExecutableName;
        const std::filesystem::path gameScriptsDll = libraryDir / "GameScripts.dll";
        const std::filesystem::path metadataYaml = libraryDir / "ScriptMetadata.yaml";
        const std::filesystem::path absToolsExe = std::filesystem::absolute(toolsExe);
        const std::filesystem::path absGameScriptsDll = std::filesystem::absolute(gameScriptsDll);
        const std::filesystem::path absMetadataYaml = std::filesystem::absolute(metadataYaml);
        if (!std::filesystem::exists(absToolsExe))
        {
            throw std::runtime_error("元数据提取工具 " + toolExecutableName + " 未找到: " + toolsExe.string());
        }
        if (!std::filesystem::exists(absGameScriptsDll))
        {
            throw std::runtime_error("编译产物 GameScripts.dll 未在输出目录中找到: " + gameScriptsDll.string());
        }
        std::string toolsExeStr = absToolsExe.string();
        std::string gameScriptsDllStr = absGameScriptsDll.string();
        std::string metadataYamlStr = absMetadataYaml.string();
#ifdef _WIN32
        char shortToolPath[MAX_PATH];
        char shortDllPath[MAX_PATH];
        char shortYamlPath[MAX_PATH];
        if (GetShortPathNameA(toolsExeStr.c_str(), shortToolPath, MAX_PATH) > 0)
        {
            toolsExeStr = shortToolPath;
        }
        if (GetShortPathNameA(gameScriptsDllStr.c_str(), shortDllPath, MAX_PATH) > 0)
        {
            gameScriptsDllStr = shortDllPath;
        }
        if (GetShortPathNameA(metadataYamlStr.c_str(), shortYamlPath, MAX_PATH) > 0)
        {
            metadataYamlStr = shortYamlPath;
        }
#endif
        const std::string extractCmd = toolsExeStr + " " + gameScriptsDllStr + " " + metadataYamlStr;
        if (!ExecuteCommand(extractCmd, "MetadataTool"))
        {
            throw std::runtime_error("脚本元数据提取失败。");
        }
        ScriptMetadataRegistry::GetInstance().Initialize(metadataYaml.string());
        statusMessage = "脚本编译成功！";
        EventBus::GetInstance().Publish(CSharpScriptCompiledEvent());
        return true;
    }
    catch (const std::exception& e)
    {
        statusMessage = std::string("编译失败: ") + e.what();
        return false;
    }
}
void ToolbarPanel::drawPreferencesPopup()
{
    ImGui::Text("外部工具");
    ImGui::Separator();
    auto& settings = PreferenceSettings::GetInstance();
    IDE currentIDE = settings.GetPreferredIDE();
    const char* ideNames[] = {"自动检测", "Visual Studio", "JetBrains Rider", "VS Code"};
    const IDE ideValues[] = {IDE::Unknown, IDE::VisualStudio, IDE::Rider, IDE::VSCode};
    const char* currentName = ideNames[0];
    for (int i = 0; i < IM_ARRAYSIZE(ideValues); ++i)
    {
        if (ideValues[i] == currentIDE)
        {
            currentName = ideNames[i];
            break;
        }
    }
    ImGui::Text("脚本编辑器:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##IDESelector", currentName))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ideNames); ++i)
        {
            const bool isSelected = (currentIDE == ideValues[i]);
            if (ImGui::Selectable(ideNames[i], isSelected)) { settings.SetPreferredIDE(ideValues[i]); }
            if (isSelected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Separator();
    ImGui::Text("Android 环境");
    ImGui::Separator();
    auto drawPathField = [](const char* label, const std::filesystem::path& value,
                            auto setter, const char* buttonLabel, auto browseAction)
    {
        std::array<char, 512> buffer{};
        const std::string pathStr = value.string();
#ifdef _WIN32
        strcpy_s(buffer.data(), buffer.size(), pathStr.c_str());
#else
        std::strncpy(buffer.data(), pathStr.c_str(), buffer.size());
        buffer[buffer.size() - 1] = '\0';
#endif
        if (ImGui::InputText(label, buffer.data(), buffer.size()))
        {
            setter(std::filesystem::path(buffer.data()));
        }
        ImGui::SameLine();
        if (ImGui::Button(buttonLabel))
        {
            browseAction();
        }
    };
    drawPathField("Android SDK 路径", settings.GetAndroidSdkPath(),
                  [&](const std::filesystem::path& path) { settings.SetAndroidSdkPath(path); },
                  "浏览...##AndroidSDK",
                  [&]()
                  {
                      if (SDL_Window* window = getSDLWindow())
                      {
                          SDL_ShowOpenFolderDialog(OnAndroidSdkFolderSelected, nullptr, window, nullptr, false);
                      }
                      else
                      {
                          LogWarn("无法打开文件对话框，SDL 窗口无效。");
                      }
                  });
    drawPathField("Android NDK 路径", settings.GetAndroidNdkPath(),
                  [&](const std::filesystem::path& path) { settings.SetAndroidNdkPath(path); },
                  "浏览...##AndroidNDK",
                  [&]()
                  {
                      if (SDL_Window* window = getSDLWindow())
                      {
                          SDL_ShowOpenFolderDialog(OnAndroidNdkFolderSelected, nullptr, window, nullptr, false);
                      }
                      else
                      {
                          LogWarn("无法打开文件对话框，SDL 窗口无效。");
                      }
                  });
    ImGui::TextDisabled("这些路径用于 Android 构建脚本、Gradle 与 NDK 工具链。");
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Separator();
    if (ImGui::Button("关闭", ImVec2(120, 0))) { PopupManager::GetInstance().Close("PreferencesPopup"); }
    ImGui::SetItemDefaultFocus();
}
void ToolbarPanel::drawScriptCompilationPopup()
{
    if (!m_isCompilingScripts) return;
    ImGui::OpenPopup("编译脚本");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowSize(ImVec2(380, 120), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("编译脚本", NULL,
                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
    {
        if (!m_compilationFinished)
        {
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x + 10.0f);
            ImGui::TextUnformatted(m_compilationStatus.c_str());
            ImGui::SameLine();
            const float spinnerRadius = 14.0f;
            const float spinnerThickness = 4.0f;
            float spinnerPosX = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - spinnerRadius * 2.0f -
                10.0f;
            ImGui::SetCursorPosX(spinnerPosX);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            pos.x += spinnerRadius;
            pos.y += spinnerRadius;
            float start_angle = (float)ImGui::GetTime() * 8.0f;
            const int num_segments = 12;
            for (int i = 0; i < num_segments; ++i)
            {
                float a = start_angle + (float)i * (2.0f * IM_PI) / (float)num_segments;
                ImU32 col = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, (float)i / (float)num_segments));
                draw_list->AddLine(
                    ImVec2(pos.x + cosf(a) * spinnerRadius, pos.y + sinf(a) * spinnerRadius),
                    ImVec2(pos.x + cosf(a - IM_PI / (num_segments / 2)) * spinnerRadius,
                           pos.y + sinf(a - IM_PI / (num_segments / 2)) * spinnerRadius),
                    col, spinnerThickness);
            }
            ImGui::Dummy(ImVec2(spinnerRadius * 2, spinnerRadius * 2));
        }
        else
        {
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImVec4 color = m_compilationSuccess ? ImVec4(0.2f, 0.9f, 0.2f, 1) : ImVec4(0.9f, 0.2f, 0.2f, 1);
            const char* icon = m_compilationSuccess ? "✔" : "✖";
            std::string text = std::string(icon) + " " + m_compilationStatus;
            float textWidth = ImGui::CalcTextSize(text.c_str()).x;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) / 2.0f);
            ImGui::TextColored(color, "%s", text.c_str());
            ImGui::Dummy(ImVec2(0.0f, 15.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            const float buttonWidth = 120.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) / 2.0f);
            if (ImGui::Button("确定", ImVec2(buttonWidth, 0)))
            {
                m_isCompilingScripts = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndPopup();
    }
}
void ToolbarPanel::Shutdown()
{
    EventBus::GetInstance().Unsubscribe(m_CSharpScriptUpdated);
}
void ToolbarPanel::drawViewportMenu()
{
    if (ImGui::BeginMenu("视图"))
    {
        auto& settings = ProjectSettings::GetInstance();
        bool isProjectLoaded = settings.IsProjectLoaded();
        if (!isProjectLoaded) ImGui::BeginDisabled();
        ImGui::Text("视口布局");
        ImGui::Separator();
        ViewportScaleMode currentMode = settings.GetViewportScaleMode();
        const char* modeNames[] = {"无布局", "固定比例", "固定宽度", "固定高度", "拉伸填充"};
        const ViewportScaleMode modeValues[] = {
            ViewportScaleMode::None,
            ViewportScaleMode::FixedAspect,
            ViewportScaleMode::FixedWidth,
            ViewportScaleMode::FixedHeight,
            ViewportScaleMode::Expand
        };
        for (int i = 0; i < IM_ARRAYSIZE(modeNames); ++i)
        {
            bool isSelected = (currentMode == modeValues[i]);
            if (ImGui::MenuItem(modeNames[i], nullptr, isSelected))
            {
                settings.SetViewportScaleMode(modeValues[i]);
                settings.Save();
            }
        }
        if (currentMode != ViewportScaleMode::None)
        {
            ImGui::Separator();
            ImGui::Text("设计分辨率");
            int designWidth = settings.GetDesignWidth();
            int designHeight = settings.GetDesignHeight();
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("##DesignWidth", &designWidth, 0, 0))
            {
                if (designWidth > 0)
                {
                    settings.SetDesignWidth(designWidth);
                    settings.Save();
                }
            }
            ImGui::SameLine();
            ImGui::Text("x");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("##DesignHeight", &designHeight, 0, 0))
            {
                if (designHeight > 0)
                {
                    settings.SetDesignHeight(designHeight);
                    settings.Save();
                }
            }
            ImGui::Separator();
            ImGui::Text("预设");
            if (ImGui::MenuItem("1920 x 1080 (16:9)"))
            {
                settings.SetDesignWidth(1920);
                settings.SetDesignHeight(1080);
                settings.Save();
            }
            if (ImGui::MenuItem("1280 x 720 (16:9)"))
            {
                settings.SetDesignWidth(1280);
                settings.SetDesignHeight(720);
                settings.Save();
            }
            if (ImGui::MenuItem("1080 x 1920 (9:16 竖屏)"))
            {
                settings.SetDesignWidth(1080);
                settings.SetDesignHeight(1920);
                settings.Save();
            }
            if (ImGui::MenuItem("2048 x 1536 (4:3)"))
            {
                settings.SetDesignWidth(2048);
                settings.SetDesignHeight(1536);
                settings.Save();
            }
        }
        if (!isProjectLoaded) ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}
void ToolbarPanel::drawWindowMenu()
{
    if (ImGui::BeginMenu("窗口"))
    {
        IEditorPanel* pluginPanel = m_context->editor->GetPanelByName("插件管理");
        IEditorPanel* consolePanel = m_context->editor->GetPanelByName("控制台");
        IEditorPanel* animPanel = m_context->editor->GetPanelByName("动画编辑器");
        IEditorPanel* aiPanel = m_context->editor->GetPanelByName("AI 助手");
        if (pluginPanel)
        {
            bool visible = pluginPanel->IsVisible();
            if (ImGui::MenuItem("插件管理", nullptr, &visible))
            {
                pluginPanel->SetVisible(visible);
            }
        }
        if (consolePanel)
        {
            bool visible = consolePanel->IsVisible();
            if (ImGui::MenuItem("控制台", nullptr, &visible))
            {
                consolePanel->SetVisible(visible);
            }
        }
        if (animPanel)
        {
            bool visible = animPanel->IsVisible();
            if (ImGui::MenuItem("动画编辑器", nullptr, &visible))
            {
                animPanel->SetVisible(visible);
            }
        }
        if (aiPanel)
        {
            bool visible = aiPanel->IsVisible();
            if (ImGui::MenuItem("AI 助手", nullptr, &visible))
            {
                aiPanel->SetVisible(visible);
            }
        }
        PluginManager::GetInstance().DrawPluginMenuItems("窗口");
        ImGui::EndMenu();
    }
}
void ToolbarPanel::drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        drawFileMenu();
        drawEditMenu();
        drawViewportMenu();
        drawProjectMenu();
        drawWindowMenu();
        PluginManager::GetInstance().DrawEditorPluginMenuBar();
        {
            float totalWidth = 0;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            float undoWidth = ImGui::CalcTextSize("撤销").x + ImGui::GetStyle().FramePadding.x * 2;
            float redoWidth = ImGui::CalcTextSize("重做").x + ImGui::GetStyle().FramePadding.x * 2;
            float playControlsWidth = 0.0f;
            if (m_context->editorState == EditorState::Editing)
            {
                playControlsWidth = ImGui::CalcTextSize("播放").x + ImGui::GetStyle().FramePadding.x * 2;
            }
            else
            {
                playControlsWidth = ImGui::CalcTextSize("停止").x + ImGui::GetStyle().FramePadding.x * 2;
                const char* pauseLabel = (m_context->editorState == EditorState::Paused) ? "继续" : "暂停";
                playControlsWidth += ImGui::CalcTextSize(pauseLabel).x + ImGui::GetStyle().FramePadding.x * 2 + spacing;
            }
            totalWidth = undoWidth + playControlsWidth + redoWidth + spacing * 2;
            float startPosX = (ImGui::GetContentRegionAvail().x - totalWidth) / 2.0f;
            ImGui::SetCursorPosX(startPosX);
            bool canUndo = SceneManager::GetInstance().CanUndo();
            if (!canUndo) ImGui::BeginDisabled();
            if (ImGui::Button("撤销")) { undo(); }
            if (!canUndo) ImGui::EndDisabled();
            ImGui::SameLine();
            drawPlayControls();
            ImGui::SameLine();
            bool canRedo = SceneManager::GetInstance().CanRedo();
            if (!canRedo) ImGui::BeginDisabled();
            if (ImGui::Button("重做")) { redo(); }
            if (!canRedo) ImGui::EndDisabled();
        }
        drawFpsDisplay();
        ImGui::EndMainMenuBar();
    }
}
void ToolbarPanel::drawProjectMenu()
{
    if (ImGui::BeginMenu("项目"))
    {
        bool isProjectLoaded = ProjectSettings::GetInstance().IsProjectLoaded();
        if (!isProjectLoaded) ImGui::BeginDisabled();
        if (ImGui::MenuItem("打包设置..."))
        {
            m_isSettingsWindowVisible = true;
        }
        ImGui::Separator();
        auto& assetManager = AssetManager::GetInstance();
        bool isPreWarming = assetManager.IsPreWarmingRunning();
        if (isPreWarming)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::MenuItem("烘焙 Shader"))
        {
            if (assetManager.StartPreWarmingShader())
            {
                LogInfo("开始烘焙 Shader...");
            }
            else
            {
                LogWarn("Shader 烘焙已在运行或已完成");
            }
        }
        if (isPreWarming)
        {
            ImGui::EndDisabled();
            auto [total, loaded] = assetManager.GetPreWarmingProgress();
            if (total > 0)
            {
                float progress = static_cast<float>(loaded) / static_cast<float>(total);
                ImGui::Text("烘焙进度: %d/%d (%.1f%%)", loaded, total, progress * 100.0f);
            }
        }
        else if (assetManager.IsPreWarmingComplete())
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ 烘焙完成");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("编译脚本")) { rebuildScripts(); }
        if (ImGui::MenuItem("清理编译产物"))
        {
            const std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
            const std::filesystem::path libraryDir = projectRoot / "Library";
            if (std::filesystem::exists(libraryDir))
            {
                std::error_code ec;
                std::filesystem::remove_all(libraryDir, ec);
                if (ec) { LogError("清理编译产物失败: {}", ec.message()); }
                else { LogInfo("编译产物已清理。"); }
            }
            else { LogInfo("Library 目录不存在，无需清理。"); }
        }
        ImGui::Separator();
        PluginManager::GetInstance().DrawPluginMenuItems("项目");
        if (!isProjectLoaded) ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}
void ToolbarPanel::drawSettingsWindow()
{
    if (!m_isSettingsWindowVisible) { return; }
    ImGui::Begin("打包设置 (Build Settings)", &m_isSettingsWindowVisible, ImGuiWindowFlags_AlwaysAutoResize);
    auto& settings = ProjectSettings::GetInstance();
    const std::filesystem::path projectRoot = settings.GetProjectRoot();
    ImGui::Text("应用信息");
    ImGui::Separator();
    char appNameBuffer[128];
#ifdef _WIN32
    strcpy_s(appNameBuffer, sizeof(appNameBuffer), settings.GetAppName().c_str());
#else
    strncpy(appNameBuffer, settings.GetAppName().c_str(), sizeof(appNameBuffer));
    appNameBuffer[sizeof(appNameBuffer) - 1] = '\0';
#endif
    if (ImGui::InputText("应用名称", appNameBuffer, sizeof(appNameBuffer)))
    {
        settings.SetAppName(appNameBuffer);
    }
    ImGui::PushID("StartupScene");
    ImGui::Text("启动场景");
    ImGui::SameLine();
    std::string sceneName = "无";
    if (settings.GetStartScene().Valid())
    {
        sceneName = AssetManager::GetInstance().GetAssetName(settings.GetStartScene());
    }
    ImGui::Button(sceneName.c_str(), ImVec2(200, 0));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
            if (meta && meta->type == AssetType::Scene) { settings.SetStartScene(handle.assetGuid); }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
    ImGui::PushID("AppIcon");
    ImGui::Text("应用图标 (.png)");
    ImGui::SameLine();
    std::string iconPath = settings.GetAppIconPath().empty() ? "无" : settings.GetAppIconPath().filename().string();
    ImGui::Button(iconPath.c_str(), ImVec2(200, 0));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
            if (meta && meta->type == AssetType::Texture) { settings.SetAppIconPath(meta->assetPath); }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
    ImGui::Spacing();
    ImGui::Text("构建目标");
    ImGui::Separator();
    TargetPlatform currentPlatform = settings.GetTargetPlatform();
    const char* platformNames[] = {"当前平台", "Windows", "Linux", "Android"};
    const TargetPlatform platformValues[] = {
        TargetPlatform::Current, TargetPlatform::Windows, TargetPlatform::Linux, TargetPlatform::Android
    };
    const char* currentPlatformName = "未知";
    for (int i = 0; i < IM_ARRAYSIZE(platformValues); ++i)
    {
        if (platformValues[i] == currentPlatform)
        {
            currentPlatformName = platformNames[i];
            break;
        }
    }
    ImGui::Text("目标平台:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##PlatformSelector", currentPlatformName))
    {
        for (int i = 0; i < IM_ARRAYSIZE(platformNames); ++i)
        {
            const bool isSelected = (currentPlatform == platformValues[i]);
            if (ImGui::Selectable(platformNames[i], isSelected))
            {
                settings.SetTargetPlatform(platformValues[i]);
            }
            if (isSelected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();
    ImGui::Text("窗口与分辨率");
    ImGui::Separator();
    int resolution[2] = {settings.GetTargetWidth(), settings.GetTargetHeight()};
    if (ImGui::InputInt2("目标分辨率", resolution))
    {
        settings.SetTargetWidth(resolution[0] > 0 ? resolution[0] : 1);
        settings.SetTargetHeight(resolution[1] > 0 ? resolution[1] : 1);
    }
    bool isFullscreen = settings.IsFullscreen();
    if (ImGui::Checkbox("默认全屏启动", &isFullscreen)) { settings.SetFullscreen(isFullscreen); }
    bool isBorderless = settings.IsBorderless();
    if (ImGui::Checkbox("无边框窗口", &isBorderless)) { settings.SetBorderless(isBorderless); }
    bool enableConsole = settings.IsConsoleEnabled();
    if (ImGui::Checkbox("启用控制台 (仅运行时)", &enableConsole)) { settings.SetConsoleEnabled(enableConsole); }
    if (settings.GetTargetPlatform() == TargetPlatform::Android)
    {
        ImGui::Spacing();
        ImGui::Text("Android 构建");
        ImGui::Separator();
        auto copyStringToBuffer = [](char* buffer, size_t size, const std::string& value)
        {
#ifdef _WIN32
            strcpy_s(buffer, size, value.c_str());
#else
            std::strncpy(buffer, value.c_str(), size);
            buffer[size - 1] = '\0';
#endif
        };
        char packageBuffer[256]{};
        copyStringToBuffer(packageBuffer, sizeof(packageBuffer), settings.GetAndroidPackageName());
        if (ImGui::InputText("包名 (Application Id)", packageBuffer, sizeof(packageBuffer)))
        {
            settings.SetAndroidPackageName(packageBuffer);
        }
        char apkNameBuffer[128]{};
        copyStringToBuffer(apkNameBuffer, sizeof(apkNameBuffer), settings.GetAndroidApkName());
        if (ImGui::InputText("APK 文件名 (不含 .apk)", apkNameBuffer, sizeof(apkNameBuffer)))
        {
            settings.SetAndroidApkName(apkNameBuffer);
        }
        const char* orientationNames[] = {"竖屏", "左横屏", "右横屏"};
        AndroidScreenOrientation orientations[] = {
            AndroidScreenOrientation::Portrait,
            AndroidScreenOrientation::LandscapeLeft,
            AndroidScreenOrientation::LandscapeRight
        };
        int orientationIndex = 0;
        for (int i = 0; i < 3; ++i)
        {
            if (orientations[i] == settings.GetAndroidScreenOrientation())
            {
                orientationIndex = i;
                break;
            }
        }
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("屏幕方向", orientationNames[orientationIndex]))
        {
            for (int i = 0; i < 3; ++i)
            {
                bool selected = (orientationIndex == i);
                if (ImGui::Selectable(orientationNames[i], selected))
                {
                    settings.SetAndroidScreenOrientation(orientations[i]);
                    orientationIndex = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        auto clampPositive = [](int value, int fallback) { return value <= 0 ? fallback : value; };
        int compileSdk = settings.GetAndroidCompileSdk();
        if (ImGui::InputInt("Compile SDK", &compileSdk))
        {
            settings.SetAndroidCompileSdk(clampPositive(compileSdk, 36));
        }
        int targetSdk = settings.GetAndroidTargetSdk();
        if (ImGui::InputInt("Target SDK", &targetSdk))
        {
            settings.SetAndroidTargetSdk(clampPositive(targetSdk, compileSdk > 0 ? compileSdk : 36));
        }
        int minSdk = settings.GetAndroidMinSdk();
        if (ImGui::InputInt("Min SDK", &minSdk))
        {
            settings.SetAndroidMinSdk(clampPositive(minSdk, 21));
        }
        int maxVersion = settings.GetAndroidMaxVersion();
        if (ImGui::InputInt("Max Version", &maxVersion))
        {
            settings.SetAndroidMaxVersion(clampPositive(maxVersion, settings.GetAndroidTargetSdk()));
        }
        int minVersion = settings.GetAndroidMinVersion();
        if (ImGui::InputInt("Min Version", &minVersion))
        {
            settings.SetAndroidMinVersion(clampPositive(minVersion, settings.GetAndroidMinSdk()));
        }
        int versionCode = settings.GetAndroidVersionCode();
        if (ImGui::InputInt("Version Code", &versionCode))
        {
            settings.SetAndroidVersionCode(clampPositive(versionCode, 1));
        }
        std::array<char, 64> versionNameBuffer{};
        copyStringToBuffer(versionNameBuffer.data(), versionNameBuffer.size(), settings.GetAndroidVersionName());
        if (ImGui::InputText("Version Name", versionNameBuffer.data(), versionNameBuffer.size()))
        {
            settings.SetAndroidVersionName(versionNameBuffer.data());
        }
        ImGui::Spacing();
        ImGui::Text("签名信息");
        ImGui::Separator();
        auto drawStringField = [&](const char* label, std::string currentValue, auto setter,
                                   ImGuiInputTextFlags flags = 0)
        {
            std::array<char, 256> buffer{};
            copyStringToBuffer(buffer.data(), buffer.size(), currentValue);
            if (ImGui::InputText(label, buffer.data(), buffer.size(), flags))
            {
                setter(std::string(buffer.data()));
            }
        };
        std::array<char, 512> keystorePathBuffer{};
        copyStringToBuffer(keystorePathBuffer.data(), keystorePathBuffer.size(),
                           settings.GetAndroidKeystorePath().string());
        if (ImGui::InputText("Keystore 路径", keystorePathBuffer.data(), keystorePathBuffer.size()))
        {
            settings.SetAndroidKeystorePath(std::filesystem::path(keystorePathBuffer.data()));
        }
        ImGui::SameLine();
        if (ImGui::Button("浏览文件..."))
        {
            if (SDL_Window* window = getSDLWindow())
            {
                const SDL_DialogFileFilter filters[] = {{"Keystore", "keystore"}, {"Keystore", "jks"}};
                SDL_ShowOpenFileDialog(OnKeystoreFileSelected, this, window, filters, 2, nullptr, false);
            }
        }
        drawStringField("Keystore 口令", settings.GetAndroidKeystorePassword(),
                        [&](const std::string& pwd) { settings.SetAndroidKeystorePassword(pwd); },
                        ImGuiInputTextFlags_Password);
        const auto& aliasEntries = settings.GetAndroidAliasEntries();
        int activeAlias = settings.GetActiveAndroidAliasIndex();
        std::string aliasLabel = (activeAlias >= 0 && activeAlias < static_cast<int>(aliasEntries.size()))
                                     ? aliasEntries[static_cast<size_t>(activeAlias)].alias
                                     : (settings.GetAndroidKeyAlias().empty() ? "未选择" : settings.GetAndroidKeyAlias());
        ImGui::Text("签名别名");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("##ActiveAlias", aliasLabel.c_str()))
        {
            for (int i = 0; i < static_cast<int>(aliasEntries.size()); ++i)
            {
                bool selected = (i == activeAlias);
                if (ImGui::Selectable(aliasEntries[static_cast<size_t>(i)].alias.c_str(), selected))
                {
                    settings.SetActiveAndroidAliasIndex(i);
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("创建别名..."))
        {
            if (settings.GetAndroidKeystorePath().empty() || settings.GetAndroidKeystorePassword().empty())
            {
                LogError("请先配置 Keystore 路径和口令。");
            }
            else
            {
                std::fill(std::begin(m_aliasPopupState.alias), std::end(m_aliasPopupState.alias), 0);
                std::fill(std::begin(m_aliasPopupState.password), std::end(m_aliasPopupState.password), 0);
                std::fill(std::begin(m_aliasPopupState.passwordConfirm), std::end(m_aliasPopupState.passwordConfirm),
                          0);
                m_aliasPopupState.errorMessage.clear();
                m_aliasPopupState.openRequested = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("删除别名"))
        {
            if (activeAlias >= 0)
            {
                settings.RemoveAndroidAliasEntry(static_cast<size_t>(activeAlias));
            }
        }
        if (aliasEntries.empty())
        {
            drawStringField("私钥别名", settings.GetAndroidKeyAlias(),
                            [&](const std::string& alias) { settings.SetAndroidKeyAlias(alias); });
            drawStringField("别名口令", settings.GetAndroidKeyPassword(),
                            [&](const std::string& pwd) { settings.SetAndroidKeyPassword(pwd); },
                            ImGuiInputTextFlags_Password);
        }
        if (ImGui::Button("创建新的 Keystore..."))
        {
            std::fill(std::begin(m_keystorePopupState.path), std::end(m_keystorePopupState.path), 0);
            std::string defaultPath = settings.GetAndroidKeystorePath().empty()
                                          ? (projectRoot / "AndroidKeystore" / "luma.keystore").string()
                                          : settings.GetAndroidKeystorePath().string();
            copyStringToBuffer(m_keystorePopupState.path, IM_ARRAYSIZE(m_keystorePopupState.path), defaultPath);
            std::fill(std::begin(m_keystorePopupState.storePassword), std::end(m_keystorePopupState.storePassword), 0);
            std::fill(std::begin(m_keystorePopupState.storePasswordConfirm),
                      std::end(m_keystorePopupState.storePasswordConfirm), 0);
            std::fill(std::begin(m_keystorePopupState.aliasPassword), std::end(m_keystorePopupState.aliasPassword), 0);
            std::fill(std::begin(m_keystorePopupState.aliasPasswordConfirm),
                      std::end(m_keystorePopupState.aliasPasswordConfirm), 0);
            copyStringToBuffer(m_keystorePopupState.alias, IM_ARRAYSIZE(m_keystorePopupState.alias),
                               settings.GetAndroidKeyAlias().empty() ? "luma_key" : settings.GetAndroidKeyAlias());
            m_keystorePopupState.errorMessage.clear();
            m_keystorePopupState.openRequested = true;
        }
        ImGui::Spacing();
        ImGui::Text("Android 图标");
        ImGui::Separator();
        const int iconSizes[] = {32, 48, 72, 96, 144, 192};
        for (int size : iconSizes)
        {
            ImGui::PushID(size);
            const auto& iconPath = settings.GetAndroidIconPath(size);
            std::string buttonLabel = iconPath.empty() ? "未设置" : iconPath.filename().string();
            ImGui::Text("%dx%d", size, size);
            ImGui::SameLine(120);
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(200, 0)))
            {
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
                {
                    AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
                    const auto* meta = AssetManager::GetInstance().GetMetadata(handle.assetGuid);
                    if (meta && meta->type == AssetType::Texture)
                    {
                        settings.SetAndroidIconPath(size, meta->assetPath);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("清除"))
            {
                settings.ClearAndroidIconPath(size);
            }
            ImGui::SameLine();
            if (!settings.GetAppIconPath().empty())
            {
                if (ImGui::SmallButton("复用应用图标"))
                {
                    settings.SetAndroidIconPath(size, settings.GetAppIconPath());
                }
            }
            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::Text("Android 权限");
        ImGui::Separator();
        static int selectedPermissionIdx = 0;
        const char* comboLabel = kAndroidPermissionOptions[selectedPermissionIdx].label;
        ImGui::SetNextItemWidth(280);
        if (ImGui::BeginCombo("常见权限", comboLabel))
        {
            for (int i = 0; i < IM_ARRAYSIZE(kAndroidPermissionOptions); ++i)
            {
                bool isSelected = (selectedPermissionIdx == i);
                if (ImGui::Selectable(kAndroidPermissionOptions[i].label, isSelected))
                {
                    selectedPermissionIdx = i;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("添加"))
        {
            settings.AddAndroidPermission(kAndroidPermissionOptions[selectedPermissionIdx].permission);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", kAndroidPermissionOptions[selectedPermissionIdx].permission);
        auto trimPermission = [](std::string& text)
        {
            auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            text.erase(text.begin(),
                       std::find_if(text.begin(), text.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            text.erase(std::find_if(text.rbegin(), text.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(),
                       text.end());
        };
        static char customPermissionBuffer[256] = "";
        ImGui::SetNextItemWidth(320);
        bool commitCustomPermission = ImGui::InputTextWithHint(
            "自定义权限", "android.permission.MY_PERMISSION",
            customPermissionBuffer, IM_ARRAYSIZE(customPermissionBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("添加权限") || commitCustomPermission)
        {
            std::string perm = customPermissionBuffer;
            trimPermission(perm);
            if (!perm.empty())
            {
                settings.AddAndroidPermission(perm);
                std::fill(std::begin(customPermissionBuffer), std::end(customPermissionBuffer), 0);
            }
        }
        const auto& currentPermissions = settings.GetAndroidPermissions();
        if (!currentPermissions.empty())
        {
            ImGui::Text("已添加的权限:");
            ImGui::BeginChild("PermissionList", ImVec2(0, 120), true);
            for (size_t i = 0; i < currentPermissions.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                ImGui::BulletText("%s", currentPermissions[i].c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("移除"))
                {
                    settings.RemoveAndroidPermission(currentPermissions[i]);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextDisabled("尚未添加任何权限，默认将包含 android.permission.VIBRATE。");
        }
        ImGui::TextDisabled("以上权限会写入 AndroidManifest.xml 的 <uses-permission/> 列表。");
        ImGui::Spacing();
        bool useCustomManifest = settings.IsCustomAndroidManifestEnabled();
        if (ImGui::Checkbox("启用自定义 AndroidManifest.xml", &useCustomManifest))
        {
            settings.SetCustomAndroidManifestEnabled(useCustomManifest);
        }
        if (useCustomManifest)
        {
            const auto manifestPath = settings.GetCustomAndroidManifestPath();
            if (!manifestPath.empty())
            {
                ImGui::TextWrapped("路径: %s", manifestPath.string().c_str());
                if (ImGui::SmallButton("打开 Android 目录"))
                {
                    platform_native::OpenDirectoryInExplorer(manifestPath.parent_path());
                }
            }
        }
        bool useCustomGradle = settings.IsCustomGradlePropertiesEnabled();
        if (ImGui::Checkbox("启用自定义 gradle.properties", &useCustomGradle))
        {
            settings.SetCustomGradlePropertiesEnabled(useCustomGradle);
        }
        if (useCustomGradle)
        {
            const auto gradlePath = settings.GetCustomGradlePropertiesPath();
            if (!gradlePath.empty())
            {
                ImGui::TextWrapped("路径: %s", gradlePath.string().c_str());
                if (ImGui::SmallButton("打开 gradle.properties 目录"))
                {
                    platform_native::OpenDirectoryInExplorer(gradlePath.parent_path());
                }
            }
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("保存设置", ImVec2(120, 30)))
    {
        settings.Save();
        LogInfo("项目设置已保存至: {}", settings.GetProjectFilePath().string());
    }
    ImGui::SameLine();
    if (ImGui::Button("打包游戏", ImVec2(120, 30)))
    {
        settings.Save();
        LogInfo("项目设置已自动保存，开始打包...");
        packageGame();
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭", ImVec2(120, 30)))
    {
        settings.Load();
        m_isSettingsWindowVisible = false;
    }
    drawKeystorePickerPopup(projectRoot);
    drawCreateKeystorePopup();
    drawCreateAliasPopup();
    ImGui::End();
}
void ToolbarPanel::drawEditMenu()
{
    if (ImGui::BeginMenu("编辑"))
    {
        if (ImGui::MenuItem("偏好设置...")) { PopupManager::GetInstance().Open("PreferencesPopup"); }
        ImGui::EndMenu();
    }
}
void ToolbarPanel::drawFileMenu()
{
    if (ImGui::BeginMenu("文件"))
    {
        if (ImGui::MenuItem("新建游戏项目...")) { m_context->editor->CreateNewProject(); }
        if (ImGui::MenuItem("新建插件项目...")) { m_context->editor->CreateNewPluginProject(); }
        if (ImGui::MenuItem("打开项目...")) { m_context->editor->OpenProject(); }
        ImGui::Separator();
        bool isProjectLoaded = ProjectSettings::GetInstance().IsProjectLoaded();
        if (!isProjectLoaded) ImGui::BeginDisabled();
        if (ImGui::MenuItem("新建场景", "Ctrl+N"))
        {
            newScene();
        }
        if (ImGui::MenuItem("保存场景", "Ctrl+S"))
        {
            saveScene();
        }
        ImGui::Separator();
        if (!isProjectLoaded) ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("退出"))
        {
        }
        PluginManager::GetInstance().DrawPluginMenuItems("文件");
        ImGui::EndMenu();
    }
}
void ToolbarPanel::drawPlayControls()
{
    if (m_context->editorState == EditorState::Editing)
    {
        if (ImGui::Button("播放")) { play(); }
    }
    else
    {
        if (ImGui::Button("停止")) { stop(); }
        ImGui::SameLine();
        const char* pauseLabel = (m_context->editorState == EditorState::Paused) ? "继续" : "暂停";
        if (ImGui::Button(pauseLabel)) { pause(); }
    }
}
void ToolbarPanel::drawFpsDisplay()
{
    std::string statsText = std::format("FPS: {0:.1f} ({1:.2f}ms) | UPS: {2:.1f} ({3:.2f}ms)",
                                        m_context->lastFps,
                                        m_context->renderLatency,
                                        m_context->lastUps,
                                        m_context->updateLatency);
    const float textWidth = ImGui::CalcTextSize(statsText.c_str()).x;
    const float rightPadding = ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - rightPadding);
    ImGui::Text("%s", statsText.c_str());
}
void ToolbarPanel::updateFps()
{
    m_context->frameCount++;
    auto currentTime = std::chrono::steady_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(currentTime - m_context->lastFpsUpdateTime).count();
    if (elapsedSeconds >= 1.0)
    {
        m_context->lastFps = static_cast<float>(m_context->frameCount / elapsedSeconds);
        m_context->frameCount = 0;
        m_context->lastFpsUpdateTime = currentTime;
    }
}
void ToolbarPanel::newScene()
{
    if (!m_context || !m_context->engineContext)
    {
        LogError("无法创建新场景：EditorContext 未初始化。");
        return;
    }
    auto queueNewScene = [ctx = m_context]()
    {
        if (ctx->activeScene)
        {
            LogInfo("停用旧场景以创建新场景");
            ctx->activeScene->Deactivate();
        }
        sk_sp<RuntimeScene> newScene = sk_make_sp<RuntimeScene>();
        newScene->SetName("NewScene");
        newScene->AddEssentialSystem<Systems::HydrateResources>();
        newScene->AddEssentialSystem<Systems::TransformSystem>();
        newScene->Activate(*ctx->engineContext);
        SceneManager::GetInstance().SetCurrentScene(newScene);
        ctx->activeScene = newScene;
        ctx->selectionType = SelectionType::NA;
        ctx->selectionList.clear();
    };
    m_context->engineContext->commandsForSim.Push(queueNewScene);
}
void ToolbarPanel::saveScene()
{
    if (m_context->editingMode == EditingMode::Prefab)
    {
        if (m_context->editingMode != EditingMode::Prefab || !m_context->activeScene) return;
        auto& rootObjects = m_context->activeScene->GetRootGameObjects();
        if (rootObjects.empty() || !rootObjects[0].IsValid())
        {
            LogError("Prefab场景为空或无效，无法保存。");
            return;
        }
        Data::PrefabNode rootNode = rootObjects[0].SerializeToPrefabData();
        Data::PrefabData prefabData;
        prefabData.root = rootNode;
        auto& assetManager = AssetManager::GetInstance();
        const AssetMetadata* meta = assetManager.GetMetadata(m_context->editingPrefabGuid);
        if (!meta)
        {
            LogError("找不到Prefab资产的元数据，GUID: {}", m_context->editingPrefabGuid.ToString());
            return;
        }
        std::filesystem::path fullPath = assetManager.GetAssetsRootPath() / meta->assetPath;
        YAML::Emitter emitter;
        emitter.SetIndent(2);
        emitter << YAML::convert<Data::PrefabData>::encode(prefabData);
        std::ofstream fout(fullPath);
        fout << emitter.c_str();
        fout.close();
        m_context->assetBrowserRefreshTimer = 0.0f;
        LogInfo("Prefab资产已成功保存到: {}", fullPath.string());
    }
    else
    {
        if (!m_context->activeScene->GetGuid().Valid()) { LogWarn("'另存为'功能尚未实现。"); }
        else { SceneManager::GetInstance().SaveScene(m_context->activeScene); }
    }
}
void ToolbarPanel::play()
{
    if (m_context->editorState != EditorState::Editing) return;
    if (m_isTransitioningPlayState)
    {
        LogWarn("正在切换到播放模式，请稍候...");
        return;
    }
    m_isTransitioningPlayState = true;
    auto switchToPlayMode = [this, ctx = m_context]()
    {
        if (ctx->editorState == EditorState::Playing)
        {
            m_isTransitioningPlayState = false;
            return;
        }
        ctx->editorState = EditorState::Playing;
        *ctx->engineContext->appMode = ApplicationMode::PIE;
        ctx->editingScene = ctx->activeScene;
        sk_sp<RuntimeScene> playScene = ctx->editingScene->CreatePlayModeCopy();
        playScene->AddEssentialSystem<Systems::HydrateResources>();
        playScene->AddEssentialSystem<Systems::TransformSystem>();
        playScene->AddSystem<Systems::PhysicsSystem>();
        playScene->AddSystem<Systems::AudioSystem>();
        playScene->AddSystem<Systems::InteractionSystem>();
        playScene->AddSystem<Systems::ButtonSystem>();
        playScene->AddSystemToMainThread<Systems::InputTextSystem>();
        playScene->AddSystem<Systems::CommonUIControlSystem>();
        playScene->AddSystem<Systems::ScriptingSystem>();
        playScene->AddSystem<Systems::AnimationSystem>();
        playScene->AddSystem<Systems::ParticleSystem>();
        SceneManager::GetInstance().SetCurrentScene(playScene);
        playScene->Activate(*ctx->engineContext);
        std::cout << "原始场景地址: " << ctx->editingScene.get() << std::endl;
        ctx->activeScene = playScene;
        m_isTransitioningPlayState = false;
        LogInfo("已通过命令队列安全进入播放模式。");
        std::cout << "场景地址: " << playScene.get() << std::endl;
    };
    m_context->engineContext->commandsForSim.Push(switchToPlayMode);
}
void ToolbarPanel::stop()
{
    if (m_context->editorState == EditorState::Editing) return;
    if (!m_context->engineContext)
    {
        LogError("无法退出播放模式：EngineContext 不可用。");
        return;
    }
    if (m_isTransitioningPlayState)
    {
        LogWarn("正在切换到编辑模式，请稍候...");
        return;
    }
    m_isTransitioningPlayState = true;
    auto stopCommand = [this, ctx = m_context]()
    {
        if (ctx->editorState == EditorState::Editing)
        {
            m_isTransitioningPlayState = false;
            return;
        }
        if (ctx->activeScene)
        {
            LogInfo("停用播放场景");
            ctx->activeScene->Deactivate();
        }
        ctx->editorState = EditorState::Editing;
        *ctx->engineContext->appMode = ApplicationMode::Editor;
        ctx->activeScene.reset();
        ctx->activeScene = ctx->editingScene;
        SceneManager::GetInstance().SetCurrentScene(ctx->activeScene);
        ctx->editingScene.reset();
        m_isTransitioningPlayState = false;
        LogInfo("退出播放模式。");
    };
    m_context->engineContext->commandsForSim.Push(stopCommand);
}
void ToolbarPanel::pause()
{
    if (m_context->editorState == EditorState::Playing)
    {
        m_context->editorState = EditorState::Paused;
        LogInfo("执行已暂停。");
    }
    else if (m_context->editorState == EditorState::Paused)
    {
        m_context->editorState = EditorState::Playing;
        LogInfo("执行已恢复。");
    }
}
void ToolbarPanel::undo() { SceneManager::GetInstance().Undo(); }
void ToolbarPanel::redo() { SceneManager::GetInstance().Redo(); }
void ToolbarPanel::drawSaveBeforePackagingPopup()
{
    ImGui::Text("当前场景有未保存的修改。\n是否要在打包前保存？");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    if (ImGui::Button("确定", ImVec2(120, 0)))
    {
        saveScene();
        startPackagingProcess();
        PopupManager::GetInstance().Close("SaveScene");
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("取消", ImVec2(120, 0)))
    {
        startPackagingProcess();
        PopupManager::GetInstance().Close("SaveScene");
    }
}
void ToolbarPanel::packageGame()
{
    if (m_isPackaging)
    {
        LogWarn("打包已在进行中。");
        return;
    }
    if (m_context->activeScene && SceneManager::GetInstance().IsCurrentSceneDirty())
    {
        PopupManager::GetInstance().Open("SaveScene");
    }
    else
    {
        startPackagingProcess();
    }
}
void ToolbarPanel::handleShortcuts()
{
    if (!m_context->editor->GetPanelByName("场景")->IsFocused())
    {
        return;
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::N.IsPressed())
    {
        newScene();
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::S.IsPressed())
    {
        saveScene();
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::P.IsPressed())
    {
        if (m_context->editorState == EditorState::Editing) { play(); }
        else if (m_context->editorState != EditorState::Editing) { stop(); }
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::D.IsPressed())
    {
        if (m_context->editorState == EditorState::Playing) { pause(); }
        else if (m_context->editorState == EditorState::Paused) { m_context->editorState = EditorState::Playing; }
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::Z.IsPressed())
    {
        undo();
    }
    if (Keyboard::LeftCtrl.IsPressed() && Keyboard::LeftShift.IsPressed() && Keyboard::Z.IsPressed())
    {
        redo();
    }
}
void ToolbarPanel::startPackagingProcess()
{
    if (m_isPackaging) return;
    m_isPackaging = true;
    m_packagingSuccess = false;
    m_lastBuildDirectory.clear();
    m_packagingProgress = 0.0f;
    m_packagingStatus = "正在准备...";
    m_packagingFuture = std::async(std::launch::async, [this]()
    {
        try
        {
            auto& settings = ProjectSettings::GetInstance();
            const std::filesystem::path projectRoot = settings.GetProjectRoot();
            const std::filesystem::path buildRoot = projectRoot / "build";
            TargetPlatform targetPlatform = settings.GetTargetPlatform();
            if (targetPlatform == TargetPlatform::Current)
            {
                targetPlatform = ProjectSettings::GetCurrentHostPlatform();
            }
            std::string targetPlatformStr = ProjectSettings::PlatformToString(targetPlatform);
            m_packagingStatus = "正在确定引擎模板包路径...";
            const std::filesystem::path editorRoot = ".";
            std::filesystem::path templateDir;
            templateDir = editorRoot / "Publish" / targetPlatformStr;
            if (!std::filesystem::exists(templateDir))
            {
                throw std::runtime_error("引擎模板包未找到，请先使用CMake构建'publish'目标。\n路径: " + templateDir.string());
            }
            m_packagingProgress = 0.0f;
            m_packagingStatus = "正在编译 C# 脚本 (目标: " + targetPlatformStr + ")...";
            std::string compileStatus;
            if (!runScriptCompilationLogicForPackaging(compileStatus, targetPlatform))
            {
                throw std::runtime_error("脚本编译失败，打包已中止。详情: " + compileStatus);
            }
            std::filesystem::path platformOutputDir = (targetPlatform == TargetPlatform::Android)
                                                          ? buildRoot / "Android"
                                                          : buildRoot;
            auto stageLibcxxShared = [&](const char* abi)
            {
                auto& prefs = PreferenceSettings::GetInstance();
                auto libcxxSource = prefs.GetLibcxxSharedPath(abi);
                if (libcxxSource.empty())
                {
                    LogWarn("未找到 libc++_shared.so (ABI: {}). 请在偏好设置中配置 Android NDK 路径。", abi);
                    return;
                }
                const auto jniLibDir = platformOutputDir / "app/src/main/jniLibs" / abi;
                std::error_code ec;
                std::filesystem::create_directories(jniLibDir, ec);
                const auto dest = jniLibDir / "libc++_shared.so";
                std::filesystem::copy(libcxxSource, dest, std::filesystem::copy_options::overwrite_existing);
                LogInfo("已复制 libc++_shared.so 到 {}", dest.string());
            };
            m_packagingProgress = 0.1f;
            m_packagingStatus = "正在清理并复制引擎模板...";
            if (std::filesystem::exists(platformOutputDir))
            {
                std::filesystem::remove_all(platformOutputDir);
            }
            if (!platformOutputDir.parent_path().empty())
            {
                std::filesystem::create_directories(platformOutputDir.parent_path());
            }
            if (targetPlatform == TargetPlatform::Android)
            {
                const auto projectAndroidDir = settings.GetProjectAndroidDirectory();
                std::filesystem::copy(templateDir, platformOutputDir,
                                      std::filesystem::copy_options::recursive |
                                      std::filesystem::copy_options::overwrite_existing);
                if (settings.IsCustomAndroidManifestEnabled())
                {
                    std::filesystem::copy(settings.GetCustomAndroidManifestPath(),
                                          platformOutputDir / "app/src/main/AndroidManifest.xml",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                if (settings.IsCustomGradlePropertiesEnabled())
                {
                    std::filesystem::copy(settings.GetCustomGradlePropertiesPath(),
                                          platformOutputDir / "gradle.properties",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                stageLibcxxShared("arm64-v8a");
            }
            else
            {
                std::filesystem::copy(templateDir, platformOutputDir,
                                      std::filesystem::copy_options::recursive |
                                      std::filesystem::copy_options::overwrite_existing);
            }
            auto assetRoot = (targetPlatform == TargetPlatform::Android)
                                 ? platformOutputDir / "app/src/main/assets"
                                 : platformOutputDir;
            std::filesystem::create_directories(assetRoot);
            const std::filesystem::path resourcesDir = assetRoot / "Resources";
            const std::filesystem::path gameDataDir = assetRoot / "GameData";
            const std::filesystem::path rawDestDir = assetRoot / "Raw";
            std::filesystem::create_directories(resourcesDir);
            std::filesystem::create_directories(gameDataDir);
            std::filesystem::create_directories(rawDestDir);
            m_packagingProgress = 0.25f;
            m_packagingStatus = "正在导出 Shader 注册表...";
            auto& shaderRegistry = Nut::ShaderRegistry::GetInstance();
            std::filesystem::path shaderRegistryPath = resourcesDir / "ShaderRegistry.yaml";
            if (!shaderRegistry.SaveToFile(shaderRegistryPath.string()))
            {
                LogWarn("Shader 注册表导出失败，运行时可能无法预热 shader");
            }
            else
            {
                LogInfo("Shader 注册表已导出到: {}", shaderRegistryPath.string());
            }
            m_packagingProgress = 0.3f;
            m_packagingStatus = "正在打包项目资源...";
            if (!AssetPacker::Pack(AssetManager::GetInstance().GetAssetDatabase(), resourcesDir))
            {
                throw std::runtime_error("资源打包失败。");
            }
            m_packagingProgress = 0.4f;
            m_packagingStatus = "正在复制 Raw 资产...";
            const auto rawSourceDir = AssetManager::GetInstance().GetAssetsRootPath() / "Raw";
            if (std::filesystem::exists(rawSourceDir))
            {
                std::filesystem::copy(rawSourceDir, rawDestDir,
                                      std::filesystem::copy_options::recursive |
                                      std::filesystem::copy_options::overwrite_existing);
            }
            if (targetPlatform == TargetPlatform::Android)
            {
                const auto androidAssetsSourceDir = AssetManager::GetInstance().GetAssetsRootPath() / "Android";
                if (std::filesystem::exists(androidAssetsSourceDir))
                {
                    m_packagingStatus = "正在复制 Android 资源...";
                    const auto androidAssetsDestDir = assetRoot / "Android";
                    std::filesystem::create_directories(androidAssetsDestDir);
                    std::filesystem::copy(androidAssetsSourceDir, androidAssetsDestDir,
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
            }
            m_packagingProgress = 0.6f;
            m_packagingStatus = "正在复制 C# 程序集...";
            const std::filesystem::path csharpSourceDir = projectRoot / "Library/Temp";
            if (!std::filesystem::exists(csharpSourceDir))
                throw std::runtime_error("项目的 Library 目录不存在，请先编译脚本。");
            for (const auto& entry : std::filesystem::directory_iterator(csharpSourceDir))
            {
                const auto& ext = entry.path().extension();
                if (ext == ".dll" || ext == ".json" || ext == ".yaml")
                {
                    if (entry.is_regular_file())
                    {
                        std::filesystem::copy(entry.path(), gameDataDir / entry.path().filename(),
                                              std::filesystem::copy_options::overwrite_existing);
                    }
                }
            }
            auto copyAndroidLauncherIcon = [&](const std::filesystem::path& sourcePath, const std::string& bucketName)
            {
                if (sourcePath.empty() || !std::filesystem::exists(sourcePath)) return;
                const auto destDir = platformOutputDir / "app/src/main/res" / bucketName;
                if (!std::filesystem::exists(destDir))
                {
                    std::filesystem::create_directories(destDir);
                }
                auto cleanExisting = [&]()
                {
                    std::error_code ec;
                    for (const auto& entry : std::filesystem::directory_iterator(destDir, ec))
                    {
                        if (!entry.is_regular_file()) continue;
                        const auto stem = entry.path().stem().string();
                        if (stem == "ic_launcher" || stem == "ic_launcher_round")
                        {
                            std::filesystem::remove(entry.path(), ec);
                        }
                    }
                };
                cleanExisting();
                std::string ext = sourcePath.extension().string();
                if (ext.empty()) ext = ".png";
                const auto iconDest = destDir / ("ic_launcher" + ext);
                const auto roundDest = destDir / ("ic_launcher_round" + ext);
                std::filesystem::copy(sourcePath, iconDest, std::filesystem::copy_options::overwrite_existing);
                std::filesystem::copy(sourcePath, roundDest, std::filesystem::copy_options::overwrite_existing);
            };
            auto configureAndroidIcons = [&]()
            {
                if (targetPlatform != TargetPlatform::Android) return;
                const auto resDir = platformOutputDir / "app/src/main/res";
                std::error_code ec;
                std::filesystem::remove_all(resDir / "mipmap-anydpi-v26", ec);
                std::filesystem::remove_all(resDir / "mipmap-anydpi", ec);
                std::filesystem::remove(resDir / "drawable/ic_launcher_background.xml", ec);
                std::filesystem::remove(resDir / "drawable/ic_launcher_foreground.xml", ec);
                std::filesystem::remove(resDir / "drawable-v24/ic_launcher_foreground.xml", ec);
                const auto& iconMap = settings.GetAndroidIconMap();
                const auto assetsDir = settings.GetAssetsDirectory();
                std::unordered_set<int> customizedSizes;
                const std::vector<std::pair<int, std::string>> sizeToBucket = {
                    {32, "mipmap-ldpi"},
                    {48, "mipmap-mdpi"},
                    {72, "mipmap-hdpi"},
                    {96, "mipmap-xhdpi"},
                    {144, "mipmap-xxhdpi"},
                    {192, "mipmap-xxxhdpi"}
                };
                for (const auto& [size, relativePath] : iconMap)
                {
                    auto bucketIt = std::find_if(sizeToBucket.begin(), sizeToBucket.end(),
                                                 [&](const auto& pair) { return pair.first == size; });
                    if (bucketIt == sizeToBucket.end()) continue;
                    const std::filesystem::path source = assetsDir / relativePath;
                    copyAndroidLauncherIcon(source, bucketIt->second);
                    customizedSizes.insert(size);
                }
                const auto defaultIconRelative = settings.GetAppIconPath();
                if (defaultIconRelative.empty()) return;
                const std::filesystem::path defaultIconFullPath = assetsDir / defaultIconRelative;
                if (!std::filesystem::exists(defaultIconFullPath)) return;
                for (const auto& [size, bucket] : sizeToBucket)
                {
                    if (customizedSizes.count(size)) continue;
                    copyAndroidLauncherIcon(defaultIconFullPath, bucket);
                }
            };
            if (targetPlatform == TargetPlatform::Android)
            {
                m_packagingProgress = 0.8f;
                m_packagingStatus = "正在配置 Android 资源...";
                UpdateAndroidStringsXml(platformOutputDir / "app/src/main/res", settings.GetAndroidApkName());
                configureAndroidIcons();
            }
            else
            {
                m_packagingProgress = 0.8f;
                m_packagingStatus = "正在复制应用图标...";
                const std::filesystem::path iconSourceRelativePath = settings.GetAppIconPath();
                if (!iconSourceRelativePath.empty())
                {
                    const std::filesystem::path iconSourceFullPath = settings.GetAssetsDirectory() /
                        iconSourceRelativePath;
                    if (std::filesystem::exists(iconSourceFullPath))
                    {
                        const std::string destExtension = iconSourceFullPath.extension().string();
                        const std::filesystem::path iconDestPath = platformOutputDir / ("icon" + destExtension);
                        std::filesystem::copy(iconSourceFullPath, iconDestPath,
                                              std::filesystem::copy_options::overwrite_existing);
                    }
                    else
                    {
                        LogWarn("应用图标文件 '{}' 未找到，已跳过。", iconSourceFullPath.string());
                    }
                }
            }
            auto writeProjectSettingsAsset = [&]()
            {
                m_packagingProgress = 0.85f;
                m_packagingStatus = "正在写入项目配置...";
                const auto projectFilePath = settings.GetProjectFilePath();
                if (std::filesystem::exists(projectFilePath))
                {
                    std::vector<unsigned char> projectData;
                    std::ifstream projectFile(projectFilePath, std::ios::binary);
                    if (!projectFile.is_open())
                        throw std::runtime_error("无法打开项目配置文件: " + projectFilePath.string());
                    projectData.assign(std::istreambuf_iterator<char>(projectFile), std::istreambuf_iterator<char>());
                    std::vector<unsigned char> encryptedData = EngineCrypto::GetInstance().Encrypt(projectData);
                    const auto outputFilePath = assetRoot / "ProjectSettings.lproj";
                    std::ofstream outFile(outputFilePath, std::ios::binary);
                    if (!outFile.is_open())
                        throw std::runtime_error("无法写入项目配置文件到输出目录: " + outputFilePath.string());
                    outFile.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
                }
                else
                {
                    LogWarn("项目配置文件 (.lproj) 未找到，已跳过。");
                }
            };
            writeProjectSettingsAsset();
            if (targetPlatform == TargetPlatform::Android)
            {
                if (settings.GetAndroidKeystorePath().empty() ||
                    settings.GetAndroidKeystorePassword().empty() ||
                    settings.GetAndroidKeyAlias().empty() ||
                    settings.GetAndroidKeyPassword().empty())
                {
                    throw std::runtime_error("Android 平台打包需要配置 Keystore 路径、口令、别名及别名口令。");
                }
                updateAndroidGradleProperties(platformOutputDir, settings);
                const auto manifestDest = platformOutputDir / "app/src/main/AndroidManifest.xml";
                if (!manifestDest.parent_path().empty())
                {
                    std::error_code manifestDirEc;
                    std::filesystem::create_directories(manifestDest.parent_path(), manifestDirEc);
                }
                auto writeGeneratedManifest = [&]()
                {
                    std::ofstream manifestOut(manifestDest, std::ios::binary | std::ios::trunc);
                    manifestOut << settings.GenerateAndroidManifest();
                    LogInfo("已生成 AndroidManifest.xml (包含屏幕方向设置)");
                };
                bool copiedCustomManifest = false;
                if (settings.IsCustomAndroidManifestEnabled())
                {
                    const auto manifestSource = settings.GetCustomAndroidManifestPath();
                    if (!manifestSource.empty() && std::filesystem::exists(manifestSource))
                    {
                        std::filesystem::copy(manifestSource, manifestDest,
                                              std::filesystem::copy_options::overwrite_existing);
                        copiedCustomManifest = true;
                        LogInfo("已使用自定义 AndroidManifest.xml");
                    }
                    else
                    {
                        LogWarn("自定义 AndroidManifest.xml 已启用但文件不存在，将回退到自动生成。");
                    }
                }
                if (!copiedCustomManifest)
                {
                    writeGeneratedManifest();
                }
                m_packagingStatus = "正在执行 Gradle Release 构建...";
                std::string gradleCmd;
#ifdef _WIN32
                gradleCmd = "cd /d \"" + platformOutputDir.string() + "\" && gradlew.bat assembleRelease";
#else
                gradleCmd = "cd \"" + platformOutputDir.string() + "\" && ./gradlew assembleRelease";
#endif
                if (!ExecuteCommand(gradleCmd, "Gradle"))
                {
                    throw std::runtime_error("Gradle assembleRelease 构建失败，请检查 Gradle 日志。");
                }
                auto locateApk = [&](const std::filesystem::path& dir) -> std::filesystem::path
                {
                    const auto primary = dir / "app-release.apk";
                    if (std::filesystem::exists(primary)) return primary;
                    const auto secondary = dir / "app-release-unsigned.apk";
                    if (std::filesystem::exists(secondary)) return secondary;
                    return {};
                };
                auto apkSource = locateApk(platformOutputDir / "app/build/outputs/apk/release");
                if (!apkSource.empty() && apkSource.filename().string().find("unsigned") != std::string::npos)
                {
                    auto signedApk = signAndroidApk(apkSource, settings);
                    if (!signedApk.empty())
                    {
                        apkSource = signedApk;
                    }
                }
                if (!apkSource.empty())
                {
                    std::string apkName = settings.GetAndroidApkName();
                    if (apkName.empty())
                    {
                        apkName = apkSource.filename().string();
                    }
                    if (std::filesystem::path(apkName).extension() != ".apk")
                    {
                        apkName += ".apk";
                    }
                    const auto apkDest = buildRoot / apkName;
                    std::error_code apkEc;
                    if (!apkDest.parent_path().empty())
                    {
                        std::filesystem::create_directories(apkDest.parent_path(), apkEc);
                    }
                    std::filesystem::copy(apkSource, apkDest, std::filesystem::copy_options::overwrite_existing);
                    LogInfo("Gradle Release APK 输出至: {}", apkDest.string());
                    m_packagingStatus = "Gradle Release 构建完成";
                }
                else
                {
                    LogWarn("未找到 Gradle 生成的 app-release.apk 或 app-release-unsigned.apk");
                }
            }
            m_packagingProgress = 0.95f;
            m_packagingStatus = "游戏打包成功！目标平台: " + targetPlatformStr;
            m_packagingSuccess = true;
            m_lastBuildDirectory = platformOutputDir;
            LogInfo("========================================");
            LogInfo("游戏打包成功！目标平台: {}", targetPlatformStr);
            LogInfo("输出目录: {}", platformOutputDir.string());
            LogInfo("========================================");
        }
        catch (const std::exception& e)
        {
            m_packagingStatus = std::string("打包失败: ") + e.what();
            m_packagingSuccess = false;
            LogError("{}", m_packagingStatus);
        }
        m_packagingProgress = 1.0f;
    });
}
void ToolbarPanel::refreshKeystoreCandidates(const std::filesystem::path& projectRoot)
{
    m_keystoreCandidates.clear();
    std::vector<std::filesystem::path> searchRoots;
    if (!projectRoot.empty())
    {
        searchRoots.push_back(projectRoot);
        searchRoots.push_back(projectRoot / "Android");
    }
    auto hasKeystoreExtension = [](const std::filesystem::path& path)
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return ext == ".keystore" || ext == ".jks" || ext == ".ks";
    };
    constexpr size_t kMaxResults = 128;
    for (const auto& root : searchRoots)
    {
        if (root.empty() || !std::filesystem::exists(root)) continue;
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(
            root, std::filesystem::directory_options::skip_permission_denied, ec);
        if (ec) continue;
        for (auto end = std::filesystem::recursive_directory_iterator(); it != end; ++it)
        {
            if (it->is_directory()) continue;
            if (it->is_regular_file() && hasKeystoreExtension(it->path()))
            {
                std::error_code canonicalEc;
                auto resolved = std::filesystem::canonical(it->path(), canonicalEc);
                m_keystoreCandidates.push_back(canonicalEc ? it->path() : resolved);
                if (m_keystoreCandidates.size() >= kMaxResults) return;
            }
        }
        if (m_keystoreCandidates.size() >= kMaxResults) break;
    }
}
void ToolbarPanel::drawKeystorePickerPopup(const std::filesystem::path& projectRoot)
{
    if (m_shouldOpenKeystorePicker)
    {
        ImGui::OpenPopup("选择 Keystore");
        m_shouldOpenKeystorePicker = false;
    }
    if (ImGui::BeginPopupModal("选择 Keystore", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::Button("重新扫描"))
        {
            refreshKeystoreCandidates(projectRoot);
        }
        ImGui::Separator();
        ImGui::BeginChild("KeystoreList", ImVec2(520, 220), true);
        if (m_keystoreCandidates.empty())
        {
            ImGui::TextDisabled("在项目目录中未找到 keystore/jks 文件。");
        }
        else
        {
            for (const auto& path : m_keystoreCandidates)
            {
                const std::string display = path.string();
                if (ImGui::Selectable(display.c_str()))
                {
                    ProjectSettings::GetInstance().SetAndroidKeystorePath(path);
                    std::fill(m_keystorePickerBuffer.begin(), m_keystorePickerBuffer.end(), 0);
                    const auto& value = ProjectSettings::GetInstance().GetAndroidKeystorePath().string();
                    size_t len = std::min(value.size(), m_keystorePickerBuffer.size() - 1);
                    std::memcpy(m_keystorePickerBuffer.data(), value.data(), len);
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
        }
        ImGui::EndChild();
        ImGui::InputText("或手动输入路径", m_keystorePickerBuffer.data(), m_keystorePickerBuffer.size());
        if (ImGui::Button("使用此路径"))
        {
            std::filesystem::path customPath(m_keystorePickerBuffer.data());
            if (!customPath.empty())
            {
                ProjectSettings::GetInstance().SetAndroidKeystorePath(customPath);
                ImGui::CloseCurrentPopup();
            }
            else
            {
                LogWarn("请选择有效的 keystore 路径。");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("关闭"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
void ToolbarPanel::drawCreateKeystorePopup()
{
    constexpr const char* kPopupTitle = "创建新的 Keystore";
    auto& settings = ProjectSettings::GetInstance();
    if (m_keystorePopupState.openRequested)
    {
        ImGui::OpenPopup(kPopupTitle);
        m_keystorePopupState.openRequested = false;
    }
    if (ImGui::BeginPopupModal(kPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("保存位置", m_keystorePopupState.path, IM_ARRAYSIZE(m_keystorePopupState.path));
        ImGui::SameLine();
        if (ImGui::Button("浏览...##CreateKeystore"))
        {
            if (SDL_Window* window = getSDLWindow())
            {
                const SDL_DialogFileFilter filters[] = {{"Keystore", "keystore"}, {"Keystore", "jks"}};
                const char* defaultFile = m_keystorePopupState.path[0] ? m_keystorePopupState.path : "luma.keystore";
                SDL_ShowSaveFileDialog(OnKeystoreSavePathSelected, this, window, filters, 2, defaultFile);
            }
            else
            {
                LogWarn("无法打开保存对话框，SDL 窗口无效。");
            }
        }
        ImGui::InputText("Keystore 口令", m_keystorePopupState.storePassword,
                         IM_ARRAYSIZE(m_keystorePopupState.storePassword), ImGuiInputTextFlags_Password);
        ImGui::InputText("确认口令", m_keystorePopupState.storePasswordConfirm,
                         IM_ARRAYSIZE(m_keystorePopupState.storePasswordConfirm), ImGuiInputTextFlags_Password);
        ImGui::InputText("别名", m_keystorePopupState.alias, IM_ARRAYSIZE(m_keystorePopupState.alias));
        ImGui::InputText("别名口令", m_keystorePopupState.aliasPassword,
                         IM_ARRAYSIZE(m_keystorePopupState.aliasPassword), ImGuiInputTextFlags_Password);
        ImGui::InputText("确认别名口令", m_keystorePopupState.aliasPasswordConfirm,
                         IM_ARRAYSIZE(m_keystorePopupState.aliasPasswordConfirm), ImGuiInputTextFlags_Password);
        if (!m_keystorePopupState.errorMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_keystorePopupState.errorMessage.c_str());
        }
        if (ImGui::Button("创建"))
        {
            m_keystorePopupState.errorMessage.clear();
            std::filesystem::path selectedPath(m_keystorePopupState.path);
            if (selectedPath.empty())
            {
                m_keystorePopupState.errorMessage = "请先选择 Keystore 保存位置。";
            }
            else
            {
                if (selectedPath.extension().empty())
                {
                    selectedPath += ".keystore";
                }
                std::error_code ec;
                if (!selectedPath.parent_path().empty())
                {
                    std::filesystem::create_directories(selectedPath.parent_path(), ec);
                    if (ec)
                    {
                        m_keystorePopupState.errorMessage = "无法创建保存目录: " + ec.message();
                    }
                }
                std::string storePassword = m_keystorePopupState.storePassword;
                std::string storePasswordConfirm = m_keystorePopupState.storePasswordConfirm;
                std::string alias = m_keystorePopupState.alias;
                std::string aliasPassword = m_keystorePopupState.aliasPassword;
                std::string aliasPasswordConfirm = m_keystorePopupState.aliasPasswordConfirm;
                if (!m_keystorePopupState.errorMessage.empty())
                {
                }
                else if (storePassword.empty())
                {
                    m_keystorePopupState.errorMessage = "Keystore 口令不能为空。";
                }
                else if (storePassword != storePasswordConfirm)
                {
                    m_keystorePopupState.errorMessage = "Keystore 口令不匹配。";
                }
                else if (alias.empty())
                {
                    m_keystorePopupState.errorMessage = "别名不能为空。";
                }
                else if (aliasPassword.empty())
                {
                    m_keystorePopupState.errorMessage = "别名口令不能为空。";
                }
                else if (aliasPassword != aliasPasswordConfirm)
                {
                    m_keystorePopupState.errorMessage = "别名口令不匹配。";
                }
                else
                {
                    const std::string keytool = ResolveKeytoolExecutable();
                    std::string command = keytool + " ";
                    command += " -genkeypair -v -storetype pkcs12";
                    command += " -keystore " + QuoteCommandArg(selectedPath.make_preferred().string());
                    command += " -storepass " + QuoteCommandArg(storePassword);
                    command += " -alias " + QuoteCommandArg(alias);
                    command += " -keypass " + QuoteCommandArg(aliasPassword);
                    command += " -keyalg RSA -keysize 2048 -validity 10000";
                    command += " -dname \"CN=LumaGame, OU=LumaEngine, O=LumaEngine, L=City, ST=State, C=CN\"";
                    if (ExecuteCommand(command, "Keytool"))
                    {
                        settings.SetAndroidKeystorePath(selectedPath);
                        settings.SetAndroidKeystorePassword(storePassword);
                        settings.AddAndroidAliasEntry(alias, aliasPassword);
                        CopyStringToFixedBuffer(m_keystorePopupState.path, IM_ARRAYSIZE(m_keystorePopupState.path),
                                                selectedPath.make_preferred().string());
                        std::fill(std::begin(m_keystorePopupState.storePassword),
                                  std::end(m_keystorePopupState.storePassword), 0);
                        std::fill(std::begin(m_keystorePopupState.storePasswordConfirm),
                                  std::end(m_keystorePopupState.storePasswordConfirm), 0);
                        std::fill(std::begin(m_keystorePopupState.aliasPassword),
                                  std::end(m_keystorePopupState.aliasPassword), 0);
                        std::fill(std::begin(m_keystorePopupState.aliasPasswordConfirm),
                                  std::end(m_keystorePopupState.aliasPasswordConfirm), 0);
                        LogInfo("Keystore 已创建: {}", selectedPath.string());
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        m_keystorePopupState.errorMessage = "keytool 执行失败，请检查命令行输出。";
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
void ToolbarPanel::drawCreateAliasPopup()
{
    constexpr const char* kPopupTitle = "创建签名别名";
    auto& settings = ProjectSettings::GetInstance();
    if (m_aliasPopupState.openRequested)
    {
        ImGui::OpenPopup(kPopupTitle);
        m_aliasPopupState.openRequested = false;
    }
    if (ImGui::BeginPopupModal(kPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const auto keystorePath = settings.GetAndroidKeystorePath();
        std::string keystoreDisplay = "未配置";
        if (!keystorePath.empty())
        {
            std::filesystem::path temp = keystorePath;
            temp.make_preferred();
            keystoreDisplay = temp.string();
        }
        ImGui::TextWrapped("Keystore: %s", keystoreDisplay.c_str());
        ImGui::InputText("别名", m_aliasPopupState.alias, IM_ARRAYSIZE(m_aliasPopupState.alias));
        ImGui::InputText("别名口令", m_aliasPopupState.password, IM_ARRAYSIZE(m_aliasPopupState.password),
                         ImGuiInputTextFlags_Password);
        ImGui::InputText("确认别名口令", m_aliasPopupState.passwordConfirm,
                         IM_ARRAYSIZE(m_aliasPopupState.passwordConfirm), ImGuiInputTextFlags_Password);
        if (!m_aliasPopupState.errorMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_aliasPopupState.errorMessage.c_str());
        }
        if (ImGui::Button("创建"))
        {
            m_aliasPopupState.errorMessage.clear();
            if (keystorePath.empty())
            {
                m_aliasPopupState.errorMessage = "请先配置 Keystore 路径。";
            }
            else if (settings.GetAndroidKeystorePassword().empty())
            {
                m_aliasPopupState.errorMessage = "请先配置 Keystore 口令。";
            }
            else
            {
                std::string alias = m_aliasPopupState.alias;
                std::string aliasPassword = m_aliasPopupState.password;
                std::string aliasPasswordConfirm = m_aliasPopupState.passwordConfirm;
                auto trim = [](std::string& text)
                {
                    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
                    text.erase(text.begin(),
                               std::find_if(text.begin(), text.end(), [&](unsigned char ch) { return !isSpace(ch); }));
                    text.erase(std::find_if(text.rbegin(), text.rend(),
                                            [&](unsigned char ch) { return !isSpace(ch); }).base(), text.end());
                };
                trim(alias);
                if (alias.empty())
                {
                    m_aliasPopupState.errorMessage = "别名不能为空。";
                }
                else if (aliasPassword.empty())
                {
                    m_aliasPopupState.errorMessage = "别名口令不能为空。";
                }
                else if (aliasPassword != aliasPasswordConfirm)
                {
                    m_aliasPopupState.errorMessage = "别名口令不匹配。";
                }
                else
                {
                    const std::string keytool = ResolveKeytoolExecutable();
                    std::filesystem::path normalizedKeystore = keystorePath;
                    normalizedKeystore.make_preferred();
                    const std::string keystorePathStr = normalizedKeystore.string();
                    std::string command = QuoteCommandArg(keytool);
                    command += " -genkeypair -v";
                    command += " -keystore " + QuoteCommandArg(keystorePathStr);
                    command += " -storepass " + QuoteCommandArg(settings.GetAndroidKeystorePassword());
                    command += " -alias " + QuoteCommandArg(alias);
                    command += " -keypass " + QuoteCommandArg(aliasPassword);
                    command += " -keyalg RSA -keysize 2048 -validity 10000";
                    command += " -dname \"CN=LumaGame, OU=LumaEngine, O=LumaEngine, L=City, ST=State, C=CN\"";
                    if (ExecuteCommand(command, "Keytool"))
                    {
                        settings.AddAndroidAliasEntry(alias, aliasPassword);
                        std::fill(std::begin(m_aliasPopupState.alias), std::end(m_aliasPopupState.alias), 0);
                        std::fill(std::begin(m_aliasPopupState.password),
                                  std::end(m_aliasPopupState.password), 0);
                        std::fill(std::begin(m_aliasPopupState.passwordConfirm),
                                  std::end(m_aliasPopupState.passwordConfirm), 0);
                        LogInfo("成功在 Keystore 中创建别名: {}", alias);
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        m_aliasPopupState.errorMessage = "keytool 执行失败，请检查命令行输出。";
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
SDL_Window* ToolbarPanel::getSDLWindow() const
{
    if (!m_context || !m_context->editor)
    {
        return nullptr;
    }
    auto* window = m_context->editor->GetPlatWindow();
    return window->GetSdlWindow();
}
void ToolbarPanel::OnKeystoreSavePathChosen(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return;
    }
    std::filesystem::path normalized = path;
    if (normalized.extension().empty())
    {
        normalized += ".keystore";
    }
    normalized = normalized.lexically_normal();
    CopyStringToFixedBuffer(m_keystorePopupState.path, IM_ARRAYSIZE(m_keystorePopupState.path),
                            normalized.make_preferred().string());
}
namespace
{
    constexpr const char* kGradleConstantBlock = R"(# --- NDK/CMake ---
ndkVersion=27.0.12077973
cmakeVersion=3.22.1
abiFilters=arm64-v8a
# 逗号分隔 CMake 参数（可覆盖默认）
cmakeArgs=-DANDROID_ABI=arm64-v8a,-DANDROID_PLATFORM=android-28,-DUSE_PREBUILT_ENGINE=ON,-DENABLE_LIGHTWEIGHT_BUILD=OFF
# 编译标志（逗号分隔）
cFlags=-v
cppFlags=-v,-std=c++20
# jni .so 打包方式（true/false）
useLegacyJniPacking=true
)";
}
void ToolbarPanel::updateAndroidGradleProperties(const std::filesystem::path& platformOutputDir,
                                                 const ProjectSettings& settings)
{
    const auto gradlePropsPath = platformOutputDir / "gradle.properties";
    if (settings.IsCustomGradlePropertiesEnabled())
    {
        const auto customPath = settings.GetCustomGradlePropertiesPath();
        if (!customPath.empty() && std::filesystem::exists(customPath))
        {
            std::filesystem::create_directories(gradlePropsPath.parent_path());
            std::filesystem::copy(customPath, gradlePropsPath, std::filesystem::copy_options::overwrite_existing);
            LogInfo("已复制自定义 gradle.properties: {}", customPath.string());
            return;
        }
        LogWarn("自定义 gradle.properties 未找到，使用自动生成。");
    }
    std::filesystem::create_directories(gradlePropsPath.parent_path());
    std::ofstream outFile(gradlePropsPath, std::ios::binary | std::ios::trunc);
    if (!outFile)
    {
        LogWarn("无法写入 gradle.properties: {}", gradlePropsPath.string());
        return;
    }
    outFile << R"(# Project-wide Gradle settings.
# IDE (e.g. Android Studio) users:
# Gradle settings configured through the IDE *will override*
# any settings specified in this file.
# For more details on how to configure your build environment visit
# http://www.gradle.org/docs/current/userguide/build_environment.html
# Specifies the JVM arguments used for the daemon process.
# The setting is particularly useful for tweaking memory settings.
org.gradle.jvmargs=-Xmx2048m -Dfile.encoding=UTF-8
# When configured, Gradle will run in incubating parallel mode.
# This option should only be used with decoupled projects. For more details, visit
# https://developer.android.com/r/tools/gradle-multi-project-decoupled-projects
# org.gradle.parallel=true
# AndroidX package structure to make it clearer which packages are bundled with the
# Android operating system, and which are packaged with your app's APK
# https://developer.android.com/topic/libraries/support-library/androidx-rn
android.useAndroidX=true
# Kotlin code style for this project: "official" or "obsolete":
kotlin.code.style=official
# Enables namespacing of each library's R class so that its R class includes only the
# resources declared in the library itself and none from the library's dependencies,
# thereby reducing the size of the R class for that library
android.nonTransitiveRClass=true
)";
    const std::string packageValue = settings.GetAndroidPackageName().empty()
                                         ? "com.lumaengine.game"
                                         : settings.GetAndroidPackageName();
    outFile << "\n# --- App 标识 ---\n";
    outFile << "appNamespace=" << packageValue << "\n";
    outFile << "applicationId=" << packageValue << "\n\n";
    outFile << "# --- SDK/版本 ---\n";
    outFile << "compileSdk=" << settings.GetAndroidCompileSdk() << "\n";
    outFile << "targetSdk=" << settings.GetAndroidTargetSdk() << "\n";
    outFile << "minSdk=" << settings.GetAndroidMinSdk() << "\n";
    outFile << "maxVersion=" << settings.GetAndroidMaxVersion() << "\n";
    outFile << "minVersion=" << settings.GetAndroidMinVersion() << "\n";
    outFile << "versionCode=" << settings.GetAndroidVersionCode() << "\n";
    outFile << "versionName=" << settings.GetAndroidVersionName() << "\n\n";
    std::filesystem::path keystorePath = settings.GetAndroidKeystorePath();
    std::string keystorePathStr;
    if (!keystorePath.empty())
    {
        std::error_code ec;
        keystorePath = std::filesystem::absolute(keystorePath, ec);
        keystorePathStr = keystorePath.generic_string();
    }
    outFile << "# --- Signing ---\n";
    outFile << "signingStoreFile=" << keystorePathStr << "\n";
    outFile << "signingStorePassword=" << settings.GetAndroidKeystorePassword() << "\n";
    outFile << "signingKeyAlias=" << settings.GetAndroidKeyAlias() << "\n";
    outFile << "signingKeyPassword=" << settings.GetAndroidKeyPassword() << "\n\n";
    outFile << kGradleConstantBlock;
    outFile.close();
    LogInfo("已生成 gradle.properties: {}", gradlePropsPath.string());
}
std::filesystem::path ToolbarPanel::signAndroidApk(const std::filesystem::path& unsignedApk,
                                                   const ProjectSettings& settings)
{
    if (unsignedApk.empty())
    {
        return {};
    }
    if (settings.GetAndroidKeystorePath().empty() ||
        settings.GetAndroidKeystorePassword().empty() ||
        settings.GetAndroidKeyAlias().empty() ||
        settings.GetAndroidKeyPassword().empty())
    {
        LogWarn("未配置完整的 Keystore 信息，无法使用 apksigner 对 APK 进行签名。");
        return {};
    }
    auto& prefs = PreferenceSettings::GetInstance();
    const auto sdkPath = prefs.GetAndroidSdkPath();
    if (sdkPath.empty() || !std::filesystem::exists(sdkPath))
    {
        LogWarn("Android SDK 路径未配置或不存在，无法调用 apksigner。");
        return {};
    }
    const auto buildToolsDir = sdkPath / "build-tools";
    if (!std::filesystem::exists(buildToolsDir))
    {
        LogWarn("在 SDK 中未找到 build-tools 目录，无法调用 apksigner。");
        return {};
    }
#ifdef _WIN32
    constexpr const char* signerExecutableName = "apksigner.bat";
#else
    constexpr const char* signerExecutableName = "apksigner";
#endif
    std::vector<std::filesystem::path> toolchainDirs;
    for (const auto& entry : std::filesystem::directory_iterator(buildToolsDir))
    {
        if (entry.is_directory())
        {
            toolchainDirs.push_back(entry.path());
        }
    }
    std::sort(toolchainDirs.begin(), toolchainDirs.end(), std::greater<>());
    std::filesystem::path apksignerPath;
    for (const auto& dir : toolchainDirs)
    {
        const auto candidate = dir / signerExecutableName;
        if (std::filesystem::exists(candidate))
        {
            apksignerPath = candidate;
            break;
        }
    }
    if (apksignerPath.empty())
    {
        LogWarn("未在 Android SDK 中找到 apksigner，可执行文件可能缺失。");
        return {};
    }
    std::error_code ec;
    auto unsignedAbs = std::filesystem::absolute(unsignedApk, ec);
    if (ec)
    {
        unsignedAbs = unsignedApk;
    }
    unsignedAbs.make_preferred();
    auto signedApk = unsignedAbs;
    const std::string filenameLower = signedApk.filename().string();
    if (filenameLower.find("unsigned") != std::string::npos)
    {
        signedApk = signedApk.parent_path() / "app-release.apk";
    }
    std::filesystem::path keystorePath = settings.GetAndroidKeystorePath();
    keystorePath = std::filesystem::absolute(keystorePath, ec);
    keystorePath.make_preferred();
    std::string command = apksignerPath.make_preferred().string();
    command += " sign";
    command += " --ks " + QuoteCommandArg(keystorePath.string());
    command += " --ks-pass pass:" + settings.GetAndroidKeystorePassword();
    command += " --ks-key-alias " + QuoteCommandArg(settings.GetAndroidKeyAlias());
    command += " --key-pass pass:" + settings.GetAndroidKeyPassword();
    command += " --out " + QuoteCommandArg(signedApk.string());
    command += " " + QuoteCommandArg(unsignedAbs.string());
    if (!ExecuteCommand(command, "ApkSigner"))
    {
        LogWarn("apksigner 签名 APK 失败，命令: {}", command);
        return {};
    }
    LogInfo("已使用 apksigner 对 APK 进行签名: {}", signedApk.string());
    return signedApk;
}
bool ToolbarPanel::runScriptCompilationLogicForPackaging(std::string& statusMessage, TargetPlatform targetPlatform)
{
    const std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
    const std::filesystem::path editorRoot = ".";
    const std::filesystem::path libraryDir = projectRoot / "Library/Temp";
    const std::filesystem::path metadataYaml = libraryDir / "ScriptMetadata.yaml";
    if (std::filesystem::exists(libraryDir))
    {
        std::error_code removeEc;
        std::filesystem::remove_all(libraryDir, removeEc);
        if (removeEc)
        {
            LogWarn("无法清理 Library 目录: {}", removeEc.message());
        }
    }
    statusMessage = "正在构建宿主平台脚本 (用于生成元数据)...";
    std::string hostStatus;
    if (!runScriptCompilationLogic(hostStatus,libraryDir))
    {
        statusMessage = "宿主平台脚本编译失败: " + hostStatus;
        return false;
    }
    std::string metadataSnapshot;
    bool metadataAvailable = false;
    if (std::filesystem::exists(metadataYaml))
    {
        metadataAvailable = true;
        std::ifstream metadataFile(metadataYaml, std::ios::binary);
        metadataSnapshot.assign(std::istreambuf_iterator<char>(metadataFile), std::istreambuf_iterator<char>());
    }
    else
    {
        statusMessage = "ScriptMetadata.yaml 未生成，无法继续打包。";
        return false;
    }
    statusMessage = "宿主脚本已就绪，开始为目标平台生成: " + ProjectSettings::PlatformToString(targetPlatform);
    std::string platformSubDir = ProjectSettings::PlatformToString(targetPlatform);
    std::string dotnetRid = "win-x64";
    switch (targetPlatform)
    {
    case TargetPlatform::Linux:
        dotnetRid = "linux-x64";
        break;
    case TargetPlatform::Android:
        dotnetRid = "android-arm64";
        break;
    case TargetPlatform::Windows:
    default:
        dotnetRid = "win-x64";
        break;
    }
    const std::filesystem::path toolsDir = editorRoot / "Tools" / platformSubDir;
    try
    {
        statusMessage = "检查并准备 C# 依赖项 (目标: " + platformSubDir + ")...";
        std::filesystem::create_directories(libraryDir);
        const std::vector<std::string> requiredFiles = {
            "Luma.SDK.dll", "Luma.SDK.deps.json", "Luma.SDK.runtimeconfig.json", "YamlDotNet.dll"
        };
        for (const auto& filename : requiredFiles)
        {
            const std::filesystem::path destFile = libraryDir / filename;
            const std::filesystem::path srcFile = toolsDir / filename;
            if (!std::filesystem::exists(srcFile))
            {
                throw std::runtime_error("关键依赖文件在 Tools/" + platformSubDir + " 目录中未找到: " + srcFile.string());
            }
            statusMessage = "正在拷贝依赖: " + filename + " (目标: " + platformSubDir + ")";
            std::filesystem::copy(srcFile, destFile, std::filesystem::copy_options::overwrite_existing);
        }
        statusMessage = "正在发布 C# 项目 (目标: " + platformSubDir + ")...";
        std::string projectRootStr = projectRoot.string();
        std::string libraryDirStr = libraryDir.string();
#ifdef _WIN32
        char shortProjectPath[MAX_PATH];
        char shortLibraryPath[MAX_PATH];
        if (GetShortPathNameA(projectRootStr.c_str(), shortProjectPath, MAX_PATH) > 0)
        {
            projectRootStr = shortProjectPath;
        }
        if (GetShortPathNameA(libraryDirStr.c_str(), shortLibraryPath, MAX_PATH) > 0)
        {
            libraryDirStr = shortLibraryPath;
        }
#endif
        std::string slnPathStr = (projectRoot / "LumaScripting.sln").string();
        const std::string publishCmd = "dotnet publish -c Release -r " + dotnetRid + " \"" +
            slnPathStr + "\" -o \"" + libraryDirStr + "\"";
        if (!ExecuteCommand(publishCmd, "Publish"))
        {
            throw std::runtime_error("dotnet publish 命令执行失败。请检查控制台输出获取详细错误信息。");
        }
        if (metadataAvailable)
        {
            std::ofstream metadataOut(metadataYaml, std::ios::binary | std::ios::trunc);
            if (!metadataOut)
            {
                LogWarn("无法写回 ScriptMetadata.yaml，打包后的项目可能缺少该文件。");
            }
            else
            {
                metadataOut.write(metadataSnapshot.data(), static_cast<std::streamsize>(metadataSnapshot.size()));
            }
        }
        ScriptMetadataRegistry::GetInstance().Initialize(metadataYaml.string());
        statusMessage = "脚本编译成功！目标平台: " + platformSubDir;
        return true;
    }
    catch (const std::exception& e)
    {
        statusMessage = std::string("编译失败 (目标: " + platformSubDir + "): ") + e.what();
        return false;
    }
}

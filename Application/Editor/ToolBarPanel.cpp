#include "ToolbarPanel.h"
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
#include "../Utils/Logger.h"
#include "../Data/SceneData.h"
#include "../../Systems/HydrateResources.h"
#include "Input/Keyboards.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#define IM_PI 3.14159265358979323846f

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


static bool ExecuteCommand(const std::string& command, const std::string& logPrefix)
{
    LogInfo("[{}] 执行命令: {}", logPrefix, command);
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = POPEN(command.c_str(), "r");
    if (!pipe)
    {
        LogError("[{}] 无法执行命令。", logPrefix);
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }
    int exitCode = PCLOSE(pipe);
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

bool ToolbarPanel::runScriptCompilationLogic(std::string& statusMessage)
{
    const std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
    const std::filesystem::path editorRoot = ".";
    const std::filesystem::path libraryDir = projectRoot / "Library";


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
            throw std::runtime_error("编译产物 GameScripts.dll 未在 Library 目录中找到: " + gameScriptsDll.string());
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
    if (ImGui::Button("关闭", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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

void ToolbarPanel::drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        drawFileMenu();
        drawEditMenu();
        drawProjectMenu();
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


        if (!isProjectLoaded) ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}


void ToolbarPanel::drawSettingsWindow()
{
    if (!m_isSettingsWindowVisible) { return; }

    ImGui::Begin("打包设置 (Build Settings)", &m_isSettingsWindowVisible, ImGuiWindowFlags_AlwaysAutoResize);

    auto& settings = ProjectSettings::GetInstance();

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
        if (ImGui::MenuItem("新建项目...")) { m_context->editor->CreateNewProject(); }
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


    auto switchToPlayMode = [ctx = m_context]()
    {
        if (ctx->editorState == EditorState::Playing) return;

        ctx->editorState = EditorState::Playing;
        ctx->engineContext->appMode = ApplicationMode::PIE;
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

        SceneManager::GetInstance().SetCurrentScene(playScene);

        playScene->Activate(*ctx->engineContext);
        std::cout << "原始场景地址: " << ctx->editingScene.get() << std::endl;
        ctx->activeScene = playScene;


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

    auto stopCommand = [ctx = m_context]()
    {
        if (ctx->editorState == EditorState::Playing)
        {
            ctx->editorState = EditorState::Paused;
            LogInfo("执行已暂停。");
        }

        ctx->editorState = EditorState::Editing;
        ctx->engineContext->appMode = ApplicationMode::Editor;
        ctx->activeScene.reset();
        ctx->activeScene = ctx->editingScene;
        SceneManager::GetInstance().SetCurrentScene(ctx->activeScene);
        ctx->editingScene.reset();
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
        ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();

    ImGui::SameLine();

    if (ImGui::Button("取消", ImVec2(120, 0)))
    {
        startPackagingProcess();
        ImGui::CloseCurrentPopup();
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
            const std::filesystem::path outputDir = projectRoot / "build";


            TargetPlatform targetPlatform = settings.GetTargetPlatform();
            if (targetPlatform == TargetPlatform::Current)
            {
                targetPlatform = ProjectSettings::GetCurrentHostPlatform();
            }
            std::string targetPlatformStr = ProjectSettings::PlatformToString(targetPlatform);

            m_packagingStatus = "正在确定引擎模板包路径...";
            const std::filesystem::path editorRoot = ".";
            const std::filesystem::path templateDir = editorRoot / "Publish" / targetPlatformStr;
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


            m_packagingProgress = 0.1f;
            m_packagingStatus = "正在清理并复制引擎模板...";
            if (std::filesystem::exists(outputDir)) std::filesystem::remove_all(outputDir);

            std::filesystem::copy(templateDir, outputDir, std::filesystem::copy_options::recursive);


            const std::filesystem::path resourcesDir = outputDir / "Resources";
            const std::filesystem::path gameDataDir = outputDir / "GameData";
            const std::filesystem::path rawDestDir = outputDir / "Raw";

            std::filesystem::create_directories(resourcesDir);
            std::filesystem::create_directories(gameDataDir);


            m_packagingProgress = 0.25f;
            m_packagingStatus = "正在打包项目资源...";
            if (!AssetPacker::Pack(AssetManager::GetInstance().GetAssetDatabase(), resourcesDir))
                throw std::runtime_error("资源打包失败。");


            m_packagingProgress = 0.4f;
            m_packagingStatus = "正在复制 Raw 资产...";
            const auto rawSourceDir = AssetManager::GetInstance().GetAssetsRootPath() / "Raw";
            if (std::filesystem::exists(rawSourceDir))
                std::filesystem::copy(rawSourceDir, rawDestDir, std::filesystem::copy_options::recursive);


            m_packagingProgress = 0.6f;
            m_packagingStatus = "正在复制 C# 程序集...";
            const std::filesystem::path csharpSourceDir = projectRoot / "Library";
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


            m_packagingProgress = 0.8f;
            m_packagingStatus = "正在复制应用图标...";
            const std::filesystem::path iconSourceRelativePath = settings.GetAppIconPath();
            if (!iconSourceRelativePath.empty())
            {
                const std::filesystem::path iconSourceFullPath = settings.GetAssetsDirectory() / iconSourceRelativePath;
                if (std::filesystem::exists(iconSourceFullPath))
                {
                    const std::string destExtension = iconSourceFullPath.extension().string();
                    const std::filesystem::path iconDestPath = outputDir / ("icon" + destExtension);
                    std::filesystem::copy(iconSourceFullPath, iconDestPath,
                                          std::filesystem::copy_options::overwrite_existing);
                }
                else
                {
                    LogWarn("应用图标文件 '{}' 未找到，已跳过。", iconSourceFullPath.string());
                }
            }


            m_packagingProgress = 0.9f;
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

                const auto outputFilePath = outputDir / "ProjectSettings.lproj";
                std::ofstream outFile(outputFilePath, std::ios::binary);
                if (!outFile.is_open())
                    throw std::runtime_error("无法写入项目配置文件到输出目录: " + outputFilePath.string());
                outFile.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
            }
            else
                LogWarn("项目配置文件 (.lproj) 未找到，已跳过。");

            m_packagingProgress = 0.95f;
            m_packagingStatus = "游戏打包成功！目标平台: " + targetPlatformStr;
            m_packagingSuccess = true;
            m_lastBuildDirectory = outputDir;

            LogInfo("========================================");
            LogInfo("游戏打包成功！目标平台: {}", targetPlatformStr);
            LogInfo("输出目录: {}", outputDir.string());
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

bool ToolbarPanel::runScriptCompilationLogicForPackaging(std::string& statusMessage, TargetPlatform targetPlatform)
{
    const std::filesystem::path projectRoot = ProjectSettings::GetInstance().GetProjectRoot();
    const std::filesystem::path editorRoot = ".";
    const std::filesystem::path libraryDir = projectRoot / "Library";


    std::string platformSubDir = ProjectSettings::PlatformToString(targetPlatform);
    std::string dotnetRid = (targetPlatform == TargetPlatform::Windows) ? "win-x64" : "linux-x64";
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
        statusMessage = "正在提取脚本元数据 (目标: " + platformSubDir + ")...";


        TargetPlatform hostPlatform = ProjectSettings::GetCurrentHostPlatform();
        std::string hostPlatformSubDir = ProjectSettings::PlatformToString(hostPlatform);
        const std::filesystem::path hostToolsDir = editorRoot / "Tools" / hostPlatformSubDir;

        std::string toolExecutableName = (hostPlatform == TargetPlatform::Windows)
                                             ? "YamlExtractor.exe"
                                             : "YamlExtractor";
        const std::filesystem::path toolsExe = hostToolsDir / toolExecutableName;
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
            throw std::runtime_error("编译产物 GameScripts.dll 未在 Library 目录中找到: " + gameScriptsDll.string());
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

        statusMessage = "脚本编译成功！目标平台: " + platformSubDir;
        return true;
    }
    catch (const std::exception& e)
    {
        statusMessage = std::string("编译失败 (目标: " + platformSubDir + "): ") + e.what();
        return false;
    }
}

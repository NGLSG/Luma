#include "../Utils/PCH.h"
#include "Editor.h"


#include "Window.h"
#include "ProjectSettings.h"
#include "Renderer/GraphicsBackend.h"
#include "Renderer/RenderSystem.h"
#include "SceneRenderer.h"
#include "ImGuiRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL_dialog.h>


#include "Resources/AssetManager.h"
#include "SceneManager.h"
#include "Utils/PopupManager.h"


#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Resources/Managers/RuntimeMaterialManager.h"
#include "Resources/Managers/RuntimeTextureManager.h"
#include "Resources/Managers/RuntimePrefabManager.h"
#include "Resources/Managers/RuntimeSceneManager.h"
#include "Systems/HydrateResources.h"
#include "Systems/InteractionSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/ScriptingSystem.h"
#include "Systems/TransformSystem.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"
#include "Components/ComponentRegistry.h"
#include "Event/LumaEvent.h"


#include "Editor/ToolbarPanel.h"
#include "Editor/SceneViewPanel.h"
#include "Editor/GameViewPanel.h"
#include "Editor/HierarchyPanel.h"
#include "Editor/InspectorPanel.h"
#include "Editor/AssetBrowserPanel.h"
#include "Editor/ConsolePanel.h"
#include "Scripting/ScriptMetadataRegistry.h"
#include <cstdio>
#include <memory>
#include <array>

#include "PreferenceSettings.h"
#include "RenderableManager.h"
#include "Editor/AIPanel.h"
#include "Editor/AITool.h"
#include "Editor/AnimationControllerEditorPanel.h"
#include "Editor/AnimationEditorPanel.h"
#include "Editor/AssetInspectorPanel.h"
#include "Editor/BlueprintPanel.h"
#include "Editor/TilesetPanel.h"
#include "Managers/RuntimeAnimationClipManager.h"
#include "Managers/RuntimeFontManager.h"
#include "RuntimeAsset/RuntimeAnimationClip.h"

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

static std::string ExecuteAndCapture(const std::string& command)
{
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN((command + " 2>&1").c_str(), "r"), PCLOSE);
    if (!pipe)
    {
        return "Error: popen() failed!";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

static void RecordLastEditingProject(const std::filesystem::path& projectPath)
{
    std::fstream file("LastProject", std::ios::out);
    if (file.is_open())
    {
        file << projectPath.string();
        file.close();
    }
    else
    {
        LogWarn("无法记录最后编辑的项目路径。");
    }
}

static std::string GetLastEditingProject()
{
    std::ifstream file("LastProject");
    if (file.is_open())
    {
        std::string path;
        std::getline(file, path);
        file.close();
        return path;
    }
    return "";
}

static void SDLCALL OnProjectFileSelected(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0])
    {
        Editor* editor = static_cast<Editor*>(userdata);
        editor->LoadProject(std::filesystem::path(filelist[0]));
    }
}

static void SDLCALL OnNewProjectFolderSelected(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0])
    {
        Editor* editor = static_cast<Editor*>(userdata);
        editor->CreateNewProjectAtPath(std::filesystem::path(filelist[0]));
    }
}


Editor::Editor(ApplicationConfig config) : ApplicationBase(config)
{
    CURRENT_MODE = ApplicationMode::Editor;

    m_uiCallbacks = std::make_unique<UIDrawData>();
    m_uiCallbacks->onFocusInHierarchy.AddListener([this](const Guid& guid)
    {
        this->RequestFocusInHierarchy(guid);
    });
    m_uiCallbacks->onFocusInAssetBrowser.AddListener([this](const Guid& guid)
    {
        this->RequestFocusInBrowser(guid);
    });
    m_uiCallbacks->onValueChanged.AddListener([this]()
    {
        if (this && m_editorContext.activeScene)
            SceneManager::GetInstance().PushUndoState(m_editorContext.activeScene);
    });
}


bool Editor::checkDotNetEnvironment()
{
    LogInfo("正在检查 .NET 环境...");


    std::string versionResult = ExecuteAndCapture("dotnet --version");
    if (versionResult.find("command not found") != std::string::npos ||
        versionResult.find("不是内部或外部命令") != std::string::npos ||
        versionResult.find("错误：") != std::string::npos)
    {
        m_window->ShowMessageBox(PlatformWindow::NoticeLevel::Error, "环境错误",
                                 "在系统PATH中未找到.NET SDK。\n"
                                 "脚本功能和构建功能将不可用。\n\n"
                                 "请从microsoft.com/net下载安装.NET SDK。");
        LogError("在PATH中未找到.NET SDK。");
        return false;
    }


    std::string sdkListResult = ExecuteAndCapture("dotnet --list-sdks");

    if (sdkListResult.find("9.") == std::string::npos)
    {
        m_window->ShowMessageBox(PlatformWindow::NoticeLevel::Error, "环境错误",
                                 "未找到所需的.NET 9 SDK。\n"
                                 "脚本系统和资源打包功能需要.NET 9支持。\n\n"
                                 "请安装.NET 9 SDK以启用这些功能。");
        LogError("未找到所需的.NET 9 SDK。已安装的SDK版本：\n{}", sdkListResult);
        return false;
    }

    LogInfo("已检测到.NET 9 SDK。环境检查通过。");
    return true;
}

Editor::~Editor() = default;




void Editor::InitializeDerived()
{
    initializeEditorContext();


    m_imguiRenderer = std::make_unique<ImGuiRenderer>(
        m_window->GetSdlWindow(),
        m_graphicsBackend->GetDevice(),
        m_graphicsBackend->GetSurfaceFormat()
    );
    m_imguiRenderer->SetFont(m_imguiRenderer->LoadFonts("Fonts/SourceBlack-Medium.otf", 1.0f));
    m_sceneRenderer = std::make_unique<SceneRenderer>();


    m_editorContext.imguiRenderer = m_imguiRenderer.get();
    m_editorContext.sceneRenderer = m_sceneRenderer.get();
    m_editorContext.editor = this;
    m_editorContext.graphicsBackend = m_graphicsBackend.get();


    m_window->OnAnyEvent.AddListener([&](const SDL_Event& e)
    {
        ImGuiRenderer::ProcessEvent(e);
    });

    initializePanels();


    registerPopups();


    auto lastProjectPath = GetLastEditingProject();
    if (!lastProjectPath.empty())
    {
        if (std::filesystem::exists(lastProjectPath))
        {
            LoadProject(lastProjectPath);
        }
        else
        {
            LogWarn("上次编辑的项目路径不存在: {}", lastProjectPath);
        }
    }
    PreferenceSettings::GetInstance().Initialize("./LumaEditor.settings");

    
    m_editorContext.lastFpsUpdateTime = std::chrono::steady_clock::now();
    m_editorContext.lastUpsUpdateTime = std::chrono::steady_clock::now();
}

void Editor::initializeEditorContext()
{
    m_editorContext.engineContext = &m_context;
    m_editorContext.engineContext->graphicsBackend = m_graphicsBackend.get();
    m_editorContext.engineContext->renderSystem = m_renderSystem.get();
    m_editorContext.engineContext->appMode = ApplicationMode::Editor;
    m_editorContext.uiCallbacks = m_uiCallbacks.get();
    m_editorContext.editor = this;
}

void Editor::initializePanels()
{
    m_panels.push_back(std::make_unique<ToolbarPanel>());
    m_panels.push_back(std::make_unique<SceneViewPanel>());
    m_panels.push_back(std::make_unique<GameViewPanel>());
    m_panels.push_back(std::make_unique<HierarchyPanel>());
    m_panels.push_back(std::make_unique<InspectorPanel>());
    m_panels.push_back(std::make_unique<AssetBrowserPanel>());
    m_panels.push_back(std::make_unique<ConsolePanel>());
    m_panels.push_back(std::make_unique<AnimationEditorPanel>());
    m_panels.push_back(std::make_unique<AnimationControllerEditorPanel>());
    m_panels.push_back(std::make_unique<TilesetPanel>());
    m_panels.push_back(std::make_unique<AssetInspectorPanel>());
    m_panels.push_back(std::make_unique<AIPanel>());
    m_panels.push_back(std::make_unique<BlueprintPanel>());

    for (auto& panel : m_panels)
    {
        panel->Initialize(&m_editorContext);
    }
}

void Editor::registerPopups()
{
    auto& popupManager = PopupManager::GetInstance();


    popupManager.Register("AddComponentPopup", [this]()
    {
        this->drawAddComponentPopupContent();
    });


    popupManager.Register("File Exists", [this]()
    {
        this->drawFileConflictPopupContent();
    }, true, ImGuiWindowFlags_AlwaysAutoResize);
}

void Editor::loadStartupScene()
{
    auto& settings = ProjectSettings::GetInstance();
    if (!settings.IsProjectLoaded())
    {
        LogWarn("没有加载任何项目，无法加载启动场景。");
        m_editorContext.activeScene = sk_make_sp<RuntimeScene>();
        m_editorContext.activeScene->SetName("未加载项目");
        SceneManager::GetInstance().SetCurrentScene(m_editorContext.activeScene);
        return;
    }

    Guid startupSceneGuid = settings.GetStartScene();
    if (startupSceneGuid.Valid())
    {
        m_editorContext.activeScene = SceneManager::GetInstance().LoadScene(startupSceneGuid);
    }

    if (m_editorContext.activeScene)
    {
        LogInfo("成功加载场景，GUID: {}", startupSceneGuid.ToString());
        m_editorContext.activeScene->AddSystem<Systems::HydrateResources>();
        m_editorContext.activeScene->AddSystem<Systems::TransformSystem>();
        m_editorContext.activeScene->Activate(*m_editorContext.engineContext);
    }
    else
    {
        if (startupSceneGuid.Valid())
        {
            LogError("加载场景失败，GUID: {}", startupSceneGuid.ToString());
        }
        m_editorContext.activeScene = sk_make_sp<RuntimeScene>();
        m_editorContext.activeScene->SetName("NewScene");
        m_editorContext.activeScene->AddSystem<Systems::HydrateResources>();
        m_editorContext.activeScene->AddSystem<Systems::TransformSystem>();
        m_editorContext.activeScene->Activate(*m_editorContext.engineContext);
        SceneManager::GetInstance().SetCurrentScene(m_editorContext.activeScene);
        m_editorContext.selectionType = SelectionType::NA;
        m_editorContext.selectionList = std::vector<Guid>();
    }
}

void Editor::Update(float fixedDeltaTime)
{
    PROFILE_FUNCTION();
    updateUps();
    {
        PROFILE_SCOPE("SceneManager::Update");
        SceneManager::GetInstance().Update(*m_editorContext.engineContext);
    }

    if (m_editorContext.activeScene)
    {
        PROFILE_SCOPE("RuntimeScene::UpdateSystems");

        bool needsTitleUpdate = false;

        if (m_editorContext.activeScene->GetName() != m_editorContext.currentSceneName)
        {
            m_editorContext.currentSceneName = m_editorContext.activeScene->GetName();
            needsTitleUpdate = true;
        }

        bool isDirty = SceneManager::GetInstance().IsCurrentSceneDirty();
        if (m_editorContext.wasSceneDirty != isDirty)
        {
            m_editorContext.wasSceneDirty = isDirty;
            needsTitleUpdate = true;
        }

        if (needsTitleUpdate)
        {
            auto& settings = ProjectSettings::GetInstance();
            std::string title = settings.IsProjectLoaded() ? settings.GetAppName() : "Luma Engine";
            std::string newTitle = title + " - " + m_editorContext.currentSceneName;
            newTitle += isDirty ? " 未保存" : "";
            m_window->SetTitle(newTitle);
        }


        m_editorContext.activeScene->Update(fixedDeltaTime, *m_editorContext.engineContext,
                                            m_editorContext.editorState == EditorState::Paused);

        Camera::GetInstance().SetProperties(m_editorContext.activeScene->GetCameraProperties());


        SceneRenderer::ExtractToRenderableManager(m_editorContext.activeScene->GetRegistry());
    }
}

void Editor::Render()
{
    PROFILE_FUNCTION();

    if (!m_graphicsBackend || !m_imguiRenderer || !m_renderSystem)
    {
        LogError("Editor::Render: 核心组件未初始化。");
        return;
    }
    {
        PROFILE_SCOPE("UI::Update");

        AssetManager::GetInstance().Update(1.f / m_context.currentFps);
        for (auto& panel : m_panels)
        {
            if (panel->IsVisible())
            {
                panel->Update(1.f / m_context.currentFps);
            }
        }
    }


    m_editorContext.renderQueue = RenderableManager::GetInstance().GetInterpolationData();
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - m_editorContext.lastFrameTime).count();
    m_editorContext.lastFrameTime = currentTime;

    if (!m_graphicsBackend->BeginFrame()) return;

    {
        PROFILE_SCOPE("ImGui::NewFrame");
        m_imguiRenderer->NewFrame();
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    Profiler::GetInstance().DrawUI();
    {
        PROFILE_SCOPE("UI::DrawPanels");

        for (auto& panel : m_panels)
        {
            if (panel->IsVisible())
            {
                panel->Draw();
            }
        }
    }

    PopupManager::GetInstance().Render();

    m_imguiRenderer->EndFrame(*m_graphicsBackend);

    {
        PROFILE_SCOPE("GraphicsBackend::PresentFrame");
        m_graphicsBackend->PresentFrame();
    }

    updateFps();
}


void Editor::ShutdownDerived()
{
    for (auto& panel : m_panels)
    {
        panel->Shutdown();
    }
    m_panels.clear();


    SceneManager::GetInstance().Shutdown();
    RuntimeTextureManager::GetInstance().Shutdown();
    RuntimeMaterialManager::GetInstance().Shutdown();
    RuntimePrefabManager::GetInstance().Shutdown();
    RuntimeSceneManager::GetInstance().Shutdown();


    m_editorContext.activeScene.reset();
    m_editorContext.editingScene.reset();
    m_imguiRenderer.reset();
    m_sceneRenderer.reset();
    m_uiCallbacks.reset();
}

void Editor::CreateNewProject()
{
    SDL_ShowOpenFolderDialog(OnNewProjectFolderSelected, this, m_window->GetSdlWindow(), nullptr, false);
}

void Editor::OpenProject()
{
    const SDL_DialogFileFilter filters[] = {
        {"Luma Project", "lproj"}
    };
    SDL_ShowOpenFileDialog(OnProjectFileSelected, this, m_window->GetSdlWindow(), filters, 1, nullptr, false);
}

void Editor::LoadProject(const std::filesystem::path& projectPath)
{
    AssetManager::GetInstance().Shutdown();
    SceneManager::GetInstance().Shutdown();
    RuntimeTextureManager::GetInstance().Shutdown();
    RuntimeMaterialManager::GetInstance().Shutdown();
    RuntimePrefabManager::GetInstance().Shutdown();
    RuntimeSceneManager::GetInstance().Shutdown();
    RuntimeAnimationClipManager::GetInstance().Shutdown();
    RuntimeFontManager::GetInstance().Shutdown();
    m_editorContext.activeScene.reset();
    m_editorContext.editingScene.reset();
    if (!std::filesystem::exists(projectPath))
    {
        LogError("项目文件不存在: {}", projectPath.string());
        return;
    }


    auto& settings = ProjectSettings::GetInstance();
    settings.Load(projectPath);
    LogInfo("已加载项目: {}", settings.GetAppName());

    AssetManager::GetInstance().Initialize(ApplicationMode::Editor, settings.GetProjectRoot());
    ScriptMetadataRegistry::GetInstance().Initialize(
        settings.GetProjectRoot().string() + "/Library/ScriptMetadata.yaml");
    SceneManager::GetInstance().Shutdown();
    RecordLastEditingProject(projectPath);
    loadStartupScene();
}

void Editor::CreateNewProjectAtPath(const std::filesystem::path& projectPath)
{
    std::string projectName = projectPath.filename().string();
    std::filesystem::path projectFilePath = projectPath / (projectName + ".lproj");
    std::filesystem::path assetsPath = projectPath / "Assets";

    if (std::filesystem::exists(projectFilePath))
    {
        LogError("项目文件 '{}' 已存在。", projectFilePath.string());
        return;
    }

    const std::filesystem::path templatePath = "./template";
    if (!std::filesystem::exists(templatePath))
    {
        LogError("项目模板目录 './template' 未找到。请确保它与编辑器可执行文件位于同一目录。");
        return;
    }

    try
    {
        if (!std::filesystem::exists(projectPath))
        {
            std::filesystem::create_directory(projectPath);
        }

        LogInfo("正在从模板创建项目结构...");

        for (const auto& entry : std::filesystem::directory_iterator(templatePath))
        {
            if (entry.path().filename() != "GameScripts.csproj")
            {
                std::filesystem::copy(entry.path(), projectPath / entry.path().filename(),
                                      std::filesystem::copy_options::recursive);
            }
        }


        if (!std::filesystem::exists(assetsPath))
        {
            std::filesystem::create_directory(assetsPath);
        }


        std::filesystem::path csprojSource = templatePath / "GameScripts.csproj";
        if (std::filesystem::exists(csprojSource))
        {
            std::filesystem::copy(csprojSource, assetsPath / "GameScripts.csproj");
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        LogError("创建项目目录或复制模板失败: {}", e.what());
        return;
    }


    auto& settings = ProjectSettings::GetInstance();
    settings.SetAppName(projectName);
    settings.SetStartScene(Guid::Invalid());
    settings.SetFullscreen(false);
    settings.SetAppIconPath("");
    settings.Save(projectFilePath);

    LogInfo("成功创建新项目: {}", projectName);
    LoadProject(projectFilePath);
}

IEditorPanel* Editor::GetPanelByName(const std::string& name)
{
    for (auto& panel : m_panels)
    {
        if (panel->GetPanelName() == name)
        {
            return panel.get();
        }
    }
    return nullptr;
}

void Editor::drawAddComponentPopupContent()
{
    static char searchBuffer[256] = {0};
    ImGui::InputTextWithHint("##SearchComponents", "搜索组件", searchBuffer, sizeof(searchBuffer));
    ImGui::Separator();

    if (m_editorContext.selectionType != SelectionType::GameObject || m_editorContext.selectionList.empty())
    {
        ImGui::Text("请先选择至少一个游戏对象。");
        return;
    }

    std::vector<RuntimeGameObject> selectedObjects;
    for (const auto& guid : m_editorContext.selectionList)
    {
        RuntimeGameObject obj = m_editorContext.activeScene->FindGameObjectByGuid(guid);
        if (obj.IsValid())
        {
            selectedObjects.push_back(obj);
        }
    }

    if (selectedObjects.empty())
    {
        ImGui::Text("选中的对象无效。");
        return;
    }

    auto& registry = m_editorContext.activeScene->GetRegistry();
    const auto& componentRegistry = ComponentRegistry::GetInstance();

    if (selectedObjects.size() == 1)
    {
        ImGui::Text("为对象 '%s' 添加组件", selectedObjects[0].GetName().c_str());
    }
    else
    {
        ImGui::Text("为 %d 个对象批量添加组件", static_cast<int>(selectedObjects.size()));
    }
    ImGui::Separator();

    std::string filter = searchBuffer;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    for (const auto& componentName : componentRegistry.GetAllRegisteredNames())
    {
        const ComponentRegistration* compInfo = componentRegistry.Get(componentName);
        if (!compInfo || !compInfo->isExposedInEditor) continue;

        std::string lowerCaseName = componentName;
        std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);
        if (!filter.empty() && lowerCaseName.find(filter) == std::string::npos) continue;

        bool allHaveComponent = true;
        for (const auto& obj : selectedObjects)
        {
            if (!compInfo->has(registry, static_cast<entt::entity>(obj)))
            {
                allHaveComponent = false;
                break;
            }
        }

        if (allHaveComponent)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::MenuItem(componentName.c_str()))
        {
            m_editorContext.uiCallbacks->onValueChanged.Invoke();

            for (const auto& obj : selectedObjects)
            {
                if (!compInfo->has(registry, static_cast<entt::entity>(obj)))
                {
                    compInfo->add(registry, static_cast<entt::entity>(obj));
                }
            }
            ImGui::CloseCurrentPopup();
        }

        if (allHaveComponent)
        {
            ImGui::EndDisabled();
        }
    }
}

void Editor::drawFileConflictPopupContent()
{
    std::filesystem::path file(m_editorContext.conflictDestPath);
    ImGui::Text("文件 '%s' 在此目录中已存在。", file.filename().string().c_str());
    ImGui::Text("您想要覆盖它吗？");
    ImGui::Separator();

    if (ImGui::Button("覆盖", ImVec2(120, 0)))
    {
        std::filesystem::copy(m_editorContext.conflictSourcePath, m_editorContext.conflictDestPath,
                              std::filesystem::copy_options::overwrite_existing);
        LogInfo("资产已覆盖: {}", file.filename().string());
        ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();

    if (ImGui::Button("重命名", ImVec2(120, 0)))
    {
        std::filesystem::path destPath(m_editorContext.conflictDestPath);
        std::filesystem::path parentDir = destPath.parent_path();
        std::string stem = destPath.stem().string();
        std::string extension = destPath.extension().string();

        int counter = 1;
        std::filesystem::path newPath;
        do
        {
            std::string newFilename = stem + "_" + std::to_string(counter) + extension;
            newPath = parentDir / newFilename;
            counter++;
        }
        while (std::filesystem::exists(newPath));

        try
        {
            std::filesystem::copy(m_editorContext.conflictSourcePath, newPath);
            LogInfo("资产已重命名并复制: {}", newPath.filename().string());
        }
        catch (const std::exception& e)
        {
            LogError("重命名并复制资产失败: {}", e.what());
        }

        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    if (ImGui::Button("取消", ImVec2(120, 0)))
    {
        ImGui::CloseCurrentPopup();
    }
}




void Editor::updateUps()
{
    m_editorContext.updateCount++;
    auto currentTime = std::chrono::steady_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(currentTime - m_editorContext.lastUpsUpdateTime).count();

    
    if (elapsedSeconds >= 1.0)
    {
        const int updateCount = m_editorContext.updateCount;
        if (updateCount > 0)
        {
            m_editorContext.lastUps = static_cast<float>(updateCount / elapsedSeconds);
            m_editorContext.updateLatency = static_cast<float>((elapsedSeconds * 1000.0) / updateCount);
        }
        m_editorContext.updateCount = 0;
        m_editorContext.lastUpsUpdateTime = currentTime;
    }
}




void Editor::updateFps()
{
    m_editorContext.frameCount++;
    auto currentTime = std::chrono::steady_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(currentTime - m_editorContext.lastFpsUpdateTime).count();

    
    if (elapsedSeconds >= 1.0)
    {
        const int frameCount = m_editorContext.frameCount;
        if (frameCount > 0)
        {
            m_editorContext.lastFps = static_cast<float>(frameCount / elapsedSeconds);
            m_editorContext.renderLatency = static_cast<float>((elapsedSeconds * 1000.0) / frameCount);
        }
        m_editorContext.frameCount = 0;
        m_editorContext.lastFpsUpdateTime = currentTime;
    }
}

void Editor::RequestFocusInHierarchy(const Guid& guid)
{
    m_editorContext.objectToFocusInHierarchy = guid;
}

void Editor::RequestFocusInBrowser(const Guid& guid)
{
    m_editorContext.assetToFocusInBrowser = guid;
}

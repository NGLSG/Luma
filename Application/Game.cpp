#include "Game.h"

#include "Window.h"
#include "Renderer/Camera.h"
#include "Renderer/GraphicsBackend.h"
#include "Renderer/RenderSystem.h"
#include "Resources/AssetManager.h"
#include "SceneManager.h"
#include "Utils/Logger.h"


#include "Resources/RuntimeAsset/RuntimeScene.h"
#include "Managers/RuntimeMaterialManager.h"
#include "Managers/RuntimePrefabManager.h"
#include "Managers/RuntimeSceneManager.h"
#include "Managers/RuntimeTextureManager.h"

#include <Path.h>
#include "ProjectSettings.h"
#include "RenderableManager.h"

Game::Game(ApplicationConfig config) : ApplicationBase(config)
{
    CURRENT_MODE = ApplicationMode::Runtime;
}

void Game::InitializeDerived()
{
    ProjectSettings::GetInstance().LoadInRuntime();

    if (!ProjectSettings::GetInstance().GetAppIconPath().string().empty())
        m_window->SetIcon("icon" + Path::GetFileExtension(ProjectSettings::GetInstance().GetAppIconPath().string()));
#if !defined(__ANDROID__)
    m_window->FullScreen(ProjectSettings::GetInstance().IsFullscreen());
    m_window->BroaderLess(ProjectSettings::GetInstance().IsBorderless());
#endif
    AssetManager::GetInstance().Initialize(ApplicationMode::Runtime, "Resources/package.manifest");

    m_sceneRenderer = std::make_unique<SceneRenderer>();


    m_context.sceneRenderer = m_sceneRenderer.get();
    m_context.window = m_window.get();
    m_context.graphicsBackend = m_graphicsBackend.get();
    m_context.renderSystem = m_renderSystem.get();
    m_context.appMode = ApplicationMode::Runtime;
    SceneManager::GetInstance().Initialize(&m_context);


    Guid startupSceneGuid = ProjectSettings::GetInstance().GetStartScene();
    sk_sp<RuntimeScene> scene = SceneManager::GetInstance().LoadScene(startupSceneGuid);

    if (scene)
    {
        LogInfo("游戏模式：成功加载启动场景，GUID: {}", startupSceneGuid.ToString());
    }
    else
    {
        LogError("致命错误：无法加载启动场景，GUID: {}", startupSceneGuid.ToString());
    }
}

void Game::Update(float deltaTime)
{
    AssetManager::GetInstance().Update(deltaTime);


    SceneManager::GetInstance().Update(m_context);


    if (auto activeScene = SceneManager::GetInstance().GetCurrentScene())
    {
        activeScene->UpdateSimulation(deltaTime, m_context, false);


        auto& cameraProps = activeScene->GetCameraProperties();


        cameraProps.viewport = SkRect::MakeWH(m_window->GetWidth(), m_window->GetHeight());


        Camera::GetInstance().SetProperties(cameraProps);
        m_sceneRenderer->ExtractToRenderableManager(activeScene->GetRegistry());
    }
}


void Game::Render()
{
    if (!m_graphicsBackend || m_graphicsBackend->IsDeviceLost())
    {
        return;
    }
    if (auto activeScene = SceneManager::GetInstance().GetCurrentScene())
    {
        activeScene->UpdateMainThread(1.f / m_context.currentFps, m_context, false);
    }
    if (!m_graphicsBackend->BeginFrame())
    {
        return;
    }

    if (auto activeScene = SceneManager::GetInstance().GetCurrentScene())
    {
        RenderableManager::GetInstance().SetExternalAlpha(m_context.interpolationAlpha);
        std::vector<RenderPacket> renderQueue = RenderableManager::GetInstance().GetInterpolationData();

        for (const auto& packet : renderQueue)
        {
            m_renderSystem->Submit(packet);
        }
        m_renderSystem->Flush();
    }
    else
    {
        m_renderSystem->Clear(SkColor4f{0.0f, 0.0f, 0.0f, 1.0f});
        m_renderSystem->Flush();
    }

    m_graphicsBackend->PresentFrame();
}

void Game::ShutdownDerived()
{
    SceneManager::GetInstance().Shutdown();

    RuntimeTextureManager::GetInstance().Shutdown();
    RuntimeMaterialManager::GetInstance().Shutdown();
    RuntimePrefabManager::GetInstance().Shutdown();
    RuntimeSceneManager::GetInstance().Shutdown();

    m_sceneRenderer.reset();
}
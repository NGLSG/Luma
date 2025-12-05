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
#include "../Particles/ParticleRenderer.h"
#include "Renderer/Nut/ShaderStruct.h"
#include "Renderer/Nut/TextureA.h"
#include "Renderer/Nut/RenderPass.h"
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
    m_particleRenderer = std::make_unique<Particles::ParticleRenderer>();
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
        float windowWidth = m_window->GetWidth();
        float windowHeight = m_window->GetHeight();
        auto scaleMode = ProjectSettings::GetInstance().GetViewportScaleMode();
        float designWidth = static_cast<float>(ProjectSettings::GetInstance().GetDesignWidth());
        float designHeight = static_cast<float>(ProjectSettings::GetInstance().GetDesignHeight());
        cameraProps.zoomFactor = {1.0f, 1.0f};
        switch (scaleMode)
        {
        case ViewportScaleMode::None:
            cameraProps.viewport = SkRect::MakeWH(windowWidth, windowHeight);
            break;
        case ViewportScaleMode::FixedAspect:
            {
                float designAspect = designWidth / designHeight;
                float windowAspect = windowWidth / windowHeight;
                if (windowAspect > designAspect)
                {
                    float scale = windowHeight / designHeight;
                    float scaledWidth = designWidth * scale;
                    float offsetX = (windowWidth - scaledWidth) * 0.5f;
                    cameraProps.viewport = SkRect::MakeXYWH(offsetX, 0, scaledWidth, windowHeight);
                    cameraProps.zoomFactor = {scale, scale};
                }
                else
                {
                    float scale = windowWidth / designWidth;
                    float scaledHeight = designHeight * scale;
                    float offsetY = (windowHeight - scaledHeight) * 0.5f;
                    cameraProps.viewport = SkRect::MakeXYWH(0, offsetY, windowWidth, scaledHeight);
                    cameraProps.zoomFactor = {scale, scale};
                }
            }
            break;
        case ViewportScaleMode::FixedWidth:
            {
                float scale = windowWidth / designWidth;
                cameraProps.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                cameraProps.zoomFactor = {scale, scale};
            }
            break;
        case ViewportScaleMode::FixedHeight:
            {
                float scale = windowHeight / designHeight;
                cameraProps.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                cameraProps.zoomFactor = {scale, scale};
            }
            break;
        case ViewportScaleMode::Expand:
            {
                float scaleX = windowWidth / designWidth;
                float scaleY = windowHeight / designHeight;
                cameraProps.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                cameraProps.zoomFactor = {scaleX, scaleY};
            }
            break;
        }
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
        m_graphicsBackend->Submit();
        RenderParticles();
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
    if (m_particleRenderer)
    {
        m_particleRenderer->Shutdown();
        m_particleRenderer.reset();
    }
    m_sceneRenderer.reset();
}
void Game::RenderParticles()
{
    auto activeScene = SceneManager::GetInstance().GetCurrentScene();
    if (!activeScene)
        return;
    auto nutContext = m_graphicsBackend->GetNutContext();
    if (!nutContext)
        return;
    if (!m_particleRendererInitialized && m_particleRenderer)
    {
        m_particleRenderer->Initialize(nutContext);
        m_particleRendererInitialized = true;
    }
    if (!m_particleRendererInitialized)
        return;
    auto& registry = activeScene->GetRegistry();
    m_particleRenderer->PrepareRender(registry);
    if (m_particleRenderer->GetTotalParticleCount() == 0)
        return;
    EngineData engineData{};
    Camera::GetInstance().FillEngineData(engineData);
    engineData.CameraScaleY *= -1.0f;
    auto targetTexture = nutContext->AcquireSwapChainTexture();
    if (!targetTexture)
        return;
    auto attachmentBuilder = Nut::ColorAttachmentBuilder();
    attachmentBuilder.SetTexture(targetTexture)
                     .SetLoadOnOpen(Nut::LoadOnOpen::Load)
                     .SetStoreOnOpen(Nut::StoreOnOpen::Store);
    auto renderPass = nutContext->BeginRenderFrame()
                                .AddColorAttachment(attachmentBuilder.Build())
                                .Build();
    m_particleRenderer->Render(renderPass, engineData);
    nutContext->Submit({nutContext->EndRenderFrame(renderPass)});
}

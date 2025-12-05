#include "../Utils/PCH.h"
#include "GameViewPanel.h"
#include "ImGuiRenderer.h"
#include "RenderableManager.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/RenderSystem.h"
#include "Renderer/Camera.h"
#include "SceneRenderer.h"
#include "../Utils/Profiler.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "../ProjectSettings.h"
#include "../../Components/ParticleComponent.h"
#include "../Renderer/Nut/ShaderStruct.h"
#include "../Renderer/Nut/TextureA.h"
#include "../Renderer/Nut/RenderPass.h"
void GameViewPanel::Initialize(EditorContext* context)
{
    m_context = context;
    m_particleRenderer = std::make_unique<Particles::ParticleRenderer>();
}
void GameViewPanel::Update(float deltaTime)
{
}
void GameViewPanel::Draw()
{
    PROFILE_FUNCTION();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (m_context->editorState != EditorState::Editing)
    {
        m_context->engineContext->sceneViewRect = ECS::RectF(viewportScreenPos.x, viewportScreenPos.y, viewportSize.x,
                                                             viewportSize.y);
        m_context->engineContext->isSceneViewFocused =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
            ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    }
    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        m_gameViewTarget = m_context->graphicsBackend->CreateOrGetRenderTarget("GameView",
                                                                               (uint16_t)viewportSize.x,
                                                                               (uint16_t)viewportSize.y);
        if (m_gameViewTarget)
        {
            if (m_context->editorState != EditorState::Editing && m_context->activeScene)
            {
                auto cameraProperties = m_context->activeScene->GetCameraProperties();
                float windowWidth = viewportSize.x;
                float windowHeight = viewportSize.y;
                auto scaleMode = ProjectSettings::GetInstance().GetViewportScaleMode();
                float designWidth = static_cast<float>(ProjectSettings::GetInstance().GetDesignWidth());
                float designHeight = static_cast<float>(ProjectSettings::GetInstance().GetDesignHeight());
                cameraProperties.zoomFactor = {1.0f, 1.0f};
                switch (scaleMode)
                {
                case ViewportScaleMode::None:
                    cameraProperties.viewport = SkRect::MakeWH(windowWidth, windowHeight);
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
                            cameraProperties.viewport = SkRect::MakeXYWH(offsetX, 0, scaledWidth, windowHeight);
                            cameraProperties.zoomFactor = {scale, scale};
                        }
                        else
                        {
                            float scale = windowWidth / designWidth;
                            float scaledHeight = designHeight * scale;
                            float offsetY = (windowHeight - scaledHeight) * 0.5f;
                            cameraProperties.viewport = SkRect::MakeXYWH(0, offsetY, windowWidth, scaledHeight);
                            cameraProperties.zoomFactor = {scale, scale};
                        }
                    }
                    break;
                case ViewportScaleMode::FixedWidth:
                    {
                        float scale = windowWidth / designWidth;
                        cameraProperties.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                        cameraProperties.zoomFactor = {scale, scale};
                    }
                    break;
                case ViewportScaleMode::FixedHeight:
                    {
                        float scale = windowHeight / designHeight;
                        cameraProperties.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                        cameraProperties.zoomFactor = {scale, scale};
                    }
                    break;
                case ViewportScaleMode::Expand:
                    {
                        float scaleX = windowWidth / designWidth;
                        float scaleY = windowHeight / designHeight;
                        cameraProperties.viewport = SkRect::MakeWH(windowWidth, windowHeight);
                        cameraProperties.zoomFactor = {scaleX, scaleY};
                    }
                    break;
                }
                Camera::GetInstance().SetProperties(cameraProperties);
                m_context->graphicsBackend->SetActiveRenderTarget(m_gameViewTarget);
                for (const auto& packet : m_context->renderQueue)
                {
                    m_context->engineContext->renderSystem->Submit(packet);
                }
                m_context->engineContext->renderSystem->Flush();
                m_context->graphicsBackend->Submit();
                renderParticlesGPU();
            }
            else
            {
                m_context->graphicsBackend->SetActiveRenderTarget(m_gameViewTarget);
                m_context->engineContext->renderSystem->Clear({0.0f, 0.0f, 0.0f, 1.0f});
                m_context->graphicsBackend->Submit();
            }
            ImTextureID textureId = m_context->imguiRenderer->GetOrCreateTextureIdFor(m_gameViewTarget->GetTexture());
            ImGui::Image(textureId, viewportSize, ImVec2(0, 0), ImVec2(1, 1));
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
void GameViewPanel::Shutdown()
{
    m_gameViewTarget.reset();
    if (m_particleRenderer)
    {
        m_particleRenderer->Shutdown();
        m_particleRenderer.reset();
    }
}
void GameViewPanel::renderParticlesGPU()
{
    if (!m_context || !m_context->activeScene || !m_gameViewTarget)
        return;
    auto nutContext = m_context->graphicsBackend->GetNutContext();
    if (!nutContext)
        return;
    if (!m_particleRendererInitialized && m_particleRenderer)
    {
        m_particleRenderer->Initialize(nutContext);
        m_particleRendererInitialized = true;
    }
    if (!m_particleRendererInitialized)
        return;
    auto& registry = m_context->activeScene->GetRegistry();
    m_particleRenderer->PrepareRender(registry);
    if (m_particleRenderer->GetTotalParticleCount() == 0)
        return;
    EngineData engineData{};
    Camera::GetInstance().FillEngineData(engineData);
    engineData.CameraScaleY *= -1.0f;
    auto targetTexture = Nut::TextureA::CreateTextureA(
        m_gameViewTarget->GetTexture(), nutContext);
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

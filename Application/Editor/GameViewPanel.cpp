#include "../Utils/PCH.h"
#include "GameViewPanel.h"

#include "ImGuiRenderer.h"
#include "RenderableManager.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/RenderSystem.h"
#include "SceneRenderer.h"
#include "../Utils/Profiler.h"
#include "RuntimeAsset/RuntimeScene.h"

void GameViewPanel::Initialize(EditorContext* context)
{
    m_context = context;
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


                cameraProperties.viewport = SkRect::MakeWH(viewportSize.x, viewportSize.y);


                Camera::GetInstance().SetProperties(cameraProperties);


                m_context->graphicsBackend->SetActiveRenderTarget(m_gameViewTarget);

                for (const auto& packet : m_context->renderQueue)
                {
                    m_context->engineContext->renderSystem->Submit(packet);
                }


                m_context->engineContext->renderSystem->Flush();
                m_context->graphicsBackend->Submit();
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
}

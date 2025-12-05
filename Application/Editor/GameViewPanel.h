#ifndef GAMEVIEWPANEL_H
#define GAMEVIEWPANEL_H
#include "IEditorPanel.h"
#include "../Renderer/RenderTarget.h"
#include "../Particles/ParticleRenderer.h"
#include <memory>
class GameViewPanel : public IEditorPanel
{
public:
    GameViewPanel() = default;
    ~GameViewPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "游戏视图"; }
private:
    void renderParticlesGPU();
    std::shared_ptr<RenderTarget> m_gameViewTarget; 
    std::unique_ptr<Particles::ParticleRenderer> m_particleRenderer; 
    bool m_particleRendererInitialized = false; 
};
#endif

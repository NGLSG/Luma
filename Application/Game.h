#ifndef LUMAENGINEPROJECT_GAME_H
#define LUMAENGINEPROJECT_GAME_H
#include "ApplicationBase.h"
#include "SceneRenderer.h"
namespace Particles { class ParticleRenderer; }
class LUMA_API Game final : public ApplicationBase
{
public:
    Game(ApplicationConfig config);
    ~Game() override = default;
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
protected:
    void InitializeDerived() override;
    void ShutdownDerived() override;
    void Update(float deltaTime) override;
    void Render() override;
private:
    void RenderParticles();
    std::unique_ptr<SceneRenderer> m_sceneRenderer;
    std::unique_ptr<Particles::ParticleRenderer> m_particleRenderer;
    bool m_particleRendererInitialized = false;
};
#endif

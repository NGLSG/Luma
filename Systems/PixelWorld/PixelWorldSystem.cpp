#include "PixelWorldSystem.h"
#include "PixelWorld.h"
#include "../../Components/PixelWorldComponent.h"
#include "../../Renderer/GraphicsBackend.h"
#include "RuntimeAsset/RuntimeScene.h"
#include <entt/entt.hpp>

namespace Systems
{
    void PixelWorldSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        if (!engineCtx.graphicsBackend) return;
        m_nutContext = engineCtx.graphicsBackend->GetNutContext();
    }

    void PixelWorldSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!scene || !m_nutContext) return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::PixelWorldComponent>();

        for (auto entity : view)
        {
            auto& comp = view.get<ECS::PixelWorldComponent>(entity);
            if (!comp.Enable) continue;

            if (!comp.world)
            {
                comp.world = std::make_shared<PixelWorld>(comp.worldWidth, comp.worldHeight);
                comp.world->Initialize(m_nutContext);
            }

            if (!comp.paused)
            {
                comp.world->Step(deltaTime);
            }
        }
    }

    void PixelWorldSystem::OnDestroy(RuntimeScene* scene)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::PixelWorldComponent>();
        for (auto entity : view)
        {
            auto& comp = view.get<ECS::PixelWorldComponent>(entity);
            comp.world.reset();
        }
        m_nutContext.reset();
    }
}

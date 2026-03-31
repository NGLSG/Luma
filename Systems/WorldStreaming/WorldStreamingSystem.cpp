#include "WorldStreamingSystem.h"
#include "../../Components/ChunkWorldComponent.h"
#include "../../Components/Transform.h"
#include "../../Resources/RuntimeAsset/RuntimeScene.h"
#include "../../Renderer/Camera.h"

namespace Systems
{
    void WorldStreamingSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
    }

    void WorldStreamingSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::ChunkWorldComponent>();

        for (auto entity : view)
        {
            auto& world = view.get<ECS::ChunkWorldComponent>(entity);
            if (!world.Enable || !world.chunkManager)
                continue;

            world.chunkManager->SetLoadRadius(world.loadRadius);
            world.chunkManager->SetUnloadRadius(world.unloadRadius);
            world.chunkManager->SetTileSize(world.tileSize);

            auto& cam = CameraManager::GetInstance().GetActiveCamera();
            auto props = cam.GetProperties();
            world.chunkManager->SetViewCenter(props.position.x(), props.position.y());

            world.chunkManager->Update();
        }
    }

    void WorldStreamingSystem::OnDestroy(RuntimeScene* scene)
    {
    }
}

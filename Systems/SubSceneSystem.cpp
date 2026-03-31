#include "SubSceneSystem.h"
#include "../Application/SceneManager.h"
#include "../Components/Transform.h"
#include "../Renderer/Camera.h"
#include "../Utils/Logger.h"

namespace Systems
{
    void SubSceneSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
    }

    void SubSceneSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();

        auto view = registry.view<ECS::SubSceneComponent>();
        for (auto entity : view)
        {
            auto& sub = view.get<ECS::SubSceneComponent>(entity);
            if (!sub.Enable) continue;

            if (sub.autoLoad && !sub.isLoaded)
            {
                LoadSubScene(scene, entity, sub);
                continue;
            }

            if (sub.loadDistance > 0.0f)
            {
                auto* transform = registry.try_get<ECS::TransformComponent>(entity);
                if (!transform) continue;

                auto& cameraManager = CameraManager::GetInstance();
                auto* cam = cameraManager.GetCamera("main");
                if (!cam) continue;

                auto camPos = cam->GetProperties().position;
                float dx = transform->position.x - camPos.x();
                float dy = transform->position.y - camPos.y();
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist <= sub.loadDistance && !sub.isLoaded)
                {
                    LoadSubScene(scene, entity, sub);
                }
                else if (dist > sub.loadDistance * 1.2f && sub.isLoaded)
                {
                    UnloadSubScene(scene, entity, sub);
                }
            }
        }
    }

    void SubSceneSystem::LoadSubScene(RuntimeScene* scene, entt::entity entity, ECS::SubSceneComponent& sub)
    {
        if (sub.isLoaded || !sub.sceneGuid.IsValid()) return;

        auto loadedScene = SceneManager::GetInstance().LoadScene(sub.sceneGuid);
        if (!loadedScene)
        {
            LogWarn("SubSceneSystem: Failed to load sub-scene {}", sub.sceneGuid.ToString());
            return;
        }

        auto& srcRegistry = loadedScene->GetRegistry();
        auto& dstRegistry = scene->GetRegistry();
        std::vector<entt::entity> spawned;

        srcRegistry.view<ECS::TransformComponent>().each(
            [&](entt::entity srcEntity, const ECS::TransformComponent& srcTransform)
            {
                auto newEntity = dstRegistry.create();
                dstRegistry.emplace<ECS::TransformComponent>(newEntity, srcTransform);
                spawned.push_back(newEntity);
            }
        );

        m_loadedEntities[sub.sceneGuid] = std::move(spawned);
        sub.isLoaded = true;

        LogInfo("SubSceneSystem: Loaded sub-scene {}", sub.sceneGuid.ToString());
    }

    void SubSceneSystem::UnloadSubScene(RuntimeScene* scene, entt::entity entity, ECS::SubSceneComponent& sub)
    {
        if (!sub.isLoaded) return;

        auto it = m_loadedEntities.find(sub.sceneGuid);
        if (it != m_loadedEntities.end())
        {
            auto& registry = scene->GetRegistry();
            for (auto e : it->second)
            {
                if (registry.valid(e))
                    registry.destroy(e);
            }
            m_loadedEntities.erase(it);
        }

        sub.isLoaded = false;
        LogInfo("SubSceneSystem: Unloaded sub-scene {}", sub.sceneGuid.ToString());
    }

    void SubSceneSystem::OnDestroy(RuntimeScene* scene)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();

        auto view = registry.view<ECS::SubSceneComponent>();
        for (auto entity : view)
        {
            auto& sub = view.get<ECS::SubSceneComponent>(entity);
            if (sub.isLoaded)
                UnloadSubScene(scene, entity, sub);
        }

        m_loadedEntities.clear();
    }
}

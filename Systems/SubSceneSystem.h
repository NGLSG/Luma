#ifndef SUBSCENE_SYSTEM_H
#define SUBSCENE_SYSTEM_H

#include "ISystem.h"
#include "../Components/SubSceneComponent.h"
#include "../Utils/Guid.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>

namespace Systems
{
    class SubSceneSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
        void OnDestroy(RuntimeScene* scene) override;

        void LoadSubScene(RuntimeScene* scene, entt::entity entity, ECS::SubSceneComponent& sub);
        void UnloadSubScene(RuntimeScene* scene, entt::entity entity, ECS::SubSceneComponent& sub);

    private:
        std::unordered_map<Guid, std::vector<entt::entity>> m_loadedEntities;
    };
}

#endif

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include "ISystem.h"
#include "../Components/ParticleComponent.h"
#include "../Components/Transform.h"
#include <entt/entt.hpp>
namespace Systems
{
    class PhysicsSystem;
    
    class ParticleSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
        void OnDestroy(RuntimeScene* scene) override;
    private:
        void UpdateParticleSystem(ECS::ParticleSystemComponent& ps, 
                                  const ECS::TransformComponent* transform,
                                  float deltaTime,
                                  PhysicsSystem* physicsSystem,
                                  entt::registry* registry);
        glm::vec3 GetWorldPosition(const ECS::TransformComponent* transform);
        glm::vec2 GetWorldScale(const ECS::TransformComponent* transform);
    };
}
#endif 

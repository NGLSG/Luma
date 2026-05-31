#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include "ISystem.h"
#include "../Components/ParticleComponent.h"
#include "../Components/Transform.h"
#include "../Event/JobSystem.h"
#include <entt/entt.hpp>
namespace Systems
{
    class PhysicsSystem;

    struct ParticleUpdateJob : public IJob
    {
        ECS::ParticleSystemComponent* ps;
        const ECS::TransformComponent* transform;
        float deltaTime;
        glm::vec3 worldPos;
        glm::vec2 worldScale;
        glm::vec3 positionDelta;

        ParticleUpdateJob(ECS::ParticleSystemComponent* ps_, const ECS::TransformComponent* transform_,
                          float deltaTime_, glm::vec3 worldPos_, glm::vec2 worldScale_, glm::vec3 positionDelta_)
            : ps(ps_), transform(transform_), deltaTime(deltaTime_),
              worldPos(worldPos_), worldScale(worldScale_), positionDelta(positionDelta_) {}

        void Execute() override;
    };

    struct ParticleCollisionJob : public IJob
    {
        ECS::ParticleSystemComponent* ps;
        PhysicsSystem* physicsSystem;

        void Execute() override;
    };
    
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

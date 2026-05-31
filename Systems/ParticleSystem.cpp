#include "ParticleSystem.h"
#include "PhysicsSystem.h"
#include "RuntimeAsset/RuntimeScene.h"
#include <box2d/box2d.h>

namespace Systems
{
    // Box2D 单位转换常量
    static constexpr float PIXELS_PER_METER = 32.0f;
    static constexpr float METER_PER_PIXEL = 1.0f / PIXELS_PER_METER;
    void ParticleSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::ParticleSystemComponent>();
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            ps.Initialize();
            if (ps.playOnAwake && ps.Enable)
            {
                ps.Play();
            }
        }
    }
    void ParticleUpdateJob::Execute()
    {
        if (!ps || ps->playState != ECS::ParticlePlayState::Playing) return;

        float scaledDeltaTime = deltaTime * ps->simulationSpeed;
        ps->systemTime += scaledDeltaTime;

        bool shouldEmit = true;
        if (!ps->loop && ps->systemTime >= ps->duration)
        {
            shouldEmit = false;
            if (!ps->HasActiveParticles())
            {
                ps->Stop(false);
                return;
            }
        }
        else if (ps->loop && ps->systemTime >= ps->duration)
        {
            ps->systemTime = 0.0f;
        }

        ps->currentVelocity = positionDelta / std::max(scaledDeltaTime, 0.0001f);
        ps->lastPosition = worldPos;

        if (ps->emitter && ps->pool && shouldEmit)
        {
            ps->emitter->Update(*ps->pool, scaledDeltaTime, worldPos, ps->currentVelocity, worldScale);
        }

        if (ps->pool && !ps->pool->Empty())
        {
            ps->affectors.UpdateBatch(ps->pool->GetParticles(), scaledDeltaTime);

            if (ps->collisionEnabled)
            {
                for (auto& particle : ps->pool->GetParticles())
                {
                    float distance = glm::dot(particle.position - ps->collisionPlanePoint, ps->collisionPlaneNormal);
                    if (distance < 0.0f)
                    {
                        if (ps->collisionKillOnHit)
                        {
                            particle.age = particle.lifetime;
                        }
                        else
                        {
                            particle.position -= ps->collisionPlaneNormal * distance;
                            float normalVelocity = glm::dot(particle.velocity, ps->collisionPlaneNormal);
                            if (normalVelocity < 0.0f)
                            {
                                glm::vec3 normalComponent = ps->collisionPlaneNormal * normalVelocity;
                                glm::vec3 tangentComponent = particle.velocity - normalComponent;
                                particle.velocity = tangentComponent * (1.0f - ps->collisionFriction) -
                                                    normalComponent * ps->collisionBounciness;
                            }
                        }
                    }
                }
            }

            if (ps->simulationSpace == ECS::ParticleSimulationSpace::Local && transform)
            {
                for (auto& particle : ps->pool->GetParticles())
                {
                    particle.position += positionDelta;
                }
            }
        }
    }

    void ParticleCollisionJob::Execute()
    {
        if (!ps || !ps->physicsCollisionEnabled || !physicsSystem || !ps->pool || ps->pool->Empty())
            return;

        b2WorldId worldId = physicsSystem->GetWorld();
        if (!b2World_IsValid(worldId))
            return;

        float radiusMeters = ps->particleRadius * METER_PER_PIXEL;

        for (auto& particle : ps->pool->GetParticles())
        {
            if (particle.IsDead()) continue;

            b2Vec2 particlePos = {
                particle.position.x * METER_PER_PIXEL,
                -particle.position.y * METER_PER_PIXEL
            };

            b2AABB aabb;
            aabb.lowerBound = {particlePos.x - radiusMeters, particlePos.y - radiusMeters};
            aabb.upperBound = {particlePos.x + radiusMeters, particlePos.y + radiusMeters};

            struct ParticleCollisionContext
            {
                b2Vec2 particlePos;
                float radius;
                bool hasCollision;
                b2Vec2 collisionNormal;
                b2ShapeId hitShape;
            };

            ParticleCollisionContext ctx = {particlePos, radiusMeters, false, {0, -1}, b2_nullShapeId};

            auto overlapCallback = [](b2ShapeId shapeId, void* context) -> bool
            {
                auto* ctx = static_cast<ParticleCollisionContext*>(context);
                b2BodyId bodyId = b2Shape_GetBody(shapeId);
                b2BodyType bodyType = b2Body_GetType(bodyId);

                if (b2Shape_IsSensor(shapeId))
                    return true;

                if (bodyType == b2_staticBody || bodyType == b2_dynamicBody || bodyType == b2_kinematicBody)
                {
                    b2AABB shapeAABB = b2Shape_GetAABB(shapeId);

                    if (ctx->particlePos.x + ctx->radius > shapeAABB.lowerBound.x &&
                        ctx->particlePos.x - ctx->radius < shapeAABB.upperBound.x &&
                        ctx->particlePos.y + ctx->radius > shapeAABB.lowerBound.y &&
                        ctx->particlePos.y - ctx->radius < shapeAABB.upperBound.y)
                    {
                        ctx->hasCollision = true;
                        ctx->hitShape = shapeId;
                        b2Vec2 shapeCenter = {
                            (shapeAABB.lowerBound.x + shapeAABB.upperBound.x) * 0.5f,
                            (shapeAABB.lowerBound.y + shapeAABB.upperBound.y) * 0.5f
                        };
                        b2Vec2 diff = {ctx->particlePos.x - shapeCenter.x, ctx->particlePos.y - shapeCenter.y};
                        float len = sqrtf(diff.x * diff.x + diff.y * diff.y);
                        if (len > 0.0001f)
                        {
                            ctx->collisionNormal = {diff.x / len, diff.y / len};
                        }
                        return false;
                    }
                }
                return true;
            };

            b2World_OverlapAABB(worldId, aabb, b2DefaultQueryFilter(), overlapCallback, &ctx);

            if (ctx.hasCollision)
            {
                if (ps->physicsCollisionKillOnHit)
                {
                    particle.age = particle.lifetime;
                }
                else
                {
                    float pushDistance = ps->particleRadius * 0.5f;
                    particle.position.x += ctx.collisionNormal.x * pushDistance;
                    particle.position.y -= ctx.collisionNormal.y * pushDistance;

                    glm::vec3 normal(ctx.collisionNormal.x, -ctx.collisionNormal.y, 0.0f);
                    float normalVelocity = glm::dot(particle.velocity, normal);

                    if (normalVelocity < 0.0f)
                    {
                        glm::vec3 normalComponent = normal * normalVelocity;
                        glm::vec3 tangentComponent = particle.velocity - normalComponent;
                        particle.velocity = tangentComponent * (1.0f - ps->physicsCollisionFriction) -
                                            normalComponent * ps->physicsCollisionBounciness;
                    }
                }
            }
        }
    }

    void ParticleSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();
        PhysicsSystem* physicsSystem = scene->GetSystem<PhysicsSystem>();
        auto view = registry.view<ECS::ParticleSystemComponent>();

        // Phase 1: Initialize dirty components (must be sequential - registry access)
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable) continue;

            bool needsInit = ps.configDirty || !ps.pool || !ps.emitter;
            if (needsInit)
            {
                ps.Initialize();
                if (ps.emitter) ps.emitter->SetConfig(ps.emitterConfig);
                if (ps.playOnAwake && ps.playState == ECS::ParticlePlayState::Stopped)
                    ps.Play();
            }
        }

        // Phase 2: Parallel particle simulation (emission + affectors + plane collision)
        std::vector<ParticleUpdateJob> updateJobs;
        std::vector<JobHandle> jobHandles;
        auto& jobSystem = JobSystem::GetInstance();

        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable || ps.playState != ECS::ParticlePlayState::Playing) continue;

            const ECS::TransformComponent* transform = nullptr;
            if (registry.all_of<ECS::TransformComponent>(entity))
                transform = &registry.get<ECS::TransformComponent>(entity);

            glm::vec3 worldPos = GetWorldPosition(transform);
            glm::vec2 worldScale = GetWorldScale(transform);
            glm::vec3 positionDelta = worldPos - ps.lastPosition;

            updateJobs.push_back(ParticleUpdateJob{&ps, transform, deltaTime, worldPos, worldScale, positionDelta});
        }

        for (auto& job : updateJobs)
            jobHandles.push_back(jobSystem.Schedule(&job));
        JobSystem::CompleteAll(jobHandles);

        // Phase 3: Box2D physics collision (sequential - Box2D is not thread-safe)
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable || !ps.physicsCollisionEnabled || !ps.pool || ps.pool->Empty())
                continue;

            ParticleCollisionJob collisionJob;
            collisionJob.ps = &ps;
            collisionJob.physicsSystem = physicsSystem;
            collisionJob.Execute();
        }

        // Phase 4: Cleanup and GPU sync (parallel)
        std::vector<ParticleUpdateJob*> syncList;
        jobHandles.clear();

        struct SyncJob : public IJob
        {
            ECS::ParticleSystemComponent* ps;
            void Execute() override
            {
                if (ps && ps->pool)
                {
                    ps->pool->RemoveDeadParticles();
                    ps->pool->SyncToGPU();
                }
            }
        };

        std::vector<SyncJob> syncJobs;
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable || !ps.pool || ps.pool->Empty()) continue;
            SyncJob sj;
            sj.ps = &ps;
            syncJobs.push_back(std::move(sj));
        }
        for (auto& job : syncJobs)
            jobHandles.push_back(jobSystem.Schedule(&job));
        JobSystem::CompleteAll(jobHandles);
    }
    void ParticleSystem::OnDestroy(RuntimeScene* scene)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::ParticleSystemComponent>();
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            ps.Stop(true);
            ps.pool.reset();
            ps.emitter.reset();
            ps.affectors.Clear();
        }
    }
    void ParticleSystem::UpdateParticleSystem(ECS::ParticleSystemComponent& ps,
                                              const ECS::TransformComponent* transform,
                                              float deltaTime,
                                              PhysicsSystem* physicsSystem,
                                              entt::registry* registry)
    {
        // Legacy single-threaded path kept for compatibility; new code uses job-based OnUpdate
    }
    glm::vec3 ParticleSystem::GetWorldPosition(const ECS::TransformComponent* transform)
    {
        if (!transform)
        {
            return glm::vec3(0.0f);
        }
        return glm::vec3(transform->position.x, transform->position.y, 0.0f);
    }
    glm::vec2 ParticleSystem::GetWorldScale(const ECS::TransformComponent* transform)
    {
        if (!transform)
        {
            return glm::vec2(1.0f);
        }
        return transform->scale;
    }
} 

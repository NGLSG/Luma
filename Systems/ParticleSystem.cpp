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
    void ParticleSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!scene) return;
        auto& registry = scene->GetRegistry();
        // 获取物理系统用于粒子碰撞检测
        PhysicsSystem* physicsSystem = scene->GetSystem<PhysicsSystem>();
        auto view = registry.view<ECS::ParticleSystemComponent>();
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable) continue;
            const ECS::TransformComponent* transform = nullptr;
            if (registry.all_of<ECS::TransformComponent>(entity))
            {
                transform = &registry.get<ECS::TransformComponent>(entity);
            }
            UpdateParticleSystem(ps, transform, deltaTime, physicsSystem, &registry);
        }
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
        bool needsInit = ps.configDirty || !ps.pool || !ps.emitter;
        if (needsInit)
        {
            ps.Initialize();
            if (ps.emitter)
            {
                ps.emitter->SetConfig(ps.emitterConfig);
            }
            if (ps.playOnAwake && ps.playState == ECS::ParticlePlayState::Stopped)
            {
                ps.Play();
            }
        }
        if (ps.playState != ECS::ParticlePlayState::Playing)
            return;
        float scaledDeltaTime = deltaTime * ps.simulationSpeed;
        ps.systemTime += scaledDeltaTime;
        bool shouldEmit = true;
        if (!ps.loop && ps.systemTime >= ps.duration)
        {
            shouldEmit = false;
            if (!ps.HasActiveParticles())
            {
                ps.Stop(false);
                return;
            }
        }
        else if (ps.loop && ps.systemTime >= ps.duration)
        {
            ps.systemTime = 0.0f;
        }
        glm::vec3 worldPos = GetWorldPosition(transform);
        glm::vec2 worldScale = GetWorldScale(transform);
        glm::vec3 positionDelta = worldPos - ps.lastPosition;
        ps.currentVelocity = positionDelta / std::max(scaledDeltaTime, 0.0001f);
        ps.lastPosition = worldPos;
        if (ps.emitter && ps.pool && shouldEmit)
        {
            ps.emitter->Update(*ps.pool, scaledDeltaTime, worldPos, ps.currentVelocity, worldScale);
        }
        if (ps.pool && !ps.pool->Empty())
        {
            ps.affectors.UpdateBatch(ps.pool->GetParticles(), scaledDeltaTime);
            if (ps.collisionEnabled)
            {
                for (auto& particle : ps.pool->GetParticles())
                {
                    float distance = glm::dot(particle.position - ps.collisionPlanePoint, ps.collisionPlaneNormal);
                    if (distance < 0.0f)
                    {
                        if (ps.collisionKillOnHit)
                        {
                            particle.age = particle.lifetime; 
                        }
                        else
                        {
                            particle.position -= ps.collisionPlaneNormal * distance;
                            float normalVelocity = glm::dot(particle.velocity, ps.collisionPlaneNormal);
                            if (normalVelocity < 0.0f)
                            {
                                glm::vec3 normalComponent = ps.collisionPlaneNormal * normalVelocity;
                                glm::vec3 tangentComponent = particle.velocity - normalComponent;
                                particle.velocity = tangentComponent * (1.0f - ps.collisionFriction) - 
                                                    normalComponent * ps.collisionBounciness;
                            }
                        }
                    }
                }
            }
            // Box2D 物理碰撞检测 - 使用射线检测
            if (ps.physicsCollisionEnabled && physicsSystem && registry)
            {
                b2WorldId worldId = physicsSystem->GetWorld();
                if (b2World_IsValid(worldId))
                {
                    float radiusMeters = ps.particleRadius * METER_PER_PIXEL;
                    
                    for (auto& particle : ps.pool->GetParticles())
                    {
                        if (particle.IsDead()) continue;
                        
                        // 转换粒子位置到Box2D坐标系 (Y轴翻转)
                        b2Vec2 particlePos = {
                            particle.position.x * METER_PER_PIXEL,
                            -particle.position.y * METER_PER_PIXEL
                        };
                        
                        // 使用AABB查询检测重叠
                        b2AABB aabb;
                        aabb.lowerBound = {particlePos.x - radiusMeters, particlePos.y - radiusMeters};
                        aabb.upperBound = {particlePos.x + radiusMeters, particlePos.y + radiusMeters};
                        
                        // 查询结构体
                        struct ParticleCollisionContext
                        {
                            b2Vec2 particlePos;
                            float radius;
                            bool hasCollision;
                            b2Vec2 collisionNormal;
                            b2ShapeId hitShape;
                        };
                        
                        ParticleCollisionContext ctx = {particlePos, radiusMeters, false, {0, -1}, b2_nullShapeId};
                        
                        // AABB重叠查询回调
                        auto overlapCallback = [](b2ShapeId shapeId, void* context) -> bool
                        {
                            auto* ctx = static_cast<ParticleCollisionContext*>(context);
                            
                            b2BodyId bodyId = b2Shape_GetBody(shapeId);
                            b2BodyType bodyType = b2Body_GetType(bodyId);
                            
                            // 只与静态和动态物体碰撞 (跳过sensor)
                            if (b2Shape_IsSensor(shapeId))
                                return true;
                            
                            if (bodyType == b2_staticBody || bodyType == b2_dynamicBody || bodyType == b2_kinematicBody)
                            {
                                // 检查粒子点是否在形状内
                                b2Transform bodyTransform = b2Body_GetTransform(bodyId);
                                
                                // 使用形状测试点来判断是否碰撞
                                // 对于圆形粒子，检查粒子中心到形状的距离
                                b2Vec2 localPoint = b2InvTransformPoint(bodyTransform, ctx->particlePos);
                                
                                // 获取形状AABB来近似检测
                                b2AABB shapeAABB = b2Shape_GetAABB(shapeId);
                                
                                // 检查粒子是否在形状AABB范围内（考虑粒子半径）
                                if (ctx->particlePos.x + ctx->radius > shapeAABB.lowerBound.x &&
                                    ctx->particlePos.x - ctx->radius < shapeAABB.upperBound.x &&
                                    ctx->particlePos.y + ctx->radius > shapeAABB.lowerBound.y &&
                                    ctx->particlePos.y - ctx->radius < shapeAABB.upperBound.y)
                                {
                                    ctx->hasCollision = true;
                                    ctx->hitShape = shapeId;
                                    
                                    // 计算法线 - 从形状中心指向粒子
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
                                    return false; // 找到碰撞，停止查询
                                }
                            }
                            return true; // 继续查询
                        };
                        
                        b2World_OverlapAABB(worldId, aabb, b2DefaultQueryFilter(), overlapCallback, &ctx);
                        
                        if (ctx.hasCollision)
                        {
                            if (ps.physicsCollisionKillOnHit)
                            {
                                particle.age = particle.lifetime;
                            }
                            else
                            {
                                // 将粒子推出碰撞体
                                float pushDistance = ps.particleRadius * 0.5f;
                                particle.position.x += ctx.collisionNormal.x * pushDistance;
                                particle.position.y -= ctx.collisionNormal.y * pushDistance; // Y轴翻转
                                
                                // 计算反弹 (注意Y轴翻转)
                                glm::vec3 normal(ctx.collisionNormal.x, -ctx.collisionNormal.y, 0.0f);
                                float normalVelocity = glm::dot(particle.velocity, normal);
                                
                                if (normalVelocity < 0.0f)
                                {
                                    glm::vec3 normalComponent = normal * normalVelocity;
                                    glm::vec3 tangentComponent = particle.velocity - normalComponent;
                                    particle.velocity = tangentComponent * (1.0f - ps.physicsCollisionFriction) -
                                                        normalComponent * ps.physicsCollisionBounciness;
                                }
                            }
                        }
                    }
                }
            }
            ps.pool->RemoveDeadParticles();
            if (ps.simulationSpace == ECS::ParticleSimulationSpace::Local && transform)
            {
                for (auto& particle : ps.pool->GetParticles())
                {
                    particle.position += positionDelta;
                }
            }
            ps.pool->SyncToGPU();
        }
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

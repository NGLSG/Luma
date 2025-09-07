#include "PhysicsSystem.h"


#include "TagComponent.h"
#include "../Resources/RuntimeAsset/RuntimePhysicsMaterial.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Resources/Loaders/PhysicsMaterialLoader.h"


#include "../Components/ColliderComponent.h"
#include "../Components/IDComponent.h"
#include "../Components/RigidBody.h"
#include "../Components/Transform.h"


#include "TaskSystem.h"
#include "../Data/EngineContext.h"

namespace Systems
{
    static constexpr float PIXELS_PER_METER = 32.0f;
    static constexpr float METER_PER_PIXEL = 1.0f / PIXELS_PER_METER;
    static constexpr float PI = 3.14159265358979323846f;

    namespace
    {
        
        
        
        struct RayCastCallbackContext
        {
            std::vector<RayCastResult>* hits;
            bool penetrate;
        };

        
        
        
        
        
        
        
        float RayCastCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
        {
            auto* queryContext = static_cast<RayCastCallbackContext*>(context);

            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            void* userData = b2Body_GetUserData(bodyId);
            if (userData == nullptr)
            {
                
                return 1.0f;
            }
            entt::entity hitEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userData));

            
            queryContext->hits->emplace_back(RayCastResult{
                hitEntity,
                {point.x * PIXELS_PER_METER, -point.y * PIXELS_PER_METER},
                {normal.x, -normal.y},
                fraction
            });

            
            return queryContext->penetrate ? 1.0f : fraction;
        }

        
        
        
        struct CircleCheckCallbackContext
        {
            entt::registry& registry;
            const std::vector<std::string>& tags;
            b2Vec2 queryCenter;

            float closestDistanceSq = std::numeric_limits<float>::max();
            entt::entity bestHit = entt::null;
        };

        
        
        
        
        bool CircleCheckCallback(b2ShapeId shapeId, void* context)
        {
            auto* queryContext = static_cast<CircleCheckCallbackContext*>(context);

            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            void* userData = b2Body_GetUserData(bodyId);
            if (userData == nullptr)
            {
                return true; 
            }
            entt::entity hitEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userData));

            
            if (!queryContext->tags.empty())
            {
                auto* tagComponent = queryContext->registry.try_get<ECS::TagComponent>(hitEntity);
                bool tagMatch = false;
                if (tagComponent)
                {
                    for (const auto& tag : queryContext->tags)
                    {
                        if (tagComponent->tag == tag)
                        {
                            tagMatch = true;
                            break;
                        }
                    }
                }
                if (!tagMatch)
                {
                    return true; 
                }
            }

            b2Vec2 bodyPos = b2Body_GetPosition(bodyId);
            float distanceSq = b2DistanceSquared(bodyPos, queryContext->queryCenter);
            if (distanceSq < queryContext->closestDistanceSq)
            {
                queryContext->closestDistanceSq = distanceSq;
                queryContext->bestHit = hitEntity;
            }

            return true; 
        }
    }

    b2Rot AngleToB2Rot(float angle)
    {
        return b2MakeRot(angle);
    }

    PhysicsSystem::PhysicsSystem() : m_world(b2_nullWorldId)
    {
    }

    PhysicsSystem::~PhysicsSystem()
    {
        if (Destroyed)
        {
            return;
        }
        if (m_world.index1 != B2_NULL_INDEX)
        {
            Destroyed = true;
            b2DestroyWorld(m_world);
        }
    }

    PhysicsSystem::PhysicsSystem(PhysicsSystem&& other) noexcept
        : m_world(other.m_world), m_accumulator(other.m_accumulator)
    {
        other.m_world = b2_nullWorldId;
    }


    PhysicsSystem& PhysicsSystem::operator=(PhysicsSystem&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }


        if (m_world.index1 != B2_NULL_INDEX)
        {
            b2DestroyWorld(m_world);
        }


        m_world = other.m_world;
        m_accumulator = other.m_accumulator;


        other.m_world = b2_nullWorldId;

        return *this;
    }

    static void* EnqueueTask_Static(b2TaskCallback* task, int itemCount, int minRange, void* taskContext,
                                    void* userContext)
    {
        return static_cast<TaskSystem*>(userContext)->ParallelFor(task, itemCount, minRange, taskContext);
    }

    static void FinishTask_Static(void* userTask, void* userContext)
    {
        static_cast<TaskSystem*>(userContext)->Finish(userTask);
    }

    void PhysicsSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        m_scene = scene;
        int threadCount = std::thread::hardware_concurrency();
        int workerCount = std::max(1, threadCount - 2);


        m_taskSystem = std::make_unique<TaskSystem>(workerCount);

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, -9.8f};


        worldDef.workerCount = workerCount;
        worldDef.enqueueTask = EnqueueTask_Static;
        worldDef.finishTask = FinishTask_Static;
        worldDef.userTaskContext = m_taskSystem.get();

        m_world = b2CreateWorld(&worldDef);

        auto& registry = scene->GetRegistry();
        auto bodyView = registry.view<ECS::TransformComponent, ECS::RigidBodyComponent>();

        for (auto entity : bodyView)
        {
            auto [transform, rb] = bodyView.get<ECS::TransformComponent, ECS::RigidBodyComponent>(entity);
            b2BodyDef bodyDef = b2DefaultBodyDef();

            switch (rb.bodyType)
            {
            case ECS::BodyType::Static: bodyDef.type = b2_staticBody;
                break;
            case ECS::BodyType::Kinematic: bodyDef.type = b2_kinematicBody;
                break;
            case ECS::BodyType::Dynamic: bodyDef.type = b2_dynamicBody;
                break;
            }

            bodyDef.position = {
                transform.position.x / PIXELS_PER_METER,
                -transform.position.y / PIXELS_PER_METER
            };


            bodyDef.rotation = b2MakeRot(-transform.rotation);

            bodyDef.linearDamping = rb.linearDamping;
            bodyDef.angularDamping = rb.angularDamping;
            bodyDef.gravityScale = rb.gravityScale;
            bodyDef.isAwake = (rb.sleepingMode != ECS::SleepingMode::StartAsleep);
            bodyDef.enableSleep = (rb.sleepingMode != ECS::SleepingMode::NeverSleep);


            bodyDef.motionLocks.linearX = rb.constraints.freezePositionX;
            bodyDef.motionLocks.linearY = rb.constraints.freezePositionY;
            bodyDef.motionLocks.angularZ = rb.constraints.freezeRotation;
            bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entity));
            bodyDef.isBullet = (rb.collisionDetection == ECS::CollisionDetectionType::Continuous);

            b2BodyId bodyId = b2CreateBody(m_world, &bodyDef);
            rb.runtimeBody = bodyId;
            b2Body_SetLinearVelocity(bodyId, {rb.linearVelocity.x, -rb.linearVelocity.y});
            b2Body_SetAngularVelocity(bodyId, -rb.angularVelocity);

            CreateShapesForEntity(entity, registry, transform);
        }
    }

    void PhysicsSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        if (m_world.index1 == B2_NULL_INDEX) return;
        if (m_scene != scene)
        {
            m_scene = scene;
        }
        auto& registry = scene->GetRegistry();
        {
            std::unordered_set<entt::entity> dirtyEntities;

            auto collectDirty = [&](const auto& view)
            {
                for (const auto& [entity, component] : view.each())
                {
                    if (component.isDirty)
                    {
                        dirtyEntities.insert(entity);
                    }
                }
            };

            collectDirty(registry.view<ECS::BoxColliderComponent>());
            collectDirty(registry.view<ECS::CircleColliderComponent>());
            collectDirty(registry.view<ECS::PolygonColliderComponent>());
            collectDirty(registry.view<ECS::EdgeColliderComponent>());
            collectDirty(registry.view<ECS::CapsuleColliderComponent>());
            collectDirty(registry.view<ECS::TilemapColliderComponent>());


            for (const auto entity : dirtyEntities)
            {
                if (registry.valid(entity))
                {
                    RecreateAllShapesForEntity(entity, registry);


                    if (auto* c = registry.try_get<ECS::BoxColliderComponent>(entity)) c->isDirty = false;
                    if (auto* c = registry.try_get<ECS::CircleColliderComponent>(entity)) c->isDirty = false;
                    if (auto* c = registry.try_get<ECS::PolygonColliderComponent>(entity)) c->isDirty = false;
                    if (auto* c = registry.try_get<ECS::EdgeColliderComponent>(entity)) c->isDirty = false;
                    if (auto* c = registry.try_get<ECS::CapsuleColliderComponent>(entity)) c->isDirty = false;
                    if (auto* c = registry.try_get<ECS::TilemapColliderComponent>(entity)) c->isDirty = false;
                }
            }
        }


        const float timeStep = 1.0f / 60.0f;
        int32_t subStepCount = 8;
        const float highFpsThreshold = 120.0f;
        const float lowFpsThreshold = 50.0f;


        if (context.currentFps > highFpsThreshold)
        {
            subStepCount = std::min(12, subStepCount + 1);
        }
        else if (context.currentFps < lowFpsThreshold)
        {
            subStepCount = std::max(1, subStepCount - 1);
        }


        auto kinematicView = registry.view<ECS::TransformComponent, ECS::RigidBodyComponent>();
        for (auto entity : kinematicView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            auto [transform, rb] = kinematicView.get<ECS::TransformComponent, ECS::RigidBodyComponent>(entity);
            if (!rb.Enable)
                continue;
            if (rb.bodyType == ECS::BodyType::Kinematic && rb.runtimeBody.index1 != B2_NULL_INDEX)
            {
                b2Vec2 currentPos = b2Body_GetPosition(rb.runtimeBody);
                b2Vec2 desiredPos = {transform.position.x / PIXELS_PER_METER, -transform.position.y / PIXELS_PER_METER};
                b2Vec2 displacement = b2Sub(desiredPos, currentPos);
                b2Vec2 velocity = b2MulSV(1.0f / timeStep, displacement);
                b2Body_SetLinearVelocity(rb.runtimeBody, velocity);

                float currentAngle = b2Rot_GetAngle(b2Body_GetRotation(rb.runtimeBody));
                float desiredAngle = -transform.rotation;
                float angleDiff = desiredAngle - currentAngle;
                b2Body_SetAngularVelocity(rb.runtimeBody, angleDiff / timeStep);
            }
        }


        const float maxDeltaTime = 0.032f;
        m_accumulator += std::min(deltaTime, maxDeltaTime);
        int maxStepsPerFrame = 5;
        while (m_accumulator >= timeStep && maxStepsPerFrame > 0)
        {
            b2World_Step(m_world, timeStep, subStepCount);
            m_accumulator -= timeStep;
            maxStepsPerFrame--;
        }


        b2ContactEvents contactEvents = b2World_GetContactEvents(m_world);
        b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_world);

        auto& eventBus = EventBus::GetInstance();


        for (const auto& pair : m_currentContacts)
        {
            eventBus.Publish(PhysicsContactEvent{
                PhysicsContactEvent::ContactType::CollisionStay, pair.entityA, pair.entityB
            });
        }


        for (const auto& pair : m_currentTriggers)
        {
            eventBus.Publish(PhysicsContactEvent{
                PhysicsContactEvent::ContactType::TriggerStay, pair.entityA, pair.entityB
            });
        }


        for (int i = 0; i < contactEvents.endCount; ++i)
        {
            const auto& event = contactEvents.endEvents[i];


            b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

            void* userDataA = b2Body_GetUserData(bodyA);
            void* userDataB = b2Body_GetUserData(bodyB);

            if (userDataA == nullptr || userDataB == nullptr)
            {
                continue;
            }

            entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userDataA));
            entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userDataB));


            if (m_currentContacts.erase({entityA, entityB}) > 0 || m_currentContacts.erase({entityB, entityA}) > 0)
            {
                eventBus.Publish(PhysicsContactEvent{
                    PhysicsContactEvent::ContactType::CollisionExit, entityA, entityB
                });
            }
        }


        for (int i = 0; i < sensorEvents.endCount; ++i)
        {
            const auto& event = sensorEvents.endEvents[i];

            b2BodyId sensorBody = b2Shape_GetBody(event.sensorShapeId);
            b2BodyId visitorBody = b2Shape_GetBody(event.visitorShapeId);

            void* sensorUserData = b2Body_GetUserData(sensorBody);
            void* visitorUserData = b2Body_GetUserData(visitorBody);

            if (sensorUserData == nullptr || visitorUserData == nullptr)
            {
                continue;
            }

            entt::entity sensorEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(sensorUserData));
            entt::entity visitorEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(visitorUserData));

            if (m_currentTriggers.erase({sensorEntity, visitorEntity}) > 0 ||
                m_currentTriggers.erase({visitorEntity, sensorEntity}) > 0)
            {
                eventBus.Publish(PhysicsContactEvent{
                    PhysicsContactEvent::ContactType::TriggerExit, sensorEntity, visitorEntity
                });
            }
        }


        for (int i = 0; i < contactEvents.beginCount; ++i)
        {
            const auto& event = contactEvents.beginEvents[i];

            b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

            void* userDataA = b2Body_GetUserData(bodyA);
            void* userDataB = b2Body_GetUserData(bodyB);

            if (userDataA == nullptr || userDataB == nullptr)
            {
                continue;
            }

            entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userDataA));
            entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userDataB));


            m_currentContacts.insert({entityA, entityB});
            eventBus.Publish(PhysicsContactEvent{
                PhysicsContactEvent::ContactType::CollisionEnter, entityA, entityB
            });
        }


        for (int i = 0; i < sensorEvents.beginCount; ++i)
        {
            const auto& event = sensorEvents.beginEvents[i];

            b2BodyId sensorBody = b2Shape_GetBody(event.sensorShapeId);
            b2BodyId visitorBody = b2Shape_GetBody(event.visitorShapeId);

            void* sensorUserData = b2Body_GetUserData(sensorBody);
            void* visitorUserData = b2Body_GetUserData(visitorBody);

            if (sensorUserData == nullptr || visitorUserData == nullptr)
            {
                continue;
            }

            entt::entity sensorEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(sensorUserData));
            entt::entity visitorEntity = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(visitorUserData));

            m_currentTriggers.insert({sensorEntity, visitorEntity});
            eventBus.Publish(PhysicsContactEvent{
                PhysicsContactEvent::ContactType::TriggerEnter, sensorEntity, visitorEntity
            });
        }


        auto dynamicView = registry.view<ECS::TransformComponent, ECS::RigidBodyComponent>();
        for (auto entity : dynamicView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            auto [transform, rb] = dynamicView.get<ECS::TransformComponent, ECS::RigidBodyComponent>(entity);
            if (!rb.Enable)
                continue;
            if (rb.bodyType == ECS::BodyType::Dynamic && rb.runtimeBody.index1 != B2_NULL_INDEX)
            {
                if (b2Body_IsAwake(rb.runtimeBody))
                {
                    b2Vec2 position = b2Body_GetPosition(rb.runtimeBody);
                    b2Rot rotation = b2Body_GetRotation(rb.runtimeBody);
                    transform.position = {position.x * PIXELS_PER_METER, -position.y * PIXELS_PER_METER};
                    transform.rotation = -b2Rot_GetAngle(rotation);
                }
            }
        }
    }

    void PhysicsSystem::OnDestroy(RuntimeScene* scene)
    {
        if (Destroyed)
        {
            return;
        }
        if (m_world.index1 != B2_NULL_INDEX)
        {
            b2DestroyWorld(m_world);
            m_world = b2_nullWorldId;
            Destroyed = true;
        }
    }

    b2WorldId PhysicsSystem::GetWorld() const
    {
        return m_world;
    }

    
    
    
    
    
    
    
    std::optional<RayCastResults> PhysicsSystem::RayCast(const ECS::Vector2f& startPoint, const ECS::Vector2f& endPoint,
                                                         bool penetrate) const
    {
        if (m_world.index1 == B2_NULL_INDEX)
        {
            return std::nullopt;
        }

        
        b2Vec2 origin = {startPoint.x * METER_PER_PIXEL, -startPoint.y * METER_PER_PIXEL};
        b2Vec2 target = {endPoint.x * METER_PER_PIXEL, -endPoint.y * METER_PER_PIXEL};
        b2Vec2 translation = b2Sub(target, origin);

        std::vector<RayCastResult> allHits;
        RayCastCallbackContext context = {&allHits, penetrate};

        
        b2World_CastRay(m_world, origin, translation, b2DefaultQueryFilter(), RayCastCallback, &context);

        if (allHits.empty())
        {
            return std::nullopt;
        }

        
        std::sort(allHits.begin(), allHits.end(), [](const auto& a, const auto& b)
        {
            return a.fraction < b.fraction;
        });

        return RayCastResults{std::move(allHits)};
    }


    std::optional<RayCastResult> PhysicsSystem::CircleCheck(const ECS::Vector2f& center, float radius,
                                                            entt::registry& registry,
                                                            const std::vector<std::string>& tags) const
    {
        if (m_world.index1 == B2_NULL_INDEX || radius <= 0.0f)
        {
            return std::nullopt;
        }

        
        b2Vec2 queryCenter = {center.x * METER_PER_PIXEL, -center.y * METER_PER_PIXEL};
        CircleCheckCallbackContext context = {registry, tags, queryCenter};

        float scaledRadius = radius * METER_PER_PIXEL;

        b2AABB aabb;
        aabb.lowerBound = {queryCenter.x - scaledRadius, queryCenter.y - scaledRadius};
        aabb.upperBound = {queryCenter.x + scaledRadius, queryCenter.y + scaledRadius};

        b2World_OverlapAABB(m_world, aabb, b2DefaultQueryFilter(), CircleCheckCallback, &context);

        
        if (context.bestHit != entt::null)
        {
            
            if (!registry.valid(context.bestHit) || !registry.all_of<ECS::RigidBodyComponent>(context.bestHit))
            {
                return std::nullopt;
            }

            const auto& rb = registry.get<ECS::RigidBodyComponent>(context.bestHit);
            b2Vec2 bodyPos = b2Body_GetPosition(rb.runtimeBody);

            
            
            b2Vec2 normal = b2Sub(queryCenter, bodyPos);
            if (b2LengthSquared(normal) > 1e-6f)
            {
                normal = b2Normalize(normal);
            }

            return RayCastResult{
                context.bestHit,
                {bodyPos.x * PIXELS_PER_METER, -bodyPos.y * PIXELS_PER_METER},
                {normal.x, -normal.y},
                0.0f
            };
        }

        return std::nullopt;
    }

    void PhysicsSystem::ApplyForce(entt::entity entity, const ECS::Vector2f& force, ForceMode mode)
    {
        auto& registry = m_scene->GetRegistry();
        if (!registry.valid(entity)) return;
        if (!registry.all_of<ECS::RigidBodyComponent>(entity)) return;
        auto& rb = registry.get<ECS::RigidBodyComponent>(entity);
        if (rb.runtimeBody.index1 == B2_NULL_INDEX) return;
        if (mode == ForceMode::Force)
        {
            b2Body_ApplyForceToCenter(rb.runtimeBody, {force.x, -force.y}, true);
        }
        else if (mode == ForceMode::Impulse)
        {
            b2Body_ApplyLinearImpulseToCenter(rb.runtimeBody, {force.x, -force.y}, true);
        }
    }

    void PhysicsSystem::CreateShapesForEntity(entt::entity entity, entt::registry& registry,
                                              const ECS::TransformComponent& transform)
    {
        auto& rb = registry.get<ECS::RigidBodyComponent>(entity);
        b2BodyId bodyId = rb.runtimeBody;
        const auto& scale = transform.scale;

        b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
        if (rb.physicsMaterial.Valid())
        {
            PhysicsMaterialLoader loader;
            if (sk_sp<RuntimePhysicsMaterial> matAsset = loader.LoadAsset(rb.physicsMaterial.assetGuid))
            {
                material = *matAsset;
            }
        }


        float totalArea = 0.0f;
        if (registry.all_of<ECS::BoxColliderComponent>(entity))
        {
            auto& bc = registry.get<ECS::BoxColliderComponent>(entity);
            totalArea += (bc.size.x * scale.x * METER_PER_PIXEL) * (bc.size.y * scale.y * METER_PER_PIXEL);
        }
        if (registry.all_of<ECS::CircleColliderComponent>(entity))
        {
            auto& cc = registry.get<ECS::CircleColliderComponent>(entity);
            float minScale = std::min(scale.x, scale.y);
            totalArea += PI * (cc.radius * minScale * METER_PER_PIXEL) * (cc.radius * minScale * METER_PER_PIXEL);
        }
        if (registry.all_of<ECS::CapsuleColliderComponent>(entity))
        {
            auto& cap = registry.get<ECS::CapsuleColliderComponent>(entity);
            float scaledSizeX = cap.size.x * scale.x;
            float scaledSizeY = cap.size.y * scale.y;
            float radius = (cap.direction == ECS::CapsuleDirection::Vertical) ? scaledSizeX : scaledSizeY;
            float rectHeight = (cap.direction == ECS::CapsuleDirection::Vertical) ? scaledSizeY : scaledSizeX;
            rectHeight = std::max(0.0f, rectHeight - radius);
            radius /= 2.0f;

            float circleArea = PI * (radius * METER_PER_PIXEL) * (radius * METER_PER_PIXEL);
            float rectArea = (rectHeight * METER_PER_PIXEL) * (2.0f * radius * METER_PER_PIXEL);
            totalArea += circleArea + rectArea;
        }
        if (registry.all_of<ECS::PolygonColliderComponent>(entity))
        {
            auto& poly = registry.get<ECS::PolygonColliderComponent>(entity);
            if (poly.vertices.size() >= 3)
            {
                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();

                for (const auto& v : poly.vertices)
                {
                    minX = std::min(minX, (v.x + poly.offset.x) * scale.x);
                    maxX = std::max(maxX, (v.x + poly.offset.x) * scale.x);
                    minY = std::min(minY, (v.y + poly.offset.y) * scale.y);
                    maxY = std::max(maxY, (v.y + poly.offset.y) * scale.y);
                }
                totalArea += (maxX - minX) * (maxY - minY) * METER_PER_PIXEL * METER_PER_PIXEL;
            }
        }

        float density = 0.0f;
        if (totalArea > 1e-6f)
        {
            density = rb.mass / totalArea;
        }


        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.material = material;
        shapeDef.density = density;
        shapeDef.enableContactEvents = true;


        if (registry.all_of<ECS::BoxColliderComponent>(entity))
        {
            auto& bc = registry.get<ECS::BoxColliderComponent>(entity);
            shapeDef.isSensor = bc.isTrigger;

            float scaledWidth = bc.size.x * scale.x / PIXELS_PER_METER / 2.0f;
            float scaledHeight = bc.size.y * scale.y / PIXELS_PER_METER / 2.0f;
            b2Vec2 scaledOffset = {
                bc.offset.x * scale.x / PIXELS_PER_METER,
                -bc.offset.y * scale.y / PIXELS_PER_METER
            };

            b2Polygon box = b2MakeOffsetBox(scaledWidth, scaledHeight, scaledOffset, AngleToB2Rot(0));
            bc.runtimeShape = b2CreatePolygonShape(bodyId, &shapeDef, &box);
        }

        if (registry.all_of<ECS::CircleColliderComponent>(entity))
        {
            auto& cc = registry.get<ECS::CircleColliderComponent>(entity);
            shapeDef.isSensor = cc.isTrigger;

            float minScale = std::min(scale.x, scale.y);
            float scaledRadius = cc.radius * minScale / PIXELS_PER_METER;

            b2Circle circle = {
                {
                    cc.offset.x * scale.x / PIXELS_PER_METER,
                    -cc.offset.y * scale.y / PIXELS_PER_METER
                },
                scaledRadius
            };
            cc.runtimeShape = b2CreateCircleShape(bodyId, &shapeDef, &circle);
        }

        if (registry.all_of<ECS::CapsuleColliderComponent>(entity))
        {
            auto& cap = registry.get<ECS::CapsuleColliderComponent>(entity);
            shapeDef.isSensor = cap.isTrigger;
            b2Capsule capsule;

            b2Vec2 scaledOffset = {
                cap.offset.x * scale.x / PIXELS_PER_METER,
                -cap.offset.y * scale.y / PIXELS_PER_METER
            };

            if (cap.direction == ECS::CapsuleDirection::Vertical)
            {
                float radius = (cap.size.x * scale.x) / PIXELS_PER_METER / 2.0f;
                float height = (cap.size.y * scale.y) / PIXELS_PER_METER;
                float halfHeight = std::max(0.0f, height / 2.0f - radius);
                capsule = {
                    {scaledOffset.x, scaledOffset.y - halfHeight},
                    {scaledOffset.x, scaledOffset.y + halfHeight},
                    radius
                };
            }
            else
            {
                float radius = (cap.size.y * scale.y) / PIXELS_PER_METER / 2.0f;
                float width = (cap.size.x * scale.x) / PIXELS_PER_METER;
                float halfWidth = std::max(0.0f, width / 2.0f - radius);
                capsule = {
                    {scaledOffset.x - halfWidth, scaledOffset.y},
                    {scaledOffset.x + halfWidth, scaledOffset.y},
                    radius
                };
            }
            cap.runtimeShape = b2CreateCapsuleShape(bodyId, &shapeDef, &capsule);
        }

        if (registry.all_of<ECS::PolygonColliderComponent>(entity))
        {
            auto& poly = registry.get<ECS::PolygonColliderComponent>(entity);
            if (poly.vertices.size() >= 3)
            {
                shapeDef.isSensor = poly.isTrigger;
                std::vector<b2Vec2> b2Vertices;
                b2Vertices.reserve(poly.vertices.size());
                for (const auto& v : poly.vertices)
                {
                    b2Vertices.push_back({
                        (v.x + poly.offset.x) * scale.x / PIXELS_PER_METER,
                        -(v.y + poly.offset.y) * scale.y / PIXELS_PER_METER
                    });
                }
                b2Hull hull = b2ComputeHull(b2Vertices.data(), static_cast<int32_t>(b2Vertices.size()));
                b2Polygon polygon = b2MakePolygon(&hull, 0.0f);
                poly.runtimeShape = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
            }
        }

        if (registry.all_of<ECS::EdgeColliderComponent>(entity))
        {
            auto& edge = registry.get<ECS::EdgeColliderComponent>(entity);
            if (edge.vertices.size() >= 2)
            {
                std::vector<b2Vec2> b2Vertices;
                b2Vertices.reserve(edge.vertices.size());
                for (const auto& v : edge.vertices)
                {
                    b2Vertices.push_back({
                        (v.x + edge.offset.x) * scale.x / PIXELS_PER_METER,
                        -(v.y + edge.offset.y) * scale.y / PIXELS_PER_METER
                    });
                }
                b2ChainDef chainDef = b2DefaultChainDef();
                chainDef.points = b2Vertices.data();
                chainDef.count = static_cast<int32_t>(b2Vertices.size());
                chainDef.isLoop = edge.loop;
                edge.runtimeChain = b2CreateChain(bodyId, &chainDef);
            }
        }


        if (registry.all_of<ECS::TilemapColliderComponent>(entity))
        {
            auto& tilemapCollider = registry.get<ECS::TilemapColliderComponent>(entity);

            for (const auto& chain : tilemapCollider.generatedChains)
            {
                if (chain.size() < 2) continue;

                std::vector<b2Vec2> b2Vertices;
                b2Vertices.reserve(chain.size());
                for (const auto& v : chain)
                {
                    b2Vertices.push_back({
                        (v.x + tilemapCollider.offset.x) * scale.x * METER_PER_PIXEL,
                        -(v.y + tilemapCollider.offset.y) * scale.y * METER_PER_PIXEL
                    });
                }

                b2ChainDef chainDef = b2DefaultChainDef();
                chainDef.points = b2Vertices.data();
                chainDef.count = static_cast<int32_t>(b2Vertices.size());
                chainDef.isLoop = false;

                b2ChainId newChainId = b2CreateChain(bodyId, &chainDef);
                if (newChainId.index1 != B2_NULL_INDEX)
                {
                    tilemapCollider.runtimeChains.push_back(newChainId);
                }
            }
        }
    }


    void PhysicsSystem::RecreateAllShapesForEntity(entt::entity entity, entt::registry& registry)
    {
        if (!registry.all_of<ECS::RigidBodyComponent, ECS::TransformComponent>(entity))
        {
            return;
        }


        if (auto* bc = registry.try_get<ECS::BoxColliderComponent>(entity); bc && bc->runtimeShape.index1 !=
            B2_NULL_INDEX)
        {
            b2DestroyShape(bc->runtimeShape, false);
            bc->runtimeShape = b2_nullShapeId;
        }
        if (auto* cc = registry.try_get<ECS::CircleColliderComponent>(entity); cc && cc->runtimeShape.index1 !=
            B2_NULL_INDEX)
        {
            b2DestroyShape(cc->runtimeShape, false);
            cc->runtimeShape = b2_nullShapeId;
        }
        if (auto* cap = registry.try_get<ECS::CapsuleColliderComponent>(entity); cap && cap->runtimeShape.index1 !=
            B2_NULL_INDEX)
        {
            b2DestroyShape(cap->runtimeShape, false);
            cap->runtimeShape = b2_nullShapeId;
        }
        if (auto* poly = registry.try_get<ECS::PolygonColliderComponent>(entity); poly && poly->runtimeShape.index1 !=
            B2_NULL_INDEX)
        {
            b2DestroyShape(poly->runtimeShape, false);
            poly->runtimeShape = b2_nullShapeId;
        }
        if (auto* edge = registry.try_get<ECS::EdgeColliderComponent>(entity); edge && edge->runtimeChain.index1 !=
            B2_NULL_INDEX)
        {
            b2DestroyChain(edge->runtimeChain);
            edge->runtimeChain = b2_nullChainId;
        }

        if (auto* tmc = registry.try_get<ECS::TilemapColliderComponent>(entity))
        {
            for (b2ChainId chainId : tmc->runtimeChains)
            {
                if (chainId.index1 != B2_NULL_INDEX)
                {
                    b2DestroyChain(chainId);
                }
            }
            tmc->runtimeChains.clear();
        }


        const auto& transform = registry.get<ECS::TransformComponent>(entity);
        CreateShapesForEntity(entity, registry, transform);
    }

    void PhysicsSystem::OnComponentUpdate(const ComponentUpdatedEvent& event)
    {
        if (event.registry.all_of<ECS::RigidBodyComponent>(event.entity))
        {
            SyncRigidBodyProperties(event.entity, event.registry);
        }
    }

    void PhysicsSystem::SyncRigidBodyProperties(entt::entity entity, entt::registry& registry)
    {
        if (!registry.valid(entity) || !registry.all_of<ECS::RigidBodyComponent>(entity))
        {
            return;
        }

        auto& rb = registry.get<ECS::RigidBodyComponent>(entity);
        if (rb.runtimeBody.index1 == B2_NULL_INDEX)
        {
            return;
        }

        b2Body_SetType(rb.runtimeBody, static_cast<b2BodyType>(rb.bodyType));

        b2Body_SetLinearVelocity(rb.runtimeBody, {rb.linearVelocity.x, -rb.linearVelocity.y});
        b2Body_SetAngularVelocity(rb.runtimeBody, -rb.angularVelocity);

        b2Body_SetLinearDamping(rb.runtimeBody, rb.linearDamping);
        b2Body_SetAngularDamping(rb.runtimeBody, rb.angularDamping);
        b2Body_SetGravityScale(rb.runtimeBody, rb.gravityScale);

        bool isAwake = (rb.sleepingMode != ECS::SleepingMode::StartAsleep);
        bool enableSleep = (rb.sleepingMode != ECS::SleepingMode::NeverSleep);
        b2Body_SetAwake(rb.runtimeBody, enableSleep);

        b2MotionLocks locks;
        locks.linearX = rb.constraints.freezePositionX;
        locks.linearY = rb.constraints.freezePositionY;
        locks.angularZ = rb.constraints.freezeRotation;
        b2Body_SetMotionLocks(rb.runtimeBody, locks);

        b2Body_SetBullet(rb.runtimeBody, rb.collisionDetection == ECS::CollisionDetectionType::Continuous);

        RecreateAllShapesForEntity(entity, registry);
    }
}

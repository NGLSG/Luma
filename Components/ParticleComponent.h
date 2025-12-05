#ifndef PARTICLE_COMPONENT_H
#define PARTICLE_COMPONENT_H
#include "../Data/ParticleData.h"
#include "../Particles/Emitter.h"
#include "../Particles/Affector.h"
#include "IComponent.h"
#include "AssetHandle.h"
#include "ComponentRegistry.h"
namespace ECS
{
    enum class ParticlePlayState : uint8_t
    {
        Stopped, 
        Playing, 
        Paused, 
    };
    enum class ParticleSimulationSpace : uint8_t
    {
        Local, 
        World, 
    };
    struct ParticleSystemComponent : public IComponent
    {
        Particles::EmitterConfig emitterConfig;
        AssetHandle textureHandle = AssetHandle(AssetType::Texture); 
        AssetHandle materialHandle = AssetHandle(AssetType::Material); 
        Particles::BlendMode blendMode = Particles::BlendMode::Alpha;
        int zIndex = 0; 
        bool billboard = true; 
        bool useSequenceAnimation = false; 
        std::vector<AssetHandle> textureFrames; 
        enum class TextureAnimationMode { OverLifetime, FPS };
        TextureAnimationMode textureAnimationMode = TextureAnimationMode::OverLifetime;
        float textureAnimationFPS = 30.0f; 
        float textureAnimationCycles = 1.0f; 
        ParticleSimulationSpace simulationSpace = ParticleSimulationSpace::World;
        float simulationSpeed = 1.0f; 
        bool prewarm = false; 
        float prewarmTime = 1.0f; 
        float duration = 5.0f; 
        bool loop = true; 
        bool playOnAwake = true; 
        bool gravityEnabled = false;
        glm::vec3 gravity{0.0f, 98.1f, 0.0f}; 
        bool dragEnabled = false;
        float dragDamping = 0.98f; 
        bool vortexEnabled = false;
        glm::vec3 vortexCenter{0.0f}; 
        float vortexStrength = 10.0f; 
        float vortexRadius = 100.0f; 
        bool noiseEnabled = false;
        float noiseStrength = 5.0f; 
        float noiseFrequency = 1.0f; 
        float noiseSpeed = 1.0f; 
        bool attractorEnabled = false;
        glm::vec3 attractorPosition{0.0f}; 
        float attractorStrength = 50.0f; 
        float attractorRadius = 100.0f; 
        bool collisionEnabled = false; 
        glm::vec3 collisionPlanePoint{0.0f, 0.0f, 0.0f}; 
        glm::vec3 collisionPlaneNormal{0.0f, -1.0f, 0.0f}; 
        float collisionBounciness = 0.5f; 
        float collisionFriction = 0.1f; 
        bool collisionKillOnHit = false;
        // Box2D 物理碰撞
        bool physicsCollisionEnabled = false;
        float physicsCollisionBounciness = 0.5f;
        float physicsCollisionFriction = 0.1f;
        bool physicsCollisionKillOnHit = false;
        float particleRadius = 2.0f; // 粒子碰撞半径（像素）
        ParticlePlayState playState = ParticlePlayState::Stopped;
        float systemTime = 0.0f; 
        std::unique_ptr<Particles::ParticlePool> pool;
        std::unique_ptr<Particles::Emitter> emitter;
        Particles::AffectorChain affectors;
        glm::vec3 lastPosition{0.0f};
        glm::vec3 currentVelocity{0.0f};
        bool configDirty = true;
        bool editorPreviewActive = false; 
        void Initialize()
        {
            if (!pool)
            {
                pool = std::make_unique<Particles::ParticlePool>(emitterConfig.maxParticles);
            }
            if (!emitter)
            {
                emitter = std::make_unique<Particles::Emitter>(emitterConfig);
            }
            SetupDefaultAffectors();
            configDirty = false;
        }
        void RebuildAffectors()
        {
            SetupDefaultAffectors();
        }
        void SetupDefaultAffectors()
        {
            affectors.Clear();
            affectors.Add<Particles::LifetimeAffector>();
            if (gravityEnabled)
            {
                auto grav = affectors.Add<Particles::GravityAffector>();
                grav->gravity = gravity;
            }
            if (dragEnabled)
            {
                auto drag = affectors.Add<Particles::LinearDragAffector>();
                drag->dampingFactor = dragDamping;
            }
            if (vortexEnabled)
            {
                auto vortex = affectors.Add<Particles::VortexAffector>();
                vortex->center = vortexCenter;
                vortex->strength = vortexStrength;
                vortex->radius = vortexRadius;
            }
            if (noiseEnabled)
            {
                auto noise = affectors.Add<Particles::NoiseForceAffector>();
                noise->strength = noiseStrength;
                noise->frequency = noiseFrequency;
                noise->scrollSpeed = noiseSpeed;
            }
            if (attractorEnabled)
            {
                auto attractor = affectors.Add<Particles::AttractorAffector>();
                attractor->position = attractorPosition;
                attractor->strength = attractorStrength;
                attractor->radius = attractorRadius;
            }
            affectors.Add<Particles::VelocityAffector>();
            affectors.Add<Particles::ColorOverLifetimeAffector>();
            affectors.Add<Particles::SizeOverLifetimeAffector>();
            affectors.Add<Particles::RotationAffector>();
            if (useSequenceAnimation && textureFrames.size() > 1)
            {
                auto seqAnim = affectors.Add<Particles::SequenceFrameAnimationAffector>();
                seqAnim->frameCount = static_cast<uint32_t>(textureFrames.size());
                seqAnim->fps = textureAnimationFPS;
                seqAnim->cycles = textureAnimationCycles;
                switch (textureAnimationMode)
                {
                    case TextureAnimationMode::OverLifetime:
                        seqAnim->mode = Particles::TextureAnimationMode::OverLifetime;
                        break;
                    case TextureAnimationMode::FPS:
                        seqAnim->mode = Particles::TextureAnimationMode::FPS;
                        break;
                }
            }
        }
        std::shared_ptr<Particles::GravityAffector> AddGravity(const glm::vec3& gravity = {0.0f, -9.81f, 0.0f})
        {
            return affectors.Add<Particles::GravityAffector>(gravity);
        }
        std::shared_ptr<Particles::LinearDragAffector> AddDrag(float damping = 0.98f)
        {
            return affectors.Add<Particles::LinearDragAffector>(damping);
        }
        void Play()
        {
            if (playState == ParticlePlayState::Stopped)
            {
                Initialize();
                systemTime = 0.0f;
                pool->Clear();
                if (emitter)
                {
                    emitter->Reset();
                }
                if (prewarm)
                {
                    const float dt = 1.0f / 60.0f;
                    for (float t = 0.0f; t < prewarmTime; t += dt)
                    {
                        emitter->Update(*pool, dt);
                        affectors.UpdateBatch(pool->GetParticles(), dt);
                        pool->RemoveDeadParticles();
                    }
                }
            }
            playState = ParticlePlayState::Playing;
        }
        void Pause()
        {
            if (playState == ParticlePlayState::Playing)
            {
                playState = ParticlePlayState::Paused;
            }
        }
        void Stop(bool clearParticles = true)
        {
            playState = ParticlePlayState::Stopped;
            systemTime = 0.0f;
            if (clearParticles)
            {
                if (pool)
                {
                    pool->Clear();
                }
                if (emitter)
                {
                    emitter->Reset();
                }
            }
        }
        void Restart()
        {
            Stop(true);
            Play();
        }
        void Burst(uint32_t count)
        {
            Initialize();
            if (emitter && pool)
            {
                emitter->Burst(*pool, count);
            }
        }
        [[nodiscard]] size_t GetParticleCount() const
        {
            return pool ? pool->Size() : 0;
        }
        [[nodiscard]] bool HasActiveParticles() const
        {
            return pool && !pool->Empty();
        }
        [[nodiscard]] bool IsComplete() const
        {
            if (loop) return false;
            return systemTime >= duration && !HasActiveParticles();
        }
        ParticleSystemComponent() = default;
        ParticleSystemComponent(const ParticleSystemComponent& other)
            : IComponent(other)
              , emitterConfig(other.emitterConfig)
              , textureHandle(other.textureHandle)
              , materialHandle(other.materialHandle)
              , blendMode(other.blendMode)
              , zIndex(other.zIndex)
              , billboard(other.billboard)
              , useSequenceAnimation(other.useSequenceAnimation)
              , textureFrames(other.textureFrames)
              , textureAnimationMode(other.textureAnimationMode)
              , textureAnimationFPS(other.textureAnimationFPS)
              , textureAnimationCycles(other.textureAnimationCycles)
              , simulationSpace(other.simulationSpace)
              , simulationSpeed(other.simulationSpeed)
              , prewarm(other.prewarm)
              , prewarmTime(other.prewarmTime)
              , duration(other.duration)
              , loop(other.loop)
              , playOnAwake(other.playOnAwake)
              , gravityEnabled(other.gravityEnabled)
              , gravity(other.gravity)
              , dragEnabled(other.dragEnabled)
              , dragDamping(other.dragDamping)
              , vortexEnabled(other.vortexEnabled)
              , vortexCenter(other.vortexCenter)
              , vortexStrength(other.vortexStrength)
              , vortexRadius(other.vortexRadius)
              , noiseEnabled(other.noiseEnabled)
              , noiseStrength(other.noiseStrength)
              , noiseFrequency(other.noiseFrequency)
              , noiseSpeed(other.noiseSpeed)
              , attractorEnabled(other.attractorEnabled)
              , attractorPosition(other.attractorPosition)
              , attractorStrength(other.attractorStrength)
              , attractorRadius(other.attractorRadius)
              , collisionEnabled(other.collisionEnabled)
              , collisionPlanePoint(other.collisionPlanePoint)
              , collisionPlaneNormal(other.collisionPlaneNormal)
              , collisionBounciness(other.collisionBounciness)
              , collisionFriction(other.collisionFriction)
              , collisionKillOnHit(other.collisionKillOnHit)
              , physicsCollisionEnabled(other.physicsCollisionEnabled)
              , physicsCollisionBounciness(other.physicsCollisionBounciness)
              , physicsCollisionFriction(other.physicsCollisionFriction)
              , physicsCollisionKillOnHit(other.physicsCollisionKillOnHit)
              , particleRadius(other.particleRadius)
        {
            configDirty = true;
        }
        ParticleSystemComponent& operator=(const ParticleSystemComponent& other)
        {
            if (this != &other)
            {
                IComponent::operator=(other);
                emitterConfig = other.emitterConfig;
                textureHandle = other.textureHandle;
                materialHandle = other.materialHandle;
                blendMode = other.blendMode;
                zIndex = other.zIndex;
                billboard = other.billboard;
                useSequenceAnimation = other.useSequenceAnimation;
                textureFrames = other.textureFrames;
                textureAnimationMode = other.textureAnimationMode;
                textureAnimationFPS = other.textureAnimationFPS;
                textureAnimationCycles = other.textureAnimationCycles;
                simulationSpace = other.simulationSpace;
                simulationSpeed = other.simulationSpeed;
                prewarm = other.prewarm;
                prewarmTime = other.prewarmTime;
                duration = other.duration;
                loop = other.loop;
                playOnAwake = other.playOnAwake;
                gravityEnabled = other.gravityEnabled;
                gravity = other.gravity;
                dragEnabled = other.dragEnabled;
                dragDamping = other.dragDamping;
                vortexEnabled = other.vortexEnabled;
                vortexCenter = other.vortexCenter;
                vortexStrength = other.vortexStrength;
                vortexRadius = other.vortexRadius;
                noiseEnabled = other.noiseEnabled;
                noiseStrength = other.noiseStrength;
                noiseFrequency = other.noiseFrequency;
                noiseSpeed = other.noiseSpeed;
                attractorEnabled = other.attractorEnabled;
                attractorPosition = other.attractorPosition;
                attractorStrength = other.attractorStrength;
                attractorRadius = other.attractorRadius;
                collisionEnabled = other.collisionEnabled;
                collisionPlanePoint = other.collisionPlanePoint;
                collisionPlaneNormal = other.collisionPlaneNormal;
                collisionBounciness = other.collisionBounciness;
                collisionFriction = other.collisionFriction;
                collisionKillOnHit = other.collisionKillOnHit;
                physicsCollisionEnabled = other.physicsCollisionEnabled;
                physicsCollisionBounciness = other.physicsCollisionBounciness;
                physicsCollisionFriction = other.physicsCollisionFriction;
                physicsCollisionKillOnHit = other.physicsCollisionKillOnHit;
                particleRadius = other.particleRadius;
                configDirty = true;
            }
            return *this;
        }
    };
}
namespace YAML
{
    template <>
    struct convert<Particles::FloatRange>
    {
        static Node encode(const Particles::FloatRange& range)
        {
            Node node;
            node["min"] = range.min;
            node["max"] = range.max;
            return node;
        }
        static bool decode(const Node& node, Particles::FloatRange& range)
        {
            if (!node.IsMap()) return false;
            range.min = node["min"].as<float>(0.0f);
            range.max = node["max"].as<float>(1.0f);
            return true;
        }
    };
    template <>
    struct convert<Particles::Vec2Range>
    {
        static Node encode(const Particles::Vec2Range& range)
        {
            Node node;
            node["min"] = std::vector<float>{range.min.x, range.min.y};
            node["max"] = std::vector<float>{range.max.x, range.max.y};
            return node;
        }
        static bool decode(const Node& node, Particles::Vec2Range& range)
        {
            if (!node.IsMap()) return false;
            auto minVec = node["min"].as<std::vector<float>>(std::vector<float>{1.0f, 1.0f});
            auto maxVec = node["max"].as<std::vector<float>>(std::vector<float>{1.0f, 1.0f});
            range.min = glm::vec2(minVec[0], minVec[1]);
            range.max = glm::vec2(maxVec[0], maxVec[1]);
            return true;
        }
    };
    template <>
    struct convert<Particles::ColorRange>
    {
        static Node encode(const Particles::ColorRange& range)
        {
            Node node;
            node["min"] = std::vector<float>{range.min.r, range.min.g, range.min.b, range.min.a};
            node["max"] = std::vector<float>{range.max.r, range.max.g, range.max.b, range.max.a};
            return node;
        }
        static bool decode(const Node& node, Particles::ColorRange& range)
        {
            if (!node.IsMap()) return false;
            auto minVec = node["min"].as<std::vector<float>>(std::vector<float>{1, 1, 1, 1});
            auto maxVec = node["max"].as<std::vector<float>>(std::vector<float>{1, 1, 1, 0});
            range.min = glm::vec4(minVec[0], minVec[1], minVec[2], minVec[3]);
            range.max = glm::vec4(maxVec[0], maxVec[1], maxVec[2], maxVec[3]);
            return true;
        }
    };
    template <>
    struct convert<Particles::EmitterConfig>
    {
        static Node encode(const Particles::EmitterConfig& config)
        {
            Node node;
            node["emissionRate"] = config.emissionRate;
            node["burstCount"] = config.burstCount;
            node["burstInterval"] = config.burstInterval;
            node["shape"] = static_cast<int>(config.shape);
            node["shapeSize"] = std::vector<float>{config.shapeSize.x, config.shapeSize.y, config.shapeSize.z};
            node["coneAngle"] = config.coneAngle;
            node["coneRadius"] = config.coneRadius;
            node["coneLength"] = config.coneLength;
            node["emitFrom"] = static_cast<int>(config.emitFrom);
            node["emitFromEdge"] = config.emitFromEdge; 
            node["spherizeDirection"] = config.spherizeDirection;
            node["randomizeDirection"] = config.randomizeDirection;
            node["alignToDirection"] = config.alignToDirection;
            node["direction"] = std::vector<float>{config.direction.x, config.direction.y, config.direction.z};
            node["directionRandomness"] = config.directionRandomness;
            node["lifetime"] = config.lifetime;
            node["speed"] = config.speed;
            node["rotation"] = config.rotation;
            node["angularVelocity"] = config.angularVelocity;
            node["size"] = config.size;
            node["endSize"] = config.endSize;
            node["startColor"] = config.startColor;
            node["endColor"] = config.endColor;
            node["mass"] = config.mass;
            node["drag"] = config.drag;
            node["inheritVelocityMultiplier"] = config.inheritVelocityMultiplier;
            node["maxParticles"] = config.maxParticles;
            return node;
        }
        static bool decode(const Node& node, Particles::EmitterConfig& config)
        {
            if (!node.IsMap()) return false;
            config.emissionRate = node["emissionRate"].as<float>(10.0f);
            config.burstCount = node["burstCount"].as<uint32_t>(0);
            config.burstInterval = node["burstInterval"].as<float>(0.0f);
            config.shape = static_cast<Particles::EmitterShape>(node["shape"].as<int>(4)); 
            if (node["shapeSize"])
            {
                auto sz = node["shapeSize"].as<std::vector<float>>();
                config.shapeSize = glm::vec3(sz[0], sz[1], sz[2]);
            }
            config.coneAngle = node["coneAngle"].as<float>(25.0f);
            config.coneRadius = node["coneRadius"].as<float>(1.0f);
            config.coneLength = node["coneLength"].as<float>(5.0f);
            config.emitFrom = static_cast<Particles::ShapeEmitFrom>(node["emitFrom"].as<int>(0));
            config.emitFromEdge = node["emitFromEdge"].as<bool>(false);
            config.spherizeDirection = node["spherizeDirection"].as<float>(0.0f);
            config.randomizeDirection = node["randomizeDirection"].as<float>(0.0f);
            config.alignToDirection = node["alignToDirection"].as<bool>(false);
            if (node["direction"])
            {
                auto dir = node["direction"].as<std::vector<float>>();
                config.direction = glm::vec3(dir[0], dir[1], dir[2]);
            }
            config.directionRandomness = node["directionRandomness"].as<float>(0.0f);
            if (node["lifetime"]) config.lifetime = node["lifetime"].as<Particles::FloatRange>();
            if (node["speed"]) config.speed = node["speed"].as<Particles::FloatRange>();
            if (node["rotation"]) config.rotation = node["rotation"].as<Particles::FloatRange>();
            if (node["angularVelocity"]) config.angularVelocity = node["angularVelocity"].as<Particles::FloatRange>();
            if (node["size"]) config.size = node["size"].as<Particles::Vec2Range>();
            if (node["endSize"]) config.endSize = node["endSize"].as<Particles::Vec2Range>();
            if (node["startColor"]) config.startColor = node["startColor"].as<Particles::ColorRange>();
            if (node["endColor"]) config.endColor = node["endColor"].as<Particles::ColorRange>();
            if (node["mass"]) config.mass = node["mass"].as<Particles::FloatRange>();
            if (node["drag"]) config.drag = node["drag"].as<Particles::FloatRange>();
            config.inheritVelocityMultiplier = node["inheritVelocityMultiplier"].as<float>(0.0f);
            config.maxParticles = node["maxParticles"].as<uint32_t>(1000);
            return true;
        }
    };
    template <>
    struct convert<ECS::ParticleSystemComponent>
    {
        static Node encode(const ECS::ParticleSystemComponent& ps)
        {
            Node node;
            node["emitterConfig"] = ps.emitterConfig;
            node["textureHandle"] = ps.textureHandle;
            if (ps.materialHandle.Valid())
                node["materialHandle"] = ps.materialHandle;
            node["blendMode"] = static_cast<int>(ps.blendMode);
            node["zIndex"] = ps.zIndex;
            node["billboard"] = ps.billboard;
            node["useSequenceAnimation"] = ps.useSequenceAnimation;
            if (!ps.textureFrames.empty())
            {
                Node framesNode(YAML::NodeType::Sequence);
                for (const auto& frame : ps.textureFrames)
                {
                    framesNode.push_back(frame);
                }
                node["textureFrames"] = framesNode;
            }
            node["textureAnimationMode"] = static_cast<int>(ps.textureAnimationMode);
            node["textureAnimationFPS"] = ps.textureAnimationFPS;
            node["textureAnimationCycles"] = ps.textureAnimationCycles;
            node["simulationSpace"] = static_cast<int>(ps.simulationSpace);
            node["simulationSpeed"] = ps.simulationSpeed;
            node["prewarm"] = ps.prewarm;
            node["prewarmTime"] = ps.prewarmTime;
            node["duration"] = ps.duration;
            node["loop"] = ps.loop;
            node["playOnAwake"] = ps.playOnAwake;
            node["gravityEnabled"] = ps.gravityEnabled;
            node["gravity"] = std::vector<float>{ps.gravity.x, ps.gravity.y, ps.gravity.z};
            node["dragEnabled"] = ps.dragEnabled;
            node["dragDamping"] = ps.dragDamping;
            node["vortexEnabled"] = ps.vortexEnabled;
            node["vortexCenter"] = std::vector<float>{ps.vortexCenter.x, ps.vortexCenter.y, ps.vortexCenter.z};
            node["vortexStrength"] = ps.vortexStrength;
            node["vortexRadius"] = ps.vortexRadius;
            node["noiseEnabled"] = ps.noiseEnabled;
            node["noiseStrength"] = ps.noiseStrength;
            node["noiseFrequency"] = ps.noiseFrequency;
            node["noiseSpeed"] = ps.noiseSpeed;
            node["attractorEnabled"] = ps.attractorEnabled;
            node["attractorPosition"] = std::vector<float>{ps.attractorPosition.x, ps.attractorPosition.y, ps.attractorPosition.z};
            node["attractorStrength"] = ps.attractorStrength;
            node["attractorRadius"] = ps.attractorRadius;
            node["collisionEnabled"] = ps.collisionEnabled;
            node["collisionPlanePoint"] = std::vector<float>{ps.collisionPlanePoint.x, ps.collisionPlanePoint.y, ps.collisionPlanePoint.z};
            node["collisionPlaneNormal"] = std::vector<float>{ps.collisionPlaneNormal.x, ps.collisionPlaneNormal.y, ps.collisionPlaneNormal.z};
            node["collisionBounciness"] = ps.collisionBounciness;
            node["collisionFriction"] = ps.collisionFriction;
            node["collisionKillOnHit"] = ps.collisionKillOnHit;
            // Box2D 物理碰撞
            node["physicsCollisionEnabled"] = ps.physicsCollisionEnabled;
            node["physicsCollisionBounciness"] = ps.physicsCollisionBounciness;
            node["physicsCollisionFriction"] = ps.physicsCollisionFriction;
            node["physicsCollisionKillOnHit"] = ps.physicsCollisionKillOnHit;
            node["particleRadius"] = ps.particleRadius;
            return node;
        }
        static bool decode(const Node& node, ECS::ParticleSystemComponent& ps)
        {
            if (!node.IsMap()) return false;
            if (node["emitterConfig"])
                ps.emitterConfig = node["emitterConfig"].as<Particles::EmitterConfig>();
            if (node["textureHandle"])
                ps.textureHandle = node["textureHandle"].as<AssetHandle>();
            if (node["materialHandle"])
                ps.materialHandle = node["materialHandle"].as<AssetHandle>();
            ps.blendMode = static_cast<Particles::BlendMode>(node["blendMode"].as<int>(0));
            ps.zIndex = node["zIndex"].as<int>(0);
            ps.billboard = node["billboard"].as<bool>(true);
            ps.useSequenceAnimation = node["useSequenceAnimation"].as<bool>(false);
            if (node["textureFrames"] && node["textureFrames"].IsSequence())
            {
                ps.textureFrames.clear();
                for (const auto& frameNode : node["textureFrames"])
                {
                    ps.textureFrames.push_back(frameNode.as<AssetHandle>());
                }
            }
            ps.textureAnimationMode = static_cast<ECS::ParticleSystemComponent::TextureAnimationMode>(
                node["textureAnimationMode"].as<int>(0));
            ps.textureAnimationFPS = node["textureAnimationFPS"].as<float>(30.0f);
            ps.textureAnimationCycles = node["textureAnimationCycles"].as<float>(1.0f);
            ps.simulationSpace = static_cast<ECS::ParticleSimulationSpace>(
                node["simulationSpace"].as<int>(1));
            ps.simulationSpeed = node["simulationSpeed"].as<float>(1.0f);
            ps.prewarm = node["prewarm"].as<bool>(false);
            ps.prewarmTime = node["prewarmTime"].as<float>(1.0f);
            ps.duration = node["duration"].as<float>(5.0f);
            ps.loop = node["loop"].as<bool>(true);
            ps.playOnAwake = node["playOnAwake"].as<bool>(true);
            ps.gravityEnabled = node["gravityEnabled"].as<bool>(false);
            if (node["gravity"])
            {
                auto g = node["gravity"].as<std::vector<float>>();
                ps.gravity = glm::vec3(g[0], g[1], g[2]);
            }
            ps.dragEnabled = node["dragEnabled"].as<bool>(false);
            ps.dragDamping = node["dragDamping"].as<float>(0.98f);
            ps.vortexEnabled = node["vortexEnabled"].as<bool>(false);
            if (node["vortexCenter"])
            {
                auto v = node["vortexCenter"].as<std::vector<float>>();
                ps.vortexCenter = glm::vec3(v[0], v[1], v[2]);
            }
            ps.vortexStrength = node["vortexStrength"].as<float>(10.0f);
            ps.vortexRadius = node["vortexRadius"].as<float>(100.0f);
            ps.noiseEnabled = node["noiseEnabled"].as<bool>(false);
            ps.noiseStrength = node["noiseStrength"].as<float>(5.0f);
            ps.noiseFrequency = node["noiseFrequency"].as<float>(1.0f);
            ps.noiseSpeed = node["noiseSpeed"].as<float>(1.0f);
            ps.attractorEnabled = node["attractorEnabled"].as<bool>(false);
            if (node["attractorPosition"])
            {
                auto a = node["attractorPosition"].as<std::vector<float>>();
                ps.attractorPosition = glm::vec3(a[0], a[1], a[2]);
            }
            ps.attractorStrength = node["attractorStrength"].as<float>(50.0f);
            ps.attractorRadius = node["attractorRadius"].as<float>(100.0f);
            ps.collisionEnabled = node["collisionEnabled"].as<bool>(false);
            if (node["collisionPlanePoint"])
            {
                auto pt = node["collisionPlanePoint"].as<std::vector<float>>();
                ps.collisionPlanePoint = glm::vec3(pt[0], pt[1], pt[2]);
            }
            if (node["collisionPlaneNormal"])
            {
                auto n = node["collisionPlaneNormal"].as<std::vector<float>>();
                ps.collisionPlaneNormal = glm::vec3(n[0], n[1], n[2]);
            }
            ps.collisionBounciness = node["collisionBounciness"].as<float>(0.5f);
            ps.collisionFriction = node["collisionFriction"].as<float>(0.1f);
            ps.collisionKillOnHit = node["collisionKillOnHit"].as<bool>(false);
            // Box2D 物理碰撞
            ps.physicsCollisionEnabled = node["physicsCollisionEnabled"].as<bool>(false);
            ps.physicsCollisionBounciness = node["physicsCollisionBounciness"].as<float>(0.5f);
            ps.physicsCollisionFriction = node["physicsCollisionFriction"].as<float>(0.1f);
            ps.physicsCollisionKillOnHit = node["physicsCollisionKillOnHit"].as<bool>(false);
            ps.particleRadius = node["particleRadius"].as<float>(2.0f);
            ps.configDirty = true;
            return true;
        }
    };
}
namespace CustomDrawing
{
    template <>
    struct WidgetDrawer<Particles::BlendMode>
    {
        static bool Draw(const std::string& label, Particles::BlendMode& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Alpha", "Additive", "Multiply", "Premultiplied"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Particles::BlendMode>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
    template <>
    struct WidgetDrawer<Particles::EmitterShape>
    {
        static bool Draw(const std::string& label, Particles::EmitterShape& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Point", "Circle", "Sphere", "Box", "Cone", "Edge", "Hemisphere", "Rectangle"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Particles::EmitterShape>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
    template <>
    struct WidgetDrawer<Particles::ShapeEmitFrom>
    {
        static bool Draw(const std::string& label, Particles::ShapeEmitFrom& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Volume", "Shell", "Edge"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Particles::ShapeEmitFrom>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
    template <>
    struct WidgetDrawer<ECS::ParticleSimulationSpace>
    {
        static bool Draw(const std::string& label, ECS::ParticleSimulationSpace& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Local", "World"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::ParticleSimulationSpace>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
    template <>
    struct WidgetDrawer<Particles::FloatRange>
    {
        static bool Draw(const std::string& label, Particles::FloatRange& value, const UIDrawData& callbacks)
        {
            bool changed = false;
            ImGui::PushID(label.c_str());
            ImGui::Text("%s", label.c_str());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (ImGui::DragFloat("##min", &value.min, 0.1f))
            {
                changed = true;
            }
            ImGui::SameLine();
            ImGui::Text("~");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (ImGui::DragFloat("##max", &value.max, 0.1f))
            {
                changed = true;
            }
            ImGui::PopID();
            if (changed) callbacks.onValueChanged();
            return changed;
        }
    };
    template <>
    struct WidgetDrawer<Particles::Vec2Range>
    {
        static bool Draw(const std::string& label, Particles::Vec2Range& value, const UIDrawData& callbacks)
        {
            bool changed = false;
            if (ImGui::TreeNode(label.c_str()))
            {
                if (ImGui::DragFloat2("Min", &value.min.x, 0.1f)) changed = true;
                if (ImGui::DragFloat2("Max", &value.max.x, 0.1f)) changed = true;
                ImGui::TreePop();
            }
            if (changed) callbacks.onValueChanged();
            return changed;
        }
    };
    template <>
    struct WidgetDrawer<Particles::ColorRange>
    {
        static bool Draw(const std::string& label, Particles::ColorRange& value, const UIDrawData& callbacks)
        {
            bool changed = false;
            if (ImGui::TreeNode(label.c_str()))
            {
                if (ImGui::ColorEdit4("Min", &value.min.r)) changed = true;
                if (ImGui::ColorEdit4("Max", &value.max.r)) changed = true;
                ImGui::TreePop();
            }
            if (changed) callbacks.onValueChanged();
            return changed;
        }
    };
    template <>
    struct WidgetDrawer<Particles::EmitterConfig>
    {
        static bool Draw(const std::string& label, Particles::EmitterConfig& config, const UIDrawData& callbacks)
        {
            bool changed = false;
            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::SeparatorText("Emission");
                if (ImGui::DragFloat("Rate over Time", &config.emissionRate, 1.0f, 0.0f, 1000.0f)) changed = true;
                int burstCount = static_cast<int>(config.burstCount);
                if (ImGui::DragInt("Burst Count", &burstCount, 1, 0, 1000))
                {
                    config.burstCount = static_cast<uint32_t>(burstCount);
                    changed = true;
                }
                if (ImGui::DragFloat("Burst Interval", &config.burstInterval, 0.1f, 0.0f, 10.0f)) changed = true;
                int maxParticles = static_cast<int>(config.maxParticles);
                if (ImGui::DragInt("Max Particles", &maxParticles, 10, 1, 100000))
                {
                    config.maxParticles = static_cast<uint32_t>(maxParticles);
                    changed = true;
                }
                ImGui::SeparatorText("Shape");
                const char* shapeItems[] = {"Point", "Circle", "Sphere", "Box", "Cone", "Edge", "Hemisphere", "Rectangle"};
                int shapeIndex = static_cast<int>(config.shape);
                if (ImGui::Combo("Shape", &shapeIndex, shapeItems, IM_ARRAYSIZE(shapeItems)))
                {
                    config.shape = static_cast<Particles::EmitterShape>(shapeIndex);
                    changed = true;
                }
                if (config.shape == Particles::EmitterShape::Circle || 
                    config.shape == Particles::EmitterShape::Sphere ||
                    config.shape == Particles::EmitterShape::Hemisphere)
                {
                    if (ImGui::DragFloat("Radius", &config.shapeSize.x, 0.1f, 0.0f, 100.0f)) changed = true;
                }
                else if (config.shape == Particles::EmitterShape::Cone)
                {
                    if (ImGui::DragFloat("Angle", &config.coneAngle, 1.0f, 0.0f, 90.0f)) changed = true;
                    if (ImGui::DragFloat("Radius", &config.coneRadius, 0.1f, 0.0f, 100.0f)) changed = true;
                    if (ImGui::DragFloat("Length", &config.coneLength, 0.1f, 0.0f, 100.0f)) changed = true;
                }
                else if (config.shape == Particles::EmitterShape::Box || 
                         config.shape == Particles::EmitterShape::Rectangle)
                {
                    if (ImGui::DragFloat3("Box Size", &config.shapeSize.x, 0.1f, 0.0f, 100.0f)) changed = true;
                }
                else if (config.shape == Particles::EmitterShape::Edge)
                {
                    if (ImGui::DragFloat("Edge Length", &config.shapeSize.x, 0.1f, 0.0f, 100.0f)) changed = true;
                }
                else
                {
                    if (ImGui::DragFloat3("Shape Size", &config.shapeSize.x, 0.1f, 0.0f, 100.0f)) changed = true;
                }
                const char* emitFromItems[] = {"Volume", "Shell", "Edge"};
                int emitFromIndex = static_cast<int>(config.emitFrom);
                if (ImGui::Combo("Emit From", &emitFromIndex, emitFromItems, IM_ARRAYSIZE(emitFromItems)))
                {
                    config.emitFrom = static_cast<Particles::ShapeEmitFrom>(emitFromIndex);
                    changed = true;
                }
                if (ImGui::DragFloat("Spherize Direction", &config.spherizeDirection, 0.01f, 0.0f, 1.0f)) changed = true;
                ImGui::SetItemTooltip("0=使用基础方向, 1=从形状中心向外发射");
                if (ImGui::DragFloat("Randomize Direction", &config.randomizeDirection, 0.01f, 0.0f, 1.0f)) changed = true;
                ImGui::SetItemTooltip("添加随机方向扰动");
                if (ImGui::Checkbox("Align To Direction", &config.alignToDirection)) changed = true;
                ImGui::SetItemTooltip("粒子旋转对齐到运动方向");
                ImGui::SeparatorText("Velocity");
                if (config.shape == Particles::EmitterShape::Point)
                {
                    if (ImGui::DragFloat3("Direction", &config.direction.x, 0.1f, -1.0f, 1.0f)) changed = true;
                }
                if (ImGui::DragFloat("Direction Randomness (Legacy)", &config.directionRandomness, 0.01f, 0.0f, 1.0f)) changed = true;
                ImGui::SeparatorText("Start Values");
                if (WidgetDrawer<Particles::FloatRange>::Draw("Lifetime", config.lifetime, callbacks)) changed = true;
                if (WidgetDrawer<Particles::FloatRange>::Draw("Speed", config.speed, callbacks)) changed = true;
                if (WidgetDrawer<Particles::FloatRange>::Draw("Rotation", config.rotation, callbacks)) changed = true;
                if (WidgetDrawer<Particles::FloatRange>::Draw("Angular Velocity", config.angularVelocity, callbacks)) changed = true;
                if (WidgetDrawer<Particles::Vec2Range>::Draw("Start Size", config.size, callbacks)) changed = true;
                if (WidgetDrawer<Particles::Vec2Range>::Draw("End Size", config.endSize, callbacks)) changed = true;
                if (WidgetDrawer<Particles::ColorRange>::Draw("Start Color", config.startColor, callbacks)) changed = true;
                if (WidgetDrawer<Particles::ColorRange>::Draw("End Color", config.endColor, callbacks)) changed = true;
                if (WidgetDrawer<Particles::FloatRange>::Draw("Mass", config.mass, callbacks)) changed = true;
                if (WidgetDrawer<Particles::FloatRange>::Draw("Drag", config.drag, callbacks)) changed = true;
                ImGui::SeparatorText("Inheritance");
                if (ImGui::DragFloat("Inherit Velocity", &config.inheritVelocityMultiplier, 0.01f, 0.0f, 1.0f)) changed = true;
                ImGui::TreePop();
            }
            if (changed) callbacks.onValueChanged();
            return changed;
        }
    };
    template <>
    struct WidgetDrawer<ECS::ParticleSystemComponent>
    {
        static bool Draw(const std::string& label, ECS::ParticleSystemComponent& ps, const UIDrawData& callbacks)
        {
            bool changed = false;
            ImGui::SeparatorText("Playback");
            if (ImGui::Button("Play")) { ps.Play(); }
            ImGui::SameLine();
            if (ImGui::Button("Pause")) { ps.Pause(); }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) { ps.Stop(true); }
            ImGui::SameLine();
            if (ImGui::Button("Restart")) { ps.Restart(); }
            const char* stateStr = "Unknown";
            switch (ps.playState)
            {
                case ECS::ParticlePlayState::Stopped: stateStr = "Stopped"; break;
                case ECS::ParticlePlayState::Playing: stateStr = "Playing"; break;
                case ECS::ParticlePlayState::Paused: stateStr = "Paused"; break;
            }
            ImGui::Text("State: %s | Particles: %zu | Time: %.2fs", stateStr, ps.GetParticleCount(), ps.systemTime);
            static int burstTestCount = 10;
            ImGui::SetNextItemWidth(100);
            ImGui::DragInt("##burstCount", &burstTestCount, 1, 1, 1000);
            ImGui::SameLine();
            if (ImGui::Button("Burst"))
            {
                ps.Burst(static_cast<uint32_t>(burstTestCount));
            }
            ImGui::SeparatorText("Lifecycle");
            if (ImGui::DragFloat("Duration", &ps.duration, 0.1f, 0.1f, 100.0f))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (ImGui::Checkbox("Loop", &ps.loop))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (ImGui::Checkbox("Play On Awake", &ps.playOnAwake))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            ImGui::SeparatorText("Simulation");
            const char* spaceItems[] = {"Local", "World"};
            int spaceIndex = static_cast<int>(ps.simulationSpace);
            if (ImGui::Combo("Simulation Space", &spaceIndex, spaceItems, IM_ARRAYSIZE(spaceItems)))
            {
                ps.simulationSpace = static_cast<ECS::ParticleSimulationSpace>(spaceIndex);
                callbacks.onValueChanged();
                changed = true;
            }
            if (ImGui::DragFloat("Simulation Speed", &ps.simulationSpeed, 0.1f, 0.0f, 10.0f))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (ImGui::Checkbox("Prewarm", &ps.prewarm))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.prewarm)
            {
                if (ImGui::DragFloat("Prewarm Time", &ps.prewarmTime, 0.1f, 0.0f, 10.0f))
                {
                    callbacks.onValueChanged();
                    changed = true;
                }
            }
            ImGui::SeparatorText("Force Fields");
            if (ImGui::Checkbox("Gravity", &ps.gravityEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.gravityEnabled)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(200);
                if (ImGui::DragFloat3("##gravity", &ps.gravity.x, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(Y+=down)");
            }
            if (ImGui::Checkbox("Drag", &ps.dragEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.dragEnabled)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                if (ImGui::DragFloat("##dragDamping", &ps.dragDamping, 0.01f, 0.0f, 1.0f, "%.2f"))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(0=stop, 1=no drag)");
            }
            if (ImGui::Checkbox("Vortex", &ps.vortexEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.vortexEnabled)
            {
                ImGui::Indent();
                if (ImGui::DragFloat3("Center", &ps.vortexCenter.x, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Strength", &ps.vortexStrength, 0.5f, -100.0f, 100.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Radius", &ps.vortexRadius, 1.0f, 1.0f, 500.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::Unindent();
            }
            if (ImGui::Checkbox("Noise", &ps.noiseEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.noiseEnabled)
            {
                ImGui::Indent();
                if (ImGui::DragFloat("Strength##noise", &ps.noiseStrength, 0.1f, 0.0f, 50.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Frequency", &ps.noiseFrequency, 0.01f, 0.01f, 5.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Speed", &ps.noiseSpeed, 0.1f, 0.0f, 10.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::Unindent();
            }
            if (ImGui::Checkbox("Attractor", &ps.attractorEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.attractorEnabled)
            {
                ImGui::Indent();
                if (ImGui::DragFloat3("Position##attractor", &ps.attractorPosition.x, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Strength##attractor", &ps.attractorStrength, 0.5f, -200.0f, 200.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled(ps.attractorStrength >= 0 ? "(attract)" : "(repel)");
                if (ImGui::DragFloat("Radius##attractor", &ps.attractorRadius, 1.0f, 1.0f, 500.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::Unindent();
            }
            ImGui::SeparatorText("Collision");
            if (ImGui::Checkbox("Enable Collision", &ps.collisionEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.collisionEnabled)
            {
                const char* presetItems[] = {"Custom", "Floor (bottom)", "Ceiling (top)", "Left Wall", "Right Wall"};
                static int presetIndex = 0;
                if (ImGui::Combo("Plane Preset", &presetIndex, presetItems, IM_ARRAYSIZE(presetItems)))
                {
                    switch (presetIndex)
                    {
                        case 1: 
                            ps.collisionPlanePoint = glm::vec3(0, 500, 0);
                            ps.collisionPlaneNormal = glm::vec3(0, -1, 0);
                            break;
                        case 2: 
                            ps.collisionPlanePoint = glm::vec3(0, 0, 0);
                            ps.collisionPlaneNormal = glm::vec3(0, 1, 0);
                            break;
                        case 3: 
                            ps.collisionPlanePoint = glm::vec3(0, 0, 0);
                            ps.collisionPlaneNormal = glm::vec3(1, 0, 0);
                            break;
                        case 4: 
                            ps.collisionPlanePoint = glm::vec3(500, 0, 0);
                            ps.collisionPlaneNormal = glm::vec3(-1, 0, 0);
                            break;
                    }
                    if (presetIndex > 0)
                    {
                        ps.configDirty = true;
                        callbacks.onValueChanged();
                        changed = true;
                        presetIndex = 0; 
                    }
                }
                ImGui::TextDisabled("Coord: Y+ down, X+ right");
                if (ImGui::DragFloat3("Plane Point", &ps.collisionPlanePoint.x, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat3("Plane Normal", &ps.collisionPlaneNormal.x, 0.01f, -1.0f, 1.0f))
                {
                    float len = glm::length(ps.collisionPlaneNormal);
                    if (len > 0.001f)
                        ps.collisionPlaneNormal /= len;
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Bounciness", &ps.collisionBounciness, 0.01f, 0.0f, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Friction", &ps.collisionFriction, 0.01f, 0.0f, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::Checkbox("Kill On Hit", &ps.collisionKillOnHit))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
            }
            ImGui::SeparatorText("Physics Collision (Box2D)");
            if (ImGui::Checkbox("Enable Physics Collision", &ps.physicsCollisionEnabled))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            ImGui::SetItemTooltip("Enable collision with Box2D physics bodies (Rigidbody + Collider)");
            if (ps.physicsCollisionEnabled)
            {
                if (ImGui::DragFloat("Particle Radius", &ps.particleRadius, 0.1f, 0.1f, 50.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::SetItemTooltip("Collision detection radius for each particle (pixels)");
                if (ImGui::DragFloat("Bounciness##physics", &ps.physicsCollisionBounciness, 0.01f, 0.0f, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::DragFloat("Friction##physics", &ps.physicsCollisionFriction, 0.01f, 0.0f, 1.0f))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ImGui::Checkbox("Kill On Hit##physics", &ps.physicsCollisionKillOnHit))
                {
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
            }
            ImGui::SeparatorText("Rendering");
            if (WidgetDrawer<AssetHandle>::Draw("Texture", ps.textureHandle, callbacks))
            {
                changed = true;
            }
            if (WidgetDrawer<AssetHandle>::Draw("Material", ps.materialHandle, callbacks))
            {
                changed = true;
            }
            if (WidgetDrawer<Particles::BlendMode>::Draw("Blend Mode", ps.blendMode, callbacks))
            {
                changed = true;
            }
            if (ImGui::DragInt("Z Index", &ps.zIndex, 1, -1000, 1000))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (ImGui::Checkbox("Billboard", &ps.billboard))
            {
                callbacks.onValueChanged();
                changed = true;
            }
            if (WidgetDrawer<Particles::EmitterConfig>::Draw("Emitter Config", ps.emitterConfig, callbacks))
            {
                ps.configDirty = true;
                changed = true;
            }
            ImGui::SeparatorText("Sequence Animation");
            if (ImGui::Checkbox("Use Sequence Animation", &ps.useSequenceAnimation))
            {
                ps.configDirty = true;
                callbacks.onValueChanged();
                changed = true;
            }
            if (ps.useSequenceAnimation)
            {
                const char* modeItems[] = {"Over Lifetime", "FPS (Looping)"};
                int modeIndex = static_cast<int>(ps.textureAnimationMode);
                if (ImGui::Combo("Animation Mode", &modeIndex, modeItems, IM_ARRAYSIZE(modeItems)))
                {
                    ps.textureAnimationMode = static_cast<ECS::ParticleSystemComponent::TextureAnimationMode>(modeIndex);
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (ps.textureAnimationMode == ECS::ParticleSystemComponent::TextureAnimationMode::FPS)
                {
                    if (ImGui::DragFloat("Animation FPS", &ps.textureAnimationFPS, 1.0f, 1.0f, 120.0f))
                    {
                        ps.configDirty = true;
                        callbacks.onValueChanged();
                        changed = true;
                    }
                }
                else
                {
                    if (ImGui::DragFloat("Cycles", &ps.textureAnimationCycles, 0.1f, 0.1f, 10.0f))
                    {
                        ps.configDirty = true;
                        callbacks.onValueChanged();
                        changed = true;
                    }
                    ImGui::SetItemTooltip("Number of animation cycles over lifetime");
                }
                ImGui::Text("Frames: %d", static_cast<int>(ps.textureFrames.size()));
                if (ImGui::Button("Add Frame"))
                {
                    ps.textureFrames.push_back(AssetHandle(AssetType::Texture));
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear All"))
                {
                    ps.textureFrames.clear();
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                int frameToRemove = -1;
                int dragSourceIdx = -1;
                int dragTargetIdx = -1;
                for (size_t i = 0; i < ps.textureFrames.size(); ++i)
                {
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::Button("=", ImVec2(20, 0));
                    ImGui::SetItemTooltip("Drag to reorder");
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                    {
                        int srcIdx = static_cast<int>(i);
                        ImGui::SetDragDropPayload("PARTICLE_FRAME", &srcIdx, sizeof(int));
                        ImGui::Text("Frame %d", static_cast<int>(i));
                        ImGui::EndDragDropSource();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PARTICLE_FRAME"))
                        {
                            dragSourceIdx = *static_cast<const int*>(payload->Data);
                            dragTargetIdx = static_cast<int>(i);
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::SameLine();
                    ImGui::Text("[%d]", static_cast<int>(i));
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::SmallButton("X"))
                    {
                        frameToRemove = static_cast<int>(i);
                    }
                    ImGui::PopStyleColor(2);
                    ImGui::SetItemTooltip("Remove frame");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-1);
                    std::string frameLabel = "##frame" + std::to_string(i);
                    if (WidgetDrawer<AssetHandle>::Draw(frameLabel, ps.textureFrames[i], callbacks))
                    {
                        ps.configDirty = true;
                        changed = true;
                    }
                    ImGui::PopID();
                }
                if (dragSourceIdx >= 0 && dragTargetIdx >= 0 && dragSourceIdx != dragTargetIdx)
                {
                    AssetHandle temp = ps.textureFrames[dragSourceIdx];
                    ps.textureFrames.erase(ps.textureFrames.begin() + dragSourceIdx);
                    ps.textureFrames.insert(ps.textureFrames.begin() + dragTargetIdx, temp);
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
                if (frameToRemove >= 0)
                {
                    ps.textureFrames.erase(ps.textureFrames.begin() + frameToRemove);
                    ps.configDirty = true;
                    callbacks.onValueChanged();
                    changed = true;
                }
            }
            return changed;
        }
    };
}
REGISTRY
{
    Registry_<ECS::ParticleSystemComponent>("ParticleSystemComponent")
        .SetCustomDrawUI();
}
#endif 

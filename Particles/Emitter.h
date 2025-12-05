#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H
#include "../Data/ParticleData.h"
#include <random>
#include <functional>
#include <glm/gtc/constants.hpp>
namespace Particles
{
    struct EmitterConfig
    {
        float emissionRate = 10.0f;        
        uint32_t burstCount = 0;           
        float burstInterval = 0.0f;        
        EmitterShape shape = EmitterShape::Cone;  
        glm::vec3 shapeSize{1.0f, 1.0f, 1.0f};    
        float coneAngle = 25.0f;           
        float coneRadius = 1.0f;           
        float coneLength = 5.0f;           
        ShapeEmitFrom emitFrom = ShapeEmitFrom::Volume; 
        float spherizeDirection = 0.0f;    
        float randomizeDirection = 0.0f;   
        bool alignToDirection = false;     
        glm::vec3 direction{0.0f, 1.0f, 0.0f}; 
        float directionRandomness = 0.0f;      
        FloatRange lifetime{1.0f, 2.0f};       
        FloatRange speed{5.0f, 10.0f};         
        FloatRange rotation{0.0f, 0.0f};       
        FloatRange angularVelocity{0.0f, 0.0f};
        Vec2Range size{{1.0f, 1.0f}, {1.0f, 1.0f}};   
        Vec2Range endSize{{1.0f, 1.0f}, {1.0f, 1.0f}};
        ColorRange startColor{{1.0f, 1.0f, 1.0f, 1.0f}}; 
        ColorRange endColor{{1.0f, 1.0f, 1.0f, 0.0f}};   
        FloatRange mass{1.0f, 1.0f};           
        FloatRange drag{0.0f, 0.0f};           
        float inheritVelocityMultiplier = 0.0f;  
        uint32_t maxParticles = 1000;
        bool emitFromEdge = false;         
    };
    class Emitter
    {
    public:
        explicit Emitter(const EmitterConfig& config = {})
            : m_config(config)
            , m_rng(std::random_device{}())
        {
        }
        void Update(ParticlePool& pool, float deltaTime, 
                   const glm::vec3& worldPosition = glm::vec3(0.0f),
                   const glm::vec3& worldVelocity = glm::vec3(0.0f),
                   const glm::vec2& scale = glm::vec2(1.0f))
        {
            if (!m_enabled) return;
            m_emitterPosition = worldPosition;
            m_emitterVelocity = worldVelocity;
            m_emitterScale = scale;
            if (m_config.emissionRate > 0.0f)
            {
                m_emissionAccumulator += m_config.emissionRate * deltaTime;
                while (m_emissionAccumulator >= 1.0f && 
                       pool.Size() < m_config.maxParticles)
                {
                    EmitParticle(pool);
                    m_emissionAccumulator -= 1.0f;
                }
            }
            if (m_config.burstCount > 0 && m_config.burstInterval > 0.0f)
            {
                m_burstTimer += deltaTime;
                if (m_burstTimer >= m_config.burstInterval)
                {
                    Burst(pool, m_config.burstCount);
                    m_burstTimer = 0.0f;
                }
            }
        }
        void Burst(ParticlePool& pool, uint32_t count)
        {
            for (uint32_t i = 0; i < count && pool.Size() < m_config.maxParticles; ++i)
            {
                EmitParticle(pool);
            }
        }
        void EmitParticle(ParticlePool& pool)
        {
            if (pool.Size() >= m_config.maxParticles) return;
            ParticleData& p = pool.Emit();
            SpawnResult spawn = GetSpawnPositionAndDirection();
            p.position = m_emitterPosition + spawn.position;
            float speed = RandomRange(m_config.speed);
            p.velocity = spawn.direction * speed;
            if (m_config.inheritVelocityMultiplier > 0.0f)
            {
                p.velocity += m_emitterVelocity * m_config.inheritVelocityMultiplier;
            }
            p.lifetime = RandomRange(m_config.lifetime);
            p.age = 0.0f;
            if (m_config.alignToDirection && glm::length(spawn.direction) > 0.001f)
            {
                p.rotation = std::atan2(spawn.direction.y, spawn.direction.x);
            }
            else
            {
                p.rotation = RandomRange(m_config.rotation);
            }
            p.angularVelocity = RandomRange(m_config.angularVelocity);
            p.startSize = RandomRange(m_config.size) * m_emitterScale;
            p.endSize = RandomRange(m_config.endSize) * m_emitterScale;
            p.size = p.startSize;
            p.startColor = RandomRange(m_config.startColor);
            p.endColor = RandomRange(m_config.endColor);
            p.color = p.startColor;
            p.mass = RandomRange(m_config.mass);
            p.drag = RandomRange(m_config.drag);
            if (m_onParticleSpawn)
            {
                m_onParticleSpawn(p);
            }
        }
        EmitterConfig& GetConfig() { return m_config; }
        const EmitterConfig& GetConfig() const { return m_config; }
        void SetConfig(const EmitterConfig& config) { m_config = config; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }
        void Reset()
        {
            m_emissionAccumulator = 0.0f;
            m_burstTimer = 0.0f;
        }
        void SetOnParticleSpawn(std::function<void(ParticleData&)> callback)
        {
            m_onParticleSpawn = std::move(callback);
        }
    private:
        struct SpawnResult
        {
            glm::vec3 position;
            glm::vec3 direction;
        };
        SpawnResult GetSpawnPositionAndDirection()
        {
            SpawnResult result;
            result.position = glm::vec3(0.0f);
            result.direction = glm::vec3(0.0f, -1.0f, 0.0f); 
            bool fromShell = (m_config.emitFrom == ShapeEmitFrom::Shell) || m_config.emitFromEdge;
            switch (m_config.shape)
            {
                case EmitterShape::Point:
                {
                    result.position = glm::vec3(0.0f);
                    result.direction = glm::normalize(m_config.direction);
                    break;
                }
                case EmitterShape::Circle:
                {
                    float angle = RandomFloat() * glm::two_pi<float>();
                    float radius = fromShell ? 
                                   m_config.shapeSize.x : 
                                   std::sqrt(RandomFloat()) * m_config.shapeSize.x;
                    float cosA = std::cos(angle);
                    float sinA = std::sin(angle);
                    result.position = glm::vec3(cosA * radius, sinA * radius, 0.0f);
                    glm::vec3 radialDir = radius > 0.001f ? 
                        glm::normalize(glm::vec3(cosA, sinA, 0.0f)) : 
                        glm::vec3(0.0f, -1.0f, 0.0f);
                    result.direction = glm::mix(
                        glm::normalize(m_config.direction),
                        radialDir,
                        m_config.spherizeDirection
                    );
                    break;
                }
                case EmitterShape::Sphere:
                case EmitterShape::Hemisphere:
                {
                    float u = RandomFloat();
                    float v = RandomFloat();
                    float theta = u * glm::two_pi<float>();
                    float phi = (m_config.shape == EmitterShape::Hemisphere) ?
                        std::acos(v) : 
                        std::acos(2.0f * v - 1.0f); 
                    float radius = fromShell ? 
                                   m_config.shapeSize.x : 
                                   std::cbrt(RandomFloat()) * m_config.shapeSize.x;
                    glm::vec3 sphereDir(
                        std::sin(phi) * std::cos(theta),
                        std::sin(phi) * std::sin(theta),
                        std::cos(phi)
                    );
                    result.position = sphereDir * radius;
                    result.direction = glm::mix(
                        glm::normalize(m_config.direction),
                        sphereDir,
                        m_config.spherizeDirection > 0.0f ? m_config.spherizeDirection : 1.0f
                    );
                    break;
                }
                case EmitterShape::Box:
                case EmitterShape::Rectangle:
                {
                    glm::vec3 pos;
                    glm::vec3 normal(0.0f, -1.0f, 0.0f);
                    if (m_config.shape == EmitterShape::Rectangle || m_config.shapeSize.z < 0.001f)
                    {
                        pos = glm::vec3(
                            (RandomFloat() - 0.5f) * m_config.shapeSize.x,
                            (RandomFloat() - 0.5f) * m_config.shapeSize.y,
                            0.0f
                        );
                        normal = glm::vec3(0.0f, -1.0f, 0.0f);
                    }
                    else if (fromShell)
                    {
                        int face = static_cast<int>(RandomFloat() * 6);
                        switch (face)
                        {
                            case 0: pos = {-0.5f, RandomFloat() - 0.5f, RandomFloat() - 0.5f}; normal = {-1,0,0}; break;
                            case 1: pos = {0.5f, RandomFloat() - 0.5f, RandomFloat() - 0.5f}; normal = {1,0,0}; break;
                            case 2: pos = {RandomFloat() - 0.5f, -0.5f, RandomFloat() - 0.5f}; normal = {0,-1,0}; break;
                            case 3: pos = {RandomFloat() - 0.5f, 0.5f, RandomFloat() - 0.5f}; normal = {0,1,0}; break;
                            case 4: pos = {RandomFloat() - 0.5f, RandomFloat() - 0.5f, -0.5f}; normal = {0,0,-1}; break;
                            case 5: pos = {RandomFloat() - 0.5f, RandomFloat() - 0.5f, 0.5f}; normal = {0,0,1}; break;
                            default: break;
                        }
                        pos *= m_config.shapeSize;
                    }
                    else
                    {
                        pos = glm::vec3(
                            (RandomFloat() - 0.5f) * m_config.shapeSize.x,
                            (RandomFloat() - 0.5f) * m_config.shapeSize.y,
                            (RandomFloat() - 0.5f) * m_config.shapeSize.z
                        );
                    }
                    result.position = pos;
                    result.direction = glm::mix(
                        glm::normalize(m_config.direction),
                        normal,
                        m_config.spherizeDirection
                    );
                    break;
                }
                case EmitterShape::Cone:
                {
                    float angle = RandomFloat() * glm::two_pi<float>();
                    float coneAngleRad = glm::radians(m_config.coneAngle);
                    float t = fromShell ? 1.0f : RandomFloat(); 
                    float currentRadius = t * m_config.coneRadius;
                    float cosA = std::cos(angle);
                    float sinA = std::sin(angle);
                    result.position = glm::vec3(
                        cosA * currentRadius,
                        t * m_config.coneLength,  
                        sinA * currentRadius
                    );
                    glm::vec3 outward(cosA, 0.0f, sinA);
                    glm::vec3 forward(0.0f, 1.0f, 0.0f);
                    float outwardAmount = std::sin(coneAngleRad);
                    float forwardAmount = std::cos(coneAngleRad);
                    result.direction = glm::normalize(outward * outwardAmount + forward * forwardAmount);
                    break;
                }
                case EmitterShape::Edge:
                {
                    float t = RandomFloat();
                    result.position = glm::vec3(
                        (t - 0.5f) * m_config.shapeSize.x,
                        0.0f,
                        0.0f
                    );
                    result.direction = glm::normalize(m_config.direction);
                    break;
                }
                default:
                    result.direction = glm::normalize(m_config.direction);
                    break;
            }
            if (m_config.randomizeDirection > 0.0f)
            {
                glm::vec3 randomDir = glm::normalize(glm::vec3(
                    RandomFloat() * 2.0f - 1.0f,
                    RandomFloat() * 2.0f - 1.0f,
                    RandomFloat() * 2.0f - 1.0f
                ));
                result.direction = glm::normalize(glm::mix(
                    result.direction,
                    randomDir,
                    m_config.randomizeDirection
                ));
            }
            if (m_config.directionRandomness > 0.0f)
            {
                glm::vec3 randomDir = glm::normalize(glm::vec3(
                    RandomFloat() * 2.0f - 1.0f,
                    RandomFloat() * 2.0f - 1.0f,
                    RandomFloat() * 2.0f - 1.0f
                ));
                result.direction = glm::normalize(glm::mix(
                    result.direction,
                    randomDir,
                    m_config.directionRandomness
                ));
            }
            result.position.x *= m_emitterScale.x;
            result.position.y *= m_emitterScale.y;
            return result;
        }
        float RandomFloat()
        {
            return m_distribution(m_rng);
        }
        float RandomRange(const FloatRange& range)
        {
            return range.Lerp(RandomFloat());
        }
        glm::vec2 RandomRange(const Vec2Range& range)
        {
            return range.Lerp(RandomFloat());
        }
        glm::vec3 RandomRange(const Vec3Range& range)
        {
            return range.Lerp(RandomFloat());
        }
        glm::vec4 RandomRange(const ColorRange& range)
        {
            return range.Lerp(RandomFloat());
        }
    private:
        EmitterConfig m_config;
        bool m_enabled = true;
        float m_emissionAccumulator = 0.0f;
        float m_burstTimer = 0.0f;
        glm::vec3 m_emitterPosition{0.0f};
        glm::vec3 m_emitterVelocity{0.0f};
        glm::vec2 m_emitterScale{1.0f};
        std::mt19937 m_rng;
        std::uniform_real_distribution<float> m_distribution{0.0f, 1.0f};
        std::function<void(ParticleData&)> m_onParticleSpawn;
    };
} 
#endif 

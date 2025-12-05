#ifndef PARTICLE_AFFECTOR_H
#define PARTICLE_AFFECTOR_H
#include "../Data/ParticleData.h"
#include <memory>
#include <vector>
#include <functional>
#include <cmath>
#include <random>
namespace Particles
{
    class IAffector
    {
    public:
        virtual ~IAffector() = default;
        virtual void Update(ParticleData& particle, float deltaTime) = 0;
        virtual void UpdateBatch(std::vector<ParticleData>& particles, float deltaTime)
        {
            for (auto& p : particles)
            {
                if (!p.IsDead())
                {
                    Update(p, deltaTime);
                }
            }
        }
        bool enabled = true;
        float weight = 1.0f;
    };
    using AffectorPtr = std::shared_ptr<IAffector>;
    class LifetimeAffector : public IAffector
    {
    public:
        void Update(ParticleData& particle, float deltaTime) override
        {
            particle.age += deltaTime;
        }
    };
    class GravityAffector : public IAffector
    {
    public:
        glm::vec3 gravity{0.0f, -9.81f, 0.0f};
        explicit GravityAffector(const glm::vec3& g = {0.0f, -9.81f, 0.0f})
            : gravity(g) {}
        void Update(ParticleData& particle, float deltaTime) override
        {
            particle.velocity += gravity * deltaTime * weight;
        }
    };
    class DragAffector : public IAffector
    {
    public:
        float dragCoefficient = 0.1f;
        explicit DragAffector(float drag = 0.1f) : dragCoefficient(drag) {}
        void Update(ParticleData& particle, float deltaTime) override
        {
            float speed = glm::length(particle.velocity);
            if (speed > 0.001f)
            {
                float dragForce = dragCoefficient * speed * speed;
                glm::vec3 dragDir = -glm::normalize(particle.velocity);
                particle.velocity += dragDir * dragForce * deltaTime * weight;
            }
        }
    };
    class LinearDragAffector : public IAffector
    {
    public:
        float dampingFactor = 0.98f;
        explicit LinearDragAffector(float damping = 0.98f) : dampingFactor(damping) {}
        void Update(ParticleData& particle, float deltaTime) override
        {
            float factor = std::pow(dampingFactor, deltaTime * 60.0f * weight);
            particle.velocity *= factor;
        }
    };
    class VelocityLimitAffector : public IAffector
    {
    public:
        float maxSpeed = 100.0f;
        float minSpeed = 0.0f;
        void Update(ParticleData& particle, float deltaTime) override
        {
            float speed = glm::length(particle.velocity);
            if (speed > maxSpeed && speed > 0.001f)
            {
                particle.velocity = glm::normalize(particle.velocity) * maxSpeed;
            }
            else if (speed < minSpeed && speed > 0.001f)
            {
                particle.velocity = glm::normalize(particle.velocity) * minSpeed;
            }
        }
    };
    class VelocityAffector : public IAffector
    {
    public:
        void Update(ParticleData& particle, float deltaTime) override
        {
            particle.position += particle.velocity * deltaTime;
        }
    };
    class RotationAffector : public IAffector
    {
    public:
        void Update(ParticleData& particle, float deltaTime) override
        {
            particle.rotation += particle.angularVelocity * deltaTime;
        }
    };
    class AttractorAffector : public IAffector
    {
    public:
        glm::vec3 position{0.0f};    
        float strength = 10.0f;       
        float radius = 10.0f;         
        float falloff = 2.0f;         
        void Update(ParticleData& particle, float deltaTime) override
        {
            glm::vec3 toAttractor = position - particle.position;
            float distance = glm::length(toAttractor);
            if (distance < 0.001f || distance > radius)
                return;
            float normalizedDist = distance / radius;
            float force = strength * std::pow(1.0f - normalizedDist, falloff);
            particle.velocity += glm::normalize(toAttractor) * force * deltaTime * weight;
        }
    };
    class VortexAffector : public IAffector
    {
    public:
        glm::vec3 center{0.0f};
        glm::vec3 axis{0.0f, 0.0f, 1.0f};  
        float strength = 5.0f;
        float radius = 10.0f;
        void Update(ParticleData& particle, float deltaTime) override
        {
            glm::vec3 toParticle = particle.position - center;
            float distance = glm::length(toParticle);
            if (distance < 0.001f || distance > radius)
                return;
            float normalizedDist = 1.0f - (distance / radius);
            glm::vec3 tangent = glm::cross(axis, glm::normalize(toParticle));
            particle.velocity += tangent * strength * normalizedDist * deltaTime * weight;
        }
    };
    class NoiseForceAffector : public IAffector
    {
    public:
        float strength = 5.0f;
        float frequency = 1.0f;
        float scrollSpeed = 1.0f;
        void Update(ParticleData& particle, float deltaTime) override
        {
            float time = particle.age * scrollSpeed;
            glm::vec3 noisePos = particle.position * frequency + glm::vec3(time);
            glm::vec3 force;
            force.x = std::sin(noisePos.y * 2.1f + noisePos.z * 1.3f) * 
                      std::cos(noisePos.x * 1.7f + time);
            force.y = std::sin(noisePos.z * 2.3f + noisePos.x * 1.5f) * 
                      std::cos(noisePos.y * 1.9f + time * 1.1f);
            force.z = std::sin(noisePos.x * 2.5f + noisePos.y * 1.7f) * 
                      std::cos(noisePos.z * 2.1f + time * 0.9f);
            particle.velocity += force * strength * deltaTime * weight;
        }
    };
    class ColorOverLifetimeAffector : public IAffector
    {
    public:
        void Update(ParticleData& particle, float deltaTime) override
        {
            float t = particle.GetNormalizedAge();
            particle.color = glm::mix(particle.startColor, particle.endColor, t);
        }
    };
    class GradientColorAffector : public IAffector
    {
    public:
        struct ColorStop
        {
            float position;  
            glm::vec4 color;
        };
        std::vector<ColorStop> gradient;
        GradientColorAffector()
        {
            gradient.push_back({0.0f, glm::vec4(1.0f)});
            gradient.push_back({1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)});
        }
        void Update(ParticleData& particle, float deltaTime) override
        {
            if (gradient.empty()) return;
            float t = particle.GetNormalizedAge();
            size_t i = 0;
            while (i < gradient.size() - 1 && gradient[i + 1].position < t)
            {
                ++i;
            }
            if (i >= gradient.size() - 1)
            {
                particle.color = gradient.back().color;
            }
            else
            {
                float localT = (t - gradient[i].position) / 
                              (gradient[i + 1].position - gradient[i].position);
                particle.color = glm::mix(gradient[i].color, gradient[i + 1].color, localT);
            }
        }
    };
    class SizeOverLifetimeAffector : public IAffector
    {
    public:
        void Update(ParticleData& particle, float deltaTime) override
        {
            float t = particle.GetNormalizedAge();
            particle.size = glm::mix(particle.startSize, particle.endSize, t);
        }
    };
    class SizeCurveAffector : public IAffector
    {
    public:
        std::function<float(float)> curve = [](float t) { return 1.0f - t; };
        void Update(ParticleData& particle, float deltaTime) override
        {
            float t = particle.GetNormalizedAge();
            float scale = curve(t);
            particle.size = particle.startSize * scale;
        }
    };
    class AlphaFadeAffector : public IAffector
    {
    public:
        float fadeInTime = 0.1f;   
        float fadeOutTime = 0.3f;  
        void Update(ParticleData& particle, float deltaTime) override
        {
            float t = particle.GetNormalizedAge();
            float alpha = 1.0f;
            if (t < fadeInTime)
            {
                alpha = t / fadeInTime;
            }
            else if (t > (1.0f - fadeOutTime))
            {
                alpha = (1.0f - t) / fadeOutTime;
            }
            particle.color.a = particle.startColor.a * alpha;
        }
    };
    enum class TextureAnimationMode
    {
        OverLifetime,  
        FPS            
    };
    class SequenceFrameAnimationAffector : public IAffector
    {
    public:
        uint32_t frameCount = 1;      
        float fps = 30.0f;            
        float cycles = 1.0f;          
        TextureAnimationMode mode = TextureAnimationMode::OverLifetime;
        void Update(ParticleData& particle, float deltaTime) override
        {
            if (frameCount <= 1)
            {
                particle.textureIndex = 0;
                return;
            }
            uint32_t frame = 0;
            switch (mode)
            {
                case TextureAnimationMode::OverLifetime:
                {
                    float t = particle.GetNormalizedAge() * cycles;
                    t = t - std::floor(t); 
                    frame = static_cast<uint32_t>(t * frameCount);
                    frame = std::min(frame, frameCount - 1);
                    break;
                }
                case TextureAnimationMode::FPS:
                {
                    float animTime = particle.age * fps;
                    frame = static_cast<uint32_t>(animTime) % frameCount;
                    break;
                }
            }
            particle.textureIndex = frame;
        }
    };
    class PlaneCollisionAffector : public IAffector
    {
    public:
        glm::vec3 planePoint{0.0f};
        glm::vec3 planeNormal{0.0f, 1.0f, 0.0f};
        float bounciness = 0.5f;
        float friction = 0.1f;
        bool killOnCollision = false;
        void Update(ParticleData& particle, float deltaTime) override
        {
            float distance = glm::dot(particle.position - planePoint, planeNormal);
            if (distance < 0.0f)
            {
                if (killOnCollision)
                {
                    particle.age = particle.lifetime;
                    return;
                }
                particle.position -= planeNormal * distance;
                float normalVelocity = glm::dot(particle.velocity, planeNormal);
                if (normalVelocity < 0.0f)
                {
                    glm::vec3 normalComponent = planeNormal * normalVelocity;
                    glm::vec3 tangentComponent = particle.velocity - normalComponent;
                    particle.velocity = tangentComponent * (1.0f - friction) - 
                                        normalComponent * bounciness;
                }
            }
        }
    };
    class AffectorChain
    {
    public:
        template<typename T, typename... Args>
        std::shared_ptr<T> Add(Args&&... args)
        {
            auto affector = std::make_shared<T>(std::forward<Args>(args)...);
            m_affectors.push_back(affector);
            return affector;
        }
        void Add(AffectorPtr affector)
        {
            m_affectors.push_back(std::move(affector));
        }
        void Remove(const AffectorPtr& affector)
        {
            m_affectors.erase(
                std::remove(m_affectors.begin(), m_affectors.end(), affector),
                m_affectors.end()
            );
        }
        void Clear()
        {
            m_affectors.clear();
        }
        void Update(ParticleData& particle, float deltaTime)
        {
            for (auto& affector : m_affectors)
            {
                if (affector && affector->enabled)
                {
                    affector->Update(particle, deltaTime);
                }
            }
        }
        void UpdateBatch(std::vector<ParticleData>& particles, float deltaTime)
        {
            for (auto& affector : m_affectors)
            {
                if (affector && affector->enabled)
                {
                    affector->UpdateBatch(particles, deltaTime);
                }
            }
        }
        [[nodiscard]] const std::vector<AffectorPtr>& GetAffectors() const 
        { 
            return m_affectors; 
        }
        [[nodiscard]] size_t Size() const { return m_affectors.size(); }
    private:
        std::vector<AffectorPtr> m_affectors;
    };
} 
#endif 

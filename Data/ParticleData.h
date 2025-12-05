#ifndef PARTICLE_DATA_H
#define PARTICLE_DATA_H
#include <vector>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
namespace Particles
{
    struct alignas(16) ParticleData
    {
        glm::vec3 position{0.0f};      
        float age{0.0f};               
        glm::vec3 velocity{0.0f};      
        float lifetime{1.0f};          
        glm::vec4 color{1.0f};         
        glm::vec2 size{1.0f};          
        float rotation{0.0f};          
        float angularVelocity{0.0f};   
        glm::vec4 startColor{1.0f};    
        glm::vec4 endColor{1.0f};      
        glm::vec2 startSize{1.0f};     
        glm::vec2 endSize{1.0f};       
        uint32_t textureIndex{0};      
        uint32_t flags{0};             
        float mass{1.0f};              
        float drag{0.0f};              
        [[nodiscard]] float GetNormalizedAge() const
        {
            return lifetime > 0.0f ? age / lifetime : 1.0f;
        }
        [[nodiscard]] bool IsDead() const
        {
            return age >= lifetime;
        }
    };
    struct alignas(16) ParticleGPUData
    {
        glm::vec4 positionAndRotation; 
        glm::vec4 color;               
        glm::vec4 sizeAndUV;           
        glm::vec4 uvScaleAndIndex;     
    };
    class ParticlePool
    {
    public:
        explicit ParticlePool(size_t initialCapacity = 1024)
        {
            Reserve(initialCapacity);
        }
        void Reserve(size_t capacity)
        {
            m_particles.reserve(capacity);
            m_gpuData.reserve(capacity);
        }
        ParticleData& Emit()
        {
            m_particles.emplace_back();
            m_gpuData.emplace_back();
            return m_particles.back();
        }
        size_t EmitBatch(size_t count)
        {
            size_t startIndex = m_particles.size();
            m_particles.resize(m_particles.size() + count);
            m_gpuData.resize(m_gpuData.size() + count);
            return startIndex;
        }
        size_t RemoveDeadParticles()
        {
            size_t removed = 0;
            size_t i = 0;
            while (i < m_particles.size())
            {
                if (m_particles[i].IsDead())
                {
                    if (i != m_particles.size() - 1)
                    {
                        std::swap(m_particles[i], m_particles.back());
                        std::swap(m_gpuData[i], m_gpuData.back());
                    }
                    m_particles.pop_back();
                    m_gpuData.pop_back();
                    ++removed;
                }
                else
                {
                    ++i;
                }
            }
            return removed;
        }
        void Clear()
        {
            m_particles.clear();
            m_gpuData.clear();
        }
        void SyncToGPU()
        {
            for (size_t i = 0; i < m_particles.size(); ++i)
            {
                const auto& p = m_particles[i];
                auto& gpu = m_gpuData[i];
                gpu.positionAndRotation = glm::vec4(p.position, p.rotation);
                gpu.color = p.color;
                gpu.sizeAndUV = glm::vec4(p.size, 0.0f, 0.0f);
                gpu.uvScaleAndIndex = glm::vec4(1.0f, 1.0f, static_cast<float>(p.textureIndex), 0.0f);
            }
        }
        [[nodiscard]] size_t Size() const { return m_particles.size(); }
        [[nodiscard]] bool Empty() const { return m_particles.empty(); }
        [[nodiscard]] size_t Capacity() const { return m_particles.capacity(); }
        [[nodiscard]] ParticleData& operator[](size_t index) { return m_particles[index]; }
        [[nodiscard]] const ParticleData& operator[](size_t index) const { return m_particles[index]; }
        [[nodiscard]] std::vector<ParticleData>& GetParticles() { return m_particles; }
        [[nodiscard]] const std::vector<ParticleData>& GetParticles() const { return m_particles; }
        [[nodiscard]] std::vector<ParticleGPUData>& GetGPUData() { return m_gpuData; }
        [[nodiscard]] const std::vector<ParticleGPUData>& GetGPUData() const { return m_gpuData; }
        [[nodiscard]] const void* GetGPUDataPtr() const
        {
            return m_gpuData.empty() ? nullptr : m_gpuData.data();
        }
        [[nodiscard]] size_t GetGPUDataSize() const
        {
            return m_gpuData.size() * sizeof(ParticleGPUData);
        }
        auto begin() { return m_particles.begin(); }
        auto end() { return m_particles.end(); }
        auto begin() const { return m_particles.begin(); }
        auto end() const { return m_particles.end(); }
    private:
        std::vector<ParticleData> m_particles;    
        std::vector<ParticleGPUData> m_gpuData;   
    };
    enum class EmitterShape : uint8_t
    {
        Point,      
        Circle,     
        Sphere,     
        Box,        
        Cone,       
        Edge,       
        Hemisphere, 
        Rectangle,  
    };
    enum class ShapeEmitFrom : uint8_t
    {
        Volume,     
        Shell,      
        Edge,       
    };
    enum class BlendMode : uint8_t
    {
        Alpha,      
        Additive,   
        Multiply,   
        Premultiplied, 
    };
    template<typename T>
    struct ValueRange
    {
        T min{};
        T max{};
        ValueRange() = default;
        ValueRange(T value) : min(value), max(value) {}
        ValueRange(T minVal, T maxVal) : min(minVal), max(maxVal) {}
        [[nodiscard]] T Lerp(float t) const
        {
            if constexpr (std::is_same_v<T, float>)
            {
                return min + (max - min) * t;
            }
            else if constexpr (std::is_same_v<T, glm::vec2>)
            {
                return glm::mix(min, max, t);
            }
            else if constexpr (std::is_same_v<T, glm::vec3>)
            {
                return glm::mix(min, max, t);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                return glm::mix(min, max, t);
            }
            else
            {
                return min;
            }
        }
        [[nodiscard]] bool IsConstant() const
        {
            if constexpr (std::is_same_v<T, float>)
            {
                return min == max;
            }
            else
            {
                return min == max;
            }
        }
    };
    using FloatRange = ValueRange<float>;
    using Vec2Range = ValueRange<glm::vec2>;
    using Vec3Range = ValueRange<glm::vec3>;
    using ColorRange = ValueRange<glm::vec4>;
} 
#endif 

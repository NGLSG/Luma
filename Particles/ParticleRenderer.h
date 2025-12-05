#ifndef PARTICLE_RENDERER_H
#define PARTICLE_RENDERER_H
#include "../Data/ParticleData.h"
#include "../Components/ParticleComponent.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/Nut/Pipeline.h"
#include "../Renderer/Nut/Buffer.h"
#include "../Renderer/Nut/RenderPass.h"
#include "../Renderer/Nut/Shader.h"
#include <memory>
#include <unordered_map>
namespace Particles
{
    struct ParticleVertex
    {
        float position[2];
        float uv[2];
    };
    struct ParticleBatch
    {
        entt::entity entity;
        ECS::ParticleSystemComponent* component;
        int zIndex;
        BlendMode blendMode;
    };
    struct TextureSubBatch
    {
        uint32_t textureIndex;          
        size_t startIndex;              
        size_t particleCount;           
        AssetHandle textureHandle;      
    };
    class ParticleRenderer
    {
    public:
        ParticleRenderer();
        ~ParticleRenderer();
        void Initialize(const std::shared_ptr<Nut::NutContext>& context);
        void Shutdown();
        void PrepareRender(entt::registry& registry);
        void Render(Nut::RenderPass& renderPass, const EngineData& engineData);
        void UpdateEngineData(const EngineData& engineData);
        [[nodiscard]] size_t GetTotalParticleCount() const { return m_totalParticleCount; }
        [[nodiscard]] size_t GetBatchCount() const { return m_batches.size(); }
    private:
        void CreatePipelines();
        void CreateVertexBuffer();
        void EnsureInstanceBufferCapacity(size_t requiredCapacity);
        Nut::RenderPipeline* GetPipelineForBlendMode(BlendMode mode);
        void RenderBatch(Nut::RenderPass& renderPass, const ParticleBatch& batch);
    private:
        std::shared_ptr<Nut::NutContext> m_context;
        bool m_initialized = false;
        std::unique_ptr<Nut::RenderPipeline> m_alphaPipeline;
        std::unique_ptr<Nut::RenderPipeline> m_additivePipeline;
        std::unique_ptr<Nut::RenderPipeline> m_multiplyPipeline;
        Nut::ShaderModule m_shaderModule;
        std::shared_ptr<Nut::Buffer> m_vertexBuffer;
        std::shared_ptr<Nut::Buffer> m_indexBuffer;
        std::shared_ptr<Nut::Buffer> m_engineDataBuffer;
        std::shared_ptr<Nut::Buffer> m_instanceBuffer;
        size_t m_instanceBufferCapacity = 0;
        Nut::Sampler m_sampler;
        Nut::TextureAPtr m_defaultTexture;
        std::vector<ParticleBatch> m_batches;
        size_t m_totalParticleCount = 0;
        static constexpr size_t VERTICES_PER_PARTICLE = 4;
        static constexpr size_t INDICES_PER_PARTICLE = 6;
    };
} 
#endif 

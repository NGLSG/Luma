#include "ParticleRenderer.h"
#include "../Renderer/Nut/ShaderModuleRegistry.h"
#include "../Resources/Managers/RuntimeTextureManager.h"
#include <algorithm>
#include <map>
namespace Particles
{
    static const std::vector<ParticleVertex> s_quadVertices = {
        {{-0.5f, -0.5f}, {0.0f, 1.0f}}, 
        {{0.5f, -0.5f}, {1.0f, 1.0f}}, 
        {{0.5f, 0.5f}, {1.0f, 0.0f}}, 
        {{-0.5f, 0.5f}, {0.0f, 0.0f}}, 
    }; 
    static const std::vector<uint16_t> s_quadIndices = {
        0, 1, 2, 
        0, 2, 3 
    };
    ParticleRenderer::ParticleRenderer() = default;
    ParticleRenderer::~ParticleRenderer()
    {
        Shutdown();
    }
    void ParticleRenderer::Initialize(const std::shared_ptr<Nut::NutContext>& context)
    {
        if (m_initialized || !context)
            return;
        m_context = context;
        CreateVertexBuffer();
        Nut::BufferLayout engineDataLayout{
            Nut::BufferBuilder::GetCommonUniformUsage(),
            sizeof(EngineData),
            false
        };
        m_engineDataBuffer = Nut::Buffer::Create(engineDataLayout, m_context);
        m_sampler.SetWrapModeU(Nut::WrapMode::ClampToEdge)
                 .SetWrapModeV(Nut::WrapMode::ClampToEdge)
                 .SetMagFilter(wgpu::FilterMode::Linear)
                 .SetMinFilter(wgpu::FilterMode::Linear)
                 .SetMipmapFilter(Nut::MipmapFilterMode::Linear)
                 .Build(m_context);
        std::vector<uint8_t> whitePixel = {255, 255, 255, 255};
        m_defaultTexture = Nut::TextureBuilder()
                           .SetPixelData(whitePixel.data(), 1, 1, 4)
                           .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
                           .SetUsage(wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst)
                           .Build(m_context);
        CreatePipelines();
        m_initialized = true;
    }
    void ParticleRenderer::Shutdown()
    {
        m_batches.clear();
        m_alphaPipeline.reset();
        m_additivePipeline.reset();
        m_multiplyPipeline.reset();
        m_vertexBuffer.reset();
        m_indexBuffer.reset();
        m_engineDataBuffer.reset();
        m_instanceBuffer.reset();
        m_defaultTexture.reset();
        m_context.reset();
        m_initialized = false;
    }
    void ParticleRenderer::CreateVertexBuffer()
    {
        auto vertexData = s_quadVertices;
        m_vertexBuffer = Nut::BufferBuilder()
                         .SetUsage(Nut::BufferBuilder::GetCommonVertexUsage())
                         .SetData(vertexData)
                         .BuildPtr(m_context);
        auto indexData = s_quadIndices;
        m_indexBuffer = Nut::BufferBuilder()
                        .SetUsage(Nut::BufferBuilder::GetCommonIndexUsage())
                        .SetData(indexData)
                        .BuildPtr(m_context);
    }
    void ParticleRenderer::CreatePipelines()
    {
        const char* inlineShader = R"(
            struct EngineData {
                cameraPosition: vec2<f32>,
                cameraScaleX: f32,
                cameraScaleY: f32,
                cameraSinR: f32,
                cameraCosR: f32,
                viewportSize: vec2<f32>,
                timeData: vec2<f32>,
                mousePosition: vec2<f32>,
            };
            struct ParticleInstance {
                positionAndRotation: vec4<f32>,
                color: vec4<f32>,
                sizeAndUV: vec4<f32>,
                uvScaleAndIndex: vec4<f32>,
            };
            struct VertexInput {
                @location(0) position: vec2<f32>,
                @location(1) uv: vec2<f32>,
            };
            struct VertexOutput {
                @builtin(position) clipPosition: vec4<f32>,
                @location(0) uv: vec2<f32>,
                @location(1) color: vec4<f32>,
            };
            @group(0) @binding(0) var<uniform> engineData: EngineData;
            @group(0) @binding(1) var<storage, read> particles: array<ParticleInstance>;
            @group(0) @binding(2) var particleTexture: texture_2d<f32>;
            @group(0) @binding(3) var particleSampler: sampler;
            fn rotate2D(pos: vec2<f32>, sinR: f32, cosR: f32) -> vec2<f32> {
                return vec2<f32>(pos.x * cosR - pos.y * sinR, pos.x * sinR + pos.y * cosR);
            }
            @vertex
            fn vs_main(input: VertexInput, @builtin(instance_index) instanceIndex: u32) -> VertexOutput {
                var output: VertexOutput;
                let particle = particles[instanceIndex];
                let particlePos = particle.positionAndRotation.xyz;
                let particleRotation = particle.positionAndRotation.w;
                let particleSize = particle.sizeAndUV.xy;
                // 纹理图集UV参数
                let uvOffset = particle.sizeAndUV.zw;  // UV偏移
                let uvScale = particle.uvScaleAndIndex.xy;  // UV缩放 (1/tilesX, 1/tilesY)
                let sinR = sin(particleRotation);
                let cosR = cos(particleRotation);
                var localPos = input.position * particleSize;
                localPos = rotate2D(localPos, sinR, cosR);
                var worldPos = localPos + particlePos.xy;
                var cameraSpace = worldPos - engineData.cameraPosition;
                cameraSpace = rotate2D(cameraSpace, -engineData.cameraSinR, engineData.cameraCosR);
                cameraSpace = cameraSpace * vec2<f32>(engineData.cameraScaleX, engineData.cameraScaleY);
                let clipPos = cameraSpace / (engineData.viewportSize * 0.5);
                output.clipPosition = vec4<f32>(clipPos, particlePos.z, 1.0);
                // 计算纹理图集UV: offset + baseUV * scale
                output.uv = uvOffset + input.uv * uvScale;
                output.color = particle.color;
                return output;
            }
            @fragment
            fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
                var texColor = textureSample(particleTexture, particleSampler, input.uv);
                var finalColor = texColor * input.color;
                finalColor = vec4<f32>(finalColor.rgb * finalColor.a, finalColor.a);
                if (finalColor.a < 0.001) { discard; }
                return finalColor;
            }
            @fragment
            fn fs_additive(input: VertexOutput) -> @location(0) vec4<f32> {
                var texColor = textureSample(particleTexture, particleSampler, input.uv);
                var finalColor = texColor * input.color;
                return vec4<f32>(finalColor.rgb * finalColor.a, 0.0);
            }
        )";
        m_shaderModule = Nut::ShaderModule(inlineShader, m_context);
        Nut::VertexBufferLayout vertexLayout;
        vertexLayout.arrayStride = sizeof(ParticleVertex);
        vertexLayout.stepMode = Nut::VertexStepMode::Vertex;
        vertexLayout.attributes.push_back(Nut::VertexAttribute(&ParticleVertex::position, 0));
        vertexLayout.attributes.push_back(Nut::VertexAttribute(&ParticleVertex::uv, 1));
        auto surfaceFormat = wgpu::TextureFormat::RGBA8Unorm;
        {
            Nut::BlendState alphaBlend = Nut::BlendState::AlphaBlend();
            Nut::RenderPipelineBuilder builder(m_context);
            builder.SetShaderModule(m_shaderModule)
                   .SetVertexEntry("vs_main")
                   .SetFragmentEntry("fs_main")
                   .AddVertexBuffer(vertexLayout)
                   .AddColorTarget(surfaceFormat, &alphaBlend)
                   .SetRasterization(Nut::RasterizationState::NoCull())
                   .SetMultisample(Nut::MultisampleState::None())
                   .SetLabel("ParticleAlphaPipeline");
            m_alphaPipeline = builder.Build();
        }
        {
            Nut::BlendState additiveBlend = Nut::BlendState::Additive();
            Nut::RenderPipelineBuilder builder(m_context);
            builder.SetShaderModule(m_shaderModule)
                   .SetVertexEntry("vs_main")
                   .SetFragmentEntry("fs_additive")
                   .AddVertexBuffer(vertexLayout)
                   .AddColorTarget(surfaceFormat, &additiveBlend)
                   .SetRasterization(Nut::RasterizationState::NoCull())
                   .SetMultisample(Nut::MultisampleState::None())
                   .SetLabel("ParticleAdditivePipeline");
            m_additivePipeline = builder.Build();
        }
    }
    void ParticleRenderer::EnsureInstanceBufferCapacity(size_t requiredCapacity)
    {
        if (requiredCapacity <= m_instanceBufferCapacity)
            return;
        size_t newCapacity = std::max(m_instanceBufferCapacity * 2, requiredCapacity);
        newCapacity = std::max(newCapacity, size_t(1024)); 
        size_t bufferSize = newCapacity * sizeof(ParticleGPUData);
        Nut::BufferLayout instanceLayout{
            Nut::BufferUsage::Storage | Nut::BufferUsage::CopyDst,
            static_cast<uint32_t>(bufferSize),
            false
        };
        m_instanceBuffer = Nut::Buffer::Create(instanceLayout, m_context);
        m_instanceBufferCapacity = newCapacity;
    }
    void ParticleRenderer::PrepareRender(entt::registry& registry)
    {
        m_batches.clear();
        m_totalParticleCount = 0;
        auto view = registry.view<ECS::ParticleSystemComponent>();
        for (auto entity : view)
        {
            auto& ps = view.get<ECS::ParticleSystemComponent>(entity);
            if (!ps.Enable)
                continue;
            if (!ps.pool || ps.pool->Empty())
                continue;
            ParticleBatch batch;
            batch.entity = entity;
            batch.component = &ps;
            batch.zIndex = ps.zIndex;
            batch.blendMode = ps.blendMode;
            m_batches.push_back(batch);
            m_totalParticleCount += ps.pool->Size();
        }
        std::sort(m_batches.begin(), m_batches.end(),
                  [](const ParticleBatch& a, const ParticleBatch& b)
                  {
                      return a.zIndex < b.zIndex;
                  });
        if (m_totalParticleCount > 0)
        {
            EnsureInstanceBufferCapacity(m_totalParticleCount);
        }
    }
    void ParticleRenderer::UpdateEngineData(const EngineData& engineData)
    {
        if (m_engineDataBuffer)
        {
            m_engineDataBuffer->WriteBuffer(&engineData, sizeof(EngineData));
        }
    }
    Nut::RenderPipeline* ParticleRenderer::GetPipelineForBlendMode(BlendMode mode)
    {
        switch (mode)
        {
        case BlendMode::Additive:
            return m_additivePipeline.get();
        case BlendMode::Multiply:
            return m_multiplyPipeline ? m_multiplyPipeline.get() : m_alphaPipeline.get();
        case BlendMode::Alpha:
        case BlendMode::Premultiplied:
        default:
            return m_alphaPipeline.get();
        }
    }
    static Nut::TextureAPtr GetTextureFromHandle(const AssetHandle& handle, Nut::TextureAPtr defaultTexture)
    {
        if (!handle.Valid())
            return defaultTexture;
        auto& textureManager = RuntimeTextureManager::GetInstance();
        sk_sp<RuntimeTexture> runtimeTexture;
        if (textureManager.TryGetAsset(handle.assetGuid, runtimeTexture) && runtimeTexture)
        {
            return runtimeTexture->getNutTexture();
        }
        return defaultTexture;
    }
    void ParticleRenderer::Render(Nut::RenderPass& renderPass, const EngineData& engineData)
    {
        if (!m_initialized || m_batches.empty())
            return;
        UpdateEngineData(engineData);
        std::vector<ParticleGPUData> allGPUData;
        allGPUData.reserve(m_totalParticleCount);
        struct BatchRenderInfo
        {
            const ParticleBatch* batch;
            size_t globalStartOffset; 
            std::vector<TextureSubBatch> subBatches; 
        };
        std::vector<BatchRenderInfo> batchInfos;
        for (const auto& batch : m_batches)
        {
            BatchRenderInfo info;
            info.batch = &batch;
            info.globalStartOffset = allGPUData.size();
            const auto& particles = batch.component->pool->GetParticles();
            const auto& gpuData = batch.component->pool->GetGPUData();
            if (batch.component->useSequenceAnimation && !batch.component->textureFrames.empty())
            {
                std::map<uint32_t, std::vector<size_t>> particlesByTexture;
                for (size_t i = 0; i < particles.size(); ++i)
                {
                    uint32_t texIdx = std::min(particles[i].textureIndex,
                                               static_cast<uint32_t>(batch.component->textureFrames.size() - 1));
                    particlesByTexture[texIdx].push_back(i);
                }
                for (const auto& [texIdx, indices] : particlesByTexture)
                {
                    TextureSubBatch subBatch;
                    subBatch.textureIndex = texIdx;
                    subBatch.startIndex = allGPUData.size();
                    subBatch.particleCount = indices.size();
                    subBatch.textureHandle = batch.component->textureFrames[texIdx];
                    for (size_t idx : indices)
                    {
                        allGPUData.push_back(gpuData[idx]);
                    }
                    info.subBatches.push_back(subBatch);
                }
            }
            else
            {
                TextureSubBatch subBatch;
                subBatch.textureIndex = 0;
                subBatch.startIndex = allGPUData.size();
                subBatch.particleCount = gpuData.size();
                subBatch.textureHandle = batch.component->textureHandle;
                allGPUData.insert(allGPUData.end(), gpuData.begin(), gpuData.end());
                info.subBatches.push_back(subBatch);
            }
            batchInfos.push_back(std::move(info));
        }
        if (allGPUData.empty())
            return;
        m_instanceBuffer->WriteBuffer(allGPUData.data(),
                                      static_cast<uint32_t>(allGPUData.size() * sizeof(ParticleGPUData)));
        renderPass.SetVertexBuffer(0, *m_vertexBuffer);
        renderPass.SetIndexBuffer(*m_indexBuffer, wgpu::IndexFormat::Uint16);
        BlendMode currentBlendMode = m_batches[0].blendMode;
        Nut::RenderPipeline* currentPipeline = GetPipelineForBlendMode(currentBlendMode);
        for (const auto& info : batchInfos)
        {
            const auto* batch = info.batch;
            if (batch->blendMode != currentBlendMode)
            {
                currentBlendMode = batch->blendMode;
                currentPipeline = GetPipelineForBlendMode(currentBlendMode);
            }
            for (const auto& subBatch : info.subBatches)
            {
                if (subBatch.particleCount == 0)
                    continue;
                currentPipeline->SetBinding(0, 0, *m_engineDataBuffer);
                currentPipeline->SetBinding(0, 1, *m_instanceBuffer);
                Nut::TextureAPtr texture = GetTextureFromHandle(subBatch.textureHandle, m_defaultTexture);
                currentPipeline->SetBinding(0, 2, texture);
                currentPipeline->SetBinding(0, 3, m_sampler);
                currentPipeline->BuildBindings(m_context);
                renderPass.SetPipeline(*currentPipeline);
                renderPass.DrawIndexed(
                    INDICES_PER_PARTICLE, 
                    static_cast<uint32_t>(subBatch.particleCount), 
                    0, 
                    0, 
                    static_cast<uint32_t>(subBatch.startIndex)
                );
            }
        }
    }
    void ParticleRenderer::RenderBatch(Nut::RenderPass& renderPass, const ParticleBatch& batch)
    {
    }
} 

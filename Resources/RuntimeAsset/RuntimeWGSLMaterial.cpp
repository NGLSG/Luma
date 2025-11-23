#include "RuntimeWGSLMaterial.h"
#include "Logger.h"
#include "Renderer/Nut/NutContext.h"
#include "Renderer/Nut/Pipeline.h"
#include "Renderer/Nut/ShaderStruct.h"

bool RuntimeWGSLMaterial::Initialize(const std::shared_ptr<Nut::NutContext>& context, const std::string& shaderCode,
                                     wgpu::TextureFormat colorFormat, uint32_t initialSampleCount)
{
    if (!context)
    {
        LogError("RuntimeWGSLMaterial::Initialize - NutContext is null");
        return false;
    }
    m_context = context;

    m_cachedShaderCode = shaderCode;
    m_cachedColorFormat = colorFormat;

    m_shaderModule = Nut::ShaderManager::GetFromString(shaderCode, context);
    if (!m_shaderModule.Get())
    {
        LogError("RuntimeWGSLMaterial::Initialize - Failed to create shader module");
        return false;
    }

    if (GetOrBuildPipeline(initialSampleCount) == nullptr)
    {
        return false;
    }

    return true;
}

Nut::RenderPipeline* RuntimeWGSLMaterial::GetOrBuildPipeline(uint32_t sampleCount)
{
    auto it = m_pipelineCache.find(sampleCount);
    if (it != m_pipelineCache.end())
    {
        return it->second.get();
    }

    Nut::RenderPipelineBuilder builder(m_context);

    builder.SetShaderModule(m_shaderModule);
    builder.SetVertexEntry("vs_main");
    builder.SetFragmentEntry("fs_main");
    builder.SetLabel("RuntimeWGSL_SpritePipeline_Samples" + std::to_string(sampleCount));

    Nut::VertexBufferLayout vertexLayout;
    vertexLayout.arrayStride = sizeof(Vertex);
    vertexLayout.stepMode = wgpu::VertexStepMode::Vertex;
    vertexLayout.attributes.push_back(Nut::VertexAttribute(&Vertex::position, 0));
    vertexLayout.attributes.push_back(Nut::VertexAttribute(&Vertex::texCoord, 1));
    builder.AddVertexBuffer(vertexLayout);

    Nut::BlendState blendState = Nut::BlendState::AlphaBlend();
    builder.AddColorTarget(m_cachedColorFormat, &blendState);

    Nut::RasterizationState raster;
    raster.SetCullMode(wgpu::CullMode::None);
    builder.SetRasterization(raster);

    Nut::MultisampleState msaa;
    msaa.SetCount(sampleCount > 0 ? sampleCount : 1);
    builder.SetMultisample(msaa);

    auto pipeline = builder.Build();
    if (!pipeline)
    {
        LogError("RuntimeWGSLMaterial: Failed to build pipeline for sample count {}", sampleCount);
        return nullptr;
    }

    Nut::RenderPipeline* ptr = pipeline.get();
    m_pipelineCache[sampleCount] = std::move(pipeline);
    return ptr;
}

Nut::RenderPipeline* RuntimeWGSLMaterial::GetPipeline(uint32_t sampleCount)
{
    return GetOrBuildPipeline(sampleCount);
}

void RuntimeWGSLMaterial::SetTexture(const std::string& name, const Nut::TextureAPtr& texture, uint32_t binding,
                                     uint32_t group)
{
    TextureBinding textureBinding;
    textureBinding.name = name;
    textureBinding.texture = texture;
    textureBinding.binding = binding;
    textureBinding.group = group;

    m_textures[name] = textureBinding;
}

void RuntimeWGSLMaterial::UpdateUniformBuffer()
{
    if (!m_uniformBufferDirty || m_uniforms.empty())
    {
        return;
    }

    size_t totalSize = 0;
    size_t currentOffset = 0;

    for (auto& [name, uniformData] : m_uniforms)
    {
        size_t uniformSize = std::visit([](auto&& value) -> size_t
        {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, float>) return sizeof(float);
            else if constexpr (std::is_same_v<T, int>) return sizeof(int);
            else if constexpr (std::is_same_v<T, std::array<float, 2>>) return sizeof(float) * 2;
            else if constexpr (std::is_same_v<T, std::array<float, 3>>) return sizeof(float) * 3;
            else if constexpr (std::is_same_v<T, std::array<float, 4>>) return sizeof(float) * 4;
            else if constexpr (std::is_same_v<T, std::array<float, 16>>) return sizeof(float) * 16;
            return 0;
        }, uniformData.value);

        if (currentOffset % 16 != 0)
        {
            currentOffset = (currentOffset + 15) & ~15;
        }

        uniformData.offset = currentOffset;
        uniformData.size = uniformSize;
        currentOffset += uniformSize;
        totalSize = currentOffset;
    }

    totalSize = (totalSize + 15) & ~15;

    if (!m_uniformBuffer || m_uniformBuffer->GetSize() < totalSize)
    {
        Nut::BufferLayout uniformLayout;
        uniformLayout.size = totalSize;
        uniformLayout.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        uniformLayout.mapped = false;
        m_uniformBuffer = Nut::Buffer::Create(uniformLayout, m_context);
    }

    std::vector<uint8_t> bufferData(totalSize, 0);

    for (const auto& [name, uniformData] : m_uniforms)
    {
        std::visit([&](auto&& value)
        {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, float>)
                std::memcpy(bufferData.data() + uniformData.offset, &value, sizeof(float));
            else if constexpr (std::is_same_v<T, int>)
                std::memcpy(bufferData.data() + uniformData.offset, &value, sizeof(int));
            else if constexpr (std::is_same_v<T, std::array<float, 2>>)
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 2);
            else if constexpr (std::is_same_v<T, std::array<float, 3>>)
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 3);
            else if constexpr (std::is_same_v<T, std::array<float, 4>>)
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 4);
            else if constexpr (std::is_same_v<T, std::array<float, 16>>)
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 16);
        }, uniformData.value);
    }

    m_uniformBuffer->WriteBuffer(bufferData.data(), totalSize);
    m_uniformBufferDirty = false;
}

void RuntimeWGSLMaterial::Bind(const Nut::RenderPass& renderPass)
{
    if (!IsValid())
    {
        return;
    }

    UpdateUniformBuffer();
}
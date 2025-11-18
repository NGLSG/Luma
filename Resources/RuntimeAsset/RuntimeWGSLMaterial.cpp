#include "RuntimeWGSLMaterial.h"
#include "Logger.h"
#include "Renderer/Nut/NutContext.h"
#include <cstring>

bool RuntimeWGSLMaterial::Initialize(
    const std::shared_ptr<Nut::NutContext>& context,
    const std::string& shaderCode,
    wgpu::TextureFormat colorFormat)
{
    if (!context)
    {
        LogError("RuntimeWGSLMaterial::Initialize - NutContext is null");
        return false;
    }

    m_context = context;

    
    m_shaderModule = Nut::ShaderModule(shaderCode, context);
    if (!m_shaderModule.Get())
    {
        LogError("RuntimeWGSLMaterial::Initialize - Failed to create shader module");
        return false;
    }

    
    m_pipeline = Nut::PipelinePresets::Create2DSprite(context, m_shaderModule, colorFormat);
    if (!m_pipeline)
    {
        LogError("RuntimeWGSLMaterial::Initialize - Failed to create render pipeline");
        return false;
    }

    
    m_sampler.SetMagFilter(Nut::FilterMode::Linear)
            .SetMinFilter(Nut::FilterMode::Linear)
            .SetWrapModeU(Nut::WrapMode::ClampToEdge)
            .SetWrapModeV(Nut::WrapMode::ClampToEdge);
    m_sampler.Build(context);

    return true;
}

void RuntimeWGSLMaterial::SetTexture(const std::string& name, const Nut::TextureA& texture, uint32_t binding, uint32_t group)
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
            if constexpr (std::is_same_v<T, float>)
                return sizeof(float);
            else if constexpr (std::is_same_v<T, int>)
                return sizeof(int);
            else if constexpr (std::is_same_v<T, std::array<float, 2>>)
                return sizeof(float) * 2;
            else if constexpr (std::is_same_v<T, std::array<float, 3>>)
                return sizeof(float) * 3;
            else if constexpr (std::is_same_v<T, std::array<float, 4>>)
                return sizeof(float) * 4;
            else if constexpr (std::is_same_v<T, std::array<float, 16>>)
                return sizeof(float) * 16;
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
            {
                std::memcpy(bufferData.data() + uniformData.offset, &value, sizeof(float));
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                std::memcpy(bufferData.data() + uniformData.offset, &value, sizeof(int));
            }
            else if constexpr (std::is_same_v<T, std::array<float, 2>>)
            {
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 2);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 3>>)
            {
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 3);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 4>>)
            {
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 4);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 16>>)
            {
                std::memcpy(bufferData.data() + uniformData.offset, value.data(), sizeof(float) * 16);
            }
        }, uniformData.value);
    }

    m_uniformBuffer->WriteBuffer(bufferData.data(), totalSize);
    m_uniformBufferDirty = false;
}

void RuntimeWGSLMaterial::Bind(Nut::RenderPass& renderPass)
{
    if (!IsValid())
    {
        LogWarn("RuntimeWGSLMaterial::Bind - Material is not valid");
        return;
    }

    
    UpdateUniformBuffer();

    
    renderPass.SetPipeline(*m_pipeline);

    
    if (m_uniformBuffer && !m_uniforms.empty())
    {
        m_pipeline->SetBinding(0, 0, *m_uniformBuffer);
    }

    
    m_pipeline->SetBinding(0, 0, m_sampler);

    
    for (const auto& [name, textureBinding] : m_textures)
    {
        m_pipeline->SetBinding(textureBinding.group, textureBinding.binding, textureBinding.texture);
    }

    
    m_pipeline->BuildBindings(m_context);
    m_pipeline->ForeachGroup([&renderPass](size_t groupIdx, Nut::BindGroup& group)
    {
        renderPass.SetBindGroup(groupIdx, group);
    });
}

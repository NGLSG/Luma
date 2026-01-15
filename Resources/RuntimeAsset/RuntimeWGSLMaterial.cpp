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
    
    // 检测是否使用了 Lighting 模块
    m_usesLightingModule = (shaderCode.find("import Lighting") != std::string::npos);

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

bool RuntimeWGSLMaterial::Initialize(const std::shared_ptr<Nut::NutContext>& context,
                                     const sk_sp<RuntimeShader>& runtimeShader, wgpu::TextureFormat colorFormat,
                                     uint32_t initialSampleCount)
{
    if (!context)
    {
        LogError("RuntimeWGSLMaterial::Initialize - NutContext is null");
        return false;
    }
    if (!runtimeShader)
    {
        LogError("RuntimeWGSLMaterial::Initialize - RuntimeShader is null");
        return false;
    }
    if (runtimeShader->GetLanguage() != Data::ShaderLanguage::WGSL ||
        runtimeShader->GetShaderType() != Data::ShaderType::VertFrag)
    {
        LogError("RuntimeWGSLMaterial::Initialize - RuntimeShader is not a WGSL vertex-fragment shader");
        return false;
    }

    m_context = context;
    m_cachedShaderCode = "";
    m_cachedColorFormat = colorFormat;
    
    // 检测是否使用了 Lighting 模块（从 RuntimeShader 的源代码检测）
    const std::string& sourceCode = runtimeShader->GetShaderCode();
    m_usesLightingModule = (sourceCode.find("import Lighting") != std::string::npos);

    m_shaderModule = runtimeShader->GetWGPUShader();
    if (!m_shaderModule.Get())
    {
        LogError("RuntimeWGSLMaterial::Initialize - Failed to get shader module from RuntimeShader");
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

RuntimeWGSLMaterial::RuntimeWGSLMaterial() = default;
RuntimeWGSLMaterial::~RuntimeWGSLMaterial() = default;

void RuntimeWGSLMaterial::SetUniformVec2(const std::string& name, float x, float y)
{
    SetUniform(name, std::array<float, 2>{x, y});
}

void RuntimeWGSLMaterial::SetUniformVec3(const std::string& name, float x, float y, float z)
{
    SetUniform(name, std::array<float, 3>{x, y, z});
}

void RuntimeWGSLMaterial::SetUniformVec4(const std::string& name, float r, float g, float b, float a)
{
    SetUniform(name, std::array<float, 4>{r, g, b, a});
}

void RuntimeWGSLMaterial::SetUniformStruct(const std::string& name, const void* data, size_t size)
{
    std::vector<uint8_t> byteData(size);
    std::memcpy(byteData.data(), data, size);

    if (m_uniforms.contains(name))
    {
        m_uniforms[name].value = std::move(byteData);
        m_uniformBufferDirty = true;
    }
    else
    {
        UniformData uniformData;
        uniformData.name = name;
        uniformData.value = std::move(byteData);
        m_uniforms[name] = uniformData;
        m_uniformBufferDirty = true;
    }
}

const Nut::ShaderModule& RuntimeWGSLMaterial::GetShaderModule() const
{
    return m_shaderModule;
}

bool RuntimeWGSLMaterial::IsValid()
{
    return m_shaderModule.Get() != nullptr;
}

void RuntimeWGSLMaterial::SetSourceGuid(const Guid& guid)
{
    m_sourceGuid = guid;
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

    auto pipeline = GetPipeline();
    if (!pipeline)
    {
        LogError("RuntimeWGSLMaterial::UpdateUniformBuffer - Pipeline is null");
        return;
    }

    
    
    for (const auto& [name, uniformData] : m_uniforms)
    {
        std::visit([&](auto&& value)
        {
            using T = std::decay_t<decltype(value)>;
            
            if constexpr (std::is_same_v<T, float>)
            {
                pipeline->UpdateUniformBuffer(name, &value, sizeof(float));
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                pipeline->UpdateUniformBuffer(name, &value, sizeof(int));
            }
            else if constexpr (std::is_same_v<T, std::array<float, 2>>)
            {
                pipeline->UpdateUniformBuffer(name, value.data(), sizeof(float) * 2);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 3>>)
            {
                pipeline->UpdateUniformBuffer(name, value.data(), sizeof(float) * 3);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 4>>)
            {
                pipeline->UpdateUniformBuffer(name, value.data(), sizeof(float) * 4);
            }
            else if constexpr (std::is_same_v<T, std::array<float, 16>>)
            {
                pipeline->UpdateUniformBuffer(name, value.data(), sizeof(float) * 16);
            }
            else if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
            {
                pipeline->UpdateUniformBuffer(name, value.data(), value.size());
            }
        }, uniformData.value);
    }

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

bool RuntimeWGSLMaterial::UsesLightingModule() const
{
    return m_usesLightingModule;
}

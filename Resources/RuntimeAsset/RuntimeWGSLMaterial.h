#pragma once

#include "IRuntimeAsset.h"
#include "Renderer/Nut/Pipeline.h"
#include "Renderer/Nut/Shader.h"
#include "Renderer/Nut/Buffer.h"
#include "Renderer/Nut/TextureA.h"
#include <unordered_map>
#include <variant>
#include <memory>
#include <string>
#include <array>

#include "RuntimeShader.h"

namespace Nut
{
    class NutContext;
}

class LUMA_API RuntimeWGSLMaterial : public IRuntimeAsset
{
public:
    using UniformValue = std::variant<
        float,
        int,
        std::array<float, 2>,
        std::array<float, 3>,
        std::array<float, 4>,
        std::array<float, 16>
    >;

    struct UniformData
    {
        std::string name;
        UniformValue value;
        size_t offset = 0;
        size_t size = 0;
    };

    struct TextureBinding
    {
        std::string name;
        Nut::TextureAPtr texture;
        uint32_t binding = 0;
        uint32_t group = 0;
    };

private:
    std::shared_ptr<Nut::NutContext> m_context;

    std::unordered_map<uint32_t, std::unique_ptr<Nut::RenderPipeline>> m_pipelineCache;

    std::string m_cachedShaderCode;
    wgpu::TextureFormat m_cachedColorFormat = wgpu::TextureFormat::BGRA8Unorm;

    Nut::ShaderModule m_shaderModule;

    std::unordered_map<std::string, UniformData> m_uniforms;
    std::unordered_map<std::string, TextureBinding> m_textures;

    std::unique_ptr<Nut::Buffer> m_uniformBuffer;
    bool m_uniformBufferDirty = true;

    Nut::RenderPipeline* GetOrBuildPipeline(uint32_t sampleCount);

public:
    RuntimeWGSLMaterial() = default;
    ~RuntimeWGSLMaterial() override = default;

    bool Initialize(
        const std::shared_ptr<Nut::NutContext>& context,
        const std::string& shaderCode,
        wgpu::TextureFormat colorFormat = wgpu::TextureFormat::BGRA8Unorm,
        uint32_t initialSampleCount = 1
    );

    bool Initialize(
        const std::shared_ptr<Nut::NutContext>& context,
        const sk_sp<RuntimeShader>& runtimeShader,
        wgpu::TextureFormat colorFormat = wgpu::TextureFormat::BGRA8Unorm,
        uint32_t initialSampleCount = 1
    );

    template <typename T>
    void SetUniform(const std::string& name, const T& value)
    {
        UniformValue uniformValue;

        if constexpr (std::is_same_v<T, float>)
        {
            uniformValue = value;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            uniformValue = value;
        }
        else if constexpr (std::is_same_v<T, std::array<float, 2>>)
        {
            uniformValue = value;
        }
        else if constexpr (std::is_same_v<T, std::array<float, 3>>)
        {
            uniformValue = value;
        }
        else if constexpr (std::is_same_v<T, std::array<float, 4>>)
        {
            uniformValue = value;
        }
        else if constexpr (std::is_same_v<T, std::array<float, 16>>)
        {
            uniformValue = value;
        }

        if (m_uniforms.find(name) != m_uniforms.end())
        {
            m_uniforms[name].value = uniformValue;
            m_uniformBufferDirty = true;
        }
        else
        {
            UniformData data;
            data.name = name;
            data.value = uniformValue;
            m_uniforms[name] = data;
            m_uniformBufferDirty = true;
        }
    }

    void SetUniformVec2(const std::string& name, float x, float y)
    {
        SetUniform(name, std::array<float, 2>{x, y});
    }

    void SetUniformVec3(const std::string& name, float x, float y, float z)
    {
        SetUniform(name, std::array<float, 3>{x, y, z});
    }

    void SetUniformVec4(const std::string& name, float r, float g, float b, float a)
    {
        SetUniform(name, std::array<float, 4>{r, g, b, a});
    }

    void SetTexture(const std::string& name, const Nut::TextureAPtr& texture, uint32_t binding = 1, uint32_t group = 0);

    void UpdateUniformBuffer();

    void Bind(const Nut::RenderPass& renderPass);

    Nut::RenderPipeline* GetPipeline(uint32_t sampleCount = 1);

    const Nut::ShaderModule& GetShaderModule() const { return m_shaderModule; }

    bool IsValid() { return m_shaderModule.Get() != nullptr; }

    void SetSourceGuid(const Guid& guid) { m_sourceGuid = guid; }
};

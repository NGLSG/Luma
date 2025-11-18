#pragma once

#include "IRuntimeAsset.h"
#include "../../Renderer/Nut/Pipeline.h"
#include "../../Renderer/Nut/Shader.h"
#include "../../Renderer/Nut/Buffer.h"
#include "../../Renderer/Nut/TextureA.h"
#include <unordered_map>
#include <variant>
#include <memory>

namespace Nut {
    class NutContext;
}

/**
 * @brief WGSL材质运行时资产
 * 
 * 封装了基于WGPU/WGSL的材质系统，替代原有的SkSL材质系统。
 * 包含渲染管线、着色器模块、uniform数据和纹理绑定。
 */
class RuntimeWGSLMaterial : public IRuntimeAsset
{
public:
    /**
     * @brief Uniform变量类型
     */
    using UniformValue = std::variant<
        float,              // float
        int,                // int
        std::array<float, 2>,  // vec2
        std::array<float, 3>,  // vec3
        std::array<float, 4>,  // vec4
        std::array<float, 16>  // mat4
    >;

    /**
     * @brief Uniform数据结构
     */
    struct UniformData
    {
        std::string name;
        UniformValue value;
        size_t offset = 0;     // 在uniform buffer中的偏移
        size_t size = 0;       // 数据大小
    };

    /**
     * @brief 纹理绑定数据
     */
    struct TextureBinding
    {
        std::string name;
        Nut::TextureA texture;
        uint32_t binding = 0;
        uint32_t group = 0;
    };

private:
    std::shared_ptr<Nut::NutContext> m_context;
    std::unique_ptr<Nut::RenderPipeline> m_pipeline;
    Nut::ShaderModule m_shaderModule;
    
    std::unordered_map<std::string, UniformData> m_uniforms;
    std::unordered_map<std::string, TextureBinding> m_textures;
    
    std::unique_ptr<Nut::Buffer> m_uniformBuffer;
    bool m_uniformBufferDirty = true;

    // 采样器
    Nut::Sampler m_sampler;

public:
    RuntimeWGSLMaterial() = default;
    ~RuntimeWGSLMaterial() override = default;

    /**
     * @brief 初始化材质
     * @param context Nut图形上下文
     * @param shaderCode WGSL着色器代码
     * @param colorFormat 颜色目标格式
     */
    bool Initialize(
        const std::shared_ptr<Nut::NutContext>& context,
        const std::string& shaderCode,
        wgpu::TextureFormat colorFormat = wgpu::TextureFormat::BGRA8Unorm
    );

    /**
     * @brief 设置uniform变量
     */
    template<typename T>
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

    /**
     * @brief 设置vec2 uniform
     */
    void SetUniformVec2(const std::string& name, float x, float y)
    {
        SetUniform(name, std::array<float, 2>{x, y});
    }

    /**
     * @brief 设置vec3 uniform
     */
    void SetUniformVec3(const std::string& name, float x, float y, float z)
    {
        SetUniform(name, std::array<float, 3>{x, y, z});
    }

    /**
     * @brief 设置vec4 uniform (颜色)
     */
    void SetUniformVec4(const std::string& name, float r, float g, float b, float a)
    {
        SetUniform(name, std::array<float, 4>{r, g, b, a});
    }

    /**
     * @brief 绑定纹理
     */
    void SetTexture(const std::string& name, const Nut::TextureA& texture, uint32_t binding = 1, uint32_t group = 0);

    /**
     * @brief 更新uniform buffer
     */
    void UpdateUniformBuffer();

    /**
     * @brief 绑定材质到pipeline
     */
    void Bind(Nut::RenderPass& renderPass);

    /**
     * @brief 获取渲染管线
     */
    Nut::RenderPipeline* GetPipeline() const { return m_pipeline.get(); }

    /**
     * @brief 获取着色器模块
     */
    const Nut::ShaderModule& GetShaderModule() const { return m_shaderModule; }

    /**
     * @brief 检查材质是否有效
     */
    bool IsValid() const { return m_pipeline != nullptr; }

    /**
     * @brief 设置源GUID
     */
    void SetSourceGuid(const Guid& guid) { m_sourceGuid = guid; }
};

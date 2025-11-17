#ifndef NOAI_PIPELINE_H
#define NOAI_PIPELINE_H
#include <string>
#include <vector>

#include "BindGroup.h"
#include "RenderPass.h"
#include "Shader.h"
#include "dawn/webgpu_cpp.h"

namespace Nut {

class NutContext;

using VertexFormat = wgpu::VertexFormat;
using VertexStepMode = wgpu::VertexStepMode;

class VertexAttribute : public wgpu::VertexAttribute
{
public:
    VertexAttribute() = default;

    VertexAttribute& SetLocation(size_t loc);

    VertexAttribute& SetFormat(VertexFormat format);

    VertexAttribute& SetOffset(size_t offset);

    template <typename Class, typename Member>
    VertexAttribute(Member Class::* member_ptr, size_t loc)
    {
        // 计算成员偏移
        std::size_t offset = reinterpret_cast<std::size_t>(
            &(reinterpret_cast<Class*>(0)->*member_ptr)
        );
        SetOffset(offset);

        // 根据成员类型自动推断 format
        using MemberType = std::remove_reference_t<Member>;
        SetFormat(DeduceFormat<MemberType>());
        SetLocation(loc);
    }

private:
    template <typename T>
    static constexpr VertexFormat DeduceFormat()
    {
        // Float 类型推导
        if constexpr (std::is_same_v<T, float>)
            return VertexFormat::Float32;
        else if constexpr (std::is_same_v<T, float[2]>)
            return VertexFormat::Float32x2;
        else if constexpr (std::is_same_v<T, float[3]>)
            return VertexFormat::Float32x3;
        else if constexpr (std::is_same_v<T, float[4]>)
            return VertexFormat::Float32x4;

            // Uint32 类型推导
        else if constexpr (std::is_same_v<T, uint32_t>)
            return VertexFormat::Uint32;
        else if constexpr (std::is_same_v<T, uint32_t[2]>)
            return VertexFormat::Uint32x2;
        else if constexpr (std::is_same_v<T, uint32_t[3]>)
            return VertexFormat::Uint32x3;
        else if constexpr (std::is_same_v<T, uint32_t[4]>)
            return VertexFormat::Uint32x4;

            // Sint32 类型推导
        else if constexpr (std::is_same_v<T, int32_t>)
            return VertexFormat::Sint32;
        else if constexpr (std::is_same_v<T, int32_t[2]>)
            return VertexFormat::Sint32x2;
        else if constexpr (std::is_same_v<T, int32_t[3]>)
            return VertexFormat::Sint32x3;
        else if constexpr (std::is_same_v<T, int32_t[4]>)
            return VertexFormat::Sint32x4;

            // Uint16 类型推导
        else if constexpr (std::is_same_v<T, uint16_t>)
            return VertexFormat::Uint16;
        else if constexpr (std::is_same_v<T, uint16_t[2]>)
            return VertexFormat::Uint16x2;
        else if constexpr (std::is_same_v<T, uint16_t[4]>)
            return VertexFormat::Uint16x4;

            // Sint16 类型推导
        else if constexpr (std::is_same_v<T, int16_t>)
            return VertexFormat::Sint16;
        else if constexpr (std::is_same_v<T, int16_t[2]>)
            return VertexFormat::Sint16x2;
        else if constexpr (std::is_same_v<T, int16_t[4]>)
            return VertexFormat::Sint16x4;

            // Uint8 类型推导
        else if constexpr (std::is_same_v<T, uint8_t>)
            return VertexFormat::Uint8;
        else if constexpr (std::is_same_v<T, uint8_t[2]>)
            return VertexFormat::Uint8x2;
        else if constexpr (std::is_same_v<T, uint8_t[4]>)
            return VertexFormat::Uint8x4;

            // Sint8 类型推导
        else if constexpr (std::is_same_v<T, int8_t>)
            return VertexFormat::Sint8;
        else if constexpr (std::is_same_v<T, int8_t[2]>)
            return VertexFormat::Sint8x2;
        else if constexpr (std::is_same_v<T, int8_t[4]>)
            return VertexFormat::Sint8x4;

        else
        {
            static_assert(!sizeof(T), "Unsupported vertex attribute type!");
        }
    }
};

class VertexBufferLayout
{
public:
    VertexStepMode stepMode = VertexStepMode::Vertex;
    uint64_t arrayStride;
    std::vector<VertexAttribute> attributes;
};

class VertexState
{
    wgpu::VertexState state;
    std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts;
    std::string entryPointStorage;

public:
    VertexState(const std::vector<VertexBufferLayout>& layouts, ShaderModule module,
                const std::string& entry = "vs_main");
    const wgpu::VertexState& Get() const;
};

class ColorTargetState : public wgpu::ColorTargetState
{
public:
    ColorTargetState& SetFormat(wgpu::TextureFormat format);

    ColorTargetState& SetBlendState(wgpu::BlendState const* blend);

    ColorTargetState& SetColorWriteMask(wgpu::ColorWriteMask colorWriteMask);
};

class FragmentState
{
    wgpu::FragmentState state;
    std::vector<ColorTargetState> targetsStorage; //管理生命周期
    std::string entryPointStorage;

public:
    const wgpu::FragmentState* Get() const;
    FragmentState(const std::vector<ColorTargetState>& layouts, ShaderModule module,
                  const std::string& entry = "fs_main");
};

struct RenderPipelineDescriptor
{
    VertexState const& vertex;
    FragmentState const& fragment;
    ShaderModule const& shaderModule;
    const std::shared_ptr<NutContext>& context;
};

using WrapMode = wgpu::AddressMode;
using FilterMode = wgpu::FilterMode;
using MipmapFilterMode = wgpu::MipmapFilterMode;

class Sampler
{
private:
    wgpu::SamplerDescriptor m_descriptor;
    wgpu::Sampler m_sampler;
    bool m_isBuild = false;

public:
    Sampler& SetWrapModeU(WrapMode mode);

    Sampler& SetWrapModeW(WrapMode mode);

    Sampler& SetWrapModeV(WrapMode mode);

    Sampler& SetMagFilter(wgpu::FilterMode mag);

    Sampler& SetMinFilter(wgpu::FilterMode mag);

    Sampler& SetMipmapFilter(MipmapFilterMode mode);

    Sampler& SetMaxAnisotropy(size_t size);

    Sampler& SetLodMinClamp(size_t clamp);

    Sampler& SetLodMaxClamp(size_t lod);

    void Build(const std::shared_ptr<NutContext>& ctx);

    wgpu::Sampler Get() const;
};

class Pipeline
{
protected:
    std::vector<std::string> m_shaderBindings;
    std::unordered_map<size_t, std::vector<std::string>> m_groupAttributes;
    std::unordered_map<size_t, BindGroup> m_groups;
    ShaderModule m_shaderModule;

public:
    virtual std::vector<std::string> GetShaderBindings() { return m_shaderBindings; }
    virtual Pipeline& SetBinding(const std::string& name, Sampler& sampler);
    virtual Pipeline& SetBinding(const std::string& name, TextureA& texture);
    virtual Pipeline& SetBinding(const std::string& name, Buffer& buffer, size_t size = 0, size_t offset = 0);
    virtual Pipeline& SetBinding(const char* str, const wgpu::TextureView& loc);
    virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, Sampler& sampler);
    virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, TextureA& texture);
    virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, const wgpu::TextureView& textureView);
    virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, Buffer& buffer, size_t size = 0, size_t offset = 0);
    virtual void ForeachGroup(const std::function<void(size_t, BindGroup&)>& func);
    virtual void BuildBindings(const std::shared_ptr<NutContext>& ctx);
};

class RenderPipeline : public Pipeline
{
private:
    wgpu::RenderPipeline pipeline;

public:
    RenderPipeline(const RenderPipelineDescriptor& desc);
    wgpu::RenderPipeline& Get() { return pipeline; }
};

struct ComputePipelineDescriptor
{
    std::string const& entryPoint;
    ShaderModule& shaderModule;
    const std::shared_ptr<NutContext>& context;
};

class ComputePipeline : public Pipeline
{
    wgpu::ComputePipeline pipeline;

public:
    ComputePipeline(const ComputePipelineDescriptor& desc);
    wgpu::ComputePipeline& Get() { return pipeline; }
};

} // namespace Nut

#endif //NOAI_PIPELINE_H

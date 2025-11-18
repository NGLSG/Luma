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
    virtual Pipeline& SetBinding(const std::string& name, const wgpu::TextureView& texture);
    virtual Pipeline& SetBinding(const std::string& name, Buffer& buffer, size_t size = 0, size_t offset = 0);

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

// ========== 高级Pipeline封装 ==========

/**
 * @brief 深度模板状态配置
 */
class DepthStencilState
{
private:
    wgpu::DepthStencilState m_state{};

public:
    DepthStencilState();

    DepthStencilState& SetFormat(wgpu::TextureFormat format);
    DepthStencilState& SetDepthWriteEnabled(bool enabled);
    DepthStencilState& SetDepthCompare(wgpu::CompareFunction func);
    DepthStencilState& SetStencilFront(const wgpu::StencilFaceState& state);
    DepthStencilState& SetStencilBack(const wgpu::StencilFaceState& state);
    DepthStencilState& SetStencilReadMask(uint32_t mask);
    DepthStencilState& SetStencilWriteMask(uint32_t mask);
    DepthStencilState& SetDepthBias(int32_t bias, float slopeScale = 0.0f, float clamp = 0.0f);

    const wgpu::DepthStencilState* Get() const { return &m_state; }

    // 预设
    static DepthStencilState Default();
    static DepthStencilState DepthOnly();
    static DepthStencilState NoDepth();
};

/**
 * @brief 光栅化状态配置
 */
class RasterizationState
{
private:
    wgpu::PrimitiveState m_primitive{};

public:
    RasterizationState();

    RasterizationState& SetTopology(wgpu::PrimitiveTopology topology);
    RasterizationState& SetStripIndexFormat(wgpu::IndexFormat format);
    RasterizationState& SetFrontFace(wgpu::FrontFace face);
    RasterizationState& SetCullMode(wgpu::CullMode mode);
    RasterizationState& SetUnclippedDepth(bool unclipped);

    const wgpu::PrimitiveState& Get() const { return m_primitive; }

    // 预设
    static RasterizationState Default();
    static RasterizationState Wireframe();
    static RasterizationState NoCull();
};

/**
 * @brief 多重采样状态配置
 */
class MultisampleState
{
private:
    wgpu::MultisampleState m_state{};

public:
    MultisampleState();

    MultisampleState& SetCount(uint32_t count);
    MultisampleState& SetMask(uint32_t mask);
    MultisampleState& SetAlphaToCoverageEnabled(bool enabled);

    const wgpu::MultisampleState& Get() const { return m_state; }

    // 预设
    static MultisampleState None();
    static MultisampleState MSAA4x();
    static MultisampleState MSAA8x();
};

/**
 * @brief 混合状态配置
 */
class BlendState
{
private:
    wgpu::BlendState m_state{};

public:
    BlendState();

    BlendState& SetColor(wgpu::BlendComponent color);
    BlendState& SetAlpha(wgpu::BlendComponent alpha);

    const wgpu::BlendState* Get() const { return &m_state; }

    // 预设
    static BlendState Opaque();
    static BlendState AlphaBlend();
    static BlendState Additive();
    static BlendState Multiply();
};

/**
 * @brief RenderPipeline Builder - 提供流式API构建Pipeline
 */
class RenderPipelineBuilder
{
private:
    std::shared_ptr<NutContext> m_context;
    ShaderModule m_shaderModule;

    // 顶点状态
    std::vector<VertexBufferLayout> m_vertexLayouts;
    std::string m_vertexEntry = "vs_main";

    // 片段状态
    std::vector<ColorTargetState> m_colorTargets;
    std::string m_fragmentEntry = "fs_main";

    // 管线状态
    std::optional<DepthStencilState> m_depthStencil;
    RasterizationState m_rasterization;
    MultisampleState m_multisample;

    std::string m_label;

public:
    explicit RenderPipelineBuilder(const std::shared_ptr<NutContext>& context);

    // Shader配置
    RenderPipelineBuilder& SetShaderModule(const ShaderModule& module);
    RenderPipelineBuilder& SetVertexEntry(const std::string& entry);
    RenderPipelineBuilder& SetFragmentEntry(const std::string& entry);

    // 顶点输入
    RenderPipelineBuilder& AddVertexBuffer(const VertexBufferLayout& layout);
    template<typename VertexType>
    RenderPipelineBuilder& AddVertexBuffer(VertexStepMode stepMode = VertexStepMode::Vertex);

    // 输出目标
    RenderPipelineBuilder& AddColorTarget(wgpu::TextureFormat format, const BlendState* blend = nullptr);
    RenderPipelineBuilder& SetDepthStencil(const DepthStencilState& state);

    // 管线状态
    RenderPipelineBuilder& SetRasterization(const RasterizationState& state);
    RenderPipelineBuilder& SetMultisample(const MultisampleState& state);
    RenderPipelineBuilder& SetPrimitiveTopology(wgpu::PrimitiveTopology topology);
    RenderPipelineBuilder& SetCullMode(wgpu::CullMode mode);

    // 标签
    RenderPipelineBuilder& SetLabel(const std::string& label);

    // 构建
    std::unique_ptr<RenderPipeline> Build();
};

/**
 * @brief Pipeline缓存管理器
 */
class PipelineCache
{
private:
    static std::unordered_map<std::string, std::shared_ptr<RenderPipeline>> s_renderPipelines;
    static std::unordered_map<std::string, std::shared_ptr<ComputePipeline>> s_computePipelines;

public:
    // 缓存管理
    static void CacheRenderPipeline(const std::string& name, std::shared_ptr<RenderPipeline> pipeline);
    static std::shared_ptr<RenderPipeline> GetRenderPipeline(const std::string& name);
    static bool HasRenderPipeline(const std::string& name);

    static void CacheComputePipeline(const std::string& name, std::shared_ptr<ComputePipeline> pipeline);
    static std::shared_ptr<ComputePipeline> GetComputePipeline(const std::string& name);
    static bool HasComputePipeline(const std::string& name);

    // 清理
    static void Clear();
    static void ClearRenderPipelines();
    static void ClearComputePipelines();
};

/**
 * @brief Pipeline预设配置
 */
class PipelinePresets
{
public:
    // 基础渲染管线
    static std::unique_ptr<RenderPipeline> CreateBasic3D(
        const std::shared_ptr<NutContext>& context,
        const ShaderModule& shader,
        wgpu::TextureFormat colorFormat,
        wgpu::TextureFormat depthFormat = wgpu::TextureFormat::Depth24PlusStencil8
    );

    // 2D渲染管线
    static std::unique_ptr<RenderPipeline> Create2DSprite(
        const std::shared_ptr<NutContext>& context,
        const ShaderModule& shader,
        wgpu::TextureFormat colorFormat
    );

    // UI渲染管线
    static std::unique_ptr<RenderPipeline> CreateUI(
        const std::shared_ptr<NutContext>& context,
        const ShaderModule& shader,
        wgpu::TextureFormat colorFormat
    );

    // 后处理管线
    static std::unique_ptr<RenderPipeline> CreatePostProcess(
        const std::shared_ptr<NutContext>& context,
        const ShaderModule& shader,
        wgpu::TextureFormat colorFormat
    );
};

} // namespace Nut

#endif //NOAI_PIPELINE_H

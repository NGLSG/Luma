#ifndef NOAI_PIPELINE_H
#define NOAI_PIPELINE_H
#include <string>
#include <vector>

#include "BindGroup.h"
#include "RenderPass.h"
#include "Shader.h"
#include "ShaderStruct.h"
#include "dawn/webgpu_cpp.h"
#include <glm/glm.hpp>

namespace Nut
{
    class NutContext;

    using VertexFormat = wgpu::VertexFormat;
    using VertexStepMode = wgpu::VertexStepMode;

    class LUMA_API VertexAttribute : public wgpu::VertexAttribute
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
            //Glm推导
            if constexpr (std::is_same_v<T, glm::vec2>)
                return VertexFormat::Float32x2;
            else if constexpr (std::is_same_v<T, glm::vec3>)
                return VertexFormat::Float32x3;
            else if constexpr (std::is_same_v<T, glm::vec4>)
                return VertexFormat::Float32x4;
            else if constexpr (std::is_same_v<T, glm::ivec2>)
                return VertexFormat::Sint32x2;
            else if constexpr (std::is_same_v<T, glm::ivec3>)
                return VertexFormat::Sint32x3;
            else if constexpr (std::is_same_v<T, glm::ivec4>)
                return VertexFormat::Sint32x4;
            else if constexpr (std::is_same_v<T, glm::uvec2>)
                return VertexFormat::Uint32x2;
            else if constexpr (std::is_same_v<T, glm::uvec3>)
                return VertexFormat::Uint32x3;
            else if constexpr (std::is_same_v<T, glm::uvec4>)
                return VertexFormat::Uint32x4;

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

    class LUMA_API VertexBufferLayout
    {
    public:
        VertexStepMode stepMode = VertexStepMode::Vertex;
        uint64_t arrayStride;
        std::vector<VertexAttribute> attributes;
    };

    class LUMA_API VertexState
    {
        wgpu::VertexState state;
        std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts;
        std::string entryPointStorage;

    public:
        VertexState(const std::vector<VertexBufferLayout>& layouts, ShaderModule module,
                    const std::string& entry = "vs_main");
        const wgpu::VertexState& Get() const;
    };

    class LUMA_API ColorTargetState : public wgpu::ColorTargetState
    {
    public:
        ColorTargetState& SetFormat(wgpu::TextureFormat format);

        ColorTargetState& SetBlendState(wgpu::BlendState const* blend);

        ColorTargetState& SetColorWriteMask(wgpu::ColorWriteMask colorWriteMask);
    };

    class LUMA_API FragmentState
    {
        wgpu::FragmentState state;
        std::vector<ColorTargetState> targetsStorage; //管理生命周期
        std::string entryPointStorage;

    public:
        const wgpu::FragmentState* Get() const;
        FragmentState(const std::vector<ColorTargetState>& layouts, ShaderModule module,
                      const std::string& entry = "fs_main");
    };

    /**
     * @brief 深度模板状态配置
     */
    class LUMA_API DepthStencilState
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
    class LUMA_API RasterizationState
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

        static RasterizationState Default();
        static RasterizationState Wireframe();
        static RasterizationState NoCull();
    };

    /**
     * @brief 多重采样状态配置
     */
    class LUMA_API MultisampleState
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
    class LUMA_API BlendState
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


    struct LUMA_API RenderPipelineDescriptor
    {
        VertexState const& vertex;
        FragmentState const& fragment;
        ShaderModule const& shaderModule;
        const std::shared_ptr<NutContext>& context;
        MultisampleState const& multisample;
        RasterizationState const& rasterization;
        std::optional<DepthStencilState> const& depthStencil;
        std::string const& label;
    };

    using WrapMode = wgpu::AddressMode;
    using FilterMode = wgpu::FilterMode;
    using MipmapFilterMode = wgpu::MipmapFilterMode;

    class LUMA_API Sampler
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

    class LUMA_API Pipeline
    {
    protected:
        std::vector<std::string> m_shaderBindings;
        std::unordered_map<size_t, std::vector<std::string>> m_groupAttributes;
        std::unordered_map<size_t, BindGroup> m_groups;
        std::unordered_map<size_t, wgpu::BindGroup> m_bindGroupCache; // 缓存特定绑定组合（主要用于组0的主纹理+采样器）
        ShaderModule m_shaderModule;
        std::shared_ptr<Buffer> m_reservedFrameBuffer;
        std::shared_ptr<Buffer> m_reservedInstanceBuffer;
        
        // 占位 Buffer 管理
        std::unordered_map<std::string, std::shared_ptr<Buffer>> m_placeholderBuffers;
        std::unordered_map<std::string, ShaderBindingInfo> m_bindingInfoMap;
        std::shared_ptr<NutContext> m_context;

        size_t ComputeBindGroupKey(size_t groupIdx, const BindGroup& group) const;
        
        // 为 UniformBuffer 创建占位 buffer
        void CreatePlaceholderBuffers();

    public:
        virtual std::vector<std::string> GetShaderBindings() { return m_shaderBindings; }
        virtual Pipeline& SetBinding(const std::string& name, Sampler& sampler);
        virtual Pipeline& SetBinding(const std::string& name, const TextureAPtr& texture);
        virtual Pipeline& SetBinding(const std::string& name, const wgpu::TextureView& texture);
        virtual Pipeline& SetBinding(const std::string& name, Buffer& buffer, size_t size = 0, size_t offset = 0);

        virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, Sampler& sampler);
        virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, const TextureAPtr& texture);
        virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, const wgpu::TextureView& textureView);
        virtual Pipeline& SetBinding(size_t groupIdx, size_t loc, Buffer& buffer, size_t size = 0, size_t offset = 0);
        virtual void ForeachGroup(const std::function<void(size_t, BindGroup&)>& func);
        virtual void BuildBindings(const std::shared_ptr<NutContext>& ctx);
        std::shared_ptr<Buffer> GetReservedFrameBuffer() { return m_reservedFrameBuffer; }
        std::shared_ptr<Buffer> GetReservedInstanceBuffer() { return m_reservedInstanceBuffer; }

        // 清理缓存（当资源整体变化时可调用）
        void ClearBindGroupCache() { m_bindGroupCache.clear(); }
        
        // 清除指定组的所有绑定条目
        void ClearBindGroupEntries(size_t groupIdx);
        
        // 移除指定组的指定绑定条目
        void RemoveBindGroupEntry(size_t groupIdx, uint32_t bindingIndex);

        // 设置引擎保留的缓冲区（group0/binding0,1），并清理缓存以防缓冲区重建后句柄变化。
        void SetReservedBuffers(const EngineData& engineData, std::vector<InstanceData>& instanceData,
                                const std::shared_ptr<NutContext>& ctx);

        /**
         * @brief 交换 group0 的纹理和采样器，同时保持所有 uniform buffer 绑定
         * @note 用于引擎保留的 group0：binding2 (texture), binding3 (sampler)
         *       会自动绑定 m_reservedFrameBuffer, m_reservedInstanceBuffer 和所有占位 uniform buffer
         *       缓存命中则复用，未命中则创建
         */
        bool SwapTexture(const TextureAPtr& texture, Sampler* sampler, const std::shared_ptr<NutContext>& ctx);
        
        /**
         * @brief 通过名称获取 uniform buffer
         * @param name binding 名称
         * @return Buffer 指针，如果不存在返回 nullptr
         */
        std::shared_ptr<Buffer> GetUniformBuffer(const std::string& name);
        
        /**
         * @brief 更新 uniform buffer 数据，如果 size 变化则自动重建 buffer 和 bindgroup
         * @param name binding 名称
         * @param data 数据指针
         * @param size 数据大小（字节）
         * @return 是否成功
         */
        bool UpdateUniformBuffer(const std::string& name, const void* data, size_t size);
    };

    class LUMA_API RenderPipeline : public Pipeline
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

    class LUMA_API ComputePipeline : public Pipeline
    {
        wgpu::ComputePipeline pipeline;

    public:
        ComputePipeline(const ComputePipelineDescriptor& desc);
        wgpu::ComputePipeline& Get() { return pipeline; }
    };

    /**
     * @brief RenderPipeline Builder - 提供流式API构建Pipeline
     */
    class LUMA_API RenderPipelineBuilder
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
        template <typename VertexType>
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
    class LUMA_API PipelineCache
    {
    private:
        static std::unordered_map<std::string, std::shared_ptr<RenderPipeline>> s_renderPipelines;
        static std::unordered_map<std::string, std::shared_ptr<ComputePipeline>> s_computePipelines;

    public:
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
} // namespace Nut

#endif //NOAI_PIPELINE_H

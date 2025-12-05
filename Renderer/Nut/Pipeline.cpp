#include "Pipeline.h"

#include <iostream>
#include <ranges>

#include "Logger.h"
#include "NutContext.h"
#include "Shader.h"
#include "ShaderStruct.h"

namespace Nut
{
    VertexAttribute& VertexAttribute::SetLocation(size_t loc)
    {
        this->shaderLocation = loc;
        return *this;
    }

    VertexAttribute& VertexAttribute::SetFormat(VertexFormat format)
    {
        this->format = format;
        return *this;
    }

    VertexAttribute& VertexAttribute::SetOffset(size_t offset)
    {
        this->offset = offset;
        return *this;
    }

    VertexState::VertexState(const std::vector<VertexBufferLayout>& layouts, ShaderModule module,
                             const std::string& entry)
    {
        entryPointStorage = entry;
        state.entryPoint = entryPointStorage.c_str();
        state.module = module.Get();
        for (auto& item : layouts)
        {
            wgpu::VertexBufferLayout layout = wgpu::VertexBufferLayout();
            layout.arrayStride = item.arrayStride;
            layout.stepMode = item.stepMode;
            layout.attributeCount = item.attributes.size();
            layout.attributes = item.attributes.data();
            vertexBufferLayouts.push_back(layout);
        }
        state.bufferCount = layouts.size();
        state.buffers = vertexBufferLayouts.data();
    }

    const wgpu::VertexState& VertexState::Get() const
    {
        return state;
    }

    ColorTargetState& ColorTargetState::SetFormat(wgpu::TextureFormat format)
    {
        this->format = format;
        return *this;
    }

    ColorTargetState& ColorTargetState::SetBlendState(wgpu::BlendState const* blend)
    {
        this->blend = blend;
        return *this;
    }

    ColorTargetState& ColorTargetState::SetColorWriteMask(wgpu::ColorWriteMask colorWriteMask)
    {
        this->writeMask = colorWriteMask;
        return *this;
    }


    const wgpu::FragmentState* FragmentState::Get() const
    {
        return &state;
    }

    FragmentState::FragmentState(const std::vector<ColorTargetState>& layouts, ShaderModule module,
                                 const std::string& entry)
    {
        entryPointStorage = entry;
        state.entryPoint = entryPointStorage.c_str();
        state.module = module.Get();
        targetsStorage = layouts;
        state.targetCount = targetsStorage.size();
        state.targets = targetsStorage.data();
    }

    Sampler& Sampler::SetWrapModeU(WrapMode mode)
    {
        m_descriptor.addressModeU = mode;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetWrapModeW(WrapMode mode)
    {
        m_descriptor.addressModeW = mode;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetWrapModeV(WrapMode mode)
    {
        m_descriptor.addressModeV = mode;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetMagFilter(wgpu::FilterMode mag)
    {
        m_descriptor.magFilter = mag;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetMinFilter(wgpu::FilterMode mag)
    {
        m_descriptor.magFilter = mag;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetMipmapFilter(MipmapFilterMode mode)
    {
        m_descriptor.mipmapFilter = mode;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetMaxAnisotropy(size_t size)
    {
        m_descriptor.maxAnisotropy = size;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetLodMinClamp(size_t clamp)
    {
        m_descriptor.lodMinClamp = clamp;
        m_isBuild = false;
        return *this;
    }

    Sampler& Sampler::SetLodMaxClamp(size_t lod)
    {
        m_descriptor.lodMaxClamp = lod;
        m_isBuild = false;
        return *this;
    }

    void Sampler::Build(const std::shared_ptr<NutContext>& ctx)
    {
        if (!ctx)
        {
            std::cerr << "Failed to create sampler context." << std::endl;
        }
        m_isBuild = true;
        m_sampler = nullptr;
        m_sampler = ctx->CreateSampler(&m_descriptor);
    }

    wgpu::Sampler Sampler::Get() const
    {
        if (!m_isBuild)
        {
            std::cerr << "Please call the Build function first to build, and then call this function" << std::endl;
        }
        return m_sampler;
    }

    Pipeline& Pipeline::SetBinding(const std::string& name, Sampler& sampler)
    {
        ShaderBindingInfo binding;
        if (!m_shaderModule.GetBindingInfo(name, binding))
        {
            std::cerr << "Failed to get binding " << name << std::endl;
            return *this;
        }
        m_groups[binding.groupIndex].SetSampler(binding.location, sampler);
        return *this;
    }

    Pipeline& Pipeline::SetBinding(const std::string& name, const TextureAPtr& texture)
    {
        ShaderBindingInfo binding;
        if (!m_shaderModule.GetBindingInfo(name, binding))
        {
            std::cerr << "Failed to get binding " << name << std::endl;
            return *this;
        }
        m_groups[binding.groupIndex].SetTexture(binding.location, texture);
        return *this;
    }


    Pipeline& Pipeline::SetBinding(const std::string& name, Buffer& buffer, size_t size, size_t offset)
    {
        ShaderBindingInfo binding;
        if (!m_shaderModule.GetBindingInfo(name, binding))
        {
            std::cerr << "Failed to get binding " << name << std::endl;
            return *this;
        }
        m_groups[binding.groupIndex].SetBuffer(binding.location, buffer, size, offset);
        return *this;
    }

    Pipeline& Pipeline::SetBinding(const std::string& name, const wgpu::TextureView& texture)
    {
        ShaderBindingInfo binding;
        if (!m_shaderModule.GetBindingInfo(name, binding))
        {
            std::cerr << "Failed to get binding " << name << std::endl;
            return *this;
        }
        m_groups[binding.groupIndex].SetTextureView(binding.location, texture);
        return *this;
    }

    Pipeline& Pipeline::SetBinding(size_t groupIdx, size_t loc, Sampler& sampler)
    {
        if (m_groups.contains(groupIdx))
        {
            m_groups[groupIdx].SetSampler(loc, sampler);
        }
        else
        {
            std::cerr << "Group " << groupIdx << " does not exist" << std::endl;
        }
        return *this;
    }

    Pipeline& Pipeline::SetBinding(size_t groupIdx, size_t loc, const TextureAPtr& texture)
    {
        if (m_groups.contains(groupIdx))
        {
            m_groups[groupIdx].SetTexture(loc, texture);
        }
        else
        {
            std::cerr << "Group " << groupIdx << " does not exist" << std::endl;
        }
        return *this;
    }

    Pipeline& Pipeline::SetBinding(size_t groupIdx, size_t loc, const wgpu::TextureView& textureView)
    {
        if (m_groups.contains(groupIdx))
        {
            m_groups[groupIdx].SetTextureView(loc, textureView);
        }
        else
        {
            std::cerr << "Group " << groupIdx << " does not exist" << std::endl;
        }
        return *this;
    }

    Pipeline& Pipeline::SetBinding(size_t groupIdx, size_t loc, Buffer& buffer, size_t size, size_t offset)
    {
        if (m_groups.contains(groupIdx))
        {
            m_groups[groupIdx].SetBuffer(loc, buffer, size, offset);
        }
        else
        {
            std::cerr << "Group " << groupIdx << " does not exist" << std::endl;
        }
        return *this;
    }

    void Pipeline::ForeachGroup(const std::function<void(size_t, BindGroup&)>& func)
    {
        for (auto& group : m_groups)
        {
            func(group.first, group.second);
        }
    }

    void Pipeline::BuildBindings(const std::shared_ptr<NutContext>& ctx)
    {
        for (auto& group : m_groups)
        {
            const size_t key = ComputeBindGroupKey(group.first, group.second);
            if (key != 0)
            {
                auto it = m_bindGroupCache.find(key);
                if (it != m_bindGroupCache.end())
                {
                    group.second.OverrideBindGroup(it->second);
                    continue;
                }
            }

            group.second.Build(ctx);

            if (key != 0)
            {
                m_bindGroupCache[key] = group.second.RawBindGroup();
            }
        }
    }

    void Pipeline::SetReservedBuffers(const EngineData& engineData, std::vector<InstanceData>& instanceData,
                                      const std::shared_ptr<NutContext>& ctx)
    {
        if (!m_reservedFrameBuffer)
        {
            m_reservedFrameBuffer = BufferBuilder().SetUsage(BufferBuilder::GetCommonUniformUsage())
                                                   .SetData(&engineData)
                                                   .BuildPtr(ctx);
        }
        else
        {
            m_reservedFrameBuffer->WriteBuffer(&engineData, sizeof(EngineData));
        }


        if (!m_reservedInstanceBuffer)
        {
            m_reservedInstanceBuffer = BufferBuilder().SetUsage(BufferBuilder::GetCommonInstanceUsage())
                                                      .SetData(instanceData)
                                                      .BuildPtr(ctx);
        }
        else
        {
            if (m_reservedInstanceBuffer->GetSize() < sizeof(InstanceData) * instanceData.size())
            {
                m_reservedInstanceBuffer = nullptr;
                ClearBindGroupCache();
                m_reservedInstanceBuffer = BufferBuilder().SetUsage(BufferBuilder::GetCommonInstanceUsage())
                                                          .SetData(instanceData)
                                                          .BuildPtr(ctx);
            }
            else
            {
                m_reservedInstanceBuffer->WriteBuffer(instanceData.data(),
                                                      sizeof(InstanceData) * instanceData.size());
            }
        }
    }


    bool Pipeline::SwapTexture(const TextureAPtr& texture, Sampler* sampler, const std::shared_ptr<NutContext>& ctx)
    {
        if (!texture || !ctx)
        {
            return false;
        }

        constexpr size_t groupIdx = 0;
        constexpr size_t texLoc = 2;
        constexpr size_t samplerLoc = 3;

        auto it = m_groups.find(groupIdx);
        if (it == m_groups.end())
        {
            LogError("Pipeline::SwapTexture failed: group 0 not found");
            return false;
        }


        if (!m_reservedFrameBuffer)
        {
            auto data = EngineData();
            m_reservedFrameBuffer = BufferBuilder()
                                    .SetUsage(BufferBuilder::GetCommonUniformUsage())

                                    .SetData(&data)
                                    .BuildPtr(ctx);
        }


        if (!m_reservedInstanceBuffer)
        {
            auto datas = std::vector<InstanceData>(1);
            m_reservedInstanceBuffer = BufferBuilder()
                                       .SetUsage(BufferBuilder::GetCommonInstanceUsage())
                                       .SetData(datas)
                                       .BuildPtr(ctx);
        }


        if (m_reservedFrameBuffer)
        {
            it->second.SetBuffer(0, *m_reservedFrameBuffer);
        }
        if (m_reservedInstanceBuffer)
        {
            it->second.SetBuffer(1, *m_reservedInstanceBuffer);
        }

        
        for (const auto& [name, buffer] : m_placeholderBuffers)
        {
            auto infoIt = m_bindingInfoMap.find(name);
            if (infoIt != m_bindingInfoMap.end())
            {
                const auto& info = infoIt->second;
                
                if (info.groupIndex == groupIdx && info.location != 0 && info.location != 1)
                {
                    it->second.SetBuffer(info.location, *buffer);
                }
            }
        }

        it->second.SetTexture(texLoc, texture);
        if (sampler)
        {
            it->second.SetSampler(samplerLoc, *sampler);
        }

        size_t key = ComputeBindGroupKey(groupIdx, it->second);
        if (key != 0)
        {
            auto cacheIt = m_bindGroupCache.find(key);
            if (cacheIt != m_bindGroupCache.end())
            {
                it->second.OverrideBindGroup(cacheIt->second);
                return true;
            }
        }

        it->second.Build(ctx);
        if (key != 0)
        {
            m_bindGroupCache[key] = it->second.RawBindGroup();
        }
        return true;
    }

    size_t Pipeline::ComputeBindGroupKey(size_t groupIdx, const BindGroup& group) const
    {
        if (groupIdx != 0)
        {
            return 0;
        }

        const auto& entries = group.GetEntries();
        if (entries.empty())
        {
            return 0;
        }


        size_t hash = 1469598103934665603ull;
        auto mix = [&hash](size_t v)
        {
            hash ^= v + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
        };

        bool hasResource = false;
        for (const auto& e : entries)
        {
            if (e.buffer || e.textureView || e.sampler)
            {
                hasResource = true;
            }
            mix(reinterpret_cast<size_t>(e.buffer.Get()));
            mix(reinterpret_cast<size_t>(e.textureView.Get()));
            mix(reinterpret_cast<size_t>(e.sampler.Get()));
            // 包含 offset 和 size 以区分不同的 buffer 绑定
            mix(static_cast<size_t>(e.offset));
            mix(static_cast<size_t>(e.size));
        }

        return hasResource ? hash : 0;
    }

    RenderPipeline::RenderPipeline(const RenderPipelineDescriptor& desc)
    {
        if (!desc.context)
        {
            LogError("Context is null");
            return;
        }
        wgpu::RenderPipelineDescriptor descriptor{};
        descriptor.fragment = desc.fragment.Get();
        descriptor.vertex = desc.vertex.Get();
        descriptor.primitive = desc.rasterization.Get();
        if (desc.depthStencil.has_value())
        {
            descriptor.depthStencil = desc.depthStencil->Get();
        }
        descriptor.multisample = desc.multisample.Get();
        descriptor.label = desc.label.c_str();


        if (!desc.context->GetWGPUDevice())
        {
            LogError("WGPU Device is null");
            return;
        }
        pipeline = desc.context->GetWGPUDevice().CreateRenderPipeline(&descriptor);
        m_context = desc.context;
        desc.shaderModule.ForeachBinding([this](const ShaderBindingInfo& info)
        {
            this->m_shaderBindings.emplace_back(info.name);
            this->m_groupAttributes[info.groupIndex].emplace_back(info.name);
            this->m_bindingInfoMap[info.name] = info;
        });
        for (const auto& groupId : m_groupAttributes | std::views::keys)
        {
            m_groups[groupId] = BindGroup::Create(groupId, this);
        }
        m_shaderModule = desc.shaderModule;

        
        CreatePlaceholderBuffers();
    }

    ComputePipeline::ComputePipeline(const ComputePipelineDescriptor& desc)
    {
        if (!desc.context)
        {
            LogError("Context is null");
            return;
        }
        wgpu::ComputePipelineDescriptor descriptor{};
        descriptor.compute.entryPoint = desc.entryPoint.c_str();
        descriptor.compute.module = desc.shaderModule.Get();
        if (!desc.context->GetWGPUDevice())
        {
            LogError("WGPU Device is null");
            return;
        }
        pipeline = desc.context->GetWGPUDevice().CreateComputePipeline(&descriptor);
        m_context = desc.context;
        desc.shaderModule.ForeachBinding([this](const ShaderBindingInfo& info)
        {
            this->m_shaderBindings.emplace_back(info.name);
            this->m_groupAttributes[info.groupIndex].emplace_back(info.name);
            this->m_bindingInfoMap[info.name] = info;
        });
        for (const auto& groupId : m_groupAttributes | std::views::keys)
        {
            m_groups[groupId] = BindGroup::Create(groupId, this);
        }
        m_shaderModule = desc.shaderModule;

        
        CreatePlaceholderBuffers();
    }


    DepthStencilState::DepthStencilState()
    {
        m_state.format = wgpu::TextureFormat::Depth24PlusStencil8;
        m_state.depthWriteEnabled = true;
        m_state.depthCompare = wgpu::CompareFunction::Less;
        m_state.stencilReadMask = 0xFFFFFFFF;
        m_state.stencilWriteMask = 0xFFFFFFFF;
    }

    DepthStencilState& DepthStencilState::SetFormat(wgpu::TextureFormat format)
    {
        m_state.format = format;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetDepthWriteEnabled(bool enabled)
    {
        m_state.depthWriteEnabled = enabled;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetDepthCompare(wgpu::CompareFunction func)
    {
        m_state.depthCompare = func;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetStencilFront(const wgpu::StencilFaceState& state)
    {
        m_state.stencilFront = state;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetStencilBack(const wgpu::StencilFaceState& state)
    {
        m_state.stencilBack = state;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetStencilReadMask(uint32_t mask)
    {
        m_state.stencilReadMask = mask;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetStencilWriteMask(uint32_t mask)
    {
        m_state.stencilWriteMask = mask;
        return *this;
    }

    DepthStencilState& DepthStencilState::SetDepthBias(int32_t bias, float slopeScale, float clamp)
    {
        m_state.depthBias = bias;
        m_state.depthBiasSlopeScale = slopeScale;
        m_state.depthBiasClamp = clamp;
        return *this;
    }

    DepthStencilState DepthStencilState::Default()
    {
        return DepthStencilState();
    }

    DepthStencilState DepthStencilState::DepthOnly()
    {
        DepthStencilState state;
        state.SetFormat(wgpu::TextureFormat::Depth32Float);
        return state;
    }

    DepthStencilState DepthStencilState::NoDepth()
    {
        DepthStencilState state;
        state.SetDepthWriteEnabled(false);
        state.SetDepthCompare(wgpu::CompareFunction::Always);
        return state;
    }


    RasterizationState::RasterizationState()
    {
        m_primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        m_primitive.frontFace = wgpu::FrontFace::CCW;
        m_primitive.cullMode = wgpu::CullMode::Back;
    }

    RasterizationState& RasterizationState::SetTopology(wgpu::PrimitiveTopology topology)
    {
        m_primitive.topology = topology;
        return *this;
    }

    RasterizationState& RasterizationState::SetStripIndexFormat(wgpu::IndexFormat format)
    {
        m_primitive.stripIndexFormat = format;
        return *this;
    }

    RasterizationState& RasterizationState::SetFrontFace(wgpu::FrontFace face)
    {
        m_primitive.frontFace = face;
        return *this;
    }

    RasterizationState& RasterizationState::SetCullMode(wgpu::CullMode mode)
    {
        m_primitive.cullMode = mode;
        return *this;
    }

    RasterizationState& RasterizationState::SetUnclippedDepth(bool unclipped)
    {
        m_primitive.unclippedDepth = unclipped;
        return *this;
    }

    RasterizationState RasterizationState::Default()
    {
        return RasterizationState();
    }

    RasterizationState RasterizationState::Wireframe()
    {
        RasterizationState state;
        state.SetTopology(wgpu::PrimitiveTopology::LineList);
        return state;
    }

    RasterizationState RasterizationState::NoCull()
    {
        RasterizationState state;
        state.SetCullMode(wgpu::CullMode::None);
        return state;
    }


    MultisampleState::MultisampleState()
    {
        m_state.count = 1;
        m_state.mask = 0xFFFFFFFF;
        m_state.alphaToCoverageEnabled = false;
    }

    MultisampleState& MultisampleState::SetCount(uint32_t count)
    {
        m_state.count = count;
        return *this;
    }

    MultisampleState& MultisampleState::SetMask(uint32_t mask)
    {
        m_state.mask = mask;
        return *this;
    }

    MultisampleState& MultisampleState::SetAlphaToCoverageEnabled(bool enabled)
    {
        m_state.alphaToCoverageEnabled = enabled;
        return *this;
    }

    MultisampleState MultisampleState::None()
    {
        return MultisampleState();
    }

    MultisampleState MultisampleState::MSAA4x()
    {
        MultisampleState state;
        state.SetCount(4);
        return state;
    }

    MultisampleState MultisampleState::MSAA8x()
    {
        MultisampleState state;
        state.SetCount(8);
        return state;
    }


    BlendState::BlendState()
    {
        m_state.color.operation = wgpu::BlendOperation::Add;
        m_state.color.srcFactor = wgpu::BlendFactor::One;
        m_state.color.dstFactor = wgpu::BlendFactor::Zero;

        m_state.alpha.operation = wgpu::BlendOperation::Add;
        m_state.alpha.srcFactor = wgpu::BlendFactor::One;
        m_state.alpha.dstFactor = wgpu::BlendFactor::Zero;
    }

    BlendState& BlendState::SetColor(wgpu::BlendComponent color)
    {
        m_state.color = color;
        return *this;
    }

    BlendState& BlendState::SetAlpha(wgpu::BlendComponent alpha)
    {
        m_state.alpha = alpha;
        return *this;
    }

    BlendState BlendState::Opaque()
    {
        return BlendState();
    }

    BlendState BlendState::AlphaBlend()
    {
        BlendState state;
        state.m_state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        state.m_state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        state.m_state.alpha.srcFactor = wgpu::BlendFactor::One;
        state.m_state.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        return state;
    }

    BlendState BlendState::Additive()
    {
        BlendState state;
        state.m_state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        state.m_state.color.dstFactor = wgpu::BlendFactor::One;
        state.m_state.alpha.srcFactor = wgpu::BlendFactor::One;
        state.m_state.alpha.dstFactor = wgpu::BlendFactor::One;
        return state;
    }

    BlendState BlendState::Multiply()
    {
        BlendState state;
        state.m_state.color.srcFactor = wgpu::BlendFactor::Dst;
        state.m_state.color.dstFactor = wgpu::BlendFactor::Zero;
        state.m_state.alpha.srcFactor = wgpu::BlendFactor::Dst;
        state.m_state.alpha.dstFactor = wgpu::BlendFactor::Zero;
        return state;
    }


    RenderPipelineBuilder::RenderPipelineBuilder(const std::shared_ptr<NutContext>& context)
        : m_context(context)
    {
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetShaderModule(const ShaderModule& module)
    {
        m_shaderModule = module;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetVertexEntry(const std::string& entry)
    {
        m_vertexEntry = entry;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetFragmentEntry(const std::string& entry)
    {
        m_fragmentEntry = entry;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::AddVertexBuffer(const VertexBufferLayout& layout)
    {
        m_vertexLayouts.push_back(layout);
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::AddColorTarget(wgpu::TextureFormat format, const BlendState* blend)
    {
        ColorTargetState target;
        target.SetFormat(format);
        if (blend)
        {
            target.SetBlendState(blend->Get());
        }
        m_colorTargets.push_back(target);
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetDepthStencil(const DepthStencilState& state)
    {
        m_depthStencil = state;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetRasterization(const RasterizationState& state)
    {
        m_rasterization = state;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetMultisample(const MultisampleState& state)
    {
        m_multisample = state;
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetPrimitiveTopology(wgpu::PrimitiveTopology topology)
    {
        m_rasterization.SetTopology(topology);
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetCullMode(wgpu::CullMode mode)
    {
        m_rasterization.SetCullMode(mode);
        return *this;
    }

    RenderPipelineBuilder& RenderPipelineBuilder::SetLabel(const std::string& label)
    {
        m_label = label;
        return *this;
    }

    std::unique_ptr<RenderPipeline> RenderPipelineBuilder::Build()
    {
        if (!m_shaderModule)
        {
            LogError("Build failed: ShaderModule is null");
            return nullptr;
        }

        if (m_colorTargets.empty())
        {
            LogError("Build failed: No color targets specified");
            return nullptr;
        }

        VertexState vertex(m_vertexLayouts, m_shaderModule, m_vertexEntry);
        FragmentState fragment(m_colorTargets, m_shaderModule, m_fragmentEntry);

        RenderPipelineDescriptor pipelineDesc{
            vertex, fragment, m_shaderModule, m_context,
            m_multisample, m_rasterization, m_depthStencil, m_label
        };
        auto pipeline = std::make_unique<RenderPipeline>(pipelineDesc);

        return pipeline;
    }


    std::unordered_map<std::string, std::shared_ptr<RenderPipeline>> PipelineCache::s_renderPipelines;
    std::unordered_map<std::string, std::shared_ptr<ComputePipeline>> PipelineCache::s_computePipelines;

    void PipelineCache::CacheRenderPipeline(const std::string& name, std::shared_ptr<RenderPipeline> pipeline)
    {
        s_renderPipelines[name] = std::move(pipeline);
    }

    std::shared_ptr<RenderPipeline> PipelineCache::GetRenderPipeline(const std::string& name)
    {
        auto it = s_renderPipelines.find(name);
        if (it != s_renderPipelines.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool PipelineCache::HasRenderPipeline(const std::string& name)
    {
        return s_renderPipelines.contains(name);
    }

    void PipelineCache::CacheComputePipeline(const std::string& name, std::shared_ptr<ComputePipeline> pipeline)
    {
        s_computePipelines[name] = std::move(pipeline);
    }

    std::shared_ptr<ComputePipeline> PipelineCache::GetComputePipeline(const std::string& name)
    {
        auto it = s_computePipelines.find(name);
        if (it != s_computePipelines.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool PipelineCache::HasComputePipeline(const std::string& name)
    {
        return s_computePipelines.contains(name);
    }

    void PipelineCache::Clear()
    {
        s_renderPipelines.clear();
        s_computePipelines.clear();
    }

    void PipelineCache::ClearRenderPipelines()
    {
        s_renderPipelines.clear();
    }

    void Pipeline::CreatePlaceholderBuffers()
    {
        if (!m_context)
        {
            LogError("Context is null");
            return;
        }

        for (const auto& [name, info] : m_bindingInfoMap)
        {
            
            if (info.type != BindingType::UniformBuffer && info.type != BindingType::StorageBuffer)
                continue;

            if (info.groupIndex == 0 && (info.location == 0 || info.location == 1))
            {
                continue;
            }

            size_t bufferSize = info.size;

            
            if (bufferSize == 0)
            {
                
                bufferSize = 1024;
            }

            
            std::vector<uint8_t> placeholderData(bufferSize, 0);

            
            auto usage = (info.type == BindingType::UniformBuffer)
                             ? BufferUsage::Uniform | BufferUsage::CopyDst
                             : BufferUsage::Storage | BufferUsage::CopyDst;


            auto buffer = BufferBuilder().SetData(placeholderData)
                                         .SetUsage(usage)
                                         .BuildPtr(m_context);

            if (buffer)
            {
                
                SetBinding(name, *buffer, 0, 0);
                m_placeholderBuffers[name] = std::move(buffer);
            }
            else
            {
                LogError("Failed to create placeholder buffer");
            }
        }
    }

    std::shared_ptr<Buffer> Pipeline::GetUniformBuffer(const std::string& name)
    {
        auto it = m_placeholderBuffers.find(name);
        if (it != m_placeholderBuffers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool Pipeline::UpdateUniformBuffer(const std::string& name, const void* data, size_t size)
    {
        if (!data || size == 0)
        {
            LogError("Invalid data or size for uniform buffer: {}", name);
            return false;
        }

        auto it = m_bindingInfoMap.find(name);
        if (it == m_bindingInfoMap.end())
        {
            LogError("Binding info not found for uniform buffer: {}", name);
            return false;
        }

        const auto& info = it->second;
        if (info.type != BindingType::UniformBuffer && info.type != BindingType::StorageBuffer)
        {
            LogError("Binding {} is not a uniform or storage buffer", name);
            return false;
        }

        auto bufferIt = m_placeholderBuffers.find(name);
        bool needRebuild = false;

        
        if (bufferIt == m_placeholderBuffers.end() || bufferIt->second->GetSize() < size)
        {
            needRebuild = true;
        }

        if (needRebuild)
        {
            
            BufferLayout layout;
            layout.usage = (info.type == BindingType::UniformBuffer)
                               ? BufferUsage::Uniform | BufferUsage::CopyDst
                               : BufferUsage::Storage | BufferUsage::CopyDst;
            layout.size = static_cast<uint32_t>(size);
            layout.mapped = false;

            auto newBuffer = Buffer::Create(layout, m_context);
            if (!newBuffer)
            {
                LogError("Failed to create new buffer");
                return false;
            }

            
            SetBinding(name, *newBuffer, 0, 0);
            m_placeholderBuffers[name] = std::move(newBuffer);

            
            ClearBindGroupCache();
        }

        
        auto& buffer = m_placeholderBuffers[name];
        buffer->WriteBuffer(data, size);

        return true;
    }

    void PipelineCache::ClearComputePipelines()
    {
        s_computePipelines.clear();
    }
}

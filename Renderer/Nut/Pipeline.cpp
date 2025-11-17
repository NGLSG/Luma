#include "Pipeline.h"

#include <iostream>
#include <ranges>

#include "NutContext.h"
#include "Shader.h"


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

VertexState::VertexState(const std::vector<VertexBufferLayout>& layouts, ShaderModule module, const std::string& entry)
{
    entryPointStorage = entry; // ensure lifetime
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

// In FragmentState class definition
const wgpu::FragmentState* FragmentState::Get() const
{
    return &state;
}

FragmentState::FragmentState(const std::vector<ColorTargetState>& layouts, ShaderModule module,
                             const std::string& entry)
{
    entryPointStorage = entry; // ensure lifetime
    state.entryPoint = entryPointStorage.c_str();
    state.module = module.Get();
    targetsStorage = layouts; // own the targets to keep pointers valid
    state.targetCount = targetsStorage.size();
    state.targets = targetsStorage.data();
}

class Sampler& Sampler::SetWrapModeU(WrapMode mode)
{
    m_descriptor.addressModeU = mode;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetWrapModeW(WrapMode mode)
{
    m_descriptor.addressModeW = mode;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetWrapModeV(WrapMode mode)
{
    m_descriptor.addressModeV = mode;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetMagFilter(wgpu::FilterMode mag)
{
    m_descriptor.magFilter = mag;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetMinFilter(wgpu::FilterMode mag)
{
    m_descriptor.magFilter = mag;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetMipmapFilter(MipmapFilterMode mode)
{
    m_descriptor.mipmapFilter = mode;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetMaxAnisotropy(size_t size)
{
    m_descriptor.maxAnisotropy = size;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetLodMinClamp(size_t clamp)
{
    m_descriptor.lodMinClamp = clamp;
    m_isBuild = false;
    return *this;
}

class Sampler& Sampler::SetLodMaxClamp(size_t lod)
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

const wgpu::Sampler& Sampler::Get()
{
    if (m_isBuild)return m_sampler;
    std::cerr << "Please call the Build function first to build, and then call this function" << std::endl;
    return nullptr;
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

Pipeline& Pipeline::SetBinding(const std::string& name, TextureA& texture)
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

Pipeline&Pipeline::SetBinding(const char* str, const wgpu::TextureView& loc)
{
    ShaderBindingInfo binding;
    if (!m_shaderModule.GetBindingInfo(str, binding))
    {
        std::cerr << "Failed to get binding " << str << std::endl;
        return *this;
    }
    m_groups[binding.groupIndex].SetTextureView(binding.location, loc);
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

Pipeline& Pipeline::SetBinding(size_t groupIdx, size_t loc, TextureA& texture)
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
        group.second.Build(ctx);
    }
}

RenderPipeline::RenderPipeline(const RenderPipelineDescriptor& desc)
{
    if (!desc.context)
    {
        std::cerr << "GraphicsContext is null" << std::endl;
        return;
    }
    wgpu::RenderPipelineDescriptor descriptor{};
    descriptor.fragment = desc.fragment.Get();
    descriptor.vertex = desc.vertex.Get();
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

    if (!desc.context->GetWGPUDevice())
    {
        std::cerr << "WGPU Device is null" << std::endl;
        return;
    }
    pipeline = desc.context->GetWGPUDevice().CreateRenderPipeline(&descriptor);
    desc.shaderModule.ForeachBinding([this](const ShaderBindingInfo& info)
    {
        this->m_shaderBindings.emplace_back(info.name);
        this->m_groupAttributes[info.groupIndex].emplace_back(info.name);
    });
    for (const auto& groupId : m_groupAttributes | std::views::keys)
    {
        m_groups[groupId] = BindGroup::Create(groupId, this);
    }
    m_shaderModule = desc.shaderModule;
}

ComputePipeline::ComputePipeline(const ComputePipelineDescriptor& desc)
{
    if (!desc.context)
    {
        std::cerr << "GraphicsContext is null" << std::endl;
        return;
    }
    wgpu::ComputePipelineDescriptor descriptor{};
    descriptor.compute.entryPoint = desc.entryPoint.c_str();
    descriptor.compute.module = desc.shaderModule.Get();
    if (!desc.context->GetWGPUDevice())
    {
        std::cerr << "WGPU Device is null" << std::endl;
        return;
    }
    pipeline = desc.context->GetWGPUDevice().CreateComputePipeline(&descriptor);

    desc.shaderModule.ForeachBinding([this](const ShaderBindingInfo& info)
    {
        this->m_shaderBindings.emplace_back(info.name);
        this->m_groupAttributes[info.groupIndex].emplace_back(info.name);
    });
    for (const auto& groupId : m_groupAttributes | std::views::keys)
    {
        m_groups[groupId] = BindGroup::Create(groupId, this);
    }
    m_shaderModule = desc.shaderModule;
}

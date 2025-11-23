#include "BindGroup.h"

#include <iostream>
#include <ostream>

#include "NutContext.h"
#include "Pipeline.h"

namespace Nut {

BindGroup BindGroup::Create(uint32_t groupIndex, RenderPipeline* Pipeline)
{
    BindGroup group;
    group.m_bindGroupLayout = Pipeline->Get().GetBindGroupLayout(groupIndex);
    return group;
}

BindGroup BindGroup::Create(uint32_t groupIndex, ComputePipeline* Pipeline)
{
    BindGroup group;
    group.m_bindGroupLayout = Pipeline->Get().GetBindGroupLayout(groupIndex);
    return group;
}

BindGroup& BindGroup::SetSampler(uint32_t bindingIndex, const wgpu::Sampler& sampler)
{
    
    for (auto& entry : m_BindGroupEntries)
    {
        if (entry.binding == bindingIndex)
        {
            
            entry.sampler = sampler;
            m_isBuild = false;
            return *this;
        }
    }
    
    
    wgpu::BindGroupEntry entry;
    entry.binding = bindingIndex;
    entry.sampler = sampler;
    m_BindGroupEntries.emplace_back(entry);
    m_isBuild = false;
    return *this;
}

BindGroup& BindGroup::SetSampler(uint32_t bindingIndex, Sampler& sampler)
{
    
    for (auto& entry : m_BindGroupEntries)
    {
        if (entry.binding == bindingIndex)
        {
            
            entry.sampler = sampler.Get();
            m_isBuild = false;
            return *this;
        }
    }
    
    
    wgpu::BindGroupEntry entry;
    entry.binding = bindingIndex;
    entry.sampler = sampler.Get();
    m_BindGroupEntries.emplace_back(entry);
    m_isBuild = false;
    return *this;
}

BindGroup& BindGroup::SetTextureView(uint32_t bindingIndex, const wgpu::TextureView& view)
{
    
    for (auto& entry : m_BindGroupEntries)
    {
        if (entry.binding == bindingIndex)
        {
            
            entry.textureView = view;
            m_isBuild = false;
            return *this;
        }
    }
    
    
    wgpu::BindGroupEntry entry;
    entry.binding = bindingIndex;
    entry.textureView = view;
    m_BindGroupEntries.emplace_back(entry);
    m_isBuild = false;
    return *this;
}

BindGroup& BindGroup::SetBuffer(uint32_t bindingIndex, Buffer& buffer, uint32_t size, uint32_t offset)
{
    
    for (auto& entry : m_BindGroupEntries)
    {
        if (entry.binding == bindingIndex)
        {
            
            entry.offset = offset;
            entry.size = (size != 0) ? size : buffer.GetSize();
            entry.buffer = buffer.GetBuffer();
            m_isBuild = false;
            return *this;
        }
    }
    
    
    wgpu::BindGroupEntry entry;
    entry.binding = bindingIndex;
    entry.offset = offset;
    if (size != 0)
        entry.size = size;
    else
        entry.size = buffer.GetSize();
    entry.buffer = buffer.GetBuffer();
    m_BindGroupEntries.emplace_back(entry);
    m_isBuild = false;
    return *this;
}

BindGroup& BindGroup::SetTexture(uint32_t bindingIndex, const TextureAPtr& texture)
{
    if (!texture)
    {
        std::cerr << "BindGroup::SetTexture: texture is null" << std::endl;
        return *this;
    }
    SetTextureView(bindingIndex, texture->GetView());
    return *this;
}

wgpu::BindGroup const& BindGroup::Get()
{
    if (!m_isBuild)
        std::cerr << "Unallowed operation: BindGroup has not been built after modification" << std::endl;
    return m_bindGroup;
}

void BindGroup::Build(const std::shared_ptr<NutContext>& ctx)
{
    if (!ctx)
    {
        std::cerr << "Bind group context is null" << std::endl;
        return;
    }
    wgpu::BindGroupDescriptor descriptor;
    descriptor.entries = m_BindGroupEntries.data();
    descriptor.entryCount = m_BindGroupEntries.size();
    descriptor.layout = m_bindGroupLayout;
    m_bindGroup = ctx->GetWGPUDevice().CreateBindGroup(&descriptor);
    m_isBuild = true;
}

void BindGroup::OverrideBindGroup(const wgpu::BindGroup& cached)
{
    m_bindGroup = cached;
    m_isBuild = true;
}

} 

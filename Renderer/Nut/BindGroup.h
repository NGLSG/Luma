#ifndef NOAI_BINDGROUP_H
#define NOAI_BINDGROUP_H
#include <cstdint>

#include "Buffer.h"
#include "TextureA.h"
#include "dawn/webgpu_cpp.h"

namespace Nut {

class ComputePipeline;
class Sampler;
class TextureA;
class RenderPipeline;

class BindGroup
{
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;
    std::vector<wgpu::BindGroupEntry> m_BindGroupEntries;
    bool m_isBuild = false;

public:
    static BindGroup Create(uint32_t groupIndex, RenderPipeline* Pipeline);
    static BindGroup Create(uint32_t groupIndex, ComputePipeline* Pipeline);
    BindGroup& SetSampler(uint32_t bindingIndex, const wgpu::Sampler& sampler);
    BindGroup& SetSampler(uint32_t bindingIndex, Sampler& sampler);
    BindGroup& SetTextureView(uint32_t bindingIndex, const wgpu::TextureView& view);
    BindGroup& SetBuffer(uint32_t bindingIndex, Buffer& buffer, uint32_t size = 0, uint32_t offset = 0);
    BindGroup& SetTexture(uint32_t bindingIndex, const TextureAPtr& texture);
    wgpu::BindGroup const& Get();

    void Build(const std::shared_ptr<NutContext>& ctx);

    void OverrideBindGroup(const wgpu::BindGroup& cached);
    const wgpu::BindGroup& RawBindGroup() const { return m_bindGroup; }
    const std::vector<wgpu::BindGroupEntry>& GetEntries() const { return m_BindGroupEntries; }
    
    /// 清除所有绑定条目
    void ClearEntries() { m_BindGroupEntries.clear(); m_isBuild = false; }
    
    /// 移除指定绑定索引的条目
    void RemoveEntry(uint32_t bindingIndex);
};

} // namespace Nut

#endif //NOAI_BINDGROUP_H

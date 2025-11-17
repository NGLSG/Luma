#ifndef NOAI_BINDGROUP_H
#define NOAI_BINDGROUP_H
#include <cstdint>

#include "Buffer.h"
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
    BindGroup& SetTexture(uint32_t bindingIndex, TextureA& texture);
    wgpu::BindGroup const& Get();

    void Build(const std::shared_ptr<NutContext>& ctx);
};

} // namespace Nut

#endif //NOAI_BINDGROUP_H

#include "RenderPass.h"

#include "NutContext.h"

namespace Nut {

QuerySetBuilder& QuerySetBuilder::SetLabel(const std::string_view& label)
{
    descriptor.label = label;
    return *this;
}

QuerySetBuilder& QuerySetBuilder::SetType(QueryType type)
{
    descriptor.type = type;
    return *this;
}

QuerySetBuilder& QuerySetBuilder::SetCount(uint32_t count)
{
    descriptor.count = count;
    return *this;
}

QuerySet QuerySetBuilder::Build(const std::shared_ptr<NutContext>& ctx) const
{
    if (ctx)
    {
        return ctx->GetWGPUDevice().CreateQuerySet(&descriptor);
    }
    return nullptr;
}

RenderPass::RenderPass(const wgpu::CommandEncoder& encoder,
                       const wgpu::RenderPassDescriptor& descriptor) : m_commandEncoder(encoder)
{
    m_passEncoder = encoder.BeginRenderPass(&descriptor);
}

void RenderPass::SetPipeline(RenderPipeline& pipel) const
{
    m_passEncoder.SetPipeline(pipel.Get());
    pipel.ForeachGroup([this](size_t idx, BindGroup& group)
    {
        SetBindGroup(idx, group);
    });
}

void RenderPass::SetIndexBuffer(Buffer& buffer, wgpu::IndexFormat format) const
{
    m_passEncoder.SetIndexBuffer(buffer.GetBuffer(), format);
}

void RenderPass::SetVertexBuffer(int slot, Buffer& buffer) const
{
    m_passEncoder.SetVertexBuffer(slot, buffer.GetBuffer());
}

void RenderPass::SetBindGroup(int idx, BindGroup& bindGroup) const
{
    m_passEncoder.SetBindGroup(idx, bindGroup.Get());
}

wgpu::RenderPassEncoder& RenderPass::Get()
{
    return m_passEncoder;
}

void RenderPass::DrawIndexed(size_t indexCount, size_t instanceCount, uint32_t firstIndex, int32_t baseVertex,
                             uint32_t firstInstance)
{
    m_passEncoder.DrawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

wgpu::CommandBuffer RenderPass::End()
{
    m_passEncoder.End();
    return m_commandEncoder.Finish();
}


void RenderPass::BeginOcclusionQuery(uint32_t queryIndex) const
{
    m_passEncoder.BeginOcclusionQuery(queryIndex);
}

void RenderPass::EndOcclusionQuery() const
{
    m_passEncoder.EndOcclusionQuery();
}


void RenderPass::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    m_passEncoder.Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderPass::DrawIndexedIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const
{
    m_passEncoder.DrawIndexedIndirect(indirectBuffer.GetBuffer(), indirectOffset);
}

void RenderPass::DrawIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const
{
    m_passEncoder.DrawIndirect(indirectBuffer.GetBuffer(), indirectOffset);
}


void RenderPass::MultiDrawIndexedIndirect(Buffer& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount,
                                          Buffer& drawCountBuffer, uint64_t drawCountBufferOffset) const
{
    if (drawCountBuffer)
    {
        m_passEncoder.MultiDrawIndexedIndirect(indirectBuffer.GetBuffer(), indirectOffset, maxDrawCount,
                                               drawCountBuffer.GetBuffer(), drawCountBufferOffset);
    }
    else
    {
        m_passEncoder.MultiDrawIndexedIndirect(indirectBuffer.GetBuffer(), indirectOffset, maxDrawCount, nullptr,
                                               drawCountBufferOffset);
    }
}

void RenderPass::MultiDrawIndirect(Buffer& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount,
                                   Buffer& drawCountBuffer, uint64_t drawCountBufferOffset) const
{
    if (drawCountBuffer)
    {
        m_passEncoder.MultiDrawIndirect(indirectBuffer.GetBuffer(), indirectOffset, maxDrawCount,
                                        drawCountBuffer.GetBuffer(), drawCountBufferOffset);
    }
    else
    {
        m_passEncoder.MultiDrawIndirect(indirectBuffer.GetBuffer(), indirectOffset, maxDrawCount, nullptr,
                                        drawCountBufferOffset);
    }
}


void RenderPass::InsertDebugMarker(const char* markerLabel) const
{
    m_passEncoder.InsertDebugMarker(markerLabel);
}

void RenderPass::PopDebugGroup() const
{
    m_passEncoder.PopDebugGroup();
}

void RenderPass::PushDebugGroup(const char* groupLabel) const
{
    m_passEncoder.PushDebugGroup(groupLabel);
}


void RenderPass::PixelLocalStorageBarrier() const
{
    m_passEncoder.PixelLocalStorageBarrier();
}


void RenderPass::SetBlendConstant(const wgpu::Color* color) const
{
    m_passEncoder.SetBlendConstant(color);
}


void RenderPass::SetImmediateData(uint32_t offset, const void* data, size_t size) const
{
    m_passEncoder.SetImmediateData(offset, data, size);
}


void RenderPass::SetLabel(const char* label) const
{
    m_passEncoder.SetLabel(label);
}


void RenderPass::SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
{
    m_passEncoder.SetScissorRect(x, y, width, height);
}


void RenderPass::SetStencilReference(uint32_t reference) const
{
    m_passEncoder.SetStencilReference(reference);
}


void RenderPass::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) const
{
    m_passEncoder.SetViewport(x, y, width, height, minDepth, maxDepth);
}


void RenderPass::WriteTimestamp(const QuerySet& querySet, uint32_t queryIndex) const
{
    m_passEncoder.WriteTimestamp(querySet.Get(), queryIndex);
}

ColorAttachmentBuilder& ColorAttachmentBuilder::SetTexture(const TextureAPtr& texture)
{
    if (!texture)
    {
        std::cerr << "Failed to set color attachment to texture" << std::endl;
        return *this;
    }
    attachment.view = texture->GetTextureView();
    return *this;
}

ColorAttachmentBuilder& ColorAttachmentBuilder::SetResolveTexture(const TextureAPtr& texture)
{
    if (!texture)
    {
        std::cerr << "Failed to set color attachment to texture" << std::endl;
        return *this;
    }
    attachment.view = texture->GetTextureView();
    return *this;
}


ColorAttachmentBuilder& ColorAttachmentBuilder::SetDepthSlice(uint32_t slice)
{
    attachment.depthSlice = slice;
    return *this;
}

ColorAttachmentBuilder& ColorAttachmentBuilder::SetLoadOnOpen(LoadOnOpen loadOnOpen)
{
    attachment.loadOp = loadOnOpen;

    return *this;
}

ColorAttachmentBuilder& ColorAttachmentBuilder::SetStoreOnOpen(StoreOnOpen storeOnOpen)
{
    attachment.storeOp = storeOnOpen;
    return *this;
}

ColorAttachmentBuilder& ColorAttachmentBuilder::SetClearColor(const Color& color)
{
    attachment.clearValue = color;
    return *this;
}

ColorAttachment ColorAttachmentBuilder::Build()
{
    return attachment;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetTexture(const TextureAPtr& texture)
{
    if (!texture)
    {
        std::cerr << "Failed to set depth stencil attachment to texture" << std::endl;
        return *this;
    }
    attachment.view = texture->GetTextureView();
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetDepthLoadOnOpen(LoadOnOpen loadOnOpen)
{
    attachment.depthLoadOp = loadOnOpen;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetDepthStoreOnOpen(StoreOnOpen storeOnOpen)
{
    attachment.depthStoreOp = storeOnOpen;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetDepth(float depth)
{
    attachment.depthClearValue = depth;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetDepthReadOnly(bool v)
{
    attachment.depthReadOnly = v;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetStencilLoadOnOpen(LoadOnOpen loadOnOpen)
{
    attachment.stencilLoadOp = loadOnOpen;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetStencilStoreOnOpen(StoreOnOpen storeOnOpen)
{
    attachment.stencilStoreOp = storeOnOpen;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetStencil(uint32_t stencil)
{
    attachment.stencilClearValue = stencil;
    return *this;
}

DepthStencilAttachmentBuilder& DepthStencilAttachmentBuilder::SetStencilReadOnly(bool v)
{
    attachment.stencilReadOnly = v;
    return *this;
}

DepthStencilAttachment DepthStencilAttachmentBuilder::Build() const
{
    return attachment;
}

RenderPassBuilder::RenderPassBuilder()
{
    m_descriptor = {};
}

RenderPassBuilder::RenderPassBuilder(wgpu::CommandEncoder& encode) : m_commandEncoder(encode)
{
}

RenderPassBuilder& RenderPassBuilder::SetLabel(const std::string& label)
{
    m_label = label;
    m_descriptor.label = m_label.c_str();
    return *this;
}

RenderPassBuilder& RenderPassBuilder::AddColorAttachment(const ColorAttachment& attachment)
{
    m_colorAttachments.push_back(attachment);
    m_descriptor.colorAttachmentCount = m_colorAttachments.size();
    m_descriptor.colorAttachments = m_colorAttachments.data();
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetDepthStencilAttachment(const DepthStencilAttachment& attachment)
{
    m_descriptor.depthStencilAttachment = &attachment;
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetOcclusionQuerySet(const QuerySet& querySet)
{
    m_descriptor.occlusionQuerySet = querySet;
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetTimestampWrites(
    const QuerySet& querySet,
    uint32_t beginningOfPassWriteIndex,
    uint32_t endOfPassWriteIndex)
{
    m_timestampWrites.querySet = querySet;
    m_timestampWrites.beginningOfPassWriteIndex = beginningOfPassWriteIndex;
    m_timestampWrites.endOfPassWriteIndex = endOfPassWriteIndex;

    m_hasTimestampWrites = true;
    m_descriptor.timestampWrites = &m_timestampWrites;

    return *this;
}

RenderPassBuilder& RenderPassBuilder::ClearColorAttachments()
{
    m_colorAttachments.clear();
    m_descriptor.colorAttachmentCount = 0;
    m_descriptor.colorAttachments = nullptr;
    return *this;
}

RenderPassBuilder& RenderPassBuilder::ClearDepthStencilAttachment()
{
    m_hasDepthStencil = false;
    m_descriptor.depthStencilAttachment = nullptr;
    return *this;
}

RenderPass RenderPassBuilder::Build()
{
    return {m_commandEncoder, m_descriptor};
}

RenderPassBuilder& RenderPassBuilder::Reset()
{
    m_descriptor = {};
    m_colorAttachments.clear();
    m_depthStencilAttachment = {};
    m_timestampWrites = {};
    m_label.clear();
    m_hasDepthStencil = false;
    m_hasTimestampWrites = false;
    return *this;
}

ComputePass::ComputePass(const wgpu::CommandEncoder& encoder, const wgpu::ComputePassDescriptor& descriptor)
{
    m_commandEncoder = encoder;
    m_passEncoder = encoder.BeginComputePass(&descriptor);
    if (!m_passEncoder)
    {
        std::cerr << "Failed to begin compute pass" << std::endl;
    }
}

void ComputePass::Dispatch(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    m_passEncoder.DispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
}

void ComputePass::DispatchIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const
{
    m_passEncoder.DispatchWorkgroupsIndirect(indirectBuffer.GetBuffer(), indirectOffset);
}

wgpu::CommandBuffer ComputePass::End()
{
    m_passEncoder.End();
    return m_commandEncoder.Finish();
}

wgpu::ComputePassEncoder& ComputePass::Get()
{
    return m_passEncoder;
}

void ComputePass::InsertDebugMarker(const std::string& markerLabel) const
{
    m_passEncoder.InsertDebugMarker(markerLabel.c_str());
}

void ComputePass::PopDebugGroup() const
{
    m_passEncoder.PopDebugGroup();
}

void ComputePass::PushDebugGroup(const std::string& groupLabel) const
{
    m_passEncoder.PushDebugGroup(groupLabel.c_str());
}

void ComputePass::SetBindGroup(uint32_t groupIndex, BindGroup& group, size_t dynamicOffsetCount,
                               uint32_t const* dynamicOffsets) const
{
    m_passEncoder.SetBindGroup(groupIndex, group.Get(), dynamicOffsetCount, dynamicOffsets);
}

void ComputePass::SetImmediateData(uint32_t offset, void const* data, size_t size) const
{
    m_passEncoder.SetImmediateData(offset, data, size);
}

void ComputePass::SetLabel(const std::string& label) const
{
    m_passEncoder.SetLabel(label.c_str());
}

void ComputePass::SetPipeline(ComputePipeline& pipeline) const
{
    m_passEncoder.SetPipeline(pipeline.Get());
    pipeline.ForeachGroup([this](size_t idx, BindGroup& group)
    {
        SetBindGroup(idx, group);
    });
}

void ComputePass::WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const
{
    m_passEncoder.WriteTimestamp(querySet.Get(), queryIndex);
}

ComputePassBuilder::ComputePassBuilder()
{
    m_descriptor = {};;
}

ComputePassBuilder::ComputePassBuilder(wgpu::CommandEncoder& encoder) : m_commandEncoder(encoder)
{
}

ComputePassBuilder& ComputePassBuilder::SetTimestampWrites(const QuerySet& querySet, uint32_t beginningOfPassWriteIndex,
                                                           uint32_t endOfPassWriteIndex)
{
    m_timestampWrites.querySet = querySet;
    m_timestampWrites.beginningOfPassWriteIndex = beginningOfPassWriteIndex;
    m_timestampWrites.endOfPassWriteIndex = endOfPassWriteIndex;

    m_hasTimestampWrites = true;
    m_descriptor.timestampWrites = &m_timestampWrites;

    return *this;
}

ComputePassBuilder& ComputePassBuilder::SetLabel(const std::string& label)
{
    m_label = label;
    m_descriptor.label = m_label.c_str();
    return *this;
}

ComputePass ComputePassBuilder::Build()
{
    return {m_commandEncoder, m_descriptor};
}

} 

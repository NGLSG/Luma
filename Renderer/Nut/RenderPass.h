#ifndef NOAI_RENDERPASS_H
#define NOAI_RENDERPASS_H
#include "BindGroup.h"
#include "Buffer.h"
#include "dawn/webgpu_cpp.h"
#include "Pipeline.h"

namespace Nut
{
    // 前置声明
    class Buffer;
    class RenderPipeline;
    class BindGroup;
    using QueryType = wgpu::QueryType;
    using QuerySet = wgpu::QuerySet;

    class QuerySetBuilder
    {
        wgpu::QuerySetDescriptor descriptor{};

    public:
        /*StringView label = {};
        QueryType type = {};
        uint32_t count;*/
        QuerySetBuilder& SetLabel(const std::string_view& label);

        QuerySetBuilder& SetType(QueryType type);

        QuerySetBuilder& SetCount(uint32_t count);

        QuerySet Build(const std::shared_ptr<NutContext>& ctx) const;
    };


    class RenderPass
    {
    private:
        wgpu::RenderPassEncoder m_passEncoder;
        wgpu::CommandEncoder m_commandEncoder;

    public:
        RenderPass(const wgpu::CommandEncoder& encoder, const wgpu::RenderPassDescriptor& descriptor);

        void SetPipeline(RenderPipeline& pipel) const;
        void SetIndexBuffer(Buffer& buffer, wgpu::IndexFormat format) const;
        void SetVertexBuffer(int slot, Buffer& buffer) const;
        void SetBindGroup(int idx, BindGroup& bindGroup) const;
        wgpu::RenderPassEncoder& Get();
        void DrawIndexed(size_t indexCount, size_t instanceCount, uint32_t firstIndex = 0, int32_t baseVertex = 0,
                         uint32_t firstInstance = 0);
        wgpu::CommandBuffer End();

        void BeginOcclusionQuery(uint32_t queryIndex) const;
        void EndOcclusionQuery() const;

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
                  uint32_t firstInstance = 0) const;
        void DrawIndexedIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const;
        void DrawIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const;

        void MultiDrawIndexedIndirect(Buffer& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount,
                                      Buffer& drawCountBuffer, uint64_t drawCountBufferOffset = 0) const;
        void MultiDrawIndirect(Buffer& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount,
                               Buffer& drawCountBuffer, uint64_t drawCountBufferOffset = 0) const;

        void InsertDebugMarker(const char* markerLabel) const;
        void PopDebugGroup() const;
        void PushDebugGroup(const char* groupLabel) const;

        void PixelLocalStorageBarrier() const;

        void SetBlendConstant(const wgpu::Color* color) const;

        void SetImmediateData(uint32_t offset, const void* data, size_t size) const;

        void SetLabel(const char* label) const;

        void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;

        void SetStencilReference(uint32_t reference) const;

        void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) const;


        void WriteTimestamp(const QuerySet& querySet, uint32_t queryIndex) const;

        operator bool() const { return m_passEncoder != nullptr; }
    };

    using LoadOnOpen = wgpu::LoadOp;
    using StoreOnOpen = wgpu::StoreOp;
    using Color = wgpu::Color;
    using ColorAttachment = wgpu::RenderPassColorAttachment;

    class ColorAttachmentBuilder
    {
        ColorAttachment attachment;

    public:
        ColorAttachmentBuilder& SetTexture(const TextureA& texture);
        ColorAttachmentBuilder& SetResolveTexture(const TextureA& texture);
        ColorAttachmentBuilder& SetDepthSlice(uint32_t slice);
        ColorAttachmentBuilder& SetLoadOnOpen(LoadOnOpen loadOnOpen);
        ColorAttachmentBuilder& SetStoreOnOpen(StoreOnOpen storeOnOpen);
        ColorAttachmentBuilder& SetClearColor(const Color& color);

        [[nodiscard]] ColorAttachment Build();
    };


    using DepthStencilAttachment = wgpu::RenderPassDepthStencilAttachment;

    class DepthStencilAttachmentBuilder
    {
        DepthStencilAttachment attachment;

    public:
        DepthStencilAttachmentBuilder& SetTexture(const TextureA& texture);
        DepthStencilAttachmentBuilder& SetDepthLoadOnOpen(LoadOnOpen loadOnOpen);
        DepthStencilAttachmentBuilder& SetDepthStoreOnOpen(StoreOnOpen storeOnOpen);
        DepthStencilAttachmentBuilder& SetDepth(float depth);
        DepthStencilAttachmentBuilder& SetDepthReadOnly(bool v);
        DepthStencilAttachmentBuilder& SetStencilLoadOnOpen(LoadOnOpen loadOnOpen);
        DepthStencilAttachmentBuilder& SetStencilStoreOnOpen(StoreOnOpen storeOnOpen);
        DepthStencilAttachmentBuilder& SetStencil(uint32_t stencil);
        DepthStencilAttachmentBuilder& SetStencilReadOnly(bool v);

        [[nodiscard]] DepthStencilAttachment Build() const;
    };

    class RenderPassBuilder
    {
    private:
        wgpu::RenderPassDescriptor m_descriptor{};
        std::vector<wgpu::RenderPassColorAttachment> m_colorAttachments;
        wgpu::RenderPassDepthStencilAttachment m_depthStencilAttachment{};
        wgpu::PassTimestampWrites m_timestampWrites{};
        std::string m_label;
        bool m_hasDepthStencil = false;
        bool m_hasTimestampWrites = false;
        wgpu::CommandEncoder m_commandEncoder;

    public:
        RenderPassBuilder();

        explicit RenderPassBuilder(wgpu::CommandEncoder& encode);

        // 设置标签
        RenderPassBuilder& SetLabel(const std::string& label);

        // 添加颜色附件
        RenderPassBuilder& AddColorAttachment(const ColorAttachment& attachment);
        // 设置深度模板附件
        RenderPassBuilder& SetDepthStencilAttachment(const DepthStencilAttachment& attachment);

        // 设置遮挡查询集
        RenderPassBuilder& SetOcclusionQuerySet(const QuerySet& querySet);

        // 设置时间戳写入
        RenderPassBuilder& SetTimestampWrites(
            const QuerySet& querySet,
            uint32_t beginningOfPassWriteIndex,
            uint32_t endOfPassWriteIndex
        );

        // 清除所有颜色附件
        RenderPassBuilder& ClearColorAttachments();

        // 清除深度模板附件
        RenderPassBuilder& ClearDepthStencilAttachment();

        // 构建描述符
        RenderPass Build();

        // 重置构建器
        RenderPassBuilder& Reset();
    };

    class ComputePass
    {
        wgpu::ComputePassEncoder m_passEncoder;
        wgpu::CommandEncoder m_commandEncoder;

    public:
        ComputePass(const wgpu::CommandEncoder& encoder, const wgpu::ComputePassDescriptor& descriptor);

        void Dispatch(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) const;
        void DispatchIndirect(Buffer& indirectBuffer, uint64_t indirectOffset) const;
        wgpu::CommandBuffer End();
        wgpu::ComputePassEncoder& Get();
        void InsertDebugMarker(const std::string& markerLabel) const;
        void PopDebugGroup() const;
        void PushDebugGroup(const std::string& groupLabel) const;
        void SetBindGroup(uint32_t groupIndex, BindGroup& group, size_t dynamicOffsetCount = 0,
                          uint32_t const* dynamicOffsets = nullptr) const;
        void SetImmediateData(uint32_t offset, void const* data, size_t size) const;
        void SetLabel(const std::string& label) const;
        void SetPipeline(ComputePipeline& pipeline) const;
        void WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const;
        operator bool() const { return m_passEncoder != nullptr; }
    };

    class ComputePassBuilder
    {
        wgpu::ComputePassDescriptor m_descriptor{};
        wgpu::PassTimestampWrites m_timestampWrites{};
        wgpu::CommandEncoder m_commandEncoder;
        std::string m_label;
        bool m_hasTimestampWrites = false;

    public:
        ComputePassBuilder();
        explicit ComputePassBuilder(wgpu::CommandEncoder& encoder);
        ComputePassBuilder& SetTimestampWrites(
            const QuerySet& querySet,
            uint32_t beginningOfPassWriteIndex,
            uint32_t endOfPassWriteIndex
        );
        ComputePassBuilder& SetLabel(const std::string& label);
        ComputePass Build();
    };
} // namespace Nut

#endif //NOAI_RENDERPASS_H

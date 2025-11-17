#include "Buffer.h"

#include <iostream>
#include <ostream>

#include "NutContext.h"

bool Buffer::ReadBuffer(void* dest, const std::function<void(bool success)>& onCompleted, uint32_t size,
                        uint32_t offset) const
{
    if (!m_context || !m_context->GetWGPUDevice())
    {
        std::cerr << "The data cannot be read because the context is empty or the device is illegal." << std::endl;
        if (onCompleted) onCompleted(false);
        return false;
    }

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    if (size == 0)
    {
        size = m_layout.size;
    }
    bufferDesc.size = size;

    wgpu::Buffer stagingBuffer = m_context->GetWGPUDevice().CreateBuffer(&bufferDesc);
    wgpu::CommandEncoder encoder = m_context->GetWGPUDevice().CreateCommandEncoder();
    encoder.CopyBufferToBuffer(m_buffer, offset, stagingBuffer, 0, size);
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    m_context->GetWGPUDevice().GetQueue().Submit(1, &commandBuffer);

    std::promise<bool> mapPromise;
    std::future<bool> mapFuture = mapPromise.get_future();

    auto sharedStagingBuffer = std::make_shared<wgpu::Buffer>(stagingBuffer);

    auto f1 = stagingBuffer.MapAsync(
        wgpu::MapMode::Read, 0, size, wgpu::CallbackMode::AllowSpontaneous,
        [dest, size, onCompleted, sharedStagingBuffer, p = std::move(mapPromise)](
        wgpu::MapAsyncStatus status, wgpu::StringView msg) mutable
        {
            bool success = (status == wgpu::MapAsyncStatus::Success);
            if (success)
            {
                const void* mappedData = sharedStagingBuffer->GetConstMappedRange(0, size);
                if (mappedData)
                {
                    memcpy(dest, mappedData, size);
                }
                else
                {
                    success = false; // 获取映射地址失败
                }
            }

            sharedStagingBuffer->Unmap();

            if (onCompleted)
            {
                onCompleted(success);
            }
            p.set_value(success);
        });

    if (wgpu::WaitStatus::Success != m_context->GetWGPUInstance().WaitAny(f1, -1))
    {
        std::cout << "Timeout while waiting for buffer mapping." << std::endl;
        stagingBuffer.Unmap();
        if (onCompleted) onCompleted(false);
        return false;
    }
    return mapFuture.get();
}

Buffer::Buffer(std::nullopt_t) noexcept
{
}

std::vector<uint8_t> Buffer::GetDataFromBuffer(const std::function<void(bool success)>& onCompleted, uint32_t size,
                                               uint32_t offset) const
{
    if (!(m_layout.usage & BufferUsage::CopySrc))
    {
        std::cerr << "CopySrc is not enabled for this buffer and cannot be obtained." << std::endl;
        return {};
    }

    if (size == 0)
    {
        size = m_layout.size;
    }

    std::vector<uint8_t> bufferData(size);

    bool success = ReadBuffer(bufferData.data(), onCompleted, size, offset);

    if (!success)
    {
        std::cerr << "Failed to read data from buffer." << std::endl;
        return {};
    }
    return bufferData;
}

Buffer::operator bool() const
{
    return m_buffer != nullptr;
}

Buffer::Buffer(BufferLayout layout, const std::shared_ptr<NutContext>& ctx) : m_context(ctx), m_layout(layout)
{
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = layout.size;
    bufferDesc.usage = layout.usage;
    bufferDesc.mappedAtCreation = layout.mapped;
    if (!ctx)
    {
        std::cerr << "Failed to create buffer context" << std::endl;
        return;
    }
    m_buffer = ctx->GetWGPUDevice().CreateBuffer(&bufferDesc);
}

std::unique_ptr<Buffer> Buffer::Create(BufferLayout layout, const std::shared_ptr<NutContext>& ctx)
{
    return std::make_unique<Buffer>(layout, ctx);
}

BufferLayout Buffer::GetVertexLayout()
{
    static constexpr BufferLayout vertexLayout{BufferUsage::Vertex | BufferUsage::CopyDst | BufferUsage::CopySrc};
    return vertexLayout;
}

BufferLayout Buffer::GetIndexLayout()
{
    static constexpr BufferLayout indexLayout{BufferUsage::Index | BufferUsage::CopyDst | BufferUsage::CopySrc};
    return indexLayout;
}

BufferLayout Buffer::GetInstanceLayout()
{
    return GetVertexLayout();
}

BufferLayout Buffer::GetUniformLayout()
{
    static constexpr BufferLayout uniformLayout{BufferUsage::Uniform | BufferUsage::CopyDst | BufferUsage::CopySrc};
    return uniformLayout;
}

BufferLayout Buffer::GetStorageLayout()
{
    static constexpr BufferLayout storeLayout{BufferUsage::Storage | BufferUsage::CopyDst | BufferUsage::CopySrc};
    return storeLayout;
}

wgpu::Buffer Buffer::GetBuffer()
{
    return m_buffer;
}

size_t Buffer::GetSize() const
{
    return m_size;
}

size_t Buffer::GetOffset() const
{
    return m_offset;
}


bool Buffer::WriteBuffer(const void* data, uint32_t size, uint32_t ofst)
{
    bool rebuilt = false;
    if (!m_context || !m_buffer)
    {
        std::cerr << "Failed to create buffer context" << std::endl;
    }
    if (size == 0)
    {
        size = m_layout.size;
    }
    else if (size > m_layout.size)
    {
        m_buffer.Destroy();
        m_layout.size = size;
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = size;
        bufferDesc.usage = m_layout.usage;
        bufferDesc.mappedAtCreation = m_layout.mapped;
        m_buffer = m_context->GetWGPUDevice().CreateBuffer(&bufferDesc);
        if (!m_buffer)
        {
            std::cerr << "Failed to recreate buffer" << std::endl;
            return false;
        }
        rebuilt = true;
    }
    m_context->GetWGPUDevice().GetQueue().WriteBuffer(m_buffer, ofst, data, size);
    m_size = m_layout.size;
    m_offset = ofst;
    return rebuilt;
}

BufferLayout const& Buffer::GetLayout() const
{
    return m_layout;
}

BufferUsage BufferBuilder::GetCommonVertexUsage()
{
    return BufferUsage::Vertex | BufferUsage::CopyDst | BufferUsage::CopySrc;
}

BufferUsage BufferBuilder::GetCommonIndexUsage()
{
    return BufferUsage::Index | BufferUsage::CopyDst | BufferUsage::CopySrc;
}

BufferUsage BufferBuilder::GetCommonUniformUsage()
{
    return BufferUsage::Uniform | BufferUsage::CopyDst | BufferUsage::CopySrc;
}

BufferUsage BufferBuilder::GetCommonStorageUsage()
{
    return BufferUsage::Storage | BufferUsage::CopyDst | BufferUsage::CopySrc;
}

BufferUsage BufferBuilder::GetCommonInstanceUsage()
{
    return GetCommonVertexUsage();
}

BufferBuilder& BufferBuilder::SetUsage(BufferUsage usage)
{
    m_layout.usage = usage;
    return *this;
}

BufferBuilder& BufferBuilder::SetMapped(bool mapped)
{
    m_layout.mapped = mapped;
    return *this;
}

BufferBuilder& BufferBuilder::SetSize(uint32_t size)
{
    m_layout.size = size;
    return *this;
}

Buffer BufferBuilder::Build(const std::shared_ptr<NutContext>& ctx) const
{
    Buffer bff(m_layout, ctx);
    bff.WriteBuffer(m_data);
    return bff;
}

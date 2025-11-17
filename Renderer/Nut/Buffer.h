#ifndef NOAI_BUFFER_H
#define NOAI_BUFFER_H
#include <future>
#include <iostream>

#include "dawn/webgpu_cpp.h"

namespace BufferUtils
{
    template <typename T>
    constexpr size_t byte_size(const T& v) noexcept;

    template <typename T>
    concept HasValueType = requires { typename T::value_type; };

    template <HasValueType Vec>
    constexpr size_t byte_size(const Vec& v) noexcept
    {
        size_t total = 0;
        for (const auto& e : v)
            total += byte_size(e);
        return total;
    }

    template <typename T>
    constexpr size_t byte_size(const T&) noexcept
    {
        return sizeof(T);
    }
}

class NutContext;
using BufferUsage = wgpu::BufferUsage;

struct BufferLayout
{
    BufferUsage usage;
    uint32_t size;
    bool mapped;
};

class Buffer
{
private:
    wgpu::Buffer m_buffer;
    BufferLayout m_layout;
    std::shared_ptr<NutContext> m_context;
    size_t m_size;
    size_t m_offset;

    /**
     * @brief 从 GPU Buffer 异步读取数据到 CPU 可访问的内存中。
     * @param dest 目标内存地址，必须由调用方分配并确保大小足够。
     * @param onCompleted 操作完成时的回调函数。
     * @param size 要读取的数据大小（字节），为 0 则读取整个 Buffer。
     * @param offset 从 Buffer 的起始位置偏移量。
     * @return bool 操作是否成功提交并完成。
     * @note 此函数通过内部循环和 Tick() 调用将异步操作转为同步阻塞行为。
     */
    bool ReadBuffer(void* dest, const std::function<void(bool success)>& onCompleted = nullptr, uint32_t size = 0,
                    uint32_t offset = 0) const;

public:
    Buffer(BufferLayout layout, const std::shared_ptr<NutContext>& ctx);

    Buffer(std::nullopt_t) noexcept;

    static std::unique_ptr<Buffer> Create(BufferLayout layout, const std::shared_ptr<NutContext>& ctx);

    static BufferLayout GetVertexLayout();

    static BufferLayout GetIndexLayout();

    static BufferLayout GetInstanceLayout();

    static BufferLayout GetUniformLayout();
    static BufferLayout GetStorageLayout();

    wgpu::Buffer GetBuffer();

    bool WriteBuffer(const void* data, uint32_t size = 0, uint32_t ofst = 0);

    std::vector<uint8_t> GetDataFromBuffer(const std::function<void(bool success)>& onCompleted = nullptr, uint32_t size = 0,
                                           uint32_t offset = 0) const;

    BufferLayout const& GetLayout() const;
    size_t GetSize() const;
    size_t GetOffset() const;

    operator bool() const;
};

class BufferBuilder
{
    BufferLayout m_layout{};
    void* m_data = nullptr;

public:
    static BufferUsage GetCommonVertexUsage();
    static BufferUsage GetCommonIndexUsage();
    static BufferUsage GetCommonUniformUsage();
    static BufferUsage GetCommonStorageUsage();
    static BufferUsage GetCommonInstanceUsage();

    BufferBuilder& SetUsage(BufferUsage usage);

    BufferBuilder& SetMapped(bool mapped);

    BufferBuilder& SetSize(uint32_t size);

    template <typename T>
    BufferBuilder& SetData(T* data);

    template <typename T>
    BufferBuilder& SetData(std::vector<T>& data);

    template <typename T>
    BufferBuilder& SetData(const std::vector<T>& data);

    Buffer Build(const std::shared_ptr<NutContext>& ctx) const;
};


template <typename T>
BufferBuilder& BufferBuilder::SetData(T* data)
{
    m_layout.size = BufferUtils::byte_size(data);
    m_data = data;
    return *this;
}

template <typename T>
BufferBuilder& BufferBuilder::SetData(std::vector<T>& data)
{
    m_layout.size = BufferUtils::byte_size(data);
    m_data = data.data();
    return *this;
}

template <typename T>
BufferBuilder& BufferBuilder::SetData(const std::vector<T>& data)
{
    m_layout.size = BufferUtils::byte_size(data);
    m_data = const_cast<void*>(static_cast<const void*>(data.data()));
    return *this;
}
#endif //NOAI_BUFFER_H

#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <algorithm>
#include <atomic>
#include <concepts>
#include <execution>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

/**
 * @brief 一个高性能、无锁、三缓冲的动态数组。
 * @tparam T 元素类型，必须是可移动和可复制的。
 * @details
 * 此实现专为高并发读取和单线程批量写入场景设计。提供了多种写入模式以应对不同性能需求。
 */
template <typename T>
    requires std::movable<T> && std::copyable<T>
class DynamicArray
{
private:
    /**
     * @brief 内部数据缓冲区，包含数据、元数据和引用计数。
     */
    struct Buffer
    {
        T* m_data = nullptr; ///< 数据指针
        size_t m_size = 0; ///< 当前元素数量
        size_t m_capacity = 0; ///< 当前分配容量
        std::atomic<int> m_refCount{0}; ///< 引用计数，用于只读视图管理

        /**
         * @brief 析构函数，释放数据内存
         */
        ~Buffer() { delete[] m_data; }

        /**
         * @brief 预分配内存空间
         * @param newCapacity 新容量
         */
        void Reserve(size_t newCapacity)
        {
            if (newCapacity <= m_capacity) return;

            T* newData = new T[newCapacity];
            if (m_data != nullptr)
            {
                if constexpr (std::is_nothrow_move_constructible_v<T>)
                    std::move(m_data, m_data + m_size, newData);
                else
                    std::copy(m_data, m_data + m_size, newData);
                delete[] m_data;
            }
            m_data = newData;
            m_capacity = newCapacity;
        }
    };

public:
    /**
     * @brief 为读取者提供的线程安全的只读数据视图。
     * @details
     * 通过 RAII 管理缓冲区的引用计数，确保在视图的生命周期内数据有效。
     * 提供与 std::vector 相似的只读接口。
     */
    class View
    {
    private:
        Buffer* m_buffer; ///< 指向底层缓冲区

    public:
        using const_iterator = const T*;

        /**
         * @brief 构造函数，增加引用计数
         * @param buffer 指向缓冲区
         */
        explicit View(Buffer* buffer) : m_buffer(buffer)
        {
            if (m_buffer)
            {
                m_buffer->m_refCount.fetch_add(1, std::memory_order_acquire);
            }
        }

        /**
         * @brief 拷贝构造函数
         * @param other 另一个视图
         */
        View(const View& other) : m_buffer(other.m_buffer)
        {
            if (m_buffer)
            {
                m_buffer->m_refCount.fetch_add(1, std::memory_order_acquire);
            }
        }

        /**
         * @brief 移动构造函数
         * @param other 另一个视图
         */
        View(View&& other) noexcept : m_buffer(other.m_buffer)
        {
            other.m_buffer = nullptr;
        }

        /**
         * @brief 拷贝赋值
         * @param other 另一个视图
         * @return 自身引用
         */
        View& operator=(const View& other)
        {
            if (this != &other)
            {
                release();
                m_buffer = other.m_buffer;
                if (m_buffer)
                {
                    m_buffer->m_refCount.fetch_add(1, std::memory_order_acquire);
                }
            }
            return *this;
        }

        /**
         * @brief 移动赋值
         * @param other 另一个视图
         * @return 自身引用
         */
        View& operator=(View&& other) noexcept
        {
            if (this != &other)
            {
                release();
                m_buffer = other.m_buffer;
                other.m_buffer = nullptr;
            }
            return *this;
        }

        /**
         * @brief 析构函数，减少引用计数
         */
        ~View() { release(); }

        /**
         * @brief 获取只读迭代器起始
         * @return 指向首元素的指针
         */
        [[nodiscard]] const_iterator begin() const noexcept { return m_buffer ? m_buffer->m_data : nullptr; }

        /**
         * @brief 获取只读迭代器末尾
         * @return 指向末尾的指针
         */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_buffer ? m_buffer->m_data + m_buffer->m_size : nullptr;
        }

        /**
         * @brief 获取只读迭代器起始
         * @return 指向首元素的指针
         */
        [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
        /**
         * @brief 获取只读迭代器末尾
         * @return 指向末尾的指针
         */
        [[nodiscard]] const_iterator cend() const noexcept { return end(); }

        /**
         * @brief 下标访问元素
         * @param index 索引
         * @return 元素常引用
         */
        [[nodiscard]] const T& operator[](size_t index) const { return m_buffer->m_data[index]; }
        /**
         * @brief 获取元素数量
         * @return 元素数量
         */
        [[nodiscard]] size_t Size() const noexcept { return m_buffer ? m_buffer->m_size : 0; }
        /**
         * @brief 判断是否为空
         * @return 是否为空
         */
        [[nodiscard]] bool IsEmpty() const noexcept { return Size() == 0; }
        /**
         * @brief 获取底层数据指针
         * @return 数据指针
         */
        [[nodiscard]] const T* Data() const noexcept { return m_buffer ? m_buffer->m_data : nullptr; }
        /**
         * @brief 获取首元素
         * @return 首元素常引用
         */
        [[nodiscard]] const T& Front() const { return m_buffer->m_data[0]; }
        /**
         * @brief 获取末元素
         * @return 末元素常引用
         */
        [[nodiscard]] const T& Back() const { return m_buffer->m_data[m_buffer->m_size - 1]; }

        /**
         * @brief 下标访问元素（可写）
         * @param index 索引
         * @return 元素引用
         */
        [[nodiscard]] T& operator[](size_t index)
        {
            if (!m_buffer) throw std::runtime_error("Buffer is null");
            return m_buffer->m_data[index];
        }

        /**
         * @brief 安全访问元素
         * @param index 索引
         * @return 元素引用
         */
        [[nodiscard]] T& at(size_t index)
        {
            if (!m_buffer) throw std::runtime_error("Buffer is null");
            if (index >= m_buffer->m_size) throw std::out_of_range("Index out of range");
            return m_buffer->m_data[index];
        }

    private:
        /**
         * @brief 释放引用计数
         */
        void release()
        {
            if (m_buffer)
            {
                m_buffer->m_refCount.fetch_sub(1, std::memory_order_release);
            }
        }
    };

    /**
     * @brief 在 Modify 回调中提供给写入者的写入代理。
     * @details 封装所有修改操作，确保它们作用于后台的 Write Buffer。
     */
    class Proxy
    {
    private:
        Buffer* m_buffer; ///< 指向写缓冲区

    public:
        /**
         * @brief 构造函数
         * @param buffer 写缓冲区指针
         */
        explicit Proxy(Buffer* buffer) : m_buffer(buffer)
        {
        }

        /**
         * @brief 在数组末尾添加一个元素 (拷贝)。
         * @param value 要添加的元素
         */
        void PushBack(const T& value)
        {
            if (m_buffer->m_size >= m_buffer->m_capacity)
            {
                size_t newCapacity = (m_buffer->m_capacity == 0)
                                         ? 8
                                         : static_cast<size_t>(m_buffer->m_capacity * 1.618);
                m_buffer->Reserve(newCapacity);
            }
            m_buffer->m_data[m_buffer->m_size++] = value;
        }

        /**
         * @brief 在数组末尾添加一个元素 (移动)。
         * @param value 要添加的元素右值
         */
        void PushBack(T&& value)
        {
            if (m_buffer->m_size >= m_buffer->m_capacity)
            {
                size_t newCapacity = (m_buffer->m_capacity == 0)
                                         ? 8
                                         : static_cast<size_t>(m_buffer->m_capacity * 1.618);
                m_buffer->Reserve(newCapacity);
            }
            m_buffer->m_data[m_buffer->m_size++] = std::move(value);
        }

        /**
         * @brief 在数组末尾就地构造一个元素。
         * @tparam Args 构造参数类型
         * @param args 构造参数
         * @return 新构造元素引用
         */
        template <typename... Args>
        T& EmplaceBack(Args&&... args)
        {
            if (m_buffer->m_size >= m_buffer->m_capacity)
            {
                size_t newCapacity = (m_buffer->m_capacity == 0)
                                         ? 8
                                         : static_cast<size_t>(m_buffer->m_capacity * 1.618);
                m_buffer->Reserve(newCapacity);
            }
            T* location = &m_buffer->m_data[m_buffer->m_size];
            new(location) T(std::forward<Args>(args)...);
            m_buffer->m_size++;
            return *location;
        }

        /**
         * @brief 移除数组的最后一个元素。
         */
        void PopBack()
        {
            if (m_buffer->m_size > 0)
            {
                m_buffer->m_size--;
            }
        }

        /**
         * @brief 清空数组所有元素，但不释放已分配的内存。
         */
        void Clear() { m_buffer->m_size = 0; }

        /**
         * @brief 对数组元素进行排序。
         * @tparam Compare 比较器类型
         * @param comp 比较器
         */
        template <typename Compare = std::less<T>>
        void Sort(Compare comp = {})
        {
            if (m_buffer->m_size > 1)
            {
#if defined(__ANDROID__)
                std::sort(m_buffer->m_data, m_buffer->m_data + m_buffer->m_size, comp);
#else
                if (m_buffer->m_size > 1000)
                {
                    // 关键算法说明：当数据量较大时，并行排序能够显著利用多核 CPU 优势，
                    // par_unseq 策略允许编译器最大程度地进行乱序和向量化优化。
                    std::sort(std::execution::par_unseq, m_buffer->m_data, m_buffer->m_data + m_buffer->m_size, comp);
                }
                else
                {
                    std::sort(m_buffer->m_data, m_buffer->m_data + m_buffer->m_size, comp);
                }
#endif
            }
        }

        /**
         * @brief 预分配内存空间。
         * @param newCapacity 新容量
         */
        void Reserve(size_t newCapacity) { m_buffer->Reserve(newCapacity); }

        /**
         * @brief 在指定位置插入一个元素 (拷贝)。
         * @param index 插入点的索引。
         * @param value 要插入的元素值。
         */
        void Insert(size_t index, const T& value)
        {
            if (index > m_buffer->m_size) { throw std::out_of_range("Insert index out of bounds"); }
            if (m_buffer->m_size >= m_buffer->m_capacity)
            {
                size_t newCapacity = (m_buffer->m_capacity == 0)
                                         ? 8
                                         : static_cast<size_t>(m_buffer->m_capacity * 1.618);
                m_buffer->Reserve(newCapacity);
            }
            T* data = m_buffer->m_data;
            std::move_backward(data + index, data + m_buffer->m_size, data + m_buffer->m_size + 1);
            data[index] = value;
            m_buffer->m_size++;
        }

        /**
         * @brief 在指定位置插入一个元素 (移动)。
         * @param index 插入点的索引。
         * @param value 要插入的元素值的右值引用。
         */
        void Insert(size_t index, T&& value)
        {
            if (index > m_buffer->m_size) { throw std::out_of_range("Insert index out of bounds"); }
            if (m_buffer->m_size >= m_buffer->m_capacity)
            {
                size_t newCapacity = (m_buffer->m_capacity == 0)
                                         ? 8
                                         : static_cast<size_t>(m_buffer->m_capacity * 1.618);
                m_buffer->Reserve(newCapacity);
            }
            T* data = m_buffer->m_data;
            std::move_backward(data + index, data + m_buffer->m_size, data + m_buffer->m_size + 1);
            data[index] = std::move(value);
            m_buffer->m_size++;
        }

        /**
         * @brief 移除指定位置的元素。
         * @param index 要移除的元素的索引。
         */
        void Erase(size_t index)
        {
            if (index >= m_buffer->m_size) { throw std::out_of_range("Erase index out of bounds"); }
            T* data = m_buffer->m_data;
            std::move(data + index + 1, data + m_buffer->m_size, data + index);
            m_buffer->m_size--;
        }

        /**
         * @brief 移除指定范围的元素 [first, last)。
         * @param first 要移除的范围的起始索引。
         * @param last 要移除的范围的结束索引（不包含）。
         */
        void Erase(size_t first, size_t last)
        {
            if (first >= last || first >= m_buffer->m_size) return;
            last = std::min(last, m_buffer->m_size);

            T* data = m_buffer->m_data;
            std::move(data + last, data + m_buffer->m_size, data + first);
            m_buffer->m_size -= (last - first);
        }

        /**
         * @brief 通过下标访问元素。
         * @param index 索引
         * @return 元素引用
         */
        [[nodiscard]] T& operator[](size_t index) { return m_buffer->m_data[index]; }
        /**
         * @brief 获取当前元素数量。
         * @return 元素数量
         */
        [[nodiscard]] size_t Size() const noexcept { return m_buffer->m_size; }
        /**
         * @brief 获取当前已分配的容量。
         * @return 容量
         */
        [[nodiscard]] size_t Capacity() const noexcept { return m_buffer->m_capacity; }
        /**
         * @brief 判断数组是否为空。
         * @return 是否为空
         */
        [[nodiscard]] bool IsEmpty() const noexcept { return m_buffer->m_size == 0; }

        /**
         * @brief 重载等于运算符,由vector直接复制给data。
         * @param vec 源vector
         * @return 当前Proxy引用
         */
        Proxy& operator=(const std::vector<T>& vec)
        {
            Clear();

            if (!vec.empty())
            {
                // 确保有足够的容量
                if (vec.size() > m_buffer->m_capacity)
                {
                    m_buffer->Reserve(vec.size());
                }

                // 复制数据
                std::copy(vec.begin(), vec.end(), m_buffer->m_data);
                m_buffer->m_size = vec.size();
            }

            return *this;
        }
    };

public:
    /**
     * @brief 默认构造函数，初始化三缓冲。
     */
    DynamicArray()
    {
        m_buffers[0] = new Buffer();
        m_buffers[1] = new Buffer();
        m_buffers[2] = new Buffer();

        m_readBuffer.store(m_buffers[0], std::memory_order_relaxed);
        m_writeBuffer = m_buffers[1];
        m_readyBuffer = m_buffers[2];
    }

    /**
     * @brief 析构函数，释放所有缓冲区
     */
    ~DynamicArray()
    {
        delete m_buffers[0];
        delete m_buffers[1];
        delete m_buffers[2];
    }

    // =========================================================================
    // <<< 新增：移动和复制语义实现 >>>
    // =========================================================================

    /**
     * @brief 复制构造函数 (深拷贝)
     * @details 创建一个拥有独立数据副本的新 DynamicArray。
     * 只复制当前 Read Buffer 的内容作为一个新的快照。
     */
    DynamicArray(const DynamicArray& other)
    {
        // 初始化自己的缓冲区
        m_buffers[0] = new Buffer();
        m_buffers[1] = new Buffer();
        m_buffers[2] = new Buffer();

        // 从 'other' 获取一个安全的只读视图
        View other_view = other.GetView();

        // 深度复制数据到我们自己的第一个缓冲区
        if (other_view.Size() > 0)
        {
            m_buffers[0]->Reserve(other_view.Size());
            std::copy(other_view.begin(), other_view.end(), m_buffers[0]->m_data);
            m_buffers[0]->m_size = other_view.Size();
        }

        // 设置内部状态为干净的初始状态
        m_readBuffer.store(m_buffers[0], std::memory_order_relaxed);
        m_writeBuffer = m_buffers[1];
        m_readyBuffer = m_buffers[2];
        // 锁状态 m_writerLock 和 m_isWritingInPlace 不需要复制
    }

    /**
     * @brief 移动构造函数
     * @details 高效地从另一个 DynamicArray 窃取资源。
     */
    DynamicArray(DynamicArray&& other) noexcept
    {
        // 将 'other' 的状态转移给我们
        std::copy(std::begin(other.m_buffers), std::end(other.m_buffers), std::begin(m_buffers));
        m_readBuffer.store(other.m_readBuffer.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_writeBuffer = other.m_writeBuffer;
        m_readyBuffer = other.m_readyBuffer;

        // 将 'other' 置于一个有效的空状态，防止其析构函数释放我们已接管的资源
        std::fill(std::begin(other.m_buffers), std::end(other.m_buffers), nullptr);
        other.m_writeBuffer = nullptr;
        other.m_readyBuffer = nullptr;
    }

    /**
     * @brief 复制赋值操作符
     */
    DynamicArray& operator=(const DynamicArray& other)
    {
        if (this != &other)
        {
            // 使用 copy-and-swap 惯用法，保证强异常安全
            DynamicArray temp(other);
            swap(*this, temp);
        }
        return *this;
    }

    /**
     * @brief 移动赋值操作符
     */
    DynamicArray& operator=(DynamicArray&& other) noexcept
    {
        if (this != &other)
        {
            // 清理我们现有的资源
            delete m_buffers[0];
            delete m_buffers[1];
            delete m_buffers[2];

            // 将 'other' 的状态转移给我们
            std::copy(std::begin(other.m_buffers), std::end(other.m_buffers), std::begin(m_buffers));
            m_readBuffer.store(other.m_readBuffer.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_writeBuffer = other.m_writeBuffer;
            m_readyBuffer = other.m_readyBuffer;

            // 将 'other' 置于有效的空状态
            std::fill(std::begin(other.m_buffers), std::end(other.m_buffers), nullptr);
            other.m_writeBuffer = nullptr;
            other.m_readyBuffer = nullptr;
        }
        return *this;
    }

    /**
     * @brief 获取一个只读的数据视图。此操作极快且通常无锁。
     * @return 一个 View 对象，封装了当前数据快照。
     * @note 如果有写入者正在执行 ModifyInPlace()，此函数会自旋等待。
     */
    [[nodiscard]] View GetView() const
    {
        while (m_isWritingInPlace.load(std::memory_order_acquire))
        {
            /* spin */
        }
        return View(m_readBuffer.load(std::memory_order_acquire));
    }

    /**
     * @brief 通过“写时复制”修改数据，读取者无任何阻塞。
     * @tparam F 修改回调类型
     * @param modifier 一个 Lambda 函数，接收一个写入代理对象 (Proxy)。
     * @details
     * 这是最安全的写入模式，保证了最高的读取并发性。
     */
    template <typename F>
    void Modify(F&& modifier)
    {
        while (m_writerLock.test_and_set(std::memory_order_acquire))
        {
            /* spin */
        }

        while (m_readyBuffer->m_refCount.load(std::memory_order_acquire) > 0)
        {
            /* spin */
        }

        std::swap(m_writeBuffer, m_readyBuffer);

        Buffer* currentReadBuffer = m_readBuffer.load(std::memory_order_acquire);
        m_writeBuffer->Reserve(currentReadBuffer->m_size);
        std::copy(currentReadBuffer->m_data, currentReadBuffer->m_data + currentReadBuffer->m_size,
                  m_writeBuffer->m_data);
        m_writeBuffer->m_size = currentReadBuffer->m_size;

        Proxy writerProxy(m_writeBuffer);
        modifier(writerProxy);

        m_readBuffer.store(m_writeBuffer, std::memory_order_release);

        m_readyBuffer = currentReadBuffer;

        m_writerLock.clear(std::memory_order_release);
    }

    /**
     * @brief 清空并完全重构数据。
     * @tparam F 修改回调类型
     * @param modifier 一个 Lambda 函数，接收一个写入代理对象 (Proxy)。
     * @details
     * 这是一个高度优化的写入路径，适用于需要丢弃所有旧数据并从零开始
     * 构建新数据的场景（例如，重新加载配置、刷新缓存）。
     * 它通过直接使用一个干净的后台缓冲区来完全避免了从 Read Buffer 拷贝旧数据的开销。
     * 读取者在此期间不会被阻塞。
     */
    template <typename F>
    void ClearAndModify(F&& modifier)
    {
        while (m_writerLock.test_and_set(std::memory_order_acquire))
        {
            /* spin */
        }

        while (m_readyBuffer->m_refCount.load(std::memory_order_acquire) > 0)
        {
            /* spin */
        }

        std::swap(m_writeBuffer, m_readyBuffer);

        Buffer* oldReadBuffer = m_readBuffer.load(std::memory_order_acquire);

        m_writeBuffer->m_size = 0;

        Proxy writerProxy(m_writeBuffer);
        modifier(writerProxy);

        m_readBuffer.store(m_writeBuffer, std::memory_order_release);

        m_readyBuffer = oldReadBuffer;

        m_writerLock.clear(std::memory_order_release);
    }

    /**
     * @brief [高性能模式] 就地修改数据，允许读取者被短暂阻塞。
     * @param func  一个 Lambda 函数，接收一个写入代理对象 (Proxy)。
     */
    void ForEach(const std::function<void(const T&)>& func) const
    {
        View view = GetView();
        for (size_t i = 0; i < view.Size(); ++i)
        {
            func(view[i]);
        }
    }

    /**
     * @brief 并行就地修改数据，允许读取者被短暂阻塞。
     * @param func  一个 Lambda 函数，接收一个写入代理对象 (Proxy)。
     * @note 当数据量较小时，使用串行以避免并行开销。
     */
    void ParallelForEach(const std::function<void(const T&)>& func) const
    {
        View view = GetView();
        if (view.Size() > 1000)
        {
#if defined(__ANDROID__)
            std::for_each(view.begin(), view.end(), func);
#else
            std::for_each(std::execution::par, view.begin(), view.end(), func);
#endif
        }
        else
        {
            std::for_each(view.begin(), view.end(), func);
        }
    }

    /**
     * @brief 查找指定值的索引。
     * @param value 要查找的值。
     * @return 如果找到，返回值的索引；否则返回 -1。
     */
    int FindIndex(const T& value) const
    {
        View view = GetView();
        auto it = std::find(view.begin(), view.end(), value);
        if (it != view.end())
        {
            return static_cast<int>(std::distance(view.begin(), it));
        }
        return -1;
    }

    /**
     * @brief 清空所有数据。
     **
     */
    void Clear()
    {
        ClearAndModify([](auto& proxy)
        {
        });
    }

    DynamicArray& operator=(std::vector<T>&& vec)
    {
        ClearAndModify([&vec](auto& proxy)
        {
            proxy.Reserve(vec.size());
            for (auto&& item : vec)
            {
                proxy.PushBack(std::move(item));
            }
        });
        return *this;
    }

    std::vector<T> ToStdVector() const
    {
        View view = GetView();
        return std::vector<T>(view.begin(), view.end());
    }

private:
    /**
    * @brief 交换两个 DynamicArray 实例的内容。
    */
    friend void swap(DynamicArray& first, DynamicArray& second) noexcept
    {
        using std::swap;
        swap(first.m_buffers, second.m_buffers);

        // 原子地交换原子指针
        Buffer* temp = second.m_readBuffer.load(std::memory_order_relaxed);
        second.m_readBuffer.store(first.m_readBuffer.load(std::memory_order_relaxed), std::memory_order_relaxed);
        first.m_readBuffer.store(temp, std::memory_order_relaxed);

        swap(first.m_writeBuffer, second.m_writeBuffer);
        swap(first.m_readyBuffer, second.m_readyBuffer);
        // 锁状态不应被交换
    }

    Buffer* m_buffers[3]; ///< 三缓冲区指针
    std::atomic<Buffer*> m_readBuffer; ///< 当前只读缓冲区
    Buffer* m_writeBuffer; ///< 当前写缓冲区
    Buffer* m_readyBuffer; ///< 准备缓冲区
    std::atomic_flag m_writerLock = ATOMIC_FLAG_INIT; ///< 写入锁
    std::atomic<bool> m_isWritingInPlace{false}; ///< 就地写入标志
};

#endif // DYNAMICARRAY_H

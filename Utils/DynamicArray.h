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
 * @brief 一个高性能、无锁（读取端）、三缓冲的动态数组。
 * @tparam T 元素类型，必须是可移动和可复制的。
 * @details
 * 此实现专为高并发读取和单线程批量写入场景设计。
 * 使用 std::vector 管理内部缓冲区，确保元素生命周期的正确性。
 */
template <typename T>
    requires std::movable<T> && std::copyable<T>
class DynamicArray
{
private:
    /**
     * @brief 内部数据缓冲区，包含数据和引用计数。
     * @details
     * 已重构为使用 std::vector<T> 管理数据，
     * 自动处理内存分配、元素构造和析构。
     */
    struct Buffer
    {
        std::vector<T> m_data; ///< 存储元素的数据向量
        std::atomic<int> m_refCount{0}; ///< 引用计数，用于只读视图管理

        /**
         * @brief 默认析构函数。std::vector 会自动释放资源。
         */
        ~Buffer() = default;

        /**
         * @brief 预分配内存空间
         * @param newCapacity 新容量
         */
        void Reserve(size_t newCapacity)
        {
            m_data.reserve(newCapacity);
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
        using const_iterator = typename std::vector<T>::const_iterator;

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
         * @return 指向首元素的常量迭代器
         */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_buffer ? m_buffer->m_data.begin() : nullptr;
        }

        /**
         * @brief 获取只读迭代器末尾
         * @return 指向末尾的常量迭代器
         */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_buffer ? m_buffer->m_data.end() : nullptr;
        }

        /**
         * @brief 获取只读迭代器起始
         * @return 指向首元素的常量迭代器
         */
        [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
        /**
         * @brief 获取只读迭代器末尾
         * @return 指向末尾的常量迭代器
         */
        [[nodiscard]] const_iterator cend() const noexcept { return end(); }

        /**
         * @brief 下标访问元素
         * @param index 索引
         * @return 元素常引用
         */
        [[nodiscard]] const T& operator[](size_t index) const { return m_buffer->m_data[index]; }

        /**
         * @brief 安全访问元素（带边界检查）
         * @param index 索引
         * @return 元素常引用
         */
        [[nodiscard]] const T& at(size_t index) const
        {
            if (!m_buffer) throw std::out_of_range("Buffer is null");
            return m_buffer->m_data.at(index);
        }

        /**
         * @brief 获取元素数量
         * @return 元素数量
         */
        [[nodiscard]] size_t Size() const noexcept { return m_buffer ? m_buffer->m_data.size() : 0; }
        /**
         * @brief 判断是否为空
         * @return 是否为空
         */
        [[nodiscard]] bool IsEmpty() const noexcept { return Size() == 0; }
        /**
         * @brief 获取底层数据指针
         * @return 数据指针
         */
        [[nodiscard]] const T* Data() const noexcept { return m_buffer ? m_buffer->m_data.data() : nullptr; }

        /**
         * @brief 获取首元素
         * @return 首元素常引用
         */
        [[nodiscard]] const T& Front() const
        {
            if (IsEmpty()) throw std::out_of_range("View is empty");
            return m_buffer->m_data.front();
        }

        /**
         * @brief 获取末元素
         * @return 末元素常引用
         */
        [[nodiscard]] const T& Back() const
        {
            if (IsEmpty()) throw std::out_of_range("View is empty");
            return m_buffer->m_data.back();
        }

        // [重大修正] 移除了非 const 的 operator[] 和 at()。
        // View 必须是只读的，以保证多线程读取安全。

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
     * 所有操作均委托给内部的 std::vector。
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
            m_buffer->m_data.push_back(value);
        }

        /**
         * @brief 在数组末尾添加一个元素 (移动)。
         * @param value 要添加的元素右值
         */
        void PushBack(T&& value)
        {
            m_buffer->m_data.push_back(std::move(value));
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
            return m_buffer->m_data.emplace_back(std::forward<Args>(args)...);
        }

        /**
         * @brief 移除数组的最后一个元素。
         */
        void PopBack()
        {
            if (!IsEmpty())
            {
                m_buffer->m_data.pop_back();
            }
        }

        /**
         * @brief 清空数组所有元素，但不释放已分配的内存。
         */
        void Clear() { m_buffer->m_data.clear(); }

        /**
         * @brief 对数组元素进行排序。
         * @tparam Compare 比较器类型
         * @param comp 比较器
         */
        template <typename Compare = std::less<T>>
        void Sort(Compare comp = {})
        {
            if (Size() > 1)
            {
#if defined(__ANDROID__)
                std::sort(m_buffer->m_data.begin(), m_buffer->m_data.end(), comp);
#else
                if (Size() > 1000)
                {
                    // 关键算法说明：当数据量较大时，并行排序能够显著利用多核 CPU 优势，
                    // par_unseq 策略允许编译器最大程度地进行乱序和向量化优化。
                    std::sort(std::execution::par_unseq, m_buffer->m_data.begin(), m_buffer->m_data.end(), comp);
                }
                else
                {
                    std::sort(m_buffer->m_data.begin(), m_buffer->m_data.end(), comp);
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
            if (index > Size()) { throw std::out_of_range("Insert index out of bounds"); }
            m_buffer->m_data.insert(m_buffer->m_data.begin() + index, value);
        }

        /**
         * @brief 在指定位置插入一个元素 (移动)。
         * @param index 插入点的索引。
         * @param value 要插入的元素值的右值引用。
         */
        void Insert(size_t index, T&& value)
        {
            if (index > Size()) { throw std::out_of_range("Insert index out of bounds"); }
            m_buffer->m_data.insert(m_buffer->m_data.begin() + index, std::move(value));
        }

        /**
         * @brief 移除指定位置的元素。
         * @param index 要移除的元素的索引。
         */
        void Erase(size_t index)
        {
            if (index >= Size()) { throw std::out_of_range("Erase index out of bounds"); }
            m_buffer->m_data.erase(m_buffer->m_data.begin() + index);
        }

        /**
         * @brief 移除指定范围的元素 [first, last)。
         * @param first 要移除的范围的起始索引。
         * @param last 要移除的范围的结束索引（不包含）。
         */
        void Erase(size_t first, size_t last)
        {
            if (first >= last || first >= Size()) return;
            last = std::min(last, Size());
            m_buffer->m_data.erase(m_buffer->m_data.begin() + first, m_buffer->m_data.begin() + last);
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
        [[nodiscard]] size_t Size() const noexcept { return m_buffer->m_data.size(); }
        /**
         * @brief 获取当前已分配的容量。
         * @return 容量
         */
        [[nodiscard]] size_t Capacity() const noexcept { return m_buffer->m_data.capacity(); }
        /**
         * @brief 判断数组是否为空。
         * @return 是否为空
         */
        [[nodiscard]] bool IsEmpty() const noexcept { return m_buffer->m_data.empty(); }

        /**
         * @brief 重载等于运算符,由vector直接复制给data。
         * @param vec 源vector
         * @return 当前Proxy引用
         */
        Proxy& operator=(const std::vector<T>& vec)
        {
            m_buffer->m_data = vec;
            return *this;
        }

        /**
         * @brief 与另一个 std::vector 交换内容。
         * @details 这是一个 O(1) 操作，用于高效接管数据。
         * @param other 要交换的向量
         */
        void Swap(std::vector<T>& other)
        {
            m_buffer->m_data.swap(other);
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

    /**
     * @brief 复制构造函数 (深拷贝)
     * @details 创建一个拥有独立数据副本的新 DynamicArray。
     * 只复制当前 Read Buffer 的内容作为一个新的快照。
     */
    DynamicArray(const DynamicArray& other)
    {
        m_buffers[0] = new Buffer();
        m_buffers[1] = new Buffer();
        m_buffers[2] = new Buffer();

        View other_view = other.GetView();

        if (!other_view.IsEmpty())
        {
            // 使用 std::vector 的拷贝构造函数进行深度复制
            m_buffers[0]->m_data = std::vector<T>(other_view.begin(), other_view.end());
        }

        m_readBuffer.store(m_buffers[0], std::memory_order_relaxed);
        m_writeBuffer = m_buffers[1];
        m_readyBuffer = m_buffers[2];
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
     * @brief 获取一个只读的数据视图。此操作极快且无锁。
     * @return 一个 View 对象，封装了当前数据快照。
     */
    [[nodiscard]] View GetView() const
    {
        // [Obsolete] 移除了 m_isWritingInPlace 标志的检查，
        // 因为该功能未实现，且会给每次读取带来不必要的原子操作开销。
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

        // 等待上一个 Ready Buffer 释放所有引用
        while (m_readyBuffer->m_refCount.load(std::memory_order_acquire) > 0)
        {
            /* spin */
        }

        // 将 Ready Buffer 切换为 Write Buffer
        std::swap(m_writeBuffer, m_readyBuffer);

        Buffer* currentReadBuffer = m_readBuffer.load(std::memory_order_acquire);

        // 关键算法：写时复制
        // 将当前 Read Buffer 的内容复制到新的 Write Buffer
        m_writeBuffer->m_data = currentReadBuffer->m_data;

        Proxy writerProxy(m_writeBuffer);
        modifier(writerProxy);

        // 发布：将 Write Buffer 切换为 Read Buffer
        m_readBuffer.store(m_writeBuffer, std::memory_order_release);

        // 将旧的 Read Buffer 切换为 Ready Buffer，等待其引用归零
        m_readyBuffer = currentReadBuffer;

        m_writerLock.clear(std::memory_order_release);
    }

    /**
     * @brief 清空并完全重构数据。
     * @tparam F 修改回调类型
     * @param modifier 一个 Lambda 函数，接收一个写入代理对象 (Proxy)。
     * @details
     * 这是一个高度优化的写入路径，适用于需要丢弃所有旧数据并从零开始
     * 构建新数据的场景。它避免了从 Read Buffer 拷贝旧数据的开销。
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

        // 优化点：不复制旧数据，直接清空 Write Buffer
        m_writeBuffer->m_data.clear();

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
        std::for_each(view.begin(), view.end(), func);
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
     */
    void Clear()
    {
        ClearAndModify([](auto& proxy)
        {
            // Proxy 已经是空的，什么都不用做
        });
    }

    /**
     * @brief 通过移动语义高效接管一个 std::vector 的数据。
     * @param vec 要从中移动数据的 std::vector（此操作后 vec 的内容将与
     * DynamicArray 的某个旧缓冲区交换）。
     * @return 当前 DynamicArray 实例的引用。
     */
    DynamicArray& operator=(std::vector<T>&& vec)
    {
        ClearAndModify([&vec](auto& proxy)
        {
            // 使用 O(1) swap 操作接管数据
            proxy.Swap(vec);
        });
        return *this;
    }

    /**
     * @brief 将当前快照转换为 std::vector。
     * @return 包含当前数据副本的 std::vector。
     */
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

    // [Obsolete] 移除了 m_isWritingInPlace，因为它用于一个未实现的功能，
    // 并且会给 GetView() 增加不必要的开销。
    // std::atomic<bool> m_isWritingInPlace{false};
};

#endif // DYNAMICARRAY_H
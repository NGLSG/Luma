#ifndef LUMAENGINE_THREAD_SAFE_VECTOR_H
#define LUMAENGINE_THREAD_SAFE_VECTOR_H

#include <vector>
#include <shared_mutex>
#include <functional>
#include <stdexcept>
#include <algorithm> // for std::find_if

/// <summary>
/// 提供 std::vector 的线程安全封装，API 命名风格与 std::vector 保持一致。
/// 使用 std::shared_mutex 实现高性能的读写分离锁定。
/// 为保证线程安全，此类不提供返回原始迭代器 (begin(), end()) 的接口，
/// 以防止迭代器失效问题。请使用 for_each 等安全迭代函数。
/// </summary>
template <class T>
class ThreadSafeVector
{
private:
    /// @brief 内部存储的 std::vector 实例。
    std::vector<T> data;

    /// @brief 用于同步访问的读写锁，mutable 允许在 const 成员函数中加锁。
    mutable std::shared_mutex mutex;

public:
    /// <summary>
    /// 默认构造函数。
    /// </summary>
    ThreadSafeVector() = default;

    // [实现] 构造函数和赋值运算符与上一版相同...
    ThreadSafeVector(const ThreadSafeVector& other) { std::shared_lock<std::shared_mutex> lock(other.mutex); data = other.data; }
    ThreadSafeVector(ThreadSafeVector&& other) noexcept { std::unique_lock<std::shared_mutex> lock(other.mutex); data = std::move(other.data); }
    ThreadSafeVector& operator=(const ThreadSafeVector& other) { if (this != &other) { std::unique_lock<std::shared_mutex> this_lock(mutex, std::defer_lock); std::shared_lock<std::shared_mutex> other_lock(other.mutex, std::defer_lock); std::lock(this_lock, other_lock); data = other.data; } return *this; }
    ThreadSafeVector& operator=(ThreadSafeVector&& other) noexcept { if (this != &other) { std::unique_lock<std::shared_mutex> this_lock(mutex, std::defer_lock); std::unique_lock<std::shared_mutex> other_lock(other.mutex, std::defer_lock); std::lock(this_lock, other_lock); data = std::move(other.data); } return *this; }


    // --- Modifiers ---
    void push_back(const T& value) { std::unique_lock<std::shared_mutex> lock(mutex); data.push_back(value); }
    template<typename... Args> void emplace_back(Args&&... args) { std::unique_lock<std::shared_mutex> lock(mutex); data.emplace_back(std::forward<Args>(args)...); }
    void pop_back() { std::unique_lock<std::shared_mutex> lock(mutex); if (!data.empty()) { data.pop_back(); } }
    void clear() { std::unique_lock<std::shared_mutex> lock(mutex); data.clear(); }

    // --- Element Access ---
    T at(size_t index) const { std::shared_lock<std::shared_mutex> lock(mutex); if (index >= data.size()) { throw std::out_of_range("ThreadSafeVector: index out of range."); } return data[index]; }

    // --- Capacity ---
    size_t size() const { std::shared_lock<std::shared_mutex> lock(mutex); return data.size(); }
    bool empty() const { std::shared_lock<std::shared_mutex> lock(mutex); return data.empty(); }

    // --- Safe Iteration & Complex Operations ---

    /// <summary>
    /// 在读锁保护下，对容器中的每个元素执行一个只读操作。
    /// 这是进行安全遍历的标准方式。
    /// </summary>
    /// <param name="func">要对每个元素执行的函数，接收 const T&。</param>
    void for_each(const std::function<void(const T&)>& func) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        std::for_each(data.begin(), data.end(), func);
    }

    /// <summary>
    /// 在写锁保护下，对容器中的每个元素执行一个可能修改元素的操作。
    /// </summary>
    /// <param name="func">要对每个元素执行的函数，接收 T&。</param>
    void for_each(const std::function<void(T&)>& func)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        std::for_each(data.begin(), data.end(), func);
    }

    /// <summary>
    /// 在读锁保护下，查找满足特定条件的第一个元素，并返回其值的拷贝。
    /// </summary>
    /// <param name="predicate">一元谓词函数，返回 bool。</param>
    /// <returns>如果找到，返回包含元素拷贝的 std::optional；否则返回 std::nullopt。</returns>
    std::optional<T> find_if(const std::function<bool(const T&)>& predicate) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto it = std::find_if(data.begin(), data.end(), predicate);
        if (it != data.end())
        {
            return *it;
        }
        return std::nullopt;
    }

    /// <summary>
    /// 在写锁保护下，对内部向量执行一个自定义的修改操作。
    /// 适用于需要直接访问迭代器进行删除、排序等复杂操作的场景。
    /// </summary>
    /// <param name="operation">一个函数或 lambda，接收对内部 std::vector 的非常量引用。</param>
    void apply_mutating_operation(const std::function<void(std::vector<T>&)>& operation)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        operation(data);
    }

    /// <summary>
    /// 创建并返回内部向量的线程安全副本。
    /// </summary>
    std::vector<T> to_vector() const { std::shared_lock<std::shared_mutex> lock(mutex); return data; }
};

#endif //LUMAENGINE_THREAD_SAFE_VECTOR_H
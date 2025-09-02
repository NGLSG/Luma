/**
 * @file JobSystem.h
 * @brief 定义了作业系统（JobSystem）及其相关接口。
 *
 * 该文件包含了作业（IJob）接口、作业句柄（JobHandle）以及核心的作业系统（JobSystem）类。
 * 作业系统负责管理和调度并发任务，利用线程池高效执行作业。
 */
#ifndef JOBSYSTEM_H
#define JOBSYSTEM_H

#include "LazySingleton.h"
#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>


/**
 * @brief 作业接口。
 *
 * 所有可由作业系统执行的任务都必须实现此接口。
 */
class IJob
{
public:
    /**
     * @brief 虚析构函数。
     *
     * 确保派生类对象能正确销毁。
     */
    virtual ~IJob() = default;

    /**
     * @brief 执行作业的具体逻辑。
     *
     * 派生类必须实现此方法来定义作业的行为。
     */
    virtual void Execute() = 0;
};


/**
 * @brief 作业句柄类型。
 *
 * 用于表示一个异步作业的未来结果，允许等待作业完成。
 */
using JobHandle = std::future<void>;


/**
 * @brief 作业系统。
 *
 * 一个基于工作窃取（work-stealing）的线程池，用于高效地调度和执行异步作业。
 * 继承自 LazySingleton，确保全局只有一个实例。
 */
class LUMA_API JobSystem : public LazySingleton<JobSystem>
{
public:
    friend class LazySingleton<JobSystem>;
    friend class ApplicationBase;

    /**
     * @brief 关闭作业系统。
     *
     * 停止所有工作线程并清理资源。
     */
    void Shutdown();

    /**
     * @brief 调度一个作业到作业系统执行。
     *
     * 作业将被放入队列，等待空闲线程执行。
     * @param job 指向要调度的作业对象的指针。作业系统会接管其所有权。
     * @return 一个 JobHandle，可用于等待作业完成。
     */
    JobHandle Schedule(IJob* job);

    /**
     * @brief 等待指定的作业完成。
     *
     * 这是一个静态方法，可以在任何地方调用以等待一个作业句柄。
     * @param handle 要等待完成的作业句柄。
     */
    static void Complete(JobHandle& handle);

    /**
     * @brief 等待所有指定的作业完成。
     *
     * 这是一个静态方法，用于等待一组作业句柄全部完成。
     * @param handles 包含要等待完成的作业句柄的向量。
     */
    static void CompleteAll(std::vector<JobHandle>& handles);

    /**
     * @brief 获取作业系统当前配置的线程数量。
     *
     * @return 作业系统使用的线程数量。
     */
    int GetThreadCount() const { return m_threadCount; }

private:
    // 私有方法和构造函数，不作为接口文档化
    void Initialize(int threadCount = 0);
    JobSystem() = default;
    ~JobSystem() override;

    void workerLoop(int threadIndex);

    std::packaged_task<void()> tryPopTaskFromLocalQueue();
    std::packaged_task<void()> tryStealTaskFromOtherQueues();

    using TaskQueue = std::deque<std::packaged_task<void()>>;

    int m_threadCount = 0; ///< 作业系统使用的线程数量。
    std::vector<std::thread> m_threads; ///< 工作线程的集合。
    std::vector<TaskQueue> m_taskQueues; ///< 每个工作线程的本地任务队列。
    std::vector<std::mutex> m_queueMutexes; ///< 保护每个任务队列的互斥锁。

    std::mutex m_globalMutex; ///< 用于保护全局状态或条件变量的互斥锁。
    std::condition_variable m_condition; ///< 用于线程等待新任务的条件变量。
    std::atomic<bool> m_stop = false; ///< 原子标志，指示作业系统是否正在停止。
    std::atomic<int> m_activeJobs = 0; ///< 当前正在执行或等待执行的作业数量。

    std::atomic<unsigned int> m_nextQueueIndex = 0; ///< 用于任务调度的下一个队列索引。
};

#endif
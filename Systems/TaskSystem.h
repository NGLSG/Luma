#ifndef TASKSYSTEM_H
#define TASKSYSTEM_H

#include <vector>
#include <future>
#include <queue>
#include <box2d/box2d.h> // 假设 b2TaskCallback 在此头文件中定义或被引用

namespace Systems
{
    /**
     * @brief 任务系统，用于管理和并行执行任务。
     *
     * 该系统维护一个线程池，可以将任务分发给空闲线程并行处理。
     */
    class TaskSystem
    {
    public:
        /**
         * @brief 构造函数，初始化任务系统。
         * @param threadCount 要创建的工作线程数量。
         */
        TaskSystem(int threadCount);

        /**
         * @brief 析构函数，清理任务系统资源，等待所有线程完成。
         */
        ~TaskSystem();

        /**
         * @brief 并行执行一个任务，类似于并行for循环。
         *
         * 将一个大任务分解成多个小任务，并分发给工作线程并行处理。
         * @param task 任务回调函数，定义了每个小任务的具体操作。
         * @param itemCount 任务项的总数量。
         * @param minRange 每个线程处理的最小范围或任务块大小。
         * @param taskContext 任务的上下文数据，将传递给任务回调函数。
         * @return 一个指向用户任务的标识符或句柄，可用于后续的Finish调用。
         */
        void* ParallelFor(b2TaskCallback* task, int itemCount, int minRange, void* taskContext);

        /**
         * @brief 等待指定的用户任务完成。
         *
         * 调用此函数将阻塞当前线程，直到由ParallelFor启动的指定用户任务完全执行完毕。
         * @param userTask 要等待完成的用户任务标识符。
         */
        void Finish(void* userTask);

    private:
        /**
         * @brief 工作线程的主循环函数。
         *
         * 每个工作线程都会运行此函数，从任务队列中取出任务并执行。
         * @param workerIndex 当前工作线程的索引。
         */
        void WorkerLoop(int workerIndex);

        std::vector<std::thread> m_threads; ///< 工作线程列表。

        std::queue<std::function<void(int workerIndex)>> m_taskQueue; ///< 待执行的任务队列。

        std::mutex m_queueMutex; ///< 保护任务队列的互斥锁。
        std::condition_variable m_condition; ///< 用于线程间通信的条件变量，通知工作线程有新任务或系统停止。
        bool m_stop = false; ///< 指示系统是否停止的标志。
    };
}

#endif
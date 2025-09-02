#include "TaskSystem.h"

namespace Systems
{
    struct TaskGroup
    {
        std::atomic<int> outstandingTasks = 0;
    };

    TaskSystem::TaskSystem(int threadCount)
    {
        for (int i = 0; i < threadCount; ++i)
        {
            m_threads.emplace_back(&TaskSystem::WorkerLoop, this, i);
        }
    }

    TaskSystem::~TaskSystem()
    {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }
        m_condition.notify_all();
        for (std::thread& thread : m_threads)
        {
            thread.join();
        }
    }

    void TaskSystem::WorkerLoop(int workerIndex)
    {
        while (true)
        {
            std::function<void(int)> task;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_condition.wait(lock, [this] { return m_stop || !m_taskQueue.empty(); });
                if (m_stop && m_taskQueue.empty()) return;
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
            task(workerIndex);
        }
    }

    void* TaskSystem::ParallelFor(b2TaskCallback* task, int itemCount, int minRange, void* taskContext)
    {
        if (itemCount == 0) return nullptr;

        TaskGroup* group = new TaskGroup();
        int taskCount = (itemCount + minRange - 1) / minRange;
        group->outstandingTasks = taskCount;

        int startIndex = 0;
        for (int i = 0; i < taskCount; ++i)
        {
            int endIndex = std::min(startIndex + minRange, itemCount);

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_taskQueue.emplace([task, startIndex, endIndex,i, taskContext, group](int workerIndex)
                {
                    task(startIndex, endIndex, workerIndex, taskContext);
                    group->outstandingTasks--;
                });
            }

            startIndex = endIndex;
        }

        m_condition.notify_all();
        return group;
    }

    void TaskSystem::Finish(void* userTask)
    {
        if (!userTask) return;
        TaskGroup* group = static_cast<TaskGroup*>(userTask);
        while (group->outstandingTasks > 0)
        {
            std::this_thread::yield();
        }
        delete group;
    }
}

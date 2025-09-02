#include "JobSystem.h"
#include <algorithm>
#include <random>


thread_local int s_threadIndex = -1;

thread_local std::mt19937 s_randomGenerator{std::random_device{}()};

JobSystem::~JobSystem()
{
    Shutdown();
}

void JobSystem::Initialize(int threadCount)
{
    if (m_threads.size() > 0) return;

    if (threadCount <= 0)
    {
        threadCount = std::max(1, (int)std::thread::hardware_concurrency() - 1);
    }
    m_threadCount = threadCount;

    m_taskQueues = std::vector<std::deque<std::packaged_task<void()>>>(m_threadCount);
    m_queueMutexes = std::vector<std::mutex>(m_threadCount);

    m_stop = false;
    for (int i = 0; i < m_threadCount; ++i)
    {
        m_threads.emplace_back(&JobSystem::workerLoop, this, i);
    }
}

void JobSystem::Shutdown()
{
    if (m_stop.exchange(true)) return;

    m_condition.notify_all();

    for (std::thread& thread : m_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    m_threads.clear();
    m_taskQueues.clear();
    m_queueMutexes.clear();
}

JobHandle JobSystem::Schedule(IJob* job)
{
    if (m_threadCount == 0)
    {
        std::lock_guard<std::mutex> lock(m_globalMutex);

        if (m_threadCount == 0)
        {
            Initialize(0);
        }
    }


    if (!job)
    {
        return {};
    }


    std::packaged_task<void()> task([job]() { job->Execute(); });
    JobHandle handle = task.get_future();

    m_activeJobs++;

    int queueIndex;
    if (s_threadIndex != -1)
    {
        queueIndex = s_threadIndex;
    }
    else
    {
        queueIndex = m_nextQueueIndex.fetch_add(1, std::memory_order_relaxed) % m_threadCount;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutexes[queueIndex]);
        m_taskQueues[queueIndex].push_back(std::move(task));
    }


    m_condition.notify_all();
    return handle;
}

void JobSystem::Complete(JobHandle& handle)
{
    if (handle.valid())
    {
        handle.wait();
    }
}

void JobSystem::CompleteAll(std::vector<JobHandle>& handles)
{
    for (auto& handle : handles)
    {
        Complete(handle);
    }
    handles.clear();
}

void JobSystem::workerLoop(int threadIndex)
{
    s_threadIndex = threadIndex;

    while (!m_stop)
    {
        std::packaged_task<void()> task;


        if (task = tryPopTaskFromLocalQueue(); !task.valid())
        {
            task = tryStealTaskFromOtherQueues();
        }

        if (task.valid())
        {
            task();
            m_activeJobs--;
        }
        else
        {
            std::unique_lock<std::mutex> lock(m_globalMutex);
            m_condition.wait(lock, [this]
            {
                return m_stop.load() || m_activeJobs.load() > 0;
            });
        }
    }
}

std::packaged_task<void()> JobSystem::tryPopTaskFromLocalQueue()
{
    std::unique_lock<std::mutex> lock(m_queueMutexes[s_threadIndex], std::try_to_lock);
    if (lock.owns_lock() && !m_taskQueues[s_threadIndex].empty())
    {
        auto task = std::move(m_taskQueues[s_threadIndex].back());
        m_taskQueues[s_threadIndex].pop_back();
        return task;
    }
    return {};
}

std::packaged_task<void()> JobSystem::tryStealTaskFromOtherQueues()
{
    std::uniform_int_distribution<int> distrib(0, m_threadCount - 1);
    int startIndex = distrib(s_randomGenerator);

    for (int i = 0; i < m_threadCount; ++i)
    {
        int victimIndex = (startIndex + i) % m_threadCount;
        if (victimIndex == s_threadIndex) continue;

        std::unique_lock<std::mutex> lock(m_queueMutexes[victimIndex], std::try_to_lock);
        if (lock.owns_lock() && !m_taskQueues[victimIndex].empty())
        {
            auto task = std::move(m_taskQueues[victimIndex].front());
            m_taskQueues[victimIndex].pop_front();
            return task;
        }
    }
    return {};
}

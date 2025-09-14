#ifndef LUMAENGINE_COMMANDQUEUE_H
#define LUMAENGINE_COMMANDQUEUE_H
#include <functional>
#include <queue>
#include <mutex>

class CommandQueue
{
public:
    /// @brief 将一个命令（函数）推入队列
    void Push(std::function<void()> command)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(command));
    }

    /// @brief 执行队列中的所有命令
    void Execute()
    {
        std::queue<std::function<void()>> commandsToExecute;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
            {
                return;
            }
            m_queue.swap(commandsToExecute);
        }

        while (!commandsToExecute.empty())
        {
            commandsToExecute.front()();
            commandsToExecute.pop();
        }
    }

private:
    std::queue<std::function<void()>> m_queue;
    std::mutex m_mutex;
};
#endif //LUMAENGINE_COMMANDQUEUE_H

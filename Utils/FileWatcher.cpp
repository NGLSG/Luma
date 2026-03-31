#include "FileWatcher.h"
#include "Logger.h"

FileWatcher::FileWatcher(const std::filesystem::path& watchDir, std::chrono::milliseconds interval)
    : m_watchDir(watchDir), m_interval(interval)
{
}

FileWatcher::~FileWatcher()
{
    Stop();
}

void FileWatcher::SetCallback(FileWatchCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callback = std::move(callback);
}

void FileWatcher::Start()
{
    if (m_running.load())
        return;

    if (!std::filesystem::exists(m_watchDir))
    {
        LogWarn("FileWatcher: Watch directory does not exist: {}", m_watchDir.string());
        return;
    }

    ScanDirectory(m_fileTimestamps);
    m_running.store(true);
    m_thread = std::thread(&FileWatcher::WatchLoop, this);
    LogInfo("FileWatcher: Started watching {}", m_watchDir.string());
}

void FileWatcher::Stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    if (m_thread.joinable())
        m_thread.join();

    LogInfo("FileWatcher: Stopped.");
}

bool FileWatcher::IsRunning() const
{
    return m_running.load();
}

void FileWatcher::ScanDirectory(std::unordered_map<std::string, std::filesystem::file_time_type>& snapshot)
{
    snapshot.clear();
    std::error_code ec;
    for (auto& entry : std::filesystem::recursive_directory_iterator(m_watchDir, ec))
    {
        if (ec)
            break;
        if (!entry.is_regular_file(ec))
            continue;

        auto path = entry.path().lexically_normal().string();
        auto lwt = entry.last_write_time(ec);
        if (!ec)
            snapshot[path] = lwt;
    }
}

void FileWatcher::WatchLoop()
{
    while (m_running.load())
    {
        std::this_thread::sleep_for(m_interval);
        if (!m_running.load())
            break;

        std::unordered_map<std::string, std::filesystem::file_time_type> current;
        ScanDirectory(current);

        FileWatchCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_callback;
        }
        if (!cb)
        {
            m_fileTimestamps = std::move(current);
            continue;
        }

        for (auto& [path, time] : current)
        {
            auto it = m_fileTimestamps.find(path);
            if (it == m_fileTimestamps.end())
            {
                cb({std::filesystem::path(path), FileChangeType::Created});
            }
            else if (it->second != time)
            {
                cb({std::filesystem::path(path), FileChangeType::Modified});
            }
        }

        for (auto& [path, time] : m_fileTimestamps)
        {
            if (current.find(path) == current.end())
            {
                cb({std::filesystem::path(path), FileChangeType::Deleted});
            }
        }

        m_fileTimestamps = std::move(current);
    }
}

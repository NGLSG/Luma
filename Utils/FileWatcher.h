#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <string>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

enum class FileChangeType
{
    Created,
    Modified,
    Deleted
};

struct FileChangeEvent
{
    std::filesystem::path path;
    FileChangeType type;
};

using FileWatchCallback = std::function<void(const FileChangeEvent&)>;

class FileWatcher
{
public:
    explicit FileWatcher(const std::filesystem::path& watchDir,
                         std::chrono::milliseconds interval = std::chrono::milliseconds(500));
    ~FileWatcher();

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    void SetCallback(FileWatchCallback callback);
    void Start();
    void Stop();
    bool IsRunning() const;

private:
    void WatchLoop();
    void ScanDirectory(std::unordered_map<std::string, std::filesystem::file_time_type>& snapshot);

    std::filesystem::path m_watchDir;
    std::chrono::milliseconds m_interval;
    FileWatchCallback m_callback;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::mutex m_callbackMutex;

    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;
};

#endif

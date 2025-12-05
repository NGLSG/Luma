#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H
#include "IEditorPanel.h"
#include "../../Utils/Logger.h"
#include "../../Event/LumaEvent.h"
#include <vector>
#include <string>
#include <chrono>
class ConsolePanel : public IEditorPanel
{
public:
    ConsolePanel() = default;
    ~ConsolePanel() override;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "控制台"; }
    void ClearLogs();
    void SetAutoScroll(bool enabled) { autoScroll = enabled; }
    size_t GetLogCount() const { return logEntries.size(); }
private:
    struct LogEntry
    {
        std::string message; 
        LogLevel level; 
        std::chrono::steady_clock::time_point timestamp; 
        int count; 
        bool isCollapsed; 
        LogEntry(std::string_view msg, LogLevel lvl)
            : message(msg), level(lvl), timestamp(std::chrono::steady_clock::now()), count(1), isCollapsed(false)
        {
        }
    };
    struct LogFilter
    {
        bool showInfo = true; 
        bool showWarning = true; 
        bool showError = true; 
        bool showDebug = true; 
    };
    void drawToolbar();
    void drawLogEntries();
    void drawLogEntry(const LogEntry& entry, int index);
    void onLogMessage(std::string_view message, LogLevel level);
    bool shouldShowLogEntry(const LogEntry& entry) const;
    ImVec4 getLogLevelColor(LogLevel level) const;
    const char* getLogLevelIcon(LogLevel level) const;
    const char* getLogLevelText(LogLevel level) const;
    std::string formatTimestamp(const std::chrono::steady_clock::time_point& timestamp) const;
    void scrollToBottom();
    void updateLogCounts();
    bool canCollapseWith(const LogEntry& entry1, const LogEntry& entry2) const;
    void collapseRepeatedMessages();
private:
    std::vector<LogEntry> logEntries; 
    LogFilter filter; 
    ListenerHandle logListenerHandle; 
    bool autoScroll = true; 
    bool scrollToBottomB = false; 
    bool collapseEnabled = true; 
    bool clearOnPlay = false; 
    char searchBuffer[256] = {0}; 
    int infoCount = 0; 
    int warningCount = 0; 
    int errorCount = 0; 
    int debugCount = 0; 
    int totalLogCount = 0; 
    static constexpr size_t MAX_LOG_ENTRIES = 2000; 
    mutable std::string timestampCache; 
    bool errorFilterActive = true; 
    bool warningFilterActive = true; 
    bool infoFilterActive = true; 
};
#endif

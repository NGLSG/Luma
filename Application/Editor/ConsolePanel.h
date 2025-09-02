#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include "IEditorPanel.h"
#include "../../Utils/Logger.h"
#include "../../Event/LumaEvent.h"
#include <vector>
#include <string>
#include <chrono>

/**
 * @brief 控制台面板类，用于在编辑器中显示和管理日志消息。
 *
 * 继承自 IEditorPanel，提供日志的显示、过滤、搜索和清除功能。
 */
class ConsolePanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    ConsolePanel() = default;
    /**
     * @brief 析构函数。
     */
    ~ConsolePanel() override;

    /**
     * @brief 初始化控制台面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新控制台面板的逻辑。
     * @param deltaTime 帧之间的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制控制台面板的用户界面。
     */
    void Draw() override;
    /**
     * @brief 关闭控制台面板。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C风格字符串。
     */
    const char* GetPanelName() const override { return "控制台"; }

    /**
     * @brief 清除所有日志消息。
     */
    void ClearLogs();

    /**
     * @brief 设置是否启用自动滚动到底部。
     * @param enabled 如果为true，则启用自动滚动；否则禁用。
     */
    void SetAutoScroll(bool enabled) { autoScroll = enabled; }

    /**
     * @brief 获取当前日志条目的数量。
     * @return 日志条目的数量。
     */
    size_t GetLogCount() const { return logEntries.size(); }

private:
    /**
     * @brief 表示单个日志条目的结构体。
     */
    struct LogEntry
    {
        std::string message; ///< 日志消息内容。
        LogLevel level; ///< 日志级别（信息、警告、错误等）。
        std::chrono::steady_clock::time_point timestamp; ///< 日志记录的时间戳。
        int count; ///< 如果消息被折叠，表示重复的次数。
        bool isCollapsed; ///< 指示此日志条目是否是折叠消息的代表。

        /**
         * @brief 构造函数。
         * @param msg 日志消息视图。
         * @param lvl 日志级别。
         */
        LogEntry(std::string_view msg, LogLevel lvl)
            : message(msg), level(lvl), timestamp(std::chrono::steady_clock::now()), count(1), isCollapsed(false)
        {
        }
    };

    /**
     * @brief 用于过滤日志显示的结构体。
     */
    struct LogFilter
    {
        bool showInfo = true; ///< 是否显示信息级别日志。
        bool showWarning = true; ///< 是否显示警告级别日志。
        bool showError = true; ///< 是否显示错误级别日志。
        bool showDebug = true; ///< 是否显示调试级别日志。
    };

    /**
     * @brief 绘制控制台面板的工具栏。
     */
    void drawToolbar();
    /**
     * @brief 绘制所有日志条目。
     */
    void drawLogEntries();
    /**
     * @brief 绘制单个日志条目。
     * @param entry 要绘制的日志条目。
     * @param index 日志条目的索引。
     */
    void drawLogEntry(const LogEntry& entry, int index);
    /**
     * @brief 处理接收到的日志消息。
     * @param message 日志消息内容。
     * @param level 日志级别。
     */
    void onLogMessage(std::string_view message, LogLevel level);
    /**
     * @brief 判断日志条目是否应该被显示。
     * @param entry 要判断的日志条目。
     * @return 如果日志条目应该被显示，则返回true；否则返回false。
     */
    bool shouldShowLogEntry(const LogEntry& entry) const;
    /**
     * @brief 获取指定日志级别的颜色。
     * @param level 日志级别。
     * @return 对应日志级别的ImVec4颜色。
     */
    ImVec4 getLogLevelColor(LogLevel level) const;
    /**
     * @brief 获取指定日志级别的图标。
     * @param level 日志级别。
     * @return 对应日志级别的图标字符串。
     */
    const char* getLogLevelIcon(LogLevel level) const;
    /**
     * @brief 获取指定日志级别的文本描述。
     * @param level 日志级别。
     * @return 对应日志级别的文本字符串。
     */
    const char* getLogLevelText(LogLevel level) const;
    /**
     * @brief 格式化时间戳为可读字符串。
     * @param timestamp 要格式化的时间点。
     * @return 格式化后的时间戳字符串。
     */
    std::string formatTimestamp(const std::chrono::steady_clock::time_point& timestamp) const;
    /**
     * @brief 滚动日志显示区域到底部。
     */
    void scrollToBottom();
    /**
     * @brief 更新日志计数（信息、警告、错误、调试）。
     */
    void updateLogCounts();
    /**
     * @brief 判断两个日志条目是否可以折叠在一起。
     * @param entry1 第一个日志条目。
     * @param entry2 第二个日志条目。
     * @return 如果可以折叠，则返回true；否则返回false。
     */
    bool canCollapseWith(const LogEntry& entry1, const LogEntry& entry2) const;
    /**
     * @brief 折叠重复的日志消息。
     */
    void collapseRepeatedMessages();

private:
    std::vector<LogEntry> logEntries; ///< 存储所有日志条目的列表。
    LogFilter filter; ///< 当前应用的日志过滤器设置。
    ListenerHandle logListenerHandle; ///< 日志事件监听器的句柄。

    bool autoScroll = true; ///< 是否启用自动滚动到底部。
    bool scrollToBottomB = false; ///< 标记是否需要滚动到底部（在下一帧处理）。
    bool collapseEnabled = true; ///< 是否启用折叠重复消息。
    bool clearOnPlay = false; ///< 是否在编辑器播放时清除日志。
    char searchBuffer[256] = {0}; ///< 用于日志搜索的缓冲区。

    int infoCount = 0; ///< 信息级别日志的数量。
    int warningCount = 0; ///< 警告级别日志的数量。
    int errorCount = 0; ///< 错误级别日志的数量。
    int debugCount = 0; ///< 调试级别日志的数量。
    int totalLogCount = 0; ///< 所有日志条目的总数。

    static constexpr size_t MAX_LOG_ENTRIES = 2000; ///< 控制台面板中允许的最大日志条目数。
    mutable std::string timestampCache; ///< 用于缓存时间戳格式化结果的字符串。

    bool errorFilterActive = true; ///< 错误过滤器是否激活。
    bool warningFilterActive = true; ///< 警告过滤器是否激活。
    bool infoFilterActive = true; ///< 信息过滤器是否激活。
};

#endif
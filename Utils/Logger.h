#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <string_view>
#include <source_location>
#include <chrono>
#include <format>
#include <mutex>
#include <utility>
#include <tuple>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../Components/Core.h"
#include "../Event/LumaEvent.h"

/**
 * @brief 日志级别枚举，定义日志输出的严重性等级。
 */
enum class LogLevel
{
    Trace,      ///< 跟踪信息
    Debug,      ///< 调试信息
    Info,       ///< 普通信息
    Warning,    ///< 警告信息
    Error,      ///< 错误信息
    Critical    ///< 严重错误
};

/**
 * @brief 日志回调类型，接收日志消息和日志级别。
 */
using LoggingCallback = LumaEvent<std::string_view, LogLevel>;

/**
 * @brief Logger 日志工具类，支持多级别日志输出、回调、控制台彩色输出等功能。
 */
class Logger
{
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 设置日志输出的最小级别。
     * @param level 最小日志级别
     */
    static void SetLevel(LogLevel level) { minLevel = level; }

    /**
     * @brief 获取当前日志输出的最小级别。
     * @return 当前最小日志级别
     */
    static LogLevel GetLevel() { return minLevel; }

    /**
     * @brief 启用或禁用控制台输出。
     * @param enabled 是否启用控制台输出
     */
    static void EnableConsoleOutput(bool enabled) { consoleOutputEnabled = enabled; }

    /**
     * @brief 获取日志消息回调对象。
     * @return 日志回调对象引用
     */
    static LoggingCallback& GetCallback() { return OnLogMessage; }

    /**
     * @brief 添加日志监听器。
     * @param listener 日志监听器函数
     * @return 监听器句柄
     */
    static ListenerHandle AddLogListener(LoggingCallback::Listener&& listener)
    {
        return OnLogMessage.AddListener(std::move(listener));
    }

    /**
     * @brief 移除日志监听器。
     * @param handle 监听器句柄
     * @return 移除成功返回 true，否则返回 false
     */
    static bool RemoveLogListener(ListenerHandle handle)
    {
        return OnLogMessage.RemoveListener(handle);
    }

public:
    /**
     * @brief 日志代理结构体，用于延迟格式化和输出日志。
     */
    struct LogProxy
    {
        LogLevel level;                    ///< 日志级别
        std::source_location location;     ///< 源码位置信息

        /**
         * @brief 日志输出操作符，支持格式化参数。
         * 修复点：不在 lambda 中构造 format_args 并返回，避免 format_args 指向已销毁的栈对象。
         * 通过先 transform 参数到 tuple，然后用 std::apply 展开并直接调用 Logger::log 模板，保证参数在 vformat 调用期间有效。
         *
         * @tparam Args 参数类型
         * @param fmt 格式化字符串
         * @param args 可变参数
         */
        template <typename... Args>
        void operator()(const char* fmt, Args&&... args) const
        {
            if (level < Logger::GetLevel())
            {
                return;
            }

            // 对参数进行必要的转换（例如 wstring -> string）
            auto transformedArgsTuple = std::make_tuple(transformLogArg(std::forward<Args>(args))...);

            // 将 tuple 展开并直接调用接收参数包的 log 模板函数，这样 std::make_format_args 会在 Logger::log 的栈帧内被建立并立即使用，
            // 避免了 format_args 指向 lambda 局部变量后悬垂的问题。
            std::apply(
                [&](auto&&... targs) {
                    Logger::log(level, location, fmt, std::forward<decltype(targs)>(targs)...);
                },
                transformedArgsTuple
            );
        }
    };

private:
    Logger() = default;

    /**
     * @brief 初始化日志系统（如设置控制台编码、VT100 支持等，仅限 Windows）。
     */
    static void initialize()
    {
        static bool isInitialized = false;
        if (isInitialized)
        {
            return;
        }

#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode))
            {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
        isInitialized = true;
    }

#ifdef _WIN32
    /**
     * @brief 将 std::wstring 转换为 UTF-8 std::string（仅限 Windows）。
     * @param wstr 宽字符串
     * @return 转换后的字符串
     */
    static std::string convertWstringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), NULL, 0, NULL,
                                             NULL);
        std::string strTo(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &strTo[0], sizeNeeded, NULL,
                            NULL);
        return strTo;
    }
#endif

    /**
     * @brief 日志参数类型转换，支持 wstring/wchar_t* 到 string 的转换。
     * @tparam T 参数类型
     * @param arg 参数
     * @return 转换后的参数
     */
    template <typename T>
    static auto transformLogArg(T&& arg)
    {
        using ArgType = std::decay_t<T>;

        if constexpr (std::is_same_v<ArgType, std::wstring>)
        {
#ifdef _WIN32
            return convertWstringToString(arg);
#else
            return "[unsupported wstring]";
#endif
        }
        else if constexpr (std::is_same_v<ArgType, const wchar_t*>)
        {
#ifdef _WIN32
            return convertWstringToString(std::wstring(arg ? arg : L""));
#else
            return "[unsupported wchar_t*]";
#endif
        }
        else
        {
            return std::forward<T>(arg);
        }
    }

    /**
     * @brief 实际日志输出实现（模板版本），接受参数包并在函数内构造 format_args 并立即用于 vformat，
     *        这样可以保证 format_args 内指针的目标在 vformat 调用期间仍然有效，从而避免悬垂指针导致的崩溃。
     *
     * @tparam Args 参数类型包
     * @param level 日志级别
     * @param location 源码位置信息
     * @param fmt 格式化字符串
     * @param args 参数包（已经经过 transform）
     */
    template <typename... Args>
    static void log(LogLevel level,
                    const std::source_location& location,
                    std::string_view fmt,
                    Args&&... args)
    {
        initialize();

        // 在当前栈帧内立即构造 format_args 并用于 vformat，保证引用安全
        const std::string message = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));

        // 触发回调，注意回调接收的是 string_view，监听方如果需要持久保存请复制内容
        OnLogMessage.Invoke(message, level);

        if (!consoleOutputEnabled) return;

        const auto now = std::chrono::system_clock::now();
        const auto timeT = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm timeInfo;
#ifdef _WIN32
        localtime_s(&timeInfo, &timeT);
#else
        localtime_r(&timeT, &timeInfo);
#endif

        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
        const std::string timestamp = std::format("{}.{:03}", timeBuffer, ms.count());

        std::string_view fileName = location.file_name();
        if (const auto pos = fileName.find_last_of("/\\"); pos != std::string_view::npos)
        {
            fileName.remove_prefix(pos + 1);
        }

        auto [levelStr, color] = getLevelMetadata(level);

        const std::string sourceInfo = std::format("{}:{}", fileName, location.line());
        const std::string fullMessage = std::format("[{}] [{:<8}] [{}] {}",
                                                    timestamp, levelStr, sourceInfo, message);

        std::scoped_lock lock(coutMutex);
        std::cout << "\033[38;2;" << static_cast<int>(color.r * 255) << ";"
            << static_cast<int>(color.g * 255) << ";"
            << static_cast<int>(color.b * 255) << "m"
            << fullMessage << "\033[0m" << std::endl;
    }

    /**
     * @brief 获取日志级别对应的字符串和颜色。
     * @param level 日志级别
     * @return 日志级别字符串和颜色
     */
    static std::pair<std::string_view, ECS::Color> getLevelMetadata(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace: return {"TRACE", {0.7f, 0.7f, 0.7f}};
        case LogLevel::Debug: return {"DEBUG", {0.5f, 0.5f, 1.0f}};
        case LogLevel::Info: return {"INFO", {0.9f, 0.9f, 0.9f}};
        case LogLevel::Warning: return {"WARNING", {1.0f, 1.0f, 0.0f}};
        case LogLevel::Error: return {"ERROR", {1.0f, 0.5f, 0.5f}};
        case LogLevel::Critical: return {"CRITICAL", {1.0f, 0.0f, 0.0f}};
        default: return {"UNKNOWN", {1.0f, 1.0f, 1.0f}};
        }
    }

    static inline LogLevel minLevel = LogLevel::Trace;           ///< 当前最小日志级别
    static inline std::mutex coutMutex;                          ///< 控制台输出互斥锁
    static inline bool consoleOutputEnabled = true;              ///< 是否启用控制台输出
    static inline LoggingCallback OnLogMessage;                  ///< 日志消息回调
};

/**
 * @brief 日志输出宏，自动带源码位置信息。
 */
#define LogTrace(fmt, ...)    Logger::LogProxy{LogLevel::Trace,    std::source_location::current()}(fmt, ##__VA_ARGS__)
#define LogInfo(fmt, ...)     Logger::LogProxy{LogLevel::Info,     std::source_location::current()}(fmt, ##__VA_ARGS__)
#define LogWarn(fmt, ...)     Logger::LogProxy{LogLevel::Warning,  std::source_location::current()}(fmt, ##__VA_ARGS__)
#define LogError(fmt, ...)    Logger::LogProxy{LogLevel::Error,    std::source_location::current()}(fmt, ##__VA_ARGS__)
#define LogCritical(fmt, ...) Logger::LogProxy{LogLevel::Critical, std::source_location::current()}(fmt, ##__VA_ARGS__)

#endif
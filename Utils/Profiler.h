#ifndef PROFILER_H
#define PROFILER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <chrono>
#include <deque>
#include <vector>
#include <imgui.h>

#include "LazySingleton.h"
#include "Platform.h"

/**
 * @brief ProfileResult 结构体，存储一次性能采样的结果。
 */
struct ProfileResult
{
    std::string name;           ///< 采样名称
    float timeMilliseconds;     ///< 执行耗时（毫秒）
    int64_t memoryDeltaBytes;   ///< 内存变化（字节）
    int callCount;              ///< 调用次数
};

/**
 * @brief Profiler 性能分析器，支持采样、历史记录、ImGui 可视化等功能。
 *
 * 继承自 LazySingleton，实现全局唯一实例。
 */
class Profiler : public LazySingleton<Profiler>
{
public:
    friend class LazySingleton<Profiler>;

    /**
     * @brief 更新采样器，定时采集性能数据。
     * @param deltaTimeSeconds 距上次更新的时间（秒）
     */
    void Update(float deltaTimeSeconds);

    /**
     * @brief 存储一次采样结果。
     * @param name 采样名称
     * @param timeMilliseconds 执行耗时（毫秒）
     * @param memoryDelta 内存变化（字节）
     */
    void StoreResult(std::string_view name, float timeMilliseconds, int64_t memoryDelta);

    /**
     * @brief 绘制性能分析器的 ImGui UI。
     */
    void DrawUI();

    /**
     * @brief 暂停采样。
     */
    void Pause();

    /**
     * @brief 恢复采样。
     */
    void Resume();

    /**
     * @brief 判断采样器是否处于暂停状态。
     * @return 暂停返回 true，否则返回 false
     */
    bool IsPaused() const;

private:
    /**
     * @brief 视图模式枚举。
     */
    enum class ViewMode
    {
        Timeline,   ///< 时间线模式
        Summary     ///< 汇总模式
    };

    Profiler();
    ~Profiler() = default;

    /**
     * @brief 采样并存储当前性能数据。
     */
    void sampleAndStore();

    /**
     * @brief 绘制时间线视图。
     */
    void drawTimelineView();

    /**
     * @brief 绘制汇总视图。
     */
    void drawSummaryView();

    const float collectionIntervalSeconds = 0.1f; ///< 采样间隔（秒）
    const size_t historySize = 600;               ///< 历史采样容量
    float timeSinceLastCollection = 0.0f;         ///< 距上次采样的时间

    std::vector<ProfileResult> m_collectingResults;                   ///< 当前采集结果
    std::deque<std::vector<ProfileResult>> m_historicalSamples;       ///< 历史采样结果
    std::deque<float> m_totalTimeHistory;                             ///< 总耗时历史
    std::unordered_map<std::string, std::deque<float>> m_scopedTimeHistory; ///< 各采样点耗时历史
    std::unordered_map<std::string, ImU32> m_scopeColors;             ///< 采样点颜色
    std::deque<std::string> m_mostExpensiveScopeHistory;              ///< 最耗时采样点历史

    bool m_isPaused = false;                  ///< 是否暂停采样
    int m_selectedSampleIndex = -1;           ///< 当前选中采样索引
    int m_hoveredSampleIndex = -1;            ///< 当前悬停采样索引
    int m_viewOffsetX = 0;                    ///< 视图 X 偏移
    int m_viewNumSamplesX = historySize;      ///< 视图显示采样数

    bool m_isFollowing = true;                ///< 是否自动跟随最新采样
    std::chrono::steady_clock::time_point m_lastInteractionTime; ///< 上次交互时间

    ViewMode m_currentViewMode = ViewMode::Timeline; ///< 当前视图模式
};

/**
 * @brief ScopedProfilerTimer 作用域性能计时器，自动记录作用域耗时和内存变化。
 *
 * 构造时记录起始时间和内存，析构时自动将结果提交到 Profiler。
 */
class ScopedProfilerTimer
{
public:
    /**
     * @brief 构造函数，开始计时和记录内存。
     * @param name 采样名称
     */
    ScopedProfilerTimer(std::string_view name)
        : m_name(name),
          m_startTime(std::chrono::high_resolution_clock::now()),
          m_startMemory(Platform::GetCurrentProcessMemoryUsage())
    {
    }

    /**
     * @brief 析构函数，自动提交采样结果。
     */
    ~ScopedProfilerTimer()
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        size_t endMemory = Platform::GetCurrentProcessMemoryUsage();

        float duration = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
        int64_t memoryDelta = static_cast<int64_t>(endMemory) - static_cast<int64_t>(m_startMemory);

        Profiler::GetInstance().StoreResult(m_name, duration, memoryDelta);
    }

private:
    std::string_view m_name;  ///< 采样名称
    size_t m_startMemory;     ///< 起始内存
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime; ///< 起始时间
};

/**
 * @brief 生成匿名变量名的辅助宏。
 */
#define ANONYMOUS_VARIABLE(str) ANONYMOUS_VARIABLE_IMPL(str, __LINE__)
#define ANONYMOUS_VARIABLE_IMPL(str, line) ANONYMOUS_VARIABLE_IMPL2(str, line)
#define ANONYMOUS_VARIABLE_IMPL2(str, line) str##line

/**
 * @brief 生成匿名变量名的辅助宏。
 */
#define ANONYMOUS_VARIABLE(str) ANONYMOUS_VARIABLE_IMPL(str, __LINE__)
#define ANONYMOUS_VARIABLE_IMPL(str, line) ANONYMOUS_VARIABLE_IMPL2(str, line)
#define ANONYMOUS_VARIABLE_IMPL2(str, line) str##line

/**
 * @brief 作用域性能分析宏，自动记录当前作用域的耗时和内存变化。
 * @param name 采样名称
 */
#define PROFILE_SCOPE(name) ScopedProfilerTimer ANONYMOUS_VARIABLE(profiler_timer)(name)

/**
 * @brief 函数级性能分析宏，自动记录当前函数的耗时和内存变化。
 *
 * 根据编译器自动选择最合适的函数签名宏。
 */
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#elif defined(__GNUC__) || defined(__clang__)
// GCC and Clang Compilers
#define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
// Fallback to standard __func__ for other compilers
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#endif


#endif // PROFILER_H
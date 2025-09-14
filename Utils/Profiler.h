#ifndef PROFILER_H
#define PROFILER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <chrono>
#include <deque>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <filesystem>
#include <imgui.h>

#include "LazySingleton.h"
#include "Platform.h"

/**
 * @struct ProfileNode
 * @brief 存储层级化性能分析数据的节点。
 *
 * 代表一个被分析的作用域，包含自身的性能数据以及所有子作用域的节点。
 */
struct ProfileNode
{
    std::string name;                                   ///< 采样名称
    float timeMilliseconds = 0.0f;                      ///< 执行总耗时（毫秒）
    int64_t memoryDeltaBytes = 0;                       ///< 内存变化（字节）
    int callCount = 0;                                  ///< 调用次数

    std::chrono::time_point<std::chrono::high_resolution_clock> startTime; ///< 作用域开始时间点
    size_t startMemory = 0;                             ///< 作用域开始时内存

    std::vector<std::unique_ptr<ProfileNode>> children; ///< 子节点列表
    ProfileNode* parent = nullptr;                      ///< 父节点指针
};

/**
 * @class Profiler
 * @brief 线程安全的性能分析器，支持层级化采样、历史记录和 ImGui 可视化。
 *
 * 继承自 LazySingleton，实现全局唯一实例。
 * 内部为每个线程维护独立的调用树，并在采样时合并，确保多线程环境下的数据准确性和稳定性。
 */
class Profiler : public LazySingleton<Profiler>
{
public:
    friend class LazySingleton<Profiler>;

    /**
     * @brief 标记一帧的结束，触发性能数据采样。
     * 此函数应在主循环的末尾每帧调用一次。
     */
    void Update();

    /**
     * @brief 开始一个新的性能分析作用域。
     * @param name 作用域的名称
     */
    void BeginScope(std::string_view name);

    /**
     * @brief 结束当前的性能分析作用域。
     */
    void EndScope();

    /**
     * @brief 绘制性能分析器的 ImGui UI。
     */
    void DrawUI();

    /**
     * @brief 将所有历史性能数据导出为 Chrome Tracing JSON 文件。
     * @param filepath 导出的目标文件路径。
     */
    void ExportToJSON(const std::filesystem::path& filepath);

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
     * @return 若暂停则返回 true，否则返回 false
     */
    bool IsPaused() const;

private:
    /**
     * @enum ViewMode
     * @brief UI 视图模式枚举。
     */
    enum class ViewMode
    {
        Timeline,   ///< 时间线视图
        Summary     ///< 汇总视图
    };

    /**
     * @struct ThreadData
     * @brief 存储每个线程独立的性能分析状态。
     */
    struct ThreadData
    {
        std::unique_ptr<ProfileNode> rootNode; ///< 线程的调用树根节点
        ProfileNode* currentNode = nullptr;    ///< 线程当前的活动节点
        bool hasData = false;                  ///< 标记本周期内是否有数据
    };

    Profiler();
    ~Profiler() = default;

    /**
     * @brief 对当前采集周期的数据进行采样并存储。
     */
    void sampleAndStore();

    /**
     * @brief 绘制时间线视图 UI。
     */
    void drawTimelineView();

    /**
     * @brief 绘制汇总视图 UI。
     */
    void drawSummaryView();

    /**
     * @brief 递归地绘制性能分析节点的树状表格。
     * @param node 当前要绘制的节点
     * @param frameRoot 整个采样帧的根节点，用于计算耗时百分比
     */
    void drawProfileNodeTree(const ProfileNode& node, const ProfileNode& frameRoot);

    /**
     * @brief 获取当前线程的性能数据结构。如果不存在则创建。
     * @return 当前线程的 ThreadData 引用
     */
    ThreadData& getCurrentThreadData();

    size_t m_historySize;                                                     ///< 历史采样容量
    std::mutex m_mutex;                                                       ///< 用于保护线程数据映射的互斥锁
    std::unordered_map<std::thread::id, ThreadData> m_threadData;             ///< 存储每个线程的性能数据

    std::deque<std::unique_ptr<ProfileNode>> m_historicalSamples; ///< 历史采样结果（每项都是一棵合并后的调用树根）
    std::deque<float> m_totalTimeHistory;                         ///< 总耗时历史
    std::unordered_map<std::string, std::deque<float>> m_scopedTimeHistory; ///< 各独立作用域耗时历史（用于绘图）
    std::unordered_map<std::string, ImU32> m_scopeColors;         ///< 作用域颜色
    std::deque<std::string> m_mostExpensiveScopeHistory;          ///< 最耗时作用域历史

    bool m_isPaused = false;                  ///< 是否暂停采样
    int m_selectedSampleIndex = -1;           ///< 当前选中采样索引
    int m_hoveredSampleIndex = -1;            ///< 当前悬停采样索引
    int m_viewOffsetX = 0;                    ///< 视图 X 轴偏移
    int m_viewNumSamplesX = 0;                ///< 视图 X 轴显示采样数

    bool m_isFollowing = true;                ///< 是否自动跟随最新采样
    std::chrono::steady_clock::time_point m_lastInteractionTime; ///< 上次交互时间

    ViewMode m_currentViewMode = ViewMode::Timeline; ///< 当前视图模式
};

/**
 * @class ScopedProfilerTimer
 * @brief 作用域性能计时器，自动记录作用域耗时和内存变化。
 */
class ScopedProfilerTimer
{
public:
    /**
     * @brief 构造函数，开始计时。
     * @param name 采样名称
     */
    ScopedProfilerTimer(std::string_view name)
    {
        Profiler::GetInstance().BeginScope(name);
    }

    /**
     * @brief 析构函数，结束计时并提交结果。
     */
    ~ScopedProfilerTimer()
    {
        Profiler::GetInstance().EndScope();
    }
};

#define ANONYMOUS_VARIABLE(str) ANONYMOUS_VARIABLE_IMPL(str, __LINE__)
#define ANONYMOUS_VARIABLE_IMPL(str, line) ANONYMOUS_VARIABLE_IMPL2(str, line)
#define ANONYMOUS_VARIABLE_IMPL2(str, line) str##line
#define PROFILE_SCOPE(name) ScopedProfilerTimer ANONYMOUS_VARIABLE(profiler_timer)(name)
#if defined(_MSC_VER)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#elif defined(__GNUC__) || defined(__clang__)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#endif

#endif // PROFILER_H
#include "Profiler.h"
#include "nlohmann/json.hpp" 
#include <SDL3/SDL_dialog.h>
#include <numeric>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>

#include "imgui_internal.h"
#define IM_PI 3.14159265358979323846f

using json = nlohmann::json;

namespace
{
    // Merge adjacent sibling nodes that share the same name to avoid duplicate rows like two consecutive "Update".
    static void CompactAdjacentDuplicates(ProfileNode& node)
    {
        // Recurse first to compact children of each child
        for (auto& child : node.children)
        {
            CompactAdjacentDuplicates(*child);
        }

        if (node.children.empty()) return;

        std::vector<std::unique_ptr<ProfileNode>> compacted;
        compacted.reserve(node.children.size());

        for (auto& current : node.children)
        {
            if (!compacted.empty() && compacted.back()->name == current->name)
            {
                // Merge current into the last compacted node
                auto& last = compacted.back();
                last->timeMilliseconds += current->timeMilliseconds;
                last->memoryDeltaBytes += current->memoryDeltaBytes;
                last->callCount += current->callCount;

                // Append children of current to last (preserve order)
                for (auto& grandChild : current->children)
                {
                    last->children.push_back(std::move(grandChild));
                }
            }
            else
            {
                compacted.push_back(std::move(current));
            }
        }

        node.children = std::move(compacted);
    }
    void AggregateNodeData(const ProfileNode& node, std::unordered_map<std::string, std::pair<float, int>>& aggregated)
    {
        if (node.name.rfind("线程", 0) != 0)
        {
            float childrenTime = 0.0f;
            for (const auto& child : node.children)
            {
                childrenTime += child->timeMilliseconds;
            }
            float selfTime = node.timeMilliseconds - childrenTime;

            auto it = aggregated.find(node.name);
            if (it == aggregated.end())
            {
                aggregated[node.name] = {selfTime, node.callCount};
            }
            else
            {
                it->second.first += selfTime;
                it->second.second += node.callCount;
            }
        }

        for (const auto& child : node.children)
        {
            AggregateNodeData(*child, aggregated);
        }
    }

    void FindMostExpensiveScope(const ProfileNode& node, std::string& mostExpensiveName, float& maxTime)
    {
        if (node.name.rfind("线程", 0) != 0)
        {
            float childrenTime = 0.0f;
            for (const auto& child : node.children)
            {
                childrenTime += child->timeMilliseconds;
            }
            float selfTime = node.timeMilliseconds - childrenTime;

            if (selfTime > maxTime)
            {
                maxTime = selfTime;
                mostExpensiveName = node.name;
            }
        }
        for (const auto& child : node.children)
        {
            FindMostExpensiveScope(*child, mostExpensiveName, maxTime);
        }
    }

    void ConvertNodeToTraceEvents(const ProfileNode& node, json& events, int pid, const std::string& tid_str, const std::chrono::time_point<std::chrono::high_resolution_clock>& globalStartTime)
    {
        if (node.name.rfind("线程", 0) == 0)
        {
             for (const auto& child : node.children)
             {
                 ConvertNodeToTraceEvents(*child, events, pid, tid_str, globalStartTime);
             }
             return;
        }

        long long ts = std::chrono::duration_cast<std::chrono::microseconds>(node.startTime - globalStartTime).count();
        long long dur = static_cast<long long>(node.timeMilliseconds * 1000.0f);

        json event;
        event["name"] = node.name;
        event["cat"] = "profiler";
        event["ph"] = "X";
        event["ts"] = ts;
        event["dur"] = dur;
        event["pid"] = pid;
        event["tid"] = tid_str;
        events.push_back(event);

        for (const auto& child : node.children)
        {
            ConvertNodeToTraceEvents(*child, events, pid, tid_str, globalStartTime);
        }
    }

    void SDLCALL OnJsonFileSelected(void* userdata, const char* const* filelist, int filter)
    {
        if (filelist && filelist[0])
        {
            Profiler* profiler = static_cast<Profiler*>(userdata);
            profiler->ExportToJSON(std::filesystem::path(filelist[0]));
        }
    }
}

Profiler::Profiler()
{
    m_historySize = 6400;
    m_viewNumSamplesX = m_historySize;
    m_lastInteractionTime = std::chrono::steady_clock::now();
}

Profiler::ThreadData& Profiler::getCurrentThreadData()
{
    std::thread::id this_id = std::this_thread::get_id();
    auto it = m_threadData.find(this_id);
    if (it == m_threadData.end())
    {
        it = m_threadData.emplace(this_id, ThreadData()).first;
        it->second.rootNode = std::make_unique<ProfileNode>();
        it->second.currentNode = it->second.rootNode.get();
    }
    return it->second;
}

void Profiler::Pause() { m_isPaused = true; }
void Profiler::Resume() { m_isPaused = false; }
bool Profiler::IsPaused() const { return m_isPaused; }

void Profiler::Update()
{
    if (m_isPaused)
    {
        return;
    }
    sampleAndStore();
}

void Profiler::BeginScope(std::string_view name)
{
    if (m_isPaused) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    ThreadData& data = getCurrentThreadData();
    data.hasData = true;

    data.currentNode->callCount++;

    auto newNode = std::make_unique<ProfileNode>();
    newNode->name = name;
    newNode->parent = data.currentNode;
    newNode->startTime = std::chrono::high_resolution_clock::now();
    newNode->startMemory = Platform::GetCurrentProcessMemoryUsage();

    data.currentNode = newNode.get();
    data.currentNode->parent->children.push_back(std::move(newNode));
}

void Profiler::EndScope()
{
    if (m_isPaused) return;

    auto endTime = std::chrono::high_resolution_clock::now();
    size_t endMemory = Platform::GetCurrentProcessMemoryUsage();

    std::lock_guard<std::mutex> lock(m_mutex);
    ThreadData& data = getCurrentThreadData();

    if (data.currentNode && data.currentNode->parent)
    {
        data.currentNode->timeMilliseconds = std::chrono::duration<float, std::milli>(endTime - data.currentNode->startTime).count();
        data.currentNode->memoryDeltaBytes = static_cast<int64_t>(endMemory) - static_cast<int64_t>(data.currentNode->startMemory);
        data.currentNode = data.currentNode->parent;
    }
}

void Profiler::sampleAndStore()
{
    auto mergedRoot = std::make_unique<ProfileNode>();
    mergedRoot->name = "[采集帧根节点]";
    float totalTime = 0.0f;

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, data] : m_threadData)
    {
        if (!data.hasData) continue;

        std::stringstream ss;
        ss << "线程 " << id;
        auto threadNode = std::make_unique<ProfileNode>();
        threadNode->name = ss.str();

        for (auto& child : data.rootNode->children)
        {
            threadNode->timeMilliseconds += child->timeMilliseconds;
            threadNode->children.push_back(std::move(child));
        }

        // Compact duplicate adjacent entries to reduce visual duplication (e.g., two consecutive Update scopes)
        CompactAdjacentDuplicates(*threadNode);
        totalTime += threadNode->timeMilliseconds;
        mergedRoot->children.push_back(std::move(threadNode));

        data.rootNode = std::make_unique<ProfileNode>();
        data.currentNode = data.rootNode.get();
        data.hasData = false;
    }

    m_historicalSamples.push_back(std::move(mergedRoot));
    while(m_historicalSamples.size() > m_historySize)
    {
        m_historicalSamples.pop_front();
        if (m_selectedSampleIndex == 0) m_selectedSampleIndex = -1;
        else if (m_selectedSampleIndex > 0) m_selectedSampleIndex--;
    }

    std::unordered_map<std::string, std::pair<float, int>> aggregated;
    if(!m_historicalSamples.empty())
    {
        AggregateNodeData(*m_historicalSamples.back(), aggregated);
    }

    std::string mostExpensiveScope;
    float maxTime = 0.0f;
    if (!m_historicalSamples.empty())
    {
        FindMostExpensiveScope(*m_historicalSamples.back(), mostExpensiveScope, maxTime);
    }

    m_mostExpensiveScopeHistory.push_back(mostExpensiveScope);
    while(m_mostExpensiveScopeHistory.size() > m_historySize)
    {
        m_mostExpensiveScopeHistory.pop_front();
    }

    m_totalTimeHistory.push_back(totalTime);
    while(m_totalTimeHistory.size() > m_historySize)
    {
        m_totalTimeHistory.pop_front();
    }

    for (const auto& [name, data] : aggregated)
    {
        m_scopedTimeHistory.try_emplace(name, std::deque<float>());
        m_scopedTimeHistory[name].push_back(data.first);
    }

    for (auto& [name, history] : m_scopedTimeHistory)
    {
        if (aggregated.find(name) == aggregated.end())
        {
            history.push_back(0.0f);
        }
        while(history.size() > m_historySize)
        {
            history.pop_front();
        }
    }
}

void Profiler::ExportToJSON(const std::filesystem::path& filepath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_historicalSamples.empty()) return;

    json traceEvents = json::array();

    auto globalStartTime = std::chrono::high_resolution_clock::time_point::max();
    for (const auto& frame : m_historicalSamples) {
        for (const auto& threadNode : frame->children) {
            if (!threadNode->children.empty()) {
                globalStartTime = std::min(globalStartTime, threadNode->children[0]->startTime);
            }
        }
    }

    if (globalStartTime == std::chrono::high_resolution_clock::time_point::max()) return;

    int pid = 1;
    for (const auto& frame : m_historicalSamples)
    {
        for (const auto& threadNode : frame->children)
        {
            ConvertNodeToTraceEvents(*threadNode, traceEvents, pid, threadNode->name, globalStartTime);
        }
    }

    std::ofstream file(filepath);
    if (file.is_open())
    {
        file << traceEvents.dump(4);
    }
}

void Profiler::drawTimelineView()
{
    static float topPaneHeight = 250.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    if (ImGui::BeginChild("ProfilerGraph", ImVec2(0, topPaneHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        const int totalSampleCount = static_cast<int>(m_totalTimeHistory.size());

        if (!m_isFollowing && !m_isPaused)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<float>(now - m_lastInteractionTime).count();
            if (elapsed > 30.0f)
            {
                m_isFollowing = true;
                m_selectedSampleIndex = -1;
            }
        }

        if (m_isFollowing)
        {
            m_viewOffsetX = std::max(0, totalSampleCount - m_viewNumSamplesX);
        }

        m_hoveredSampleIndex = -1;
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        if (canvas_sz.x < 50.0f || canvas_sz.y < 50.0f)
        {
            ImGui::EndChild();
            ImGui::PopStyleVar();
            return;
        }
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##performance_plot", canvas_sz);
        const bool isPlotHovered = ImGui::IsItemHovered();

        if (isPlotHovered && totalSampleCount > 0)
        {
            float mouseX = ImGui::GetMousePos().x - canvas_p0.x;
            m_hoveredSampleIndex = m_viewOffsetX + static_cast<int>((mouseX / canvas_sz.x) * m_viewNumSamplesX);
            m_hoveredSampleIndex = std::clamp(m_hoveredSampleIndex, 0, totalSampleCount - 1);

            auto disableFollow = [&]()
            {
                if (m_isFollowing) m_isFollowing = false;
                m_lastInteractionTime = std::chrono::steady_clock::now();
            };

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_selectedSampleIndex = m_hoveredSampleIndex;
                disableFollow();
            }

            if (ImGui::GetIO().MouseWheel != 0)
            {
                float zoomDelta = -ImGui::GetIO().MouseWheel * 20.0f;
                float mouseRatio = mouseX / canvas_sz.x;
                int newNumSamplesX = std::clamp(static_cast<int>(m_viewNumSamplesX + zoomDelta), 20, totalSampleCount);
                int newOffsetX = m_viewOffsetX + static_cast<int>(mouseRatio * (m_viewNumSamplesX - newNumSamplesX));
                m_viewNumSamplesX = newNumSamplesX;
                m_viewOffsetX = newOffsetX;
                disableFollow();
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            {
                float dragDeltaX = -ImGui::GetIO().MouseDelta.x;
                int sampleDelta = static_cast<int>((dragDeltaX / canvas_sz.x) * m_viewNumSamplesX);
                m_viewOffsetX += sampleDelta;
                disableFollow();
            }
        }

        m_viewOffsetX = std::clamp(m_viewOffsetX, 0, std::max(0, totalSampleCount - m_viewNumSamplesX));
        m_viewNumSamplesX = std::clamp(m_viewNumSamplesX, 20, totalSampleCount);

        drawList->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(40, 40, 40, 255));

        float yMax = 0.0f;
        int viewStart = m_viewOffsetX;
        int viewEnd = std::min(m_viewOffsetX + m_viewNumSamplesX, totalSampleCount);
        for (int i = viewStart; i < viewEnd; ++i) { yMax = std::max(yMax, m_totalTimeHistory[i]); }
        yMax = std::max(yMax, 16.6f) * 1.1f;

        const ImU32 colorRed = IM_COL32(255, 50, 50, 255);
        for (const auto& [name, history] : m_scopedTimeHistory)
        {
            if (history.size() < 2) continue;
            if (m_scopeColors.find(name) == m_scopeColors.end())
            {
                std::hash<std::string> hasher;
                float hue = (hasher(name) % 1000) / 1000.0f;
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.75f, 0.85f, r, g, b);
                m_scopeColors[name] = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
            }

            for (int i = viewStart; i < viewEnd - 1; ++i)
            {
                if (i + 1 >= history.size()) continue;

                ImVec2 p1(canvas_p0.x + (float)(i - viewStart) / m_viewNumSamplesX * canvas_sz.x, canvas_p1.y - (history[i] / yMax) * canvas_sz.y);
                ImVec2 p2(canvas_p0.x + (float)(i + 1 - viewStart) / m_viewNumSamplesX * canvas_sz.x, canvas_p1.y - (history[i + 1] / yMax) * canvas_sz.y);

                const bool isMostExpensive = (i < m_mostExpensiveScopeHistory.size() && m_mostExpensiveScopeHistory[i] == name);
                drawList->AddLine(p1, p2, isMostExpensive ? colorRed : m_scopeColors.at(name), 2.0f);
            }
        }

        auto drawIndicator = [&](int index, ImU32 color)
        {
            if (index >= viewStart && index < viewEnd)
            {
                float lineX = canvas_p0.x + static_cast<float>(index - viewStart) / m_viewNumSamplesX * canvas_sz.x;
                drawList->AddLine(ImVec2(lineX, canvas_p0.y), ImVec2(lineX, canvas_p1.y), color, 1.0f);
            }
        };
        drawIndicator(m_hoveredSampleIndex, IM_COL32(255, 255, 255, 100));
        drawIndicator(m_selectedSampleIndex, IM_COL32(255, 255, 255, 220));

        drawList->AddRect(canvas_p0, canvas_p1, IM_COL32(100, 100, 100, 255));
    }
    ImGui::EndChild();

    ImGui::InvisibleButton("vsplitter", ImVec2(-1, 8.0f));
    if (ImGui::IsItemActive())
    {
        topPaneHeight += ImGui::GetIO().MouseDelta.y;
        topPaneHeight = std::clamp(topPaneHeight, 50.0f, ImGui::GetContentRegionAvail().y - 50.0f);
    }

    if (ImGui::BeginChild("ProfilerDetails", ImVec2(0, 0)))
    {
        int indexToShow = (m_selectedSampleIndex != -1) ? m_selectedSampleIndex : (m_historicalSamples.empty() ? -1 : (int)m_historicalSamples.size() - 1);

        if (indexToShow != -1 && indexToShow < m_historicalSamples.size())
        {
            const ProfileNode& rootNode = *m_historicalSamples[indexToShow];
            if (ImGui::BeginTable("profilingTree", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("模块 / 函数", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("总耗时 (ms)");
                ImGui::TableSetupColumn("自身耗时 (ms)");
                ImGui::TableSetupColumn("总耗时占比");
                ImGui::TableSetupColumn("调用次数");
                ImGui::TableSetupColumn("内存变化");
                ImGui::TableHeadersRow();

                for(const auto& child : rootNode.children)
                {
                    drawProfileNodeTree(*child, rootNode);
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Text("暂无数据。(在上方时间线中点击一个采样点以查看详情)");
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void Profiler::drawProfileNodeTree(const ProfileNode& node, const ProfileNode& frameRoot)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAllColumns;
    if (node.children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    }

    bool isThreadNode = node.name.rfind("线程", 0) == 0;
    if (isThreadNode)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.8f, 1.0f, 1.0f));
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool opened = ImGui::TreeNodeEx(node.name.c_str(), flags, "%s", node.name.c_str());
    if (isThreadNode) ImGui::PopStyleColor();


    ImGui::TableNextColumn();

    float totalFrameTime = 0.0f;
    for (const auto& child : frameRoot.children) totalFrameTime += child->timeMilliseconds;

    ImGui::Text("%.4f", node.timeMilliseconds);

    ImGui::TableNextColumn();
    float childrenTime = 0.0f;
    for (const auto& child : node.children) childrenTime += child->timeMilliseconds;
    float selfTime = node.timeMilliseconds - childrenTime;
    ImGui::Text("%.4f", selfTime);

    ImGui::TableNextColumn();
    float percentage = totalFrameTime > 0 ? (node.timeMilliseconds / totalFrameTime) * 100.0f : 0.0f;
    ImGui::Text("%.2f%%", percentage);

    ImGui::TableNextColumn();
    ImGui::Text("%d", node.callCount);

    ImGui::TableNextColumn();
    if (node.memoryDeltaBytes > 0) ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "+%.2f KB", (float)node.memoryDeltaBytes / 1024.0f);
    else if (node.memoryDeltaBytes < 0) ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.2f KB", (float)node.memoryDeltaBytes / 1024.0f);
    else ImGui::Text("0 B");

    if (opened)
    {
        for (const auto& child : node.children)
        {
            drawProfileNodeTree(*child, frameRoot);
        }
        ImGui::TreePop();
    }
}

void Profiler::drawSummaryView()
{
    struct SummaryResult
    {
        std::string name;
        float averageTimePerSample;
        float averageTimePerCall;
        int64_t averageMemoryDelta;
        float proportion;
    };

    std::vector<SummaryResult> summaryResults;
    float totalAverageTime = 0.0f;

    if (!m_historicalSamples.empty())
    {
        std::unordered_map<std::string, std::tuple<double, long long, long long>> totals;
        std::function<void(const ProfileNode&)> accumulate =
            [&](const ProfileNode& node)
            {
                if (node.name.rfind("线程", 0) != 0 && node.name != "[采集帧根节点]")
                {
                    std::get<0>(totals[node.name]) += node.timeMilliseconds;
                    std::get<1>(totals[node.name]) += node.callCount;
                    std::get<2>(totals[node.name]) += node.memoryDeltaBytes;
                }
                for (const auto& child : node.children)
                {
                    accumulate(*child);
                }
            };

        for (const auto& sampleRoot : m_historicalSamples)
        {
            accumulate(*sampleRoot);
        }

        for (const auto& [name, data] : totals)
        {
            const auto& [timeSum, callCount, memSum] = data;
            if (timeSum <= 1e-5f) continue;

            SummaryResult res;
            res.name = name;
            res.averageTimePerSample = static_cast<float>(timeSum) / m_historicalSamples.size();
            res.averageTimePerCall = (callCount > 0) ? static_cast<float>(timeSum) / callCount : 0.0f;
            res.averageMemoryDelta = (callCount > 0) ? memSum / callCount : 0;
            summaryResults.push_back(res);
            totalAverageTime += res.averageTimePerSample;
        }
    }

    for (auto& res : summaryResults)
    {
        res.proportion = (totalAverageTime > 0) ? res.averageTimePerSample / totalAverageTime : 0.0f;
    }
    std::sort(summaryResults.begin(), summaryResults.end(), [](const auto& a, const auto& b)
    {
        return a.averageTimePerSample > b.averageTimePerSample;
    });

    ImGui::Text("性能汇总平均值 (最近 %zu 条采样)", m_historicalSamples.size());
    ImGui::Separator();

    if (summaryResults.empty())
    {
        ImGui::Text("暂无有效的性能数据可供汇总。");
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 region = ImGui::GetContentRegionAvail();
    const float radius = std::min(region.x * 0.4f, region.y * 0.4f);
    const ImVec2 pieCenter(ImGui::GetCursorScreenPos().x + radius + 30.0f, ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y * 0.5f);

    float startAngle = -IM_PI / 2.0f;
    const int segments = 64;

    for (const auto& res : summaryResults)
    {
        float angle = res.proportion * 2.0f * IM_PI;
        ImU32 color = m_scopeColors.count(res.name) ? m_scopeColors.at(res.name) : IM_COL32_WHITE;

        drawList->PathArcTo(pieCenter, radius, startAngle, startAngle + angle, segments);
        drawList->PathLineTo(pieCenter);
        drawList->PathFillConvex(color);

        if (res.proportion > 0.03f)
        {
            float textAngle = startAngle + angle * 0.5f;
            ImVec2 textPos(pieCenter.x + cosf(textAngle) * radius * 0.6f, pieCenter.y + sinf(textAngle) * radius * 0.6f);

            ImVec4 colorVec = ImColor(color);
            float luminance = colorVec.x * 0.299f + colorVec.y * 0.587f + colorVec.z * 0.114f;
            ImU32 textColor = luminance > 0.5f ? IM_COL32_BLACK : IM_COL32_WHITE;

            ImVec2 textSize = ImGui::CalcTextSize(res.name.c_str());
            drawList->AddText(ImVec2(textPos.x - textSize.x * 0.5f, textPos.y - textSize.y * 0.5f), textColor, res.name.c_str());
        }
        startAngle += angle;
    }

    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 delta = ImVec2(mousePos.x - pieCenter.x, mousePos.y - pieCenter.y);
    if (sqrtf(delta.x * delta.x + delta.y * delta.y) <= radius)
    {
        float angle = atan2f(delta.y, delta.x) + IM_PI / 2.0f;
        if (angle < 0) angle += 2.0f * IM_PI;
        float accumulatedProportion = 0.0f;

        for (const auto& res : summaryResults)
        {
            if (angle >= accumulatedProportion * 2.0f * IM_PI && angle < (accumulatedProportion + res.proportion) * 2.0f * IM_PI)
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImColor(m_scopeColors.count(res.name) ? m_scopeColors.at(res.name) : IM_COL32_WHITE), "■ %s", res.name.c_str());
                ImGui::Separator();
                ImGui::Text("平均耗时 / 帧:   %.4f ms", res.averageTimePerSample);
                ImGui::Text("平均耗时 / 调用:   %.4f ms", res.averageTimePerCall);
                ImGui::Text("占比:        %.2f %%", res.proportion * 100.0f);
                ImGui::EndTooltip();
                break;
            }
            accumulatedProportion += res.proportion;
        }
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(pieCenter.x + radius + 40.0f);

    if (ImGui::BeginChild("LegendChild", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar))
    {
        if (ImGui::BeginTable("summary_legend", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("##颜色", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("模块 / 函数", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("平均 / 帧 (ms)");
            ImGui::TableSetupColumn("平均 / 调用 (ms)");
            ImGui::TableSetupColumn("平均内存 (KB)");
            ImGui::TableHeadersRow();

            for (const auto& res : summaryResults)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImU32 color = m_scopeColors.count(res.name) ? m_scopeColors.at(res.name) : IM_COL32_WHITE;
                ImGui::ColorButton(res.name.c_str(), ImColor(color).Value, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(15, 15));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(res.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", res.averageTimePerSample);
                ImGui::TableNextColumn();
                ImGui::Text("%.4f", res.averageTimePerCall);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", (float)res.averageMemoryDelta / 1024.0f);
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

void Profiler::DrawUI()
{
    if (!ImGui::Begin("性能分析器"))
    {
        ImGui::End();
        return;
    }

    if (m_isPaused) { if (ImGui::Button("继续")) Resume(); }
    else { if (ImGui::Button("暂停")) Pause(); }
    ImGui::SameLine();
    ImGui::TextUnformatted(m_isPaused ? "(已暂停)" : "(运行中)");

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_currentViewMode == ViewMode::Timeline ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    if (ImGui::Button("时间线")) m_currentViewMode = ViewMode::Timeline;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_currentViewMode == ViewMode::Summary ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    if (ImGui::Button("汇总")) m_currentViewMode = ViewMode::Summary;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    if (ImGui::Button("导出JSON"))
    {
        const SDL_DialogFileFilter filters[] = { {"JSON", "json"} };
        SDL_ShowSaveFileDialog(OnJsonFileSelected, this, NULL, filters, 1, "profiler_trace.json");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    ImGui::Text("历史帧数:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    int historySize = static_cast<int>(m_historySize);
    if (ImGui::InputInt("##HistorySize", &historySize))
    {
        m_historySize = std::max(100, historySize);
    }

    if (m_currentViewMode == ViewMode::Timeline)
    {
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Checkbox("跟随最新", &m_isFollowing))
        {
            if (m_isFollowing) m_selectedSampleIndex = -1;
            m_lastInteractionTime = std::chrono::steady_clock::now();
        }
    }
    ImGui::Separator();

    switch (m_currentViewMode)
    {
    case ViewMode::Timeline:
        drawTimelineView();
        break;
    case ViewMode::Summary:
        drawSummaryView();
        break;
    }

    ImGui::End();
}

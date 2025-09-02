#include "Profiler.h"
#include <numeric>
#include <algorithm>
#include <functional>
#define IM_PI 3.14159265358979323846f

Profiler::Profiler()
{
    m_lastInteractionTime = std::chrono::steady_clock::now();
}

void Profiler::Pause() { m_isPaused = true; }
void Profiler::Resume() { m_isPaused = false; }
bool Profiler::IsPaused() const { return m_isPaused; }

void Profiler::Update(float deltaTimeSeconds)
{
    if (m_isPaused)
    {
        return;
    }

    timeSinceLastCollection += deltaTimeSeconds;
    if (timeSinceLastCollection >= collectionIntervalSeconds)
    {
        timeSinceLastCollection -= collectionIntervalSeconds;
        sampleAndStore();
    }
}

void Profiler::StoreResult(std::string_view name, float timeMilliseconds, int64_t memoryDelta)
{
    if (m_isPaused)
    {
        return;
    }
    m_collectingResults.push_back({std::string(name), timeMilliseconds, memoryDelta, 0});
}

void Profiler::sampleAndStore()
{
    std::unordered_map<std::string, ProfileResult> aggregated;
    float totalTime = 0.0f;

    for (const auto& res : m_collectingResults)
    {
        totalTime += res.timeMilliseconds;
        auto it = aggregated.find(res.name);
        if (it == aggregated.end())
        {
            aggregated[res.name] = {res.name, res.timeMilliseconds, res.memoryDeltaBytes, 1};
        }
        else
        {
            it->second.timeMilliseconds += res.timeMilliseconds;
            it->second.memoryDeltaBytes += res.memoryDeltaBytes;
            it->second.callCount++;
        }
    }
    m_collectingResults.clear();

    std::vector<ProfileResult> currentSampleResults;
    for (auto const& [name, result] : aggregated)
    {
        currentSampleResults.push_back(result);
    }

    std::sort(currentSampleResults.begin(), currentSampleResults.end(),
              [](const ProfileResult& a, const ProfileResult& b)
              {
                  return a.timeMilliseconds > b.timeMilliseconds;
              });

    m_historicalSamples.push_back(std::move(currentSampleResults));
    if (m_historicalSamples.size() > historySize)
    {
        m_historicalSamples.pop_front();
        if (m_selectedSampleIndex == 0)
        {
            m_selectedSampleIndex = -1;
        }
        else if (m_selectedSampleIndex > 0)
        {
            m_selectedSampleIndex--;
        }
    }

    std::string mostExpensiveScope;
    if (!m_historicalSamples.back().empty())
    {
        mostExpensiveScope = m_historicalSamples.back()[0].name;
    }
    m_mostExpensiveScopeHistory.push_back(mostExpensiveScope);
    if (m_mostExpensiveScopeHistory.size() > historySize)
    {
        m_mostExpensiveScopeHistory.pop_front();
    }

    m_totalTimeHistory.push_back(totalTime);
    if (m_totalTimeHistory.size() > historySize)
    {
        m_totalTimeHistory.pop_front();
    }

    for (auto const& [name, result] : aggregated)
    {
        if (m_scopedTimeHistory.find(name) == m_scopedTimeHistory.end())
        {
            m_scopedTimeHistory[name] = std::deque<float>();
        }
        m_scopedTimeHistory[name].push_back(result.timeMilliseconds);
    }

    for (auto& pair : m_scopedTimeHistory)
    {
        if (aggregated.find(pair.first) == aggregated.end())
        {
            pair.second.push_back(0.0f);
        }
        if (pair.second.size() > historySize)
        {
            pair.second.pop_front();
        }
    }
}

void Profiler::drawTimelineView()
{
    static float topPaneHeight = 250.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    if (ImGui::BeginChild("ProfilerGraph", ImVec2(0, topPaneHeight), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
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

                ImVec2 p1(canvas_p0.x + (float)(i - viewStart) / m_viewNumSamplesX * canvas_sz.x,
                          canvas_p1.y - (history[i] / yMax) * canvas_sz.y);
                ImVec2 p2(canvas_p0.x + (float)(i + 1 - viewStart) / m_viewNumSamplesX * canvas_sz.x,
                          canvas_p1.y - (history[i + 1] / yMax) * canvas_sz.y);

                const bool isMostExpensive = (i < m_mostExpensiveScopeHistory.size() && m_mostExpensiveScopeHistory[i]
                    == name);
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
        int indexToShow = (m_selectedSampleIndex != -1)
                              ? m_selectedSampleIndex
                              : (m_historicalSamples.empty() ? -1 : (int)m_historicalSamples.size() - 1);

        if (indexToShow != -1 && indexToShow < m_historicalSamples.size())
        {
            const std::vector<ProfileResult>& resultsToShow = m_historicalSamples[indexToShow];
            if (ImGui::BeginTable("profiling", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable |
                                  ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Module / Function", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Total Time (ms)");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("在一个0.1秒采样周期内，该函数运行的总时长。");
                ImGui::TableSetupColumn("Avg Time (ms)");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Total Time / Call Count.\n这是该函数单次调用的平均耗时。");
                ImGui::TableSetupColumn("Call Count");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("在一个0.1秒采样周期内，该函数被调用的总次数。");
                ImGui::TableSetupColumn("Memory Delta");
                ImGui::TableHeadersRow();

                for (const auto& result : resultsToShow)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(result.name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    if (!resultsToShow.empty() && result.name == resultsToShow[0].name)
                        ImGui::TextColored(
                            ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%.4f", result.timeMilliseconds);
                    else ImGui::Text("%.4f", result.timeMilliseconds);
                    ImGui::TableSetColumnIndex(2);
                    float avgTime = result.callCount > 0 ? result.timeMilliseconds / result.callCount : 0.0f;
                    ImGui::Text("%.4f", avgTime);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%d", result.callCount);
                    ImGui::TableSetColumnIndex(4);
                    if (result.memoryDeltaBytes > 0)
                        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "+%.2f KB",
                                           (float)result.memoryDeltaBytes / 1024.0f);
                    else if (result.memoryDeltaBytes < 0)
                        ImGui::TextColored(
                            ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.2f KB", (float)result.memoryDeltaBytes / 1024.0f);
                    else ImGui::Text("0 B");
                }
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Text("No data available. (Click a point in the timeline above to select a frame)");
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
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

        for (const auto& sample : m_historicalSamples)
        {
            for (const auto& result : sample)
            {
                std::get<0>(totals[result.name]) += result.timeMilliseconds;
                std::get<1>(totals[result.name]) += result.callCount;
                std::get<2>(totals[result.name]) += result.memoryDeltaBytes;
            }
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

    ImGui::Text("Average Performance Summary (up to last 60s)");
    ImGui::Separator();

    if (summaryResults.empty())
    {
        ImGui::Text("No significant performance data to display.");
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 region = ImGui::GetContentRegionAvail();
    const float radius = std::min(region.x * 0.4f, region.y * 0.4f);
    const ImVec2 pieCenter(ImGui::GetCursorScreenPos().x + radius + 30.0f,
                           ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y * 0.5f);

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
            ImVec2 textPos(pieCenter.x + cosf(textAngle) * radius * 0.6f,
                           pieCenter.y + sinf(textAngle) * radius * 0.6f);

            ImVec4 colorVec = ImColor(color);
            float luminance = colorVec.x * 0.299f + colorVec.y * 0.587f + colorVec.z * 0.114f;
            ImU32 textColor = luminance > 0.5f ? IM_COL32_BLACK : IM_COL32_WHITE;

            ImVec2 textSize = ImGui::CalcTextSize(res.name.c_str());
            drawList->AddText(ImVec2(textPos.x - textSize.x * 0.5f, textPos.y - textSize.y * 0.5f), textColor,
                              res.name.c_str());
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
            if (angle >= accumulatedProportion * 2.0f * IM_PI && angle < (accumulatedProportion + res.proportion) * 2.0f
                * IM_PI)
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImColor(m_scopeColors.count(res.name) ? m_scopeColors.at(res.name) : IM_COL32_WHITE),
                                   "■ %s", res.name.c_str());
                ImGui::Separator();
                ImGui::Text("Avg Time / 0.1s: %.4f ms", res.averageTimePerSample);
                ImGui::Text("Avg Time / Call:   %.4f ms", res.averageTimePerCall);
                ImGui::Text("Proportion:        %.2f %%", res.proportion * 100.0f);
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
        if (ImGui::BeginTable("summary_legend", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("##Color", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Module / Function", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Avg / 0.1s (ms)");
            ImGui::TableSetupColumn("Avg / Call (ms)");
            ImGui::TableSetupColumn("Avg Mem (KB)");
            ImGui::TableHeadersRow();

            for (const auto& res : summaryResults)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImU32 color = m_scopeColors.count(res.name) ? m_scopeColors.at(res.name) : IM_COL32_WHITE;
                ImGui::ColorButton(res.name.c_str(), ImColor(color).Value,
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(15, 15));
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
    ImGui::Begin("Profiler");

    if (m_isPaused) { if (ImGui::Button("Resume")) Resume(); }
    else { if (ImGui::Button("Pause")) Pause(); }
    ImGui::SameLine();
    ImGui::TextUnformatted(m_isPaused ? "(Paused)" : "(Running)");

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_currentViewMode == ViewMode::Timeline
                                               ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]
                                               : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    if (ImGui::Button("Timeline")) m_currentViewMode = ViewMode::Timeline;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_currentViewMode == ViewMode::Summary
                                               ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]
                                               : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    if (ImGui::Button("Summary")) m_currentViewMode = ViewMode::Summary;
    ImGui::PopStyleColor();

    if (m_currentViewMode == ViewMode::Timeline)
    {
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Checkbox("Follow Latest", &m_isFollowing))
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

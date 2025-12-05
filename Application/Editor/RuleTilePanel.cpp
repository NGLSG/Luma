#include "RuleTilePanel.h"
#include "imgui.h"
#include "Resources/AssetManager.h"
#include "Resources/Loaders/RuleTileLoader.h"
#include "Utils/Logger.h"
#include "Utils/InspectorUI.h"
#include <fstream>
#include "Resources/RuntimeAsset/RuntimeScene.h"
void RuleTilePanel::Initialize(EditorContext* context)
{
    m_context = context;
}
void RuleTilePanel::Update(float)
{
    if (!m_isVisible) return;
    if (m_context->currentEditingRuleTileGuid.Valid() &&
        m_context->currentEditingRuleTileGuid != m_currentRuleTileGuid)
    {
        openRuleTile(m_context->currentEditingRuleTileGuid);
    }
    else if (!m_context->currentEditingRuleTileGuid.Valid() && m_currentRuleTileGuid.Valid())
    {
        closeCurrentRuleTile();
    }
}
void RuleTilePanel::Draw()
{
    if (!m_isVisible) return;
    std::string title = GetPanelName();
    if (m_currentRuleTileGuid.Valid())
    {
        const auto* meta = AssetManager::GetInstance().GetMetadata(m_currentRuleTileGuid);
        if (meta) title += std::string(" - ") + meta->assetPath.filename().string();
    }
    if (ImGui::Begin(title.c_str(), &m_isVisible))
    {
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (!m_currentRuleTileGuid.Valid())
        {
            ImGui::Text("请从资源浏览器双击一个 RuleTile 资产以开始编辑");
        }
        else
        {
            if (ImGui::Button("保存")) { saveCurrentRuleTile(); }
            ImGui::SameLine();
            if (ImGui::Button("关闭")) { m_context->currentEditingRuleTileGuid = Guid(); }
            ImGui::Separator();
            if (InspectorUI::DrawAssetHandle("默认瓦片", m_editingData.defaultTileHandle, *m_context->uiCallbacks))
            {
            }
            ImGui::Separator();
            drawRuleList();
        }
    }
    ImGui::End();
}
void RuleTilePanel::Shutdown()
{
    closeCurrentRuleTile();
}
void RuleTilePanel::openRuleTile(Guid guid)
{
    m_currentRuleTileGuid = Guid();
    m_editingData = RuleTileAssetData{};
    RuleTileLoader loader;
    sk_sp<RuntimeRuleTile> rt = loader.LoadAsset(guid);
    if (!rt)
    {
        LogError("无法加载 RuleTile 资产: {}", guid.ToString());
        m_context->currentEditingRuleTileGuid = Guid();
        return;
    }
    m_currentRuleTileGuid = guid;
    m_editingData = rt->GetData();
}
void RuleTilePanel::closeCurrentRuleTile()
{
    m_currentRuleTileGuid = Guid();
    m_editingData = RuleTileAssetData{};
}
void RuleTilePanel::saveCurrentRuleTile()
{
    if (!m_currentRuleTileGuid.Valid()) return;
    const auto* meta = AssetManager::GetInstance().GetMetadata(m_currentRuleTileGuid);
    if (!meta)
    {
        LogError("找不到 RuleTile 元数据，保存失败");
        return;
    }
    YAML::Node node = YAML::convert<RuleTileAssetData>::encode(m_editingData);
    std::ofstream out(AssetManager::GetInstance().GetAssetsRootPath() / meta->assetPath);
    out << node;
    out.close();
    LogInfo("RuleTile 资产已保存: {}", meta->assetPath.string());
}
void RuleTilePanel::drawRuleList()
{
    if (ImGui::Button("添加规则"))
    {
        Rule r;
        r.resultTileHandle = {};
        r.neighbors.fill(NeighborRule::DontCare);
        m_editingData.rules.push_back(r);
    }
    for (int i = 0; i < static_cast<int>(m_editingData.rules.size()); ++i)
    {
        ImGui::PushID(i);
        bool open = ImGui::CollapsingHeader((std::string("规则 ") + std::to_string(i)).c_str(),
                                            ImGuiTreeNodeFlags_DefaultOpen);
        if (open)
        {
            {
                const char* delLabel = "删除规则";
                ImVec2 textSize = ImGui::CalcTextSize(delLabel);
                float buttonWidth = textSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
                float cursorX = ImGui::GetCursorPosX();
                float avail = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(cursorX + std::max(0.0f, avail - buttonWidth));
                if (ImGui::Button(delLabel))
                {
                    m_editingData.rules.erase(m_editingData.rules.begin() + i);
                    ImGui::PopID();
                    --i;
                    continue;
                }
            }
            ImGui::Spacing();
            InspectorUI::DrawAssetHandle("结果瓦片", m_editingData.rules[i].resultTileHandle, *m_context->uiCallbacks);
            ImGui::Text("邻居规则:");
            drawRuleGrid(i);
        }
        ImGui::PopID();
    }
}
void RuleTilePanel::drawRuleGrid(int ruleIndex)
{
    if (ruleIndex < 0 || ruleIndex >= static_cast<int>(m_editingData.rules.size())) return;
    Rule& rule = m_editingData.rules[ruleIndex];
    const float cell = 36.0f;
    const float pad = 4.0f;
    const ImU32 colBorder = IM_COL32(180, 180, 180, 255);
    const ImU32 colHover = IM_COL32(255, 255, 0, 160);
    const ImU32 colCenter = IM_COL32(0, 150, 255, 200);
    const ImU32 colText = IM_COL32(255, 255, 255, 255);
    const ImU32 colX = IM_COL32(220, 80, 80, 255);
    const ImU32 colCheck = IM_COL32(80, 220, 120, 255);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    const int map3[3][3] = {
        {0, 1, 2},
        {7, -1, 3},
        {6, 5, 4}
    };
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            ImGui::PushID(r * 3 + c);
            ImVec2 p0 = {origin.x + c * (cell + pad), origin.y + r * (cell + pad)};
            ImVec2 p1 = {p0.x + cell, p0.y + cell};
            ImGui::SetCursorScreenPos(p0);
            ImGui::InvisibleButton("cell", ImVec2(cell, cell));
            bool hovered = ImGui::IsItemHovered();
            bool clicked = ImGui::IsItemClicked();
            dl->AddRect(p0, p1, hovered ? colHover : colBorder, 4.0f, 0, 1.5f);
            int idx = map3[r][c];
            if (idx >= 0)
            {
                if (clicked)
                {
                    int v = static_cast<int>(rule.neighbors[idx]);
                    v = (v + 1) % 3;
                    rule.neighbors[idx] = static_cast<NeighborRule>(v);
                    if (m_context && m_context->uiCallbacks) m_context->uiCallbacks->onValueChanged();
                }
                NeighborRule state = rule.neighbors[idx];
                const char* mark = "";
                ImU32 markColor = colText;
                if (state == NeighborRule::MustNotBeThis)
                {
                    mark = "X";
                    markColor = colX;
                }
                else if (state == NeighborRule::MustBeThis)
                {
                    mark = "\xE2\x9C\x93";
                    markColor = colCheck;
                }
                if (mark[0] != '\0')
                {
                    ImVec2 ts = ImGui::CalcTextSize(mark);
                    ImVec2 center = {(p0.x + p1.x - ts.x) * 0.5f, (p0.y + p1.y - ts.y) * 0.5f};
                    dl->AddText(center, markColor, mark);
                }
            }
            else
            {
                ImVec2 center = {(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
                float radius = cell * 0.35f;
                dl->AddCircle(center, radius, colCenter, 24, 2.0f);
            }
            ImGui::PopID();
        }
    }
    ImGui::Dummy(ImVec2(3 * cell + 2 * pad, 3 * cell + 2 * pad));
    ImGui::TextDisabled("提示: 单击格子以在 空/\xE2\x9C\x93/X 之间切换，中心为该规则的放置内容");
}

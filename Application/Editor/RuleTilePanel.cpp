#include "RuleTilePanel.h"

#include "imgui.h"
#include "Resources/AssetManager.h"
#include "Resources/Loaders/RuleTileLoader.h"
#include "Utils/Logger.h"
#include "Utils/InspectorUI.h"
#include <fstream>

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

    static const char* kNeighborNames[8] = {"NW", "N", "NE", "E", "SE", "S", "SW", "W"};
    static const char* kRuleLabels[] = {"任意", "同类", "不同"};

    for (int i = 0; i < static_cast<int>(m_editingData.rules.size()); ++i)
    {
        ImGui::PushID(i);
        bool open = ImGui::CollapsingHeader((std::string("规则 ") + std::to_string(i)).c_str(),
                                            ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::SameLine();
        if (ImGui::SmallButton("删除"))
        {
            m_editingData.rules.erase(m_editingData.rules.begin() + i);
            ImGui::PopID();
            --i;
            continue;
        }
        if (open)
        {
            
            InspectorUI::DrawAssetHandle("结果瓦片", m_editingData.rules[i].resultTileHandle, *m_context->uiCallbacks);

            
            ImGui::Text("邻居规则:");
            for (int n = 0; n < 8; ++n)
            {
                ImGui::PushID(n);
                ImGui::SetNextItemWidth(120.0f);
                int v = static_cast<int>(m_editingData.rules[i].neighbors[n]);
                if (ImGui::Combo(kNeighborNames[n], &v, kRuleLabels, IM_ARRAYSIZE(kRuleLabels)))
                {
                    m_editingData.rules[i].neighbors[n] = static_cast<NeighborRule>(v);
                }
                if ((n % 4) != 3) ImGui::SameLine();
                ImGui::PopID();
            }
        }
        ImGui::PopID();
    }
}


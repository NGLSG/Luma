#include "AssetInspectorPanel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "AssetImporterRegistry.h"
#include "Resources/AssetManager.h"
#include "Utils/Logger.h"
#include <fstream>

#include "Profiler.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"

void AssetInspectorPanel::Initialize(EditorContext* context)
{
    m_context = context;
}

void AssetInspectorPanel::Draw()
{
    PROFILE_FUNCTION();
    ImGui::Begin(GetPanelName(), &m_isVisible);
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (m_context->selectedAssets != m_currentEditingPaths)
    {
        resetStateFromSelection();
    }

    drawInspectorUI();

    ImGui::End();
}

void AssetInspectorPanel::Shutdown()
{
    m_currentEditingPaths.clear();
    m_editingSettings.reset();
    m_mixedValueProperties.clear();
    m_dirtyProperties.clear();
}


void AssetInspectorPanel::resetStateFromSelection()
{
    m_currentEditingPaths = m_context->selectedAssets;
    m_editingSettings.reset();
    m_mixedValueProperties.clear();
    m_dirtyProperties.clear();
    m_editingAssetType = AssetType::Unknown;

    if (m_currentEditingPaths.empty()) return;

    const AssetMetadata* firstMetadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[0]);
    if (!firstMetadata)
    {
        m_currentEditingPaths.clear();
        return;
    }

    m_editingAssetType = firstMetadata->type;


    for (size_t i = 1; i < m_currentEditingPaths.size(); ++i)
    {
        const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[i]);
        if (!metadata || metadata->type != m_editingAssetType)
        {
            m_editingAssetType = AssetType::Unknown;
            m_editingSettings.reset();
            return;
        }
    }

    m_editingSettings = firstMetadata->importerSettings;


    if (m_currentEditingPaths.size() > 1)
    {
        const auto* registration = AssetImporterRegistry::GetInstance().Get(m_editingAssetType);
        if (!registration) return;

        for (const auto& [propName, propInfo] : registration->properties)
        {
            const YAML::Node& firstValueNode = m_editingSettings[propName];
            if (!firstValueNode) continue;

            for (size_t i = 1; i < m_currentEditingPaths.size(); ++i)
            {
                const AssetMetadata* otherMetadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[i]);
                const YAML::Node& otherValueNode = otherMetadata->importerSettings[propName];


                if (!otherValueNode || firstValueNode.Scalar() != otherValueNode.Scalar())
                {
                    m_mixedValueProperties.insert(propName);
                    break;
                }
            }
        }
    }
}

void AssetInspectorPanel::drawInspectorUI()
{
    if (m_currentEditingPaths.empty())
    {
        ImGui::Text("请在资源浏览器中选择一个或多个资产以查看其设置。");
        return;
    }

    if (m_editingAssetType == AssetType::Unknown)
    {
        ImGui::Text("选择了多个不同类型的资产。请选择相同类型的资产进行批量编辑。");
        return;
    }

    if (m_currentEditingPaths.size() == 1)
    {
        ImGui::Text("资产: %s", m_currentEditingPaths[0].filename().string().c_str());
    }
    else
    {
        ImGui::Text("正在编辑 %zu 个资产", m_currentEditingPaths.size());
    }
    ImGui::Separator();

    const auto* registration = AssetImporterRegistry::GetInstance().Get(m_editingAssetType);
    if (!registration || !m_editingSettings)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "无法编辑的资产 (未注册导入设置)");
        return;
    }

    for (const auto& [name, prop] : registration->properties)
    {
        if (prop.isExposedInEditor)
        {
            bool isMixed = m_mixedValueProperties.count(name);
            if (isMixed)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
            }

            if (prop.draw_ui(name, m_editingSettings))
            {
                m_dirtyProperties.insert(name);
                if (isMixed)
                {
                    m_mixedValueProperties.erase(name);
                }
            }

            if (isMixed)
            {
                ImGui::PopItemFlag();
            }
        }
    }


    if (!m_dirtyProperties.empty())
    {
        if (ImGui::Button("应用"))
        {
            applyChanges();
        }
        ImGui::SameLine();
        if (ImGui::Button("撤销"))
        {
            resetStateFromSelection();
        }
    }
}

void AssetInspectorPanel::applyChanges()
{
    if (m_currentEditingPaths.empty() || m_dirtyProperties.empty()) return;

    LogInfo("正在为 %zu 个资产应用 %zu 项设置更改...", m_currentEditingPaths.size(), m_dirtyProperties.size());

    for (const auto& assetPath : m_currentEditingPaths)
    {
        const AssetMetadata* originalMetadata = AssetManager::GetInstance().GetMetadata(assetPath);
        if (!originalMetadata)
        {
            LogWarn("找不到资产 {} 的元数据，跳过保存。", assetPath.string());
            continue;
        }

        YAML::Node newSettings = originalMetadata->importerSettings;
        for (const std::string& propName : m_dirtyProperties)
        {
            newSettings[propName] = m_editingSettings[propName];
        }

        saveMetadataToFile(*originalMetadata, newSettings);
        EventBus::GetInstance().Publish(AssetUpdatedEvent{originalMetadata->type, originalMetadata->guid});
    }


    m_dirtyProperties.clear();
    resetStateFromSelection();
}


void AssetInspectorPanel::saveMetadataToFile(const AssetMetadata& originalMetadata, const YAML::Node& newSettings)
{
    AssetMetadata updatedMeta = originalMetadata;
    updatedMeta.importerSettings = newSettings;
    AssetManager::GetInstance().ReImport(updatedMeta);
}

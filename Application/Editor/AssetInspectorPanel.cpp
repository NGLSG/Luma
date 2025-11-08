#include "AssetInspectorPanel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "AssetImporterRegistry.h"
#include "Resources/AssetManager.h"
#include "Utils/Logger.h"
#include "Profiler.h"
#include <fstream>
#include <any> 
#include "Resources/RuntimeAsset/RuntimeScene.h"

#include "Editor.h"
#include "TextureSlicerPanel.h"

void AssetInspectorPanel::Initialize(EditorContext* context)
{
    m_context = context;
}

void AssetInspectorPanel::Update(float deltaTime)
{
}

void AssetInspectorPanel::Draw()
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("AssetInspectorPanel::ImGui::Begin");
        ImGui::Begin(GetPanelName(), &m_isVisible);
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    }

    {
        PROFILE_SCOPE("AssetInspectorPanel::CheckSelectionChange");
        
        if (m_context->selectedAssets != m_currentEditingPaths)
        {
            
            resetStateFromSelection();
        }
    }

    {
        PROFILE_SCOPE("AssetInspectorPanel::drawInspectorUI");
        
        drawInspectorUI();
    }

    {
        PROFILE_SCOPE("AssetInspectorPanel::ImGui::End");
        ImGui::End();
    }
}

void AssetInspectorPanel::Shutdown()
{
    
    m_currentEditingPaths.clear();
    m_deserializedSettings.reset();
    m_isDeserialized = false;
    m_mixedValueProperties.clear();
    m_dirtyProperties.clear();
}

void AssetInspectorPanel::resetStateFromSelection()
{
    PROFILE_FUNCTION();

    
    m_currentEditingPaths = m_context->selectedAssets;

    
    m_deserializedSettings.reset();
    m_isDeserialized = false;
    m_mixedValueProperties.clear();
    m_dirtyProperties.clear();
    m_editingAssetType = AssetType::Unknown;

    if (m_currentEditingPaths.empty()) return;

    
    const AssetMetadata* firstMetadata = nullptr;
    {
        PROFILE_SCOPE("AssetInspectorPanel::GetFirstMetadata");
        firstMetadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[0]);
    }

    if (!firstMetadata)
    {
        m_currentEditingPaths.clear();
        return;
    }

    m_editingAssetType = firstMetadata->type;

    {
        PROFILE_SCOPE("AssetInspectorPanel::ValidateMultiSelection");
        for (size_t i = 1; i < m_currentEditingPaths.size(); ++i)
        {
            const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[i]);
            if (!metadata || metadata->type != m_editingAssetType)
            {
                
                m_editingAssetType = AssetType::Unknown;
                m_deserializedSettings.reset();
                m_isDeserialized = false;
                return;
            }
        }
    }

    
    const auto* registration = AssetImporterRegistry::GetInstance().Get(m_editingAssetType);
    if (!registration || !registration->Deserialize)
    {
        LogWarn("资产类型 {} (ID: {}) 没有注册反序列化函数。",
                firstMetadata->assetPath.string(), (int)m_editingAssetType);
        m_editingAssetType = AssetType::Unknown;
        return;
    }

    try
    {
        PROFILE_SCOPE("AssetInspectorPanel::DeserializeSettings");
        
        YAML::Node sourceSettings = firstMetadata->importerSettings;

        
        
        m_deserializedSettings = registration->Deserialize(sourceSettings);
        m_isDeserialized = true;
    }
    catch (const std::exception& e)
    {
        LogError("反序列化资产 {} 的设置失败: {}", firstMetadata->assetPath.string(), e.what());
        m_isDeserialized = false;
        m_deserializedSettings.reset();
    }


    
    {
        PROFILE_SCOPE("AssetInspectorPanel::DetectMixedValues");
        if (m_currentEditingPaths.size() > 1)
        {
            if (!registration) return; 

            
            for (const auto& [propName, propInfo] : registration->properties)
            {
                const YAML::Node& firstValueNode = firstMetadata->importerSettings[propName];
                if (!firstValueNode) continue;

                for (size_t i = 1; i < m_currentEditingPaths.size(); ++i)
                {
                    const AssetMetadata* otherMetadata = AssetManager::GetInstance().GetMetadata(
                        m_currentEditingPaths[i]);
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
}

void AssetInspectorPanel::drawInspectorUI()
{
    PROFILE_FUNCTION();

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

    {
        PROFILE_SCOPE("AssetInspectorPanel::DrawHeader");
        if (m_currentEditingPaths.size() == 1)
        {
            ImGui::Text("资产: %s", m_currentEditingPaths[0].filename().string().c_str());
        }
        else
        {
            ImGui::Text("正在编辑 %zu 个资产", m_currentEditingPaths.size());
        }
        ImGui::Separator();
    }

    const auto* registration = AssetImporterRegistry::GetInstance().Get(m_editingAssetType);

    
    if (!registration || !m_isDeserialized || !registration->GetDataPointer)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "无法编辑的资产 (未注册或反序列化失败)");
        return;
    }

    
    void* dataPtr = nullptr;
    try
    {
        dataPtr = registration->GetDataPointer(m_deserializedSettings);
    }
    catch (const std::exception& e)
    {
        LogError("获取数据指针失败: {}", e.what());
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "获取数据指针失败 (std::any_cast 异常)");
        return;
    }

    if (!dataPtr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "内部错误: 数据指针为空。");
        return;
    }

    bool any_property_changed = false;

    {
        PROFILE_SCOPE("AssetInspectorPanel::DrawProperties");
        for (const auto& [name, prop] : registration->properties)
        {
            
            if (name == "rawData")
            {
                continue;
            }

            if (prop.isExposedInEditor)
            {
                PROFILE_SCOPE("AssetInspectorPanel::DrawSingleProperty");

                bool isMixed = m_mixedValueProperties.count(name);
                if (isMixed)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
                }

                bool changed = false;
                {
                    std::string scopeName = "AssetInspectorPanel::Property::" + name;
                    PROFILE_SCOPE(scopeName.c_str());

                    
                    
                    changed = prop.draw_ui(name, dataPtr);
                }

                if (changed)
                {
                    any_property_changed = true; 
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
    }

    
    if (m_editingAssetType == AssetType::Texture && m_currentEditingPaths.size() == 1)
    {
        PROFILE_SCOPE("AssetInspectorPanel::TextureSlicerButton");
        ImGui::Separator();
        if (ImGui::Button("打开切片编辑器", ImVec2(-1, 30)))
        {
            openTextureSlicer();
        }
    }

    {
        PROFILE_SCOPE("AssetInspectorPanel::DrawActionButtons");
        
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
}

void AssetInspectorPanel::applyChanges()
{
    PROFILE_FUNCTION();

    if (m_currentEditingPaths.empty() || m_dirtyProperties.empty()) return;

    
    const auto* registration = AssetImporterRegistry::GetInstance().Get(m_editingAssetType);
    if (!registration || !m_isDeserialized || !registration->Serialize)
    {
        LogError("应用更改失败：找不到注册或序列化函数。");
        return;
    }

    YAML::Node newSettingsBase;
    try
    {
        PROFILE_SCOPE("AssetInspectorPanel::SerializeSettings");
        
        
        newSettingsBase = registration->Serialize(m_deserializedSettings);
    }
    catch (const std::exception& e)
    {
        LogError("序列化资产设置失败: {}", e.what());
        return;
    }

    LogInfo("正在为 %zu 个资产应用 %zu 项设置更改...", m_currentEditingPaths.size(), m_dirtyProperties.size());

    for (const auto& assetPath : m_currentEditingPaths)
    {
        PROFILE_SCOPE("AssetInspectorPanel::ApplySingleAsset");

        const AssetMetadata* originalMetadata = AssetManager::GetInstance().GetMetadata(assetPath);
        if (!originalMetadata)
        {
            LogWarn("找不到资产 {} 的元数据，跳过保存。", assetPath.string());
            continue;
        }

        
        
        YAML::Node finalSettings = originalMetadata->importerSettings;
        for (const std::string& propName : m_dirtyProperties)
        {
            
            
            if (newSettingsBase[propName])
            {
                finalSettings[propName] = newSettingsBase[propName];
            }
        }

        saveMetadataToFile(*originalMetadata, finalSettings);
        EventBus::GetInstance().Publish(AssetUpdatedEvent{originalMetadata->type, originalMetadata->guid});
    }

    
    m_dirtyProperties.clear();
    
}

void AssetInspectorPanel::saveMetadataToFile(const AssetMetadata& originalMetadata, const YAML::Node& newSettings)
{
    PROFILE_FUNCTION();

    AssetMetadata updatedMeta = originalMetadata;
    updatedMeta.importerSettings = newSettings;
    
    AssetManager::GetInstance().ReImport(updatedMeta);
}

void AssetInspectorPanel::openTextureSlicer()
{
    PROFILE_FUNCTION();

    if (m_currentEditingPaths.empty())
        return;

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(m_currentEditingPaths[0]);
    if (!metadata || metadata->type != AssetType::Texture)
        return;

    
    if (!m_context->editor)
    {
        LogError("EditorContext 中的 editor 指针为空！");
        return;
    }

    
    auto* slicerPanel = dynamic_cast<TextureSlicerPanel*>(m_context->editor->GetPanelByName("纹理切片编辑器"));
    if (slicerPanel)
    {
        slicerPanel->OpenTexture(metadata->guid);
    }
    else
    {
        LogError("无法找到纹理切片编辑器面板");
    }
}

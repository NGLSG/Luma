#include "InspectorUI.h"

#include "PopupManager.h"
#include "BuiltinShaders.h"
#include "../Application/LayerManager.h"
#include "../Resources/RuntimeAsset/RuntimeGameObject.h"
#include "../Application/SceneManager.h"
#include "../Resources/AssetManager.h"


bool InspectorUI::DrawFloat(const std::string& label, float& value, float speed)
{
    return ImGui::DragFloat(label.c_str(), &value, speed);
}

bool InspectorUI::DrawBool(const std::string& label, bool& value)
{
    return ImGui::Checkbox(label.c_str(), &value);
}

bool InspectorUI::DrawString(const std::string& label, char* buffer, size_t bufferSize)
{
    float height = ImGui::GetTextLineHeight() * 4;
    const ImVec2 size(0, height);
    return ImGui::InputTextMultiline(label.c_str(), buffer, bufferSize, size);
}

bool InspectorUI::DrawInt(const std::string& label, int& value, float speed)
{
    return ImGui::DragInt(label.c_str(), &value, speed);
}

bool InspectorUI::DrawUInt32(const std::string& label, uint32_t& value, float speed)
{
    int intValue = static_cast<int>(value);
    if (ImGui::DragInt(label.c_str(), &intValue, speed, 0, INT_MAX))
    {
        value = static_cast<uint32_t>(std::max(0, intValue));
        return true;
    }
    return false;
}

bool InspectorUI::DrawLayerMask(const std::string& label, LayerMask& value)
{
    bool changed = false;
    
    // 显示当前状态的简短描述
    const char* statusText = (value.value == 0) ? "(Nothing)" : 
                             (value.value == 0xFFFFFFFF) ? "(Everything)" : "";
    
    char treeLabel[128];
    snprintf(treeLabel, sizeof(treeLabel), "%s %s", label.c_str(), statusText);
    
    if (ImGui::TreeNode(treeLabel))
    {
        // 提示信息
        if (value.value == 0)
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Not affected by any lights");
        }
        
        // 显示32个光照层的复选框
        for (int i = 0; i < LayerManager::MAX_LAYERS; ++i)
        {
            std::string layerName = LayerManager::GetDisplayName(i);
            
            char layerLabel[64];
            snprintf(layerLabel, sizeof(layerLabel), "%d: %s", i, layerName.c_str());
            
            bool layerEnabled = value.Contains(i);
            if (ImGui::Checkbox(layerLabel, &layerEnabled))
            {
                value.Set(i, layerEnabled);
                changed = true;
            }
            
            // 每行显示2个（因为名称可能较长）
            if ((i + 1) % 2 != 0 && i < LayerManager::MAX_LAYERS - 1)
                ImGui::SameLine(200.0f);
        }
        
        ImGui::Separator();
        
        // 快捷按钮
        if (ImGui::Button("Everything"))
        {
            value = LayerMask::All();
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Nothing"))
        {
            value = LayerMask::None();
            changed = true;
        }
        
        ImGui::TreePop();
    }
    
    return changed;
}

bool InspectorUI::DrawVector2f(const std::string& label, ECS::Vector2f& value, float speed)
{
    return ImGui::DragFloat2(label.c_str(), &value.x, speed);
}

bool InspectorUI::DrawVector2i(const std::string& label, ECS::Vector2i& value, float speed)
{
    return ImGui::DragInt2(label.c_str(), &value.x, speed);
}

bool InspectorUI::DrawColor(const std::string& label, ECS::Color& value)
{
    return ImGui::ColorEdit4(label.c_str(), &value.r);
}

bool InspectorUI::DrawRect(const std::string& label, ECS::RectF& value, float speed)
{
    return ImGui::DragFloat4(label.c_str(), &value.x, speed);
}


bool InspectorUI::DrawGuid(const std::string& label, Guid& value, const UIDrawData& callbacks)
{
    bool valueChanged = false;
    ImGui::PushID(label.c_str());

    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();

    std::string displayName = "[None]";
    if (value.Valid())
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            auto go = scene->FindGameObjectByGuid(value);
            if (go.IsValid()) displayName = go.GetName();
            else displayName = "[Missing GameObject]";
        }
    }

    if (ImGui::Button(displayName.c_str(), ImVec2(-1.0f, 0.0f)))
    {
        if (value.Valid() && callbacks.onFocusInHierarchy)
        {
            callbacks.onFocusInHierarchy.Invoke(value);
        }
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT_GUIDS"))
        {
            size_t guidCount = payload->DataSize / sizeof(Guid);
            const Guid* guidArray = static_cast<const Guid*>(payload->Data);
            value = guidArray[0];
            valueChanged = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopID();
    return valueChanged;
}

bool InspectorUI::DrawAssetHandle(const std::string& label, AssetHandle& handle, const UIDrawData& callbacks)
{
    static auto addCallback = [callbacks]()
    {
        static bool initialized = false;
        if (!initialized)
        {
            PopupManager::GetInstance().Register("SelectAssetPopup",
                                                 [&]
                                                 {
                                                     InspectorUI::DrawAssetPickerPopup(callbacks);
                                                 });
        }
    };
    addCallback();
    bool valueChanged = false;
    
    // 检查是否有来自popup的值变更
    if (s_assetPickerValueChanged && s_assetPickerTarget == &handle)
    {
        valueChanged = true;
        s_assetPickerValueChanged = false;
    }
    
    ImGui::PushID(label.c_str());

    
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();

    
    std::string displayName = "[None]";
    if (handle.Valid())
    {
        // 首先检查是否为内建shader
        std::string builtinName = BuiltinShaders::GetBuiltinShaderName(handle.assetGuid);
        if (!builtinName.empty())
        {
            displayName = builtinName;
        }
        else
        {
            displayName = AssetManager::GetInstance().GetAssetName(handle.assetGuid);
            if (displayName.empty()) displayName = "[Missing Asset]";
        }
    }

    
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 availableSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing());

    
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    
    ImVec2 rectMin = cursorPos;
    ImVec2 rectMax = ImVec2(cursorPos.x + availableSize.x, cursorPos.y + availableSize.y);

    
    bool isHovered = ImGui::IsMouseHoveringRect(rectMin, rectMax);

    
    ImU32 bgColor = isHovered
                        ? IM_COL32(76, 76, 89, 255)
                        : 
                        IM_COL32(51, 51, 64, 255); 

    
    drawList->AddRectFilled(rectMin, rectMax, bgColor, 3.0f); 

    
    drawList->AddRect(rectMin, rectMax, IM_COL32(80, 80, 90, 255), 3.0f, 0, 1.0f);

    
    ImGui::InvisibleButton("##AssetHandleButton", availableSize);

    
    ImVec2 textPos = ImVec2(cursorPos.x + 5.0f, cursorPos.y + (availableSize.y - ImGui::GetTextLineHeight()) * 0.5f);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), displayName.c_str());

    
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        s_assetPickerTarget = &handle;
        s_assetPickerFilterType = handle.assetType;
        memset(s_assetPickerSearchBuffer, 0, sizeof(s_assetPickerSearchBuffer));
        PopupManager::GetInstance().Open("SelectAssetPopup");
    }

    
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && handle.Valid() && callbacks.onFocusInAssetBrowser)
    {
        callbacks.onFocusInAssetBrowser.Invoke(handle.assetGuid);
    }

    
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            const AssetHandle* droppedHandle = static_cast<const AssetHandle*>(payload->Data);
            if (handle.assetType == AssetType::Unknown || handle.assetType == droppedHandle->assetType)
            {
                handle = *droppedHandle;
                valueChanged = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopID();
    return valueChanged;
}

void InspectorUI::DrawAssetPickerPopup(const UIDrawData& callbacks)
{
    ImGui::InputText("Search", s_assetPickerSearchBuffer, sizeof(s_assetPickerSearchBuffer));
    ImGui::Separator();

    if (ImGui::Selectable("[None]"))
    {
        *s_assetPickerTarget = AssetHandle();
        s_assetPickerValueChanged = true;
        if (callbacks.onValueChanged) callbacks.onValueChanged.Invoke();
        ImGui::CloseCurrentPopup();
    }

    std::string searchFilter = s_assetPickerSearchBuffer;
    std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

    // 如果是Shader类型，显示内建shader选项
    if (s_assetPickerFilterType == AssetType::Shader)
    {
        ImGui::Separator();
        ImGui::TextDisabled("-- 内建着色器 --");
        
        for (const auto& builtin : BuiltinShaders::GetAllBuiltinShaders())
        {
            std::string lowerName = builtin.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (searchFilter.empty() || lowerName.find(searchFilter) != std::string::npos)
            {
                if (ImGui::Selectable(builtin.name.c_str()))
                {
                    s_assetPickerTarget->assetGuid = Guid::FromString(builtin.guidStr);
                    s_assetPickerValueChanged = true;
                    if (callbacks.onValueChanged) callbacks.onValueChanged.Invoke();
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        ImGui::TextDisabled("-- 项目着色器 --");
    }

    {
        const auto& assetDb = AssetManager::GetInstance().GetAssetDatabase();
        for (const auto& [guidStr, meta] : assetDb)
        {
            if (s_assetPickerFilterType == AssetType::Unknown || meta.type == s_assetPickerFilterType)
            {
                std::string assetName = meta.assetPath.filename().string();
                std::string lowerAssetName = assetName;
                std::transform(lowerAssetName.begin(), lowerAssetName.end(), lowerAssetName.begin(), ::tolower);
                
                if (searchFilter.empty() || lowerAssetName.find(searchFilter) != std::string::npos)
                {
                    if (ImGui::Selectable(assetName.c_str()))
                    {
                        s_assetPickerTarget->assetGuid = meta.guid;
                        s_assetPickerValueChanged = true;
                        if (callbacks.onValueChanged) callbacks.onValueChanged.Invoke();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }
    }
}

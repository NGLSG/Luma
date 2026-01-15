#ifndef INSPECTORUI_H
#define INSPECTORUI_H

#include <string>
#include <vector>
#include <unordered_map>
#include <format>
#include <cstdint>
#include <imgui.h>

#include "../Components/Core.h"
#include "../Components/LightingTypes.h"
#include "../Utils/Guid.h"
#include "../Utils/LayerMask.h"
#include "../Components/AssetHandle.h"

/**
 * @brief 存储 Inspector UI 绘制相关的回调和选中 GUID 信息的数据结构。
 */
struct UIDrawData
{
    LumaEvent<const Guid&> onFocusInHierarchy; ///< 层级视图聚焦事件
    LumaEvent<const Guid&> onFocusInAssetBrowser; ///< 资源浏览器聚焦事件
    LumaEvent<> onValueChanged; ///< 值变更事件
    std::vector<Guid> SelectedGuids; ///< 当前选中的 GUID 列表
};

/**
 * @brief InspectorUI 类，提供各种类型属性的 ImGui 绘制接口。
 */
class InspectorUI
{
public:
    /**
     * @brief 绘制 float 类型属性的编辑控件。
     * @param label 标签
     * @param value float 值引用
     * @param speed 调整速度，默认 0.1f
     * @return 是否有值变更
     */
    static bool DrawFloat(const std::string& label, float& value, float speed = 0.1f);

    /**
     * @brief 绘制 bool 类型属性的编辑控件。
     * @param label 标签
     * @param value bool 值引用
     * @return 是否有值变更
     */
    static bool DrawBool(const std::string& label, bool& value);

    /**
     * @brief 绘制 int 类型属性的编辑控件。
     * @param label 标签
     * @param value int 值引用
     * @param speed 调整速度，默认 1.0f
     * @return 是否有值变更
     */
    static bool DrawInt(const std::string& label, int& value, float speed = 1.0f);

    /**
     * @brief 绘制 uint32_t 类型属性的编辑控件。
     * @param label 标签
     * @param value uint32_t 值引用
     * @param speed 调整速度，默认 1.0f
     * @return 是否有值变更
     */
    static bool DrawUInt32(const std::string& label, uint32_t& value, float speed = 1.0f);

    /**
     * @brief 绘制光照层掩码属性的编辑控件（Layer 0-31 复选框）。
     * @param label 标签
     * @param value LayerMask 值引用
     * @return 是否有值变更
     */
    static bool DrawLayerMask(const std::string& label, LayerMask& value);

    /**
     * @brief 绘制字符串类型属性的编辑控件。
     * @param label 标签
     * @param buffer 字符串缓冲区
     * @param bufferSize 缓冲区大小
     * @return 是否有值变更
     */
    static bool DrawString(const std::string& label, char* buffer, size_t bufferSize);

    /**
     * @brief 绘制 Vector2f 类型属性的编辑控件。
     * @param label 标签
     * @param value Vector2f 值引用
     * @param speed 调整速度，默认 0.1f
     * @return 是否有值变更
     */
    static bool DrawVector2f(const std::string& label, ECS::Vector2f& value, float speed = 0.1f);

    /**
     * @brief 绘制 Vector2i 类型属性的编辑控件。
     * @param label 标签
     * @param value Vector2i 值引用
     * @param speed 调整速度，默认 1.0f
     * @return 是否有值变更
     */
    static bool DrawVector2i(const std::string& label, ECS::Vector2i& value, float speed = 1.0f);

    /**
     * @brief 绘制颜色属性的编辑控件。
     * @param label 标签
     * @param value 颜色值引用
     * @return 是否有值变更
     */
    static bool DrawColor(const std::string& label, ECS::Color& value);

    /**
     * @brief 绘制矩形属性的编辑控件。
     * @param label 标签
     * @param value 矩形值引用
     * @param speed 调整速度，默认 0.1f
     * @return 是否有值变更
     */
    static bool DrawRect(const std::string& label, ECS::RectF& value, float speed = 0.1f);

    /**
     * @brief 绘制 Guid 类型属性的编辑控件。
     * @param label 标签
     * @param value Guid 值引用
     * @param callbacks 回调数据
     * @return 是否有值变更
     */
    static bool DrawGuid(const std::string& label, Guid& value, const UIDrawData& callbacks);

    /**
     * @brief 绘制 AssetHandle 类型属性的编辑控件。
     * @param label 标签
     * @param handle 资源句柄引用
     * @param callbacks 回调数据
     * @return 是否有值变更
     */
    static bool DrawAssetHandle(const std::string& label, AssetHandle& handle, const UIDrawData& callbacks);

    /**
     * @brief 绘制资源选择弹窗。
     * @param callbacks 回调数据
     */
    static void DrawAssetPickerPopup(const UIDrawData& callbacks);

private:
    inline static AssetHandle* s_assetPickerTarget; ///< 当前资源选择目标
    inline static AssetType s_assetPickerFilterType; ///< 资源选择过滤类型
    inline static char s_assetPickerSearchBuffer[256]; ///< 资源选择搜索缓冲区
    inline static bool s_assetPickerValueChanged = false; ///< 资源选择是否发生变更
};

/**
 * @brief CustomDrawing 命名空间，提供类型特化的属性绘制模板。
 */
namespace CustomDrawing
{
    /**
     * @brief WidgetDrawer 模板结构体，定义类型 T 的属性绘制接口。
     * @tparam T 属性类型
     */
    template <typename T>
    struct WidgetDrawer
    {
        /**
         * @brief 绘制类型 T 的属性控件，默认未实现。
         * @param label 标签
         * @param value 属性值引用
         * @param callbacks 回调数据
         * @return 是否有值变更
         */
        static bool Draw(const std::string& label, T& value, const UIDrawData& callbacks)
        {
            ImGui::Text("%s: [UI for type %s not implemented]", label.c_str(), typeid(T).name());
            return false;
        }
    };

    // float 类型特化
    template <>
    struct WidgetDrawer<float>
    {
        static bool Draw(const std::string& label, float& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawFloat(label, value);
        }
    };

    // bool 类型特化
    template <>
    struct WidgetDrawer<bool>
    {
        static bool Draw(const std::string& label, bool& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawBool(label, value);
        }
    };

    // int 类型特化
    template <>
    struct WidgetDrawer<int>
    {
        static bool Draw(const std::string& label, int& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawInt(label, value);
        }
    };

    // uint32_t 类型特化（常规整数编辑）
    template <>
    struct WidgetDrawer<uint32_t>
    {
        static bool Draw(const std::string& label, uint32_t& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawUInt32(label, value);
        }
    };

    // LayerMask 类型特化（层掩码编辑器）
    template <>
    struct WidgetDrawer<LayerMask>
    {
        static bool Draw(const std::string& label, LayerMask& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawLayerMask(label, value);
        }
    };

    // ECS::Vector2f 类型特化
    template <>
    struct WidgetDrawer<ECS::Vector2f>
    {
        static bool Draw(const std::string& label, ECS::Vector2f& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawVector2f(label, value);
        }
    };

    // ECS::Vector2i 类型特化
    template <>
    struct WidgetDrawer<ECS::Vector2i>
    {
        static bool Draw(const std::string& label, ECS::Vector2i& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawVector2i(label, value);
        }
    };

    // ECS::Color 类型特化
    template <>
    struct WidgetDrawer<ECS::Color>
    {
        static bool Draw(const std::string& label, ECS::Color& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawColor(label, value);
        }
    };

    // ECS::RectF 类型特化
    template <>
    struct WidgetDrawer<ECS::RectF>
    {
        static bool Draw(const std::string& label, ECS::RectF& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawRect(label, value);
        }
    };

    // SkPoint 类型特化
    template <>
    struct WidgetDrawer<SkPoint>
    {
        static bool Draw(const std::string& label, SkPoint& value, const UIDrawData& callbacks)
        {
            ECS::Vector2f vec2f(value.x(), value.y());
            if (InspectorUI::DrawVector2f(label, vec2f))
            {
                callbacks.onValueChanged();
                value.set(vec2f.x, vec2f.y);
                return true;
            }
            return false;
        }
    };

    // SkColor4f 类型特化
    template <>
    struct WidgetDrawer<SkColor4f>
    {
        static bool Draw(const std::string& label, SkColor4f& value, const UIDrawData& callbacks)
        {
            ECS::Color color(value.fR, value.fG, value.fB, value.fA);
            if (InspectorUI::DrawColor(label, color))
            {
                callbacks.onValueChanged();
                value = SkColor4f(color.r, color.g, color.b, color.a);
                return true;
            }
            return false;
        }
    };

    // Guid 类型特化
    template <>
    struct WidgetDrawer<Guid>
    {
        static bool Draw(const std::string& label, Guid& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawGuid(label, value, callbacks);
        }
    };

    // AssetHandle 类型特化
    template <>
    struct WidgetDrawer<AssetHandle>
    {
        static bool Draw(const std::string& label, AssetHandle& value, const UIDrawData& callbacks)
        {
            return InspectorUI::DrawAssetHandle(label, value, callbacks);
        }
    };

    // std::string 类型特化
    template <>
    struct WidgetDrawer<std::string>
    {
        static bool Draw(const std::string& label, std::string& value, const UIDrawData& callbacks)
        {
            char buffer[1024] = {0};
#ifdef _WIN32
            strncpy_s(buffer, sizeof(buffer), value.c_str(), sizeof(buffer) - 1);
#else
            strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
#endif
            if (InspectorUI::DrawString(label, buffer, sizeof(buffer)))
            {
                value = buffer;
                return true;
            }
            return false;
        }
    };

    /**
     * @brief std::vector<T> 类型特化，支持拖拽排序、添加、删除等操作。
     * @tparam T 元素类型
     */
    template <typename T>
    struct WidgetDrawer<std::vector<T>>
    {
        static bool Draw(const std::string& label, std::vector<T>& vec, const UIDrawData& callbacks)
        {
            bool changed = false;
            if (!ImGui::TreeNode(label.c_str()))
            {
                return false;
            }

            if (ImGui::Button("Add New"))
            {
                vec.emplace_back();
                changed = true;
            }

            int sourceIndex = -1;
            int targetIndex = -1;
            int removeIndex = -1;
            const char* payloadType = "VECTOR_ELEMENT_DND";

            for (int i = 0; i < vec.size(); ++i)
            {
                ImGui::PushID(i);

                ImGui::InvisibleButton("##drop_target", ImVec2(-1, 6.0f));
                const ImGuiPayload* dndPayload = ImGui::GetDragDropPayload();
                if (dndPayload != nullptr && dndPayload->IsDataType(payloadType))
                {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    {
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        ImVec2 pMin = ImGui::GetItemRectMin();
                        ImVec2 pMax = ImGui::GetItemRectMax();
                        drawList->AddLine(
                            ImVec2(pMin.x, pMin.y + 3.0f),
                            ImVec2(pMax.x, pMin.y + 3.0f),
                            ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f);
                    }
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* acceptedPayload = ImGui::AcceptDragDropPayload(payloadType))
                    {
                        sourceIndex = *(const int*)acceptedPayload->Data;
                        targetIndex = i;
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::Button(":::");
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload(payloadType, &i, sizeof(int));
                    ImGui::Text("移动 Element %d", i);
                    ImGui::EndDragDropSource();
                }

                ImGui::SameLine();

                std::string itemLabel = std::format("Element {}", i);
                if (WidgetDrawer<T>::Draw(itemLabel, vec[i], callbacks))
                {
                    changed = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    removeIndex = i;
                }

                ImGui::PopID();
            }

            ImGui::InvisibleButton("##drop_target_end", ImVec2(-1, 6.0f));
            if (const ImGuiPayload* dndPayload = ImGui::GetDragDropPayload())
            {
                if (dndPayload->IsDataType(payloadType) && ImGui::IsItemHovered(
                    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 pMin = ImGui::GetItemRectMin();
                    ImVec2 pMax = ImGui::GetItemRectMax();
                    drawList->AddLine(
                        ImVec2(pMin.x, pMin.y + 3.0f),
                        ImVec2(pMax.x, pMin.y + 3.0f),
                        ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f);
                }
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* acceptedPayload = ImGui::AcceptDragDropPayload(payloadType))
                {
                    sourceIndex = *(const int*)acceptedPayload->Data;
                    targetIndex = static_cast<int>(vec.size());
                }
                ImGui::EndDragDropTarget();
            }

            if (sourceIndex != -1 && targetIndex != -1 && sourceIndex != targetIndex)
            {
                if (targetIndex != sourceIndex + 1)
                {
                    T item = std::move(vec[sourceIndex]);
                    vec.erase(vec.begin() + sourceIndex);
                    if (targetIndex > sourceIndex)
                    {
                        targetIndex--;
                    }
                    vec.insert(vec.begin() + targetIndex, std::move(item));
                    changed = true;
                }
            }

            if (removeIndex != -1)
            {
                vec.erase(vec.begin() + removeIndex);
                changed = true;
            }

            ImGui::TreePop();
            return changed;
        }
    };

    /**
     * @brief std::map<std::string, T> 类型特化，支持递归绘制 map 内所有元素。
     * @tparam T 元素类型
     */
    template <typename T>
    struct WidgetDrawer<std::map<std::string, T>>
    {
        static bool Draw(const std::string& label, std::map<std::string, T>& map,
                         const UIDrawData& callbacks)
        {
            bool changed = false;
            if (ImGui::TreeNode(label.c_str()))
            {
                for (auto& [key, value] : map)
                {
                    ImGui::PushID(key.c_str());
                    if (WidgetDrawer<T>::Draw(key, value, callbacks)) { changed = true; }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            return changed;
        }
    };

    /**
     * @brief std::unordered_map<std::string, T> 类型特化，支持递归绘制 unordered_map 内所有元素。
     * @tparam T 元素类型
     */
    template <typename T>
    struct WidgetDrawer<std::unordered_map<std::string, T>>
    {
        static bool Draw(const std::string& label, std::unordered_map<std::string, T>& map,
                         const UIDrawData& callbacks)
        {
            bool changed = false;
            if (ImGui::TreeNode(label.c_str()))
            {
                for (auto& [key, value] : map)
                {
                    ImGui::PushID(key.c_str());
                    if (WidgetDrawer<T>::Draw(key, value, callbacks)) { changed = true; }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            return changed;
        }
    };

    // ============================================================================
    // 增强光照组件枚举类型特化 (Requirements: 13.6)
    // ============================================================================

    // AreaLightShape 类型特化
    template <>
    struct WidgetDrawer<ECS::AreaLightShape>
    {
        static bool Draw(const std::string& label, ECS::AreaLightShape& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "矩形", "圆形" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::AreaLightShape>(currentItem);
                return true;
            }
            return false;
        }
    };

    // AmbientZoneShape 类型特化
    template <>
    struct WidgetDrawer<ECS::AmbientZoneShape>
    {
        static bool Draw(const std::string& label, ECS::AmbientZoneShape& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "矩形", "圆形" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::AmbientZoneShape>(currentItem);
                return true;
            }
            return false;
        }
    };

    // AmbientGradientMode 类型特化
    template <>
    struct WidgetDrawer<ECS::AmbientGradientMode>
    {
        static bool Draw(const std::string& label, ECS::AmbientGradientMode& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "无渐变", "垂直渐变", "水平渐变" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::AmbientGradientMode>(currentItem);
                return true;
            }
            return false;
        }
    };

    // ToneMappingMode 类型特化
    template <>
    struct WidgetDrawer<ECS::ToneMappingMode>
    {
        static bool Draw(const std::string& label, ECS::ToneMappingMode& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "无", "Reinhard", "ACES", "Filmic" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::ToneMappingMode>(currentItem);
                return true;
            }
            return false;
        }
    };

    // FogMode 类型特化
    template <>
    struct WidgetDrawer<ECS::FogMode>
    {
        static bool Draw(const std::string& label, ECS::FogMode& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "线性", "指数", "指数平方" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::FogMode>(currentItem);
                return true;
            }
            return false;
        }
    };

    // QualityLevel 类型特化
    template <>
    struct WidgetDrawer<ECS::QualityLevel>
    {
        static bool Draw(const std::string& label, ECS::QualityLevel& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "低", "中", "高", "超高", "自定义" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::QualityLevel>(currentItem);
                return true;
            }
            return false;
        }
    };

    // ShadowMethod 类型特化
    template <>
    struct WidgetDrawer<ECS::ShadowMethod>
    {
        static bool Draw(const std::string& label, ECS::ShadowMethod& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "基础", "SDF", "屏幕空间" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::ShadowMethod>(currentItem);
                return true;
            }
            return false;
        }
    };

    // AttenuationType 类型特化
    template <>
    struct WidgetDrawer<ECS::AttenuationType>
    {
        static bool Draw(const std::string& label, ECS::AttenuationType& value, const UIDrawData& callbacks)
        {
            const char* items[] = { "线性", "二次", "平方反比" };
            int currentItem = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentItem, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::AttenuationType>(currentItem);
                return true;
            }
            return false;
        }
    };

     template <>
    struct WidgetDrawer<ECS::LightType>
    {
        static bool Draw(const std::string& label, ECS::LightType& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Point", "Spot", "Directional"};
            int currentIndex = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentIndex, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::LightType>(currentIndex);
                if (callbacks.onValueChanged) callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
}
#endif

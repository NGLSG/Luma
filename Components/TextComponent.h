#ifndef TEXT_H
#define TEXT_H

#include "Core.h"
#include "IComponent.h"
#include "AssetHandle.h"
#include <include/core/SkTypeface.h>
#include <include/core/SkRefCnt.h>
#include "ComponentRegistry.h"

/**
 * @brief 文本对齐方式枚举
 */
enum class TextAlignment
{
    TopLeft = 0, ///< 左上
    TopCenter,   ///< 上中
    TopRight,    ///< 右上
    MiddleLeft,  ///< 中左
    MiddleCenter,///< 中中
    MiddleRight, ///< 中右
    BottomLeft,  ///< 左下
    BottomCenter,///< 下中
    BottomRight   ///< 右下
};

namespace ECS
{
    /**
     * @brief 文本组件结构体
     *
     * 用于存储和管理实体（Entity）的文本相关属性。
     */
    struct TextComponent : public IComponent
    {
        /// 字体资源句柄
        AssetHandle fontHandle = AssetHandle(AssetType::Font);
        /// 文本内容
        std::string text = "Text";
        /// 字体大小
        float fontSize = 16.0f;
        /// 文本颜色
        Color color = Colors::White;
        /// 文本对齐方式
        TextAlignment alignment = TextAlignment::MiddleLeft;
        /// Z 轴索引，用于渲染排序
        int zIndex = 0;
        /// 组件名称
        std::string name;

        /**
         * @brief TextComponent 构造函数
         * @param initialFontHandle 初始字体资源句柄
         * @param initialText 初始文本内容
         */
        TextComponent(const AssetHandle& initialFontHandle, const std::string& initialText = "Text")
            : fontHandle(initialFontHandle), text(initialText)
        {
        }

        /**
         * @brief TextComponent 默认构造函数
         */
        TextComponent() = default;

        /**
         * @brief TextComponent 构造函数
         * @param initialText 初始文本内容
         * @param name 组件名称
         */
        TextComponent(const std::string& initialText, const std::string& name) : text(initialText), name(name)
        {
        }

        /// 字体类型对象，通常由 Skia 库管理
        sk_sp<SkTypeface> typeface = nullptr;
        /// 上一个字体资源句柄，用于检测字体是否发生变化
        AssetHandle lastFontHandle = AssetHandle(AssetType::Font);
    };
}

namespace YAML
{
    /**
     * @brief TextAlignment 类型转换特化模板
     *
     * 提供了 TextAlignment 枚举类型与 YAML 节点之间的转换方法。
     */
    template <>
    struct convert<TextAlignment>
    {
        /**
         * @brief 将 TextAlignment 编码为 YAML 节点
         * @param alignment 要编码的对齐方式
         * @return 包含 TextAlignment 值的 YAML 节点
         */
        static Node encode(const TextAlignment& alignment)
        {
            Node node;
            node = static_cast<int>(alignment);
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 TextAlignment
         * @param node 要解码的 YAML 节点
         * @param alignment 解码后的对齐方式将被存储在此引用中
         * @return 如果解码成功则返回 true，否则返回 false
         */
        static bool decode(const Node& node, TextAlignment& alignment)
        {
            if (!node.IsScalar()) return false;
            alignment = static_cast<TextAlignment>(node.as<int>());
            return true;
        }
    };

    /**
     * @brief ECS::TextComponent 类型转换特化模板
     *
     * 提供了 ECS::TextComponent 结构体与 YAML 节点之间的转换方法。
     */
    template <>
    struct convert<ECS::TextComponent>
    {
        /**
         * @brief 将 ECS::TextComponent 编码为 YAML 节点
         * @param textComp 要编码的文本组件
         * @return 包含文本组件所有属性的 YAML 节点
         */
        static Node encode(const ECS::TextComponent& textComp)
        {
            Node node;
            node["Enable"] = textComp.Enable;
            node["fontHandle"] = textComp.fontHandle;
            node["text"] = textComp.text;
            node["fontSize"] = textComp.fontSize;
            node["color"] = textComp.color;
            node["alignment"] = textComp.alignment;
            node["zIndex"] = textComp.zIndex;
            node["name"] = textComp.name;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 ECS::TextComponent
         * @param node 要解码的 YAML 节点
         * @param textComp 解码后的文本组件将被存储在此引用中
         * @return 如果解码成功则返回 true，否则返回 false
         */
        static bool decode(const Node& node, ECS::TextComponent& textComp)
        {
            if (!node.IsMap()) return false;

            if (!node["fontHandle"] || !node["text"] || !node["fontSize"] || !node["color"])
            {
                LogError("TextComponent: Missing required fields in YAML node.");
                return false;
            }
            textComp.Enable = node["Enable"].as<bool>(true);

            textComp.fontHandle = node["fontHandle"].as<AssetHandle>();
            textComp.text = node["text"].as<std::string>();
            textComp.fontSize = node["fontSize"].as<float>();
            textComp.color = node["color"].as<ECS::Color>();
            textComp.alignment = node["alignment"].as<TextAlignment>(TextAlignment::MiddleLeft);
            textComp.zIndex = node["zIndex"].as<int>(0);
            textComp.name = node["name"].as<std::string>();

            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::TextComponent>("TextComponent")
        .property("fontHandle", &ECS::TextComponent::fontHandle)
        .property("text", &ECS::TextComponent::text)
        .property("fontSize", &ECS::TextComponent::fontSize)
        .property("color", &ECS::TextComponent::color)
        .property("alignment", &ECS::TextComponent::alignment)
        .property("name", &ECS::TextComponent::name, false)
        .property("zIndex", &ECS::TextComponent::zIndex);
}

namespace CustomDrawing
{
    /**
     * @brief TextAlignment 的 WidgetDrawer 特化模板
     *
     * 提供了在 UI 中绘制 TextAlignment 枚举值的自定义方法。
     */
    template <>
    struct WidgetDrawer<TextAlignment>
    {
        /**
         * @brief 绘制 TextAlignment 的 UI 元素
         * @param label 标签文本，用于 UI 显示
         * @param value TextAlignment 值，可被用户修改
         * @param callbacks UI 绘制回调数据，包含 ImGui 上下文等
         * @return 如果 TextAlignment 值发生改变则返回 true，否则返回 false
         */
        static bool Draw(const std::string& label, TextAlignment& value, const UIDrawData& callbacks)
        {
            bool changed = false;
            const char* items[] = {
                "TopLeft", "TopCenter", "TopRight",
                "MiddleLeft", "MiddleCenter", "MiddleRight",
                "BottomLeft", "BottomCenter", "BottomRight"
            };
            int currentIndex = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &currentIndex, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<TextAlignment>(currentIndex);
                changed = true;
            }
            return changed;
        }
    };

    /**
     * @brief ECS::TextComponent 的 WidgetDrawer 特化模板
     *
     * 提供了在 UI 中绘制 ECS::TextComponent 所有属性的自定义方法。
     */
    template <>
    struct WidgetDrawer<ECS::TextComponent>
    {
        /**
         * @brief 绘制 ECS::TextComponent 的 UI 元素
         * @param label 标签文本，通常用于组件的整体标识
         * @param component ECS::TextComponent 组件，其属性可被用户修改
         * @param callbacks UI 绘制回调数据，包含 ImGui 上下文等
         * @return 如果组件的任何属性值发生改变则返回 true，否则返回 false
         */
        static bool Draw(const std::string& label, ECS::TextComponent& component, const UIDrawData& callbacks)
        {
            bool changed = false;
            ImGui::PushID(component.name.c_str());

            // If a label is provided (i.e., drawn as a property inside another component),
            // render as a TreeNode instead of a top-level CollapsingHeader to clarify hierarchy.
            bool open = true;
            bool useTree = !label.empty();
            if (useTree)
            {
                std::string nodeLabel = label;
                if (!component.name.empty()) nodeLabel += std::string(" ( ") + component.name + ")";
                open = ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            }
            else
            {
                std::string headerLabel = "TextComponent (" + component.name + ")";
                open = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            }

            if (open)
            {
                if (WidgetDrawer<std::string>::Draw("Text", component.text, callbacks))
                {
                    changed = true;
                }
                if (WidgetDrawer<AssetHandle>::Draw("Font", component.fontHandle, callbacks))
                {
                    changed = true;
                }
                if (WidgetDrawer<float>::Draw("Font Size", component.fontSize, callbacks))
                {
                    changed = true;
                }
                if (WidgetDrawer<ECS::Color>::Draw("Color", component.color, callbacks))
                {
                    changed = true;
                }
                if (WidgetDrawer<TextAlignment>::Draw("Alignment", component.alignment, callbacks))
                {
                    changed = true;
                }
                if (WidgetDrawer<int>::Draw("Z Index", component.zIndex, callbacks))
                {
                    changed = true;
                }
            }

            if (useTree && open)
            {
                ImGui::TreePop();
            }
            ImGui::PopID();

            return changed;
        }
    };
} // namespace CustomDrawing
#endif

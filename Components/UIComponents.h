#ifndef UICOMPONENTS_H
#define UICOMPONENTS_H

#include "Core.h"
#include "IComponent.h"
#include "AssetHandle.h"
#include "../Utils/Guid.h"
#include "ComponentRegistry.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "TextComponent.h"

namespace ECS
{
    /**
     * @brief 按钮组件，用于处理用户交互和视觉反馈。
     */
    struct ButtonComponent : public IComponent
    {
        bool isInteractable = true; ///< 按钮是否可交互。

        bool isPressed = false; ///< 按钮当前是否被按下。
        bool isHovered = false; ///< 按钮当前是否被鼠标悬停。
        ECS::Color NormalColor = ECS::Colors::White; ///< 按钮的正常颜色。
        ECS::Color HoverColor = ECS::Colors::LightGray; ///< 按钮被鼠标悬停时的颜色。
        ECS::Color PressedColor = ECS::Colors::Gray; ///< 按钮被按下时的颜色。
        ECS::Color DisabledColor = ECS::Colors::DarkGray; ///< 按钮禁用时的颜色。

        std::vector<SerializableEventTarget> onClickTargets; ///< 按钮点击时触发的事件目标列表。
    };

    /**
     * @brief 输入文本组件，用于处理用户文本输入。
     */
    struct InputTextComponent : public IComponent
    {
        TextComponent text = TextComponent("", "Text"); ///< 当前输入的文本内容。
        TextComponent placeholder = TextComponent("请输入文本...", "PlaceHolder"); ///< 占位符文本，在没有输入时显示。
        int maxLength = 256; ///< 文本输入的最大长度。
        bool isReadOnly = false; ///< 文本框是否只读。
        bool isPasswordField = false; ///< 文本框是否为密码字段（输入内容显示为星号）。

        // --- 状态字段 ---
        bool isFocused = false; ///< 文本框当前是否获得焦点。
        std::string lastText = ""; ///< 上一次的文本内容，用于检测变化。
        char inputBuffer[512] = {0}; ///< 用于存储输入文本的缓冲区。

        size_t cursorPosition = 0; // 光标在 inputBuffer 中的索引
        float cursorBlinkTimer = 0.0f; // 光标闪烁计时器
        bool isCursorVisible = false; // 光标当前是否可见
        std::vector<SerializableEventTarget> onTextChangedTargets; ///< 文本内容改变时触发的事件目标列表。
        std::vector<SerializableEventTarget> onSubmitTargets; ///< 文本提交（例如按回车）时触发的事件目标列表。
    };

    /**
     * @brief 滚动视图组件，用于显示可滚动的内容。
     */
    struct ScrollViewComponent : public IComponent
    {
        Vector2f scrollPosition = {0.0f, 0.0f}; ///< 当前滚动位置。
        Vector2f contentSize = {100.0f, 100.0f}; ///< 滚动内容的实际大小。
        Vector2f viewportSize = {100.0f, 100.0f}; ///< 可见视口的大小。
        bool enableHorizontalScroll = true; ///< 是否启用水平滚动。
        bool enableVerticalScroll = true; ///< 是否启用垂直滚动。
        float scrollSensitivity = 20.0f; ///< 滚动灵敏度。
        Vector2f lastScrollPosition = {0.0f, 0.0f}; ///< 上一次的滚动位置，用于检测变化。
        std::vector<SerializableEventTarget> onScrollChangedTargets; ///< 滚动位置改变时触发的事件目标列表。
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ButtonComponent。
     */
    template <>
    struct convert<ECS::ButtonComponent>
    {
        /**
         * @brief 将 ButtonComponent 对象编码为 YAML 节点。
         * @param rhs 要编码的 ButtonComponent 对象。
         * @return 表示 ButtonComponent 的 YAML 节点。
         */
        static Node encode(const ECS::ButtonComponent& rhs)
        {
            Node node;
            node["isEnabled"] = rhs.Enable;
            node["isInteractable"] = rhs.isInteractable;

            Node onClickNode;
            for (const auto& target : rhs.onClickTargets)
            {
                onClickNode.push_back(target);
            }
            node["onClickTargets"] = onClickNode;
            node["NormalColor"] = rhs.NormalColor;
            node["HoverColor"] = rhs.HoverColor;
            node["PressedColor"] = rhs.PressedColor;
            node["DisabledColor"] = rhs.DisabledColor;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ButtonComponent 对象。
         * @param node 包含 ButtonComponent 数据的 YAML 节点。
         * @param rhs 要填充的 ButtonComponent 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::ButtonComponent& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.Enable = node["isEnabled"].as<bool>(true);
            rhs.isInteractable = node["isInteractable"].as<bool>(true);
            if (node["NormalColor"])
                rhs.NormalColor = node["NormalColor"].as<ECS::Color>(ECS::Colors::White);
            if (node["HoverColor"])
                rhs.HoverColor = node["HoverColor"].as<ECS::Color>(ECS::Colors::LightGray);
            if (node["PressedColor"])
                rhs.PressedColor = node["PressedColor"].as<ECS::Color>(ECS::Colors::Gray);
            if (node["DisabledColor"])
                rhs.DisabledColor = node["DisabledColor"].as<ECS::Color>(ECS::Colors::DarkGray);

            if (node["onClickTargets"])
            {
                rhs.onClickTargets.clear();
                const Node& targetsNode = node["onClickTargets"];
                if (targetsNode.IsSequence())
                {
                    for (const auto& targetNode : targetsNode)
                    {
                        rhs.onClickTargets.push_back(targetNode.as<ECS::SerializableEventTarget>());
                    }
                }
            }

            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::InputTextComponent。
     */
    template <>
    struct convert<ECS::InputTextComponent>
    {
        /**
         * @brief 将 InputTextComponent 对象编码为 YAML 节点。
         * @param rhs 要编码的 InputTextComponent 对象。
         * @return 表示 InputTextComponent 的 YAML 节点。
         */
        static Node encode(const ECS::InputTextComponent& rhs)
        {
            Node node;
            node["text"] = rhs.text;
            node["placeholder"] = rhs.placeholder;
            node["maxLength"] = rhs.maxLength;
            node["isEnabled"] = rhs.Enable;
            node["isReadOnly"] = rhs.isReadOnly;
            node["isPasswordField"] = rhs.isPasswordField;

            Node onTextChangedNode;
            for (const auto& target : rhs.onTextChangedTargets)
            {
                onTextChangedNode.push_back(target);
            }
            node["onTextChangedTargets"] = onTextChangedNode;

            Node onSubmitNode;
            for (const auto& target : rhs.onSubmitTargets)
            {
                onSubmitNode.push_back(target);
            }
            node["onSubmitTargets"] = onSubmitNode;

            return node;
        }

        /**
         * @brief 从 YAML 节点解码 InputTextComponent 对象。
         * @param node 包含 InputTextComponent 数据的 YAML 节点。
         * @param rhs 要填充的 InputTextComponent 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::InputTextComponent& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.text = node["text"].as<ECS::TextComponent>();
            rhs.placeholder = node["placeholder"].as<ECS::TextComponent>();
            rhs.maxLength = node["maxLength"].as<int>(256);
            rhs.Enable = node["isEnabled"].as<bool>(true);
            rhs.isReadOnly = node["isReadOnly"].as<bool>(false);
            rhs.isPasswordField = node["isPasswordField"].as<bool>(false);

            if (node["onTextChangedTargets"])
            {
                rhs.onTextChangedTargets.clear();
                const Node& targetsNode = node["onTextChangedTargets"];
                if (targetsNode.IsSequence())
                {
                    for (const auto& targetNode : targetsNode)
                    {
                        rhs.onTextChangedTargets.push_back(targetNode.as<ECS::SerializableEventTarget>());
                    }
                }
            }

            if (node["onSubmitTargets"])
            {
                rhs.onSubmitTargets.clear();
                const Node& targetsNode = node["onSubmitTargets"];
                if (targetsNode.IsSequence())
                {
                    for (const auto& targetNode : targetsNode)
                    {
                        rhs.onSubmitTargets.push_back(targetNode.as<ECS::SerializableEventTarget>());
                    }
                }
            }

            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ScrollViewComponent。
     */
    template <>
    struct convert<ECS::ScrollViewComponent>
    {
        /**
         * @brief 将 ScrollViewComponent 对象编码为 YAML 节点。
         * @param rhs 要编码的 ScrollViewComponent 对象。
         * @return 表示 ScrollViewComponent 的 YAML 节点。
         */
        static Node encode(const ECS::ScrollViewComponent& rhs)
        {
            Node node;
            node["scrollPosition"] = rhs.scrollPosition;
            node["contentSize"] = rhs.contentSize;
            node["viewportSize"] = rhs.viewportSize;
            node["enableHorizontalScroll"] = rhs.enableHorizontalScroll;
            node["enableVerticalScroll"] = rhs.enableVerticalScroll;
            node["scrollSensitivity"] = rhs.scrollSensitivity;

            Node onScrollChangedNode;
            for (const auto& target : rhs.onScrollChangedTargets)
            {
                onScrollChangedNode.push_back(target);
            }
            node["onScrollChangedTargets"] = onScrollChangedNode;

            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ScrollViewComponent 对象。
         * @param node 包含 ScrollViewComponent 数据的 YAML 节点。
         * @param rhs 要填充的 ScrollViewComponent 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::ScrollViewComponent& rhs)
        {
            if (!node.IsMap()) return false;

            rhs.scrollPosition = node["scrollPosition"].as<ECS::Vector2f>(ECS::Vector2f{0.0f, 0.0f});
            rhs.contentSize = node["contentSize"].as<ECS::Vector2f>(ECS::Vector2f{100.0f, 100.0f});
            rhs.viewportSize = node["viewportSize"].as<ECS::Vector2f>(ECS::Vector2f{100.0f, 100.0f});
            rhs.enableHorizontalScroll = node["enableHorizontalScroll"].as<bool>(true);
            rhs.enableVerticalScroll = node["enableVerticalScroll"].as<bool>(true);
            rhs.scrollSensitivity = node["scrollSensitivity"].as<float>(20.0f);

            if (node["onScrollChangedTargets"])
            {
                rhs.onScrollChangedTargets.clear();
                const Node& targetsNode = node["onScrollChangedTargets"];
                if (targetsNode.IsSequence())
                {
                    for (const auto& targetNode : targetsNode)
                    {
                        rhs.onScrollChangedTargets.push_back(targetNode.as<ECS::SerializableEventTarget>());
                    }
                }
            }

            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::ButtonComponent>("ButtonComponent")
        .property("isInteractable", &ECS::ButtonComponent::isInteractable)
        .property("NormalColor", &ECS::ButtonComponent::NormalColor)
        .property("HoverColor", &ECS::ButtonComponent::HoverColor)
        .property("PressedColor", &ECS::ButtonComponent::PressedColor)
        .property("DisabledColor", &ECS::ButtonComponent::DisabledColor)
        .property("onClickTargets", &ECS::ButtonComponent::onClickTargets);

    Registry_<ECS::InputTextComponent>("InputTextComponent")
        .property("text", &ECS::InputTextComponent::text)
        .property("placeholder", &ECS::InputTextComponent::placeholder)
        .property("maxLength", &ECS::InputTextComponent::maxLength)
        .property("isReadOnly", &ECS::InputTextComponent::isReadOnly)
        .property("isPasswordField", &ECS::InputTextComponent::isPasswordField)
        .property("onTextChangedTargets", &ECS::InputTextComponent::onTextChangedTargets)
        .property("onSubmitTargets", &ECS::InputTextComponent::onSubmitTargets);

    Registry_<ECS::ScrollViewComponent>("ScrollViewComponent")
        .property("scrollPosition", &ECS::ScrollViewComponent::scrollPosition)
        .property("contentSize", &ECS::ScrollViewComponent::contentSize)
        .property("viewportSize", &ECS::ScrollViewComponent::viewportSize)
        .property("enableHorizontalScroll", &ECS::ScrollViewComponent::enableHorizontalScroll)
        .property("enableVerticalScroll", &ECS::ScrollViewComponent::enableVerticalScroll)
        .property("scrollSensitivity", &ECS::ScrollViewComponent::scrollSensitivity)
        .property("onScrollChangedTargets", &ECS::ScrollViewComponent::onScrollChangedTargets);
}

#endif

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
#include <memory> // For std::shared_ptr
#include "include/core/SkRRect.h"
#include "TextComponent.h"
#include "include/core/SkCanvas.h"
#include "RuntimeAsset/RuntimeTexture.h"

namespace ECS
{
    /**
     * @brief 所有UI组件的接口基类。
     * @details 提供所有UI元素共有的基础属性，如矩形变换、可见性和渲染层级。
     */
    struct IUIComponent : public IComponent
    {
        RectF rect = {0, 0, 100, 30}; ///< UI组件的边界矩形，定义相对于父级的位置和大小。
        bool isVisible = true; ///< UI组件是否可见。
        int zIndex = 0; ///< UI组件的渲染层级，较高的值会渲染在更上层。
    };

    /**
     * @brief 表示按钮的当前交互状态。
     */
    enum class ButtonState
    {
        Normal, ///< 正常状态
        Hovered, ///< 悬停状态
        Pressed, ///< 按下状态
        Disabled ///< 禁用状态
    };

    /**
     * @brief 按钮组件，处理用户交互、视觉状态反馈和事件触发。
     * @details 这是一个自绘组件，其外观由内部的drawFunction定义，并通过ButtonSystem更新其状态。
     */
    struct ButtonComponent : public IUIComponent
    {
        // --- 核心属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle(); ///< 按钮的背景图像资源句柄。
        float roundness = 4.0f; ///< 按钮的圆角半径。
        bool isInteractable = true; ///< 按钮是否可交互。

        // --- 状态颜色 ---
        Color normalColor = Colors::White; ///< 正常状态下的颜色叠加。
        Color hoverColor = {0.9f, 0.9f, 0.9f, 1.0f}; ///< 悬停状态下的颜色叠加。
        Color pressedColor = {0.7f, 0.7f, 0.7f, 1.0f}; ///< 按下状态下的颜色叠加。
        Color disabledColor = {0.5f, 0.5f, 0.5f, 0.5f}; ///< 禁用状态下的颜色叠加。

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onClickTargets; ///< 点击时触发的事件。
        std::vector<SerializableEventTarget> onHoverEnterTargets; ///< 鼠标悬停开始时触发的事件。
        std::vector<SerializableEventTarget> onHoverExitTargets; ///< 鼠标悬停结束时触发的事件。

        // --- 运行时数据 (不应被序列化) ---
        ButtonState currentState = ButtonState::Normal; ///< 按钮当前的交互状态。
        sk_sp<RuntimeTexture> backgroundImageTexture; ///< 已加载的背景图像运行时纹理。
    };

    /**
     * @brief 输入文本组件，用于处理用户文本输入。
     * @details 这是一个自绘组件，能够处理文本显示、占位符、光标和背景。其逻辑由InputTextSystem驱动。
     */
    struct InputTextComponent : public IUIComponent
    {
        // --- 内容属性 ---
        TextComponent text = TextComponent("", "Text"); ///< 当前输入的文本内容。
        TextComponent placeholder = TextComponent("请输入文本...", "PlaceHolder"); ///< 占位符文本。
        int maxLength = 256; ///< 文本输入的最大长度。
        bool isReadOnly = false; ///< 文本框是否只读。
        bool isPasswordField = false; ///< 是否为密码字段（输入内容显示为星号）。

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        float roundness = 2.0f;
        Color normalBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        Color focusedBackgroundColor = {0.15f, 0.15f, 0.15f, 1.0f};
        Color readOnlyBackgroundColor = {0.05f, 0.05f, 0.05f, 1.0f};
        Color cursorColor = Colors::White;

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onTextChangedTargets; ///< 文本内容改变时触发的事件。
        std::vector<SerializableEventTarget> onSubmitTargets; ///< 文本提交（例如按回车）时触发的事件。

        // --- 运行时状态字段 (不应被序列化) ---
        bool isFocused = false; ///< 文本框当前是否获得焦点。
        std::string inputBuffer = ""; ///< 用于存储输入文本的缓冲区。
        size_t cursorPosition = 0; ///< 光标在inputBuffer中的索引。
        float cursorBlinkTimer = 0.0f; ///< 光标闪烁计时器。
        bool isCursorVisible = false; ///< 光标当前是否可见。
        sk_sp<RuntimeTexture> backgroundImageTexture;
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
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["enable"] = rhs.Enable;

            node["backgroundImage"] = rhs.backgroundImage;
            node["roundness"] = rhs.roundness;
            node["isInteractable"] = rhs.isInteractable;
            node["normalColor"] = rhs.normalColor;
            node["hoverColor"] = rhs.hoverColor;
            node["pressedColor"] = rhs.pressedColor;
            node["disabledColor"] = rhs.disabledColor;

            node["onClickTargets"] = rhs.onClickTargets;
            node["onHoverEnterTargets"] = rhs.onHoverEnterTargets;
            node["onHoverExitTargets"] = rhs.onHoverExitTargets;
            node["zIndex"] = rhs.zIndex;

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

            rhs.rect = node["rect"].as<ECS::RectF>(ECS::RectF{0, 0, 100, 50});
            rhs.isVisible = node["isVisible"].as<bool>(true);
            if (node["enable"])
            {
                rhs.Enable = node["enable"].as<bool>(true);
            }
            else
            {
                rhs.Enable = rhs.isVisible;
            }
            rhs.zIndex = node["zIndex"].as<int>(0);

            rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(AssetHandle());
            rhs.roundness = node["roundness"].as<float>(4.0f);
            rhs.isInteractable = node["isInteractable"].as<bool>(true);
            rhs.normalColor = node["normalColor"].as<ECS::Color>(ECS::Colors::White);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(ECS::Color{0.9f, 0.9f, 0.9f, 1.0f});
            rhs.pressedColor = node["pressedColor"].as<ECS::Color>(ECS::Color{0.7f, 0.7f, 0.7f, 1.0f});
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(ECS::Color{0.5f, 0.5f, 0.5f, 0.5f});

            rhs.onClickTargets = node["onClickTargets"].as<std::vector<ECS::SerializableEventTarget>>(
                std::vector<ECS::SerializableEventTarget>());
            rhs.onHoverEnterTargets = node["onHoverEnterTargets"].as<std::vector<ECS::SerializableEventTarget>>(
                std::vector<ECS::SerializableEventTarget>());
            rhs.onHoverExitTargets = node["onHoverExitTargets"].as<std::vector<ECS::SerializableEventTarget>>(
                std::vector<ECS::SerializableEventTarget>());

            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::InputTextComponent。
     */
    template <>
    struct convert<ECS::InputTextComponent>
    {
        static Node encode(const ECS::InputTextComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["text"] = rhs.text;
            node["placeholder"] = rhs.placeholder;
            node["maxLength"] = rhs.maxLength;
            node["isReadOnly"] = rhs.isReadOnly;
            node["isPasswordField"] = rhs.isPasswordField;
            node["backgroundImage"] = rhs.backgroundImage;
            node["roundness"] = rhs.roundness;
            node["normalBackgroundColor"] = rhs.normalBackgroundColor;
            node["focusedBackgroundColor"] = rhs.focusedBackgroundColor;
            node["readOnlyBackgroundColor"] = rhs.readOnlyBackgroundColor;
            node["cursorColor"] = rhs.cursorColor;
            node["onTextChangedTargets"] = rhs.onTextChangedTargets;
            node["onSubmitTargets"] = rhs.onSubmitTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::InputTextComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(ECS::RectF{0, 0, 200, 30});
            rhs.isVisible = node["isVisible"].as<bool>(true);
            rhs.zIndex = node["zIndex"].as<int>(0);
            rhs.text = node["text"].as<ECS::TextComponent>();
            rhs.placeholder = node["placeholder"].as<ECS::TextComponent>();
            rhs.maxLength = node["maxLength"].as<int>(256);
            rhs.isReadOnly = node["isReadOnly"].as<bool>(false);
            rhs.isPasswordField = node["isPasswordField"].as<bool>(false);
            rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>();
            rhs.roundness = node["roundness"].as<float>(2.0f);
            rhs.normalBackgroundColor = node["normalBackgroundColor"].as<
                ECS::Color>(ECS::Color{0.1f, 0.1f, 0.1f, 1.0f});
            rhs.focusedBackgroundColor = node["focusedBackgroundColor"].as<ECS::Color>(
                ECS::Color{0.15f, 0.15f, 0.15f, 1.0f});
            rhs.readOnlyBackgroundColor = node["readOnlyBackgroundColor"].as<ECS::Color>(
                ECS::Color{0.05f, 0.05f, 0.05f, 1.0f});
            rhs.cursorColor = node["cursorColor"].as<ECS::Color>(ECS::Colors::White);
            rhs.onTextChangedTargets = node["onTextChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>();
            rhs.onSubmitTargets = node["onSubmitTargets"].as<std::vector<ECS::SerializableEventTarget>>();
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::ButtonComponent>("ButtonComponent")
        .property("rect", &ECS::ButtonComponent::rect)
        .property("isVisible", &ECS::ButtonComponent::isVisible)
        .property("backgroundImage", &ECS::ButtonComponent::backgroundImage)
        .property("roundness", &ECS::ButtonComponent::roundness)
        .property("isInteractable", &ECS::ButtonComponent::isInteractable)
        .property("normalColor", &ECS::ButtonComponent::normalColor)
        .property("hoverColor", &ECS::ButtonComponent::hoverColor)
        .property("pressedColor", &ECS::ButtonComponent::pressedColor)
        .property("disabledColor", &ECS::ButtonComponent::disabledColor)
        .property("onClickTargets", &ECS::ButtonComponent::onClickTargets)
        .property("onHoverEnterTargets", &ECS::ButtonComponent::onHoverEnterTargets)
        .property("onHoverExitTargets", &ECS::ButtonComponent::onHoverExitTargets);

    Registry_<ECS::InputTextComponent>("InputTextComponent")
        .property("rect", &ECS::InputTextComponent::rect)
        .property("isVisible", &ECS::InputTextComponent::isVisible)
        .property("text", &ECS::InputTextComponent::text)
        .property("placeholder", &ECS::InputTextComponent::placeholder)
        .property("maxLength", &ECS::InputTextComponent::maxLength)
        .property("isReadOnly", &ECS::InputTextComponent::isReadOnly)
        .property("isPasswordField", &ECS::InputTextComponent::isPasswordField)
        .property("backgroundImage", &ECS::InputTextComponent::backgroundImage)
        .property("roundness", &ECS::InputTextComponent::roundness)
        .property("normalBackgroundColor", &ECS::InputTextComponent::normalBackgroundColor)
        .property("focusedBackgroundColor", &ECS::InputTextComponent::focusedBackgroundColor)
        .property("readOnlyBackgroundColor", &ECS::InputTextComponent::readOnlyBackgroundColor)
        .property("cursorColor", &ECS::InputTextComponent::cursorColor)
        .property("onTextChangedTargets", &ECS::InputTextComponent::onTextChangedTargets)
        .property("onSubmitTargets", &ECS::InputTextComponent::onSubmitTargets);
}

#endif

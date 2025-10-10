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
     * @brief 列表布局模式。
     */
    enum class ListBoxLayout
    {
        Vertical, ///< 垂直排列（默认）
        Horizontal, ///< 水平排列
        Grid ///< 网格排列
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
    /**
     * @brief 切换按钮组件，支持双态交互并提供可定制的视觉反馈。
     */
    struct ToggleButtonComponent : public IUIComponent
    {
        // --- 核心属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        float roundness = 4.0f;
        bool isInteractable = true;
        bool isToggled = false;
        bool allowToggleOff = true;

        // --- 状态颜色 ---
        Color normalColor = Colors::White;
        Color hoverColor = {0.92f, 0.92f, 0.92f, 1.0f};
        Color pressedColor = {0.8f, 0.8f, 0.8f, 1.0f};
        Color toggledColor = {0.25f, 0.55f, 0.95f, 1.0f};
        Color toggledHoverColor = {0.30f, 0.60f, 1.0f, 1.0f};
        Color toggledPressedColor = {0.20f, 0.50f, 0.90f, 1.0f};
        Color disabledColor = {0.5f, 0.5f, 0.5f, 0.5f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onToggleOnTargets;
        std::vector<SerializableEventTarget> onToggleOffTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        ButtonState currentState = ButtonState::Normal;
        sk_sp<RuntimeTexture> backgroundImageTexture;
    };

    /**
     * @brief 单选按钮组件，按组互斥选择，可自定义标签与外观。
     */
    struct RadioButtonComponent : public IUIComponent
    {
        // --- 内容属性 ---
        TextComponent label = TextComponent("选项", "RadioLabel");
        std::string groupId = "DefaultRadioGroup";
        bool isSelected = false;
        bool isInteractable = true;

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        AssetHandle selectionImage = AssetHandle::TextureHandle();
        float roundness = 4.0f;
        Color normalColor = Colors::White;
        Color hoverColor = {0.92f, 0.92f, 0.92f, 1.0f};
        Color selectedColor = {0.25f, 0.55f, 0.95f, 1.0f};
        Color disabledColor = {0.5f, 0.5f, 0.5f, 0.5f};
        Color indicatorColor = Colors::White;

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onSelectedTargets;
        std::vector<SerializableEventTarget> onDeselectedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        ButtonState currentState = ButtonState::Normal;
        sk_sp<RuntimeTexture> backgroundImageTexture;
        sk_sp<RuntimeTexture> selectionImageTexture;
    };

    /**
     * @brief 复选框组件，支持双态或三态选择，并提供文本标签。
     */
    struct CheckBoxComponent : public IUIComponent
    {
        // --- 内容属性 ---
        TextComponent label = TextComponent("复选框", "CheckBoxLabel");
        bool isChecked = false;
        bool allowIndeterminate = false;
        bool isIndeterminate = false;
        bool isInteractable = true;

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        AssetHandle checkmarkImage = AssetHandle::TextureHandle();
        float roundness = 4.0f;
        Color normalColor = Colors::White;
        Color hoverColor = {0.92f, 0.92f, 0.92f, 1.0f};
        Color checkedColor = {0.25f, 0.55f, 0.95f, 1.0f};
        Color indeterminateColor = {0.35f, 0.35f, 0.35f, 1.0f};
        Color disabledColor = {0.5f, 0.5f, 0.5f, 0.5f};
        Color checkmarkColor = Colors::White;

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onValueChangedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        ButtonState currentState = ButtonState::Normal;
        sk_sp<RuntimeTexture> backgroundImageTexture;
        sk_sp<RuntimeTexture> checkmarkImageTexture;
    };

    /**
     * @brief 滑块组件，支持连续或步进取值，可垂直或水平显示。
     */
    struct SliderComponent : public IUIComponent
    {
        // --- 数据属性 ---
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float value = 0.0f;
        float step = 0.0f;
        bool isInteractable = true;
        bool isVertical = false;

        // --- 外观属性 ---
        AssetHandle trackImage = AssetHandle::TextureHandle();
        AssetHandle fillImage = AssetHandle::TextureHandle();
        AssetHandle thumbImage = AssetHandle::TextureHandle();
        Color trackColor = {0.18f, 0.18f, 0.18f, 1.0f};
        Color fillColor = {0.30f, 0.60f, 0.95f, 1.0f};
        Color thumbColor = Colors::White;
        Color disabledColor = {0.45f, 0.45f, 0.45f, 0.4f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onValueChangedTargets;
        std::vector<SerializableEventTarget> onDragStartedTargets;
        std::vector<SerializableEventTarget> onDragEndedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        float normalizedValue = 0.0f;
        bool isDragging = false;
        sk_sp<RuntimeTexture> trackImageTexture;
        sk_sp<RuntimeTexture> fillImageTexture;
        sk_sp<RuntimeTexture> thumbImageTexture;
    };

    /**
     * @brief 下拉选择组件，支持静态选项或自定义输入。
     */
    struct ComboBoxComponent : public IUIComponent
    {
        // --- 数据属性 ---
        std::vector<std::string> items;
        int selectedIndex = -1;
        bool isInteractable = true;
        bool allowCustomInput = false;

        // --- 外观属性 ---
        TextComponent displayText = TextComponent("", "ComboDisplay");
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        AssetHandle dropdownIcon = AssetHandle::TextureHandle();
        float roundness = 4.0f;
        Color normalColor = Colors::White;
        Color hoverColor = {0.92f, 0.92f, 0.92f, 1.0f};
        Color pressedColor = {0.80f, 0.80f, 0.80f, 1.0f};
        Color disabledColor = {0.5f, 0.5f, 0.5f, 0.5f};
        Color dropdownBackgroundColor = {0.15f, 0.15f, 0.15f, 1.0f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onSelectionChangedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        bool isDropdownOpen = false;
        ButtonState currentState = ButtonState::Normal;
        int hoveredIndex = -1;
        sk_sp<RuntimeTexture> backgroundImageTexture;
        sk_sp<RuntimeTexture> dropdownIconTexture;
    };

    /**
     * @brief 折叠面板组件，提供可扩展/折叠的内容区域。
     */
    struct ExpanderComponent : public IUIComponent
    {
        // --- 内容属性 ---
        TextComponent header = TextComponent("折叠面板", "ExpanderHeader");
        bool isExpanded = true;
        bool isInteractable = true;
        float roundness = 4.0f;

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        Color headerColor = {0.18f, 0.18f, 0.18f, 1.0f};
        Color expandedColor = {0.22f, 0.22f, 0.22f, 1.0f};
        Color collapsedColor = {0.15f, 0.15f, 0.15f, 1.0f};
        Color disabledColor = {0.45f, 0.45f, 0.45f, 0.4f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onExpandedTargets;
        std::vector<SerializableEventTarget> onCollapsedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        sk_sp<RuntimeTexture> backgroundImageTexture;
    };

    /**
     * @brief 进度条组件，可显示确定或不确定的进度状态。
     */
    struct ProgressBarComponent : public IUIComponent
    {
        // --- 数据属性 ---
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float value = 0.0f;
        bool showPercentage = false;
        bool isIndeterminate = false;
        float indeterminateSpeed = 1.0f;

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        AssetHandle fillImage = AssetHandle::TextureHandle();
        Color backgroundColor = {0.10f, 0.10f, 0.10f, 1.0f};
        Color fillColor = {0.30f, 0.60f, 0.95f, 1.0f};
        Color borderColor = {0.05f, 0.05f, 0.05f, 1.0f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onValueChangedTargets;
        std::vector<SerializableEventTarget> onCompletedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        float indeterminatePhase = 0.0f;
        bool hasCompletedEventFired = false;
        sk_sp<RuntimeTexture> backgroundImageTexture;
        sk_sp<RuntimeTexture> fillImageTexture;
    };

    /**
     * @brief Tab 项描述结构，保存单个页签的基本信息。
     */
    struct TabItem
    {
        std::string title = "新选项卡";
        Guid contentGuid = Guid::Invalid();
        bool isVisible = true;
        bool isEnabled = true;
    };

    /**
     * @brief 选项卡容器组件，管理多个标签页的展示与切换。
     */
    struct TabControlComponent : public IUIComponent
    {
        // --- 数据属性 ---
        std::vector<TabItem> tabs = {TabItem{}};
        int activeTabIndex = 0;
        bool isInteractable = true;
        bool allowReorder = false;
        bool allowCloseTabs = false;

        // --- 外观属性 ---
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        AssetHandle tabBackgroundImage = AssetHandle::TextureHandle();
        float tabHeight = 28.0f;
        float tabSpacing = 4.0f;
        Color backgroundColor = {0.10f, 0.10f, 0.10f, 1.0f};
        Color tabColor = {0.18f, 0.18f, 0.18f, 1.0f};
        Color activeTabColor = {0.30f, 0.60f, 0.95f, 1.0f};
        Color hoverTabColor = {0.22f, 0.22f, 0.22f, 1.0f};
        Color disabledTabColor = {0.45f, 0.45f, 0.45f, 0.4f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onTabChangedTargets;
        std::vector<SerializableEventTarget> onTabClosedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        int hoveredTabIndex = -1;
        float tabScrollOffset = 0.0f;
        sk_sp<RuntimeTexture> backgroundImageTexture;
        sk_sp<RuntimeTexture> tabBackgroundImageTexture;
    };

    /**
     * @brief 列表框组件，支持单选或多选并可自定义项外观。
     */
    struct ListBoxComponent : public IUIComponent
    {
        // --- 数据属性 ---
        std::vector<std::string> items;
        Guid itemsContainerGuid = Guid::Invalid();
        std::vector<int> selectedIndices;
        bool allowMultiSelect = false;
        bool isInteractable = true;
        int visibleItemCount = 6;
        float roundness = 4.0f;
        ListBoxLayout layout = ListBoxLayout::Vertical;
        Vector2f itemSpacing = {4.0f, 4.0f};
        int maxItemsPerRow = 1;
        int maxItemsPerColumn = 1;

        // --- 外观属性 ---
        TextComponent itemTemplate = TextComponent("列表项", "ListItemTemplate");
        AssetHandle backgroundImage = AssetHandle::TextureHandle();
        Color backgroundColor = {0.12f, 0.12f, 0.12f, 1.0f};
        Color itemColor = Colors::White;
        Color hoverColor = {0.20f, 0.20f, 0.20f, 1.0f};
        Color selectedColor = {0.30f, 0.60f, 0.95f, 1.0f};
        Color disabledColor = {0.45f, 0.45f, 0.45f, 0.4f};
        bool enableVerticalScrollbar = true;
        bool verticalScrollbarAutoHide = true;
        bool enableHorizontalScrollbar = false;
        bool horizontalScrollbarAutoHide = true;
        float scrollbarThickness = 6.0f;
        Color scrollbarTrackColor = {0.18f, 0.18f, 0.18f, 1.0f};
        Color scrollbarThumbColor = {0.45f, 0.45f, 0.45f, 0.8f};

        // --- 事件目标 ---
        std::vector<SerializableEventTarget> onSelectionChangedTargets;
        std::vector<SerializableEventTarget> onItemActivatedTargets;

        // --- 运行时状态字段 (不应被序列化) ---
        int hoveredIndex = -1;
        int scrollOffset = 0;
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
    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ToggleButtonComponent。
     */
    template <>
    struct convert<ECS::ToggleButtonComponent>
    {
        static Node encode(const ECS::ToggleButtonComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["isInteractable"] = rhs.isInteractable;
            node["isToggled"] = rhs.isToggled;
            node["allowToggleOff"] = rhs.allowToggleOff;
            node["backgroundImage"] = rhs.backgroundImage;
            node["roundness"] = rhs.roundness;
            node["normalColor"] = rhs.normalColor;
            node["hoverColor"] = rhs.hoverColor;
            node["pressedColor"] = rhs.pressedColor;
            node["toggledColor"] = rhs.toggledColor;
            node["toggledHoverColor"] = rhs.toggledHoverColor;
            node["toggledPressedColor"] = rhs.toggledPressedColor;
            node["disabledColor"] = rhs.disabledColor;
            node["onToggleOnTargets"] = rhs.onToggleOnTargets;
            node["onToggleOffTargets"] = rhs.onToggleOffTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::ToggleButtonComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.isToggled = node["isToggled"].as<bool>(rhs.isToggled);
            rhs.allowToggleOff = node["allowToggleOff"].as<bool>(rhs.allowToggleOff);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            rhs.normalColor = node["normalColor"].as<ECS::Color>(rhs.normalColor);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(rhs.hoverColor);
            rhs.pressedColor = node["pressedColor"].as<ECS::Color>(rhs.pressedColor);
            rhs.toggledColor = node["toggledColor"].as<ECS::Color>(rhs.toggledColor);
            rhs.toggledHoverColor = node["toggledHoverColor"].as<ECS::Color>(rhs.toggledHoverColor);
            rhs.toggledPressedColor = node["toggledPressedColor"].as<ECS::Color>(rhs.toggledPressedColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.onToggleOnTargets = node["onToggleOnTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onToggleOnTargets);
            rhs.onToggleOffTargets = node["onToggleOffTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onToggleOffTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::RadioButtonComponent。
     */
    template <>
    struct convert<ECS::RadioButtonComponent>
    {
        static Node encode(const ECS::RadioButtonComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["label"] = rhs.label;
            node["groupId"] = rhs.groupId;
            node["isSelected"] = rhs.isSelected;
            node["isInteractable"] = rhs.isInteractable;
            node["backgroundImage"] = rhs.backgroundImage;
            node["selectionImage"] = rhs.selectionImage;
            node["roundness"] = rhs.roundness;
            node["normalColor"] = rhs.normalColor;
            node["hoverColor"] = rhs.hoverColor;
            node["selectedColor"] = rhs.selectedColor;
            node["disabledColor"] = rhs.disabledColor;
            node["indicatorColor"] = rhs.indicatorColor;
            node["onSelectedTargets"] = rhs.onSelectedTargets;
            node["onDeselectedTargets"] = rhs.onDeselectedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::RadioButtonComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.label = node["label"].as<ECS::TextComponent>(rhs.label);
            rhs.groupId = node["groupId"].as<std::string>(rhs.groupId);
            rhs.isSelected = node["isSelected"].as<bool>(rhs.isSelected);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            if (node["selectionImage"]) rhs.selectionImage = node["selectionImage"].as<AssetHandle>(rhs.selectionImage);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            rhs.normalColor = node["normalColor"].as<ECS::Color>(rhs.normalColor);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(rhs.hoverColor);
            rhs.selectedColor = node["selectedColor"].as<ECS::Color>(rhs.selectedColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.indicatorColor = node["indicatorColor"].as<ECS::Color>(rhs.indicatorColor);
            rhs.onSelectedTargets = node["onSelectedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onSelectedTargets);
            rhs.onDeselectedTargets = node["onDeselectedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onDeselectedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::CheckBoxComponent。
     */
    template <>
    struct convert<ECS::CheckBoxComponent>
    {
        static Node encode(const ECS::CheckBoxComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["label"] = rhs.label;
            node["isChecked"] = rhs.isChecked;
            node["allowIndeterminate"] = rhs.allowIndeterminate;
            node["isIndeterminate"] = rhs.isIndeterminate;
            node["isInteractable"] = rhs.isInteractable;
            node["backgroundImage"] = rhs.backgroundImage;
            node["checkmarkImage"] = rhs.checkmarkImage;
            node["roundness"] = rhs.roundness;
            node["normalColor"] = rhs.normalColor;
            node["hoverColor"] = rhs.hoverColor;
            node["checkedColor"] = rhs.checkedColor;
            node["indeterminateColor"] = rhs.indeterminateColor;
            node["disabledColor"] = rhs.disabledColor;
            node["checkmarkColor"] = rhs.checkmarkColor;
            node["onValueChangedTargets"] = rhs.onValueChangedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::CheckBoxComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.label = node["label"].as<ECS::TextComponent>(rhs.label);
            rhs.isChecked = node["isChecked"].as<bool>(rhs.isChecked);
            rhs.allowIndeterminate = node["allowIndeterminate"].as<bool>(rhs.allowIndeterminate);
            rhs.isIndeterminate = node["isIndeterminate"].as<bool>(rhs.isIndeterminate);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            if (node["checkmarkImage"]) rhs.checkmarkImage = node["checkmarkImage"].as<AssetHandle>(rhs.checkmarkImage);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            rhs.normalColor = node["normalColor"].as<ECS::Color>(rhs.normalColor);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(rhs.hoverColor);
            rhs.checkedColor = node["checkedColor"].as<ECS::Color>(rhs.checkedColor);
            rhs.indeterminateColor = node["indeterminateColor"].as<ECS::Color>(rhs.indeterminateColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.checkmarkColor = node["checkmarkColor"].as<ECS::Color>(rhs.checkmarkColor);
            rhs.onValueChangedTargets = node["onValueChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onValueChangedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::SliderComponent。
     */
    template <>
    struct convert<ECS::SliderComponent>
    {
        static Node encode(const ECS::SliderComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["minValue"] = rhs.minValue;
            node["maxValue"] = rhs.maxValue;
            node["value"] = rhs.value;
            node["step"] = rhs.step;
            node["isInteractable"] = rhs.isInteractable;
            node["isVertical"] = rhs.isVertical;
            node["trackImage"] = rhs.trackImage;
            node["fillImage"] = rhs.fillImage;
            node["thumbImage"] = rhs.thumbImage;
            node["trackColor"] = rhs.trackColor;
            node["fillColor"] = rhs.fillColor;
            node["thumbColor"] = rhs.thumbColor;
            node["disabledColor"] = rhs.disabledColor;
            node["onValueChangedTargets"] = rhs.onValueChangedTargets;
            node["onDragStartedTargets"] = rhs.onDragStartedTargets;
            node["onDragEndedTargets"] = rhs.onDragEndedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::SliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.minValue = node["minValue"].as<float>(rhs.minValue);
            rhs.maxValue = node["maxValue"].as<float>(rhs.maxValue);
            rhs.value = node["value"].as<float>(rhs.value);
            rhs.step = node["step"].as<float>(rhs.step);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.isVertical = node["isVertical"].as<bool>(rhs.isVertical);
            if (node["trackImage"]) rhs.trackImage = node["trackImage"].as<AssetHandle>(rhs.trackImage);
            if (node["fillImage"]) rhs.fillImage = node["fillImage"].as<AssetHandle>(rhs.fillImage);
            if (node["thumbImage"]) rhs.thumbImage = node["thumbImage"].as<AssetHandle>(rhs.thumbImage);
            rhs.trackColor = node["trackColor"].as<ECS::Color>(rhs.trackColor);
            rhs.fillColor = node["fillColor"].as<ECS::Color>(rhs.fillColor);
            rhs.thumbColor = node["thumbColor"].as<ECS::Color>(rhs.thumbColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.onValueChangedTargets = node["onValueChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onValueChangedTargets);
            rhs.onDragStartedTargets = node["onDragStartedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onDragStartedTargets);
            rhs.onDragEndedTargets = node["onDragEndedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onDragEndedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ComboBoxComponent。
     */
    template <>
    struct convert<ECS::ComboBoxComponent>
    {
        static Node encode(const ECS::ComboBoxComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["items"] = rhs.items;
            node["selectedIndex"] = rhs.selectedIndex;
            node["isInteractable"] = rhs.isInteractable;
            node["allowCustomInput"] = rhs.allowCustomInput;
            node["displayText"] = rhs.displayText;
            node["backgroundImage"] = rhs.backgroundImage;
            node["dropdownIcon"] = rhs.dropdownIcon;
            node["roundness"] = rhs.roundness;
            node["normalColor"] = rhs.normalColor;
            node["hoverColor"] = rhs.hoverColor;
            node["pressedColor"] = rhs.pressedColor;
            node["disabledColor"] = rhs.disabledColor;
            node["dropdownBackgroundColor"] = rhs.dropdownBackgroundColor;
            node["onSelectionChangedTargets"] = rhs.onSelectionChangedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::ComboBoxComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.items = node["items"].as<std::vector<std::string>>(rhs.items);
            rhs.selectedIndex = node["selectedIndex"].as<int>(rhs.selectedIndex);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.allowCustomInput = node["allowCustomInput"].as<bool>(rhs.allowCustomInput);
            rhs.displayText = node["displayText"].as<ECS::TextComponent>(rhs.displayText);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            if (node["dropdownIcon"]) rhs.dropdownIcon = node["dropdownIcon"].as<AssetHandle>(rhs.dropdownIcon);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            rhs.normalColor = node["normalColor"].as<ECS::Color>(rhs.normalColor);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(rhs.hoverColor);
            rhs.pressedColor = node["pressedColor"].as<ECS::Color>(rhs.pressedColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.dropdownBackgroundColor = node["dropdownBackgroundColor"].as<ECS::Color>(rhs.dropdownBackgroundColor);
            rhs.onSelectionChangedTargets = node["onSelectionChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onSelectionChangedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ExpanderComponent。
     */
    template <>
    struct convert<ECS::ExpanderComponent>
    {
        static Node encode(const ECS::ExpanderComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["header"] = rhs.header;
            node["isExpanded"] = rhs.isExpanded;
            node["isInteractable"] = rhs.isInteractable;
            node["roundness"] = rhs.roundness;
            node["backgroundImage"] = rhs.backgroundImage;
            node["headerColor"] = rhs.headerColor;
            node["expandedColor"] = rhs.expandedColor;
            node["collapsedColor"] = rhs.collapsedColor;
            node["disabledColor"] = rhs.disabledColor;
            node["onExpandedTargets"] = rhs.onExpandedTargets;
            node["onCollapsedTargets"] = rhs.onCollapsedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::ExpanderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.header = node["header"].as<ECS::TextComponent>(rhs.header);
            rhs.isExpanded = node["isExpanded"].as<bool>(rhs.isExpanded);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            rhs.headerColor = node["headerColor"].as<ECS::Color>(rhs.headerColor);
            rhs.expandedColor = node["expandedColor"].as<ECS::Color>(rhs.expandedColor);
            rhs.collapsedColor = node["collapsedColor"].as<ECS::Color>(rhs.collapsedColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.onExpandedTargets = node["onExpandedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onExpandedTargets);
            rhs.onCollapsedTargets = node["onCollapsedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onCollapsedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ProgressBarComponent。
     */
    template <>
    struct convert<ECS::ProgressBarComponent>
    {
        static Node encode(const ECS::ProgressBarComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["minValue"] = rhs.minValue;
            node["maxValue"] = rhs.maxValue;
            node["value"] = rhs.value;
            node["showPercentage"] = rhs.showPercentage;
            node["isIndeterminate"] = rhs.isIndeterminate;
            node["indeterminateSpeed"] = rhs.indeterminateSpeed;
            node["backgroundImage"] = rhs.backgroundImage;
            node["fillImage"] = rhs.fillImage;
            node["backgroundColor"] = rhs.backgroundColor;
            node["fillColor"] = rhs.fillColor;
            node["borderColor"] = rhs.borderColor;
            node["onValueChangedTargets"] = rhs.onValueChangedTargets;
            node["onCompletedTargets"] = rhs.onCompletedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::ProgressBarComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.minValue = node["minValue"].as<float>(rhs.minValue);
            rhs.maxValue = node["maxValue"].as<float>(rhs.maxValue);
            rhs.value = node["value"].as<float>(rhs.value);
            rhs.showPercentage = node["showPercentage"].as<bool>(rhs.showPercentage);
            rhs.isIndeterminate = node["isIndeterminate"].as<bool>(rhs.isIndeterminate);
            rhs.indeterminateSpeed = node["indeterminateSpeed"].as<float>(rhs.indeterminateSpeed);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            if (node["fillImage"]) rhs.fillImage = node["fillImage"].as<AssetHandle>(rhs.fillImage);
            rhs.backgroundColor = node["backgroundColor"].as<ECS::Color>(rhs.backgroundColor);
            rhs.fillColor = node["fillColor"].as<ECS::Color>(rhs.fillColor);
            rhs.borderColor = node["borderColor"].as<ECS::Color>(rhs.borderColor);
            rhs.onValueChangedTargets = node["onValueChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onValueChangedTargets);
            rhs.onCompletedTargets = node["onCompletedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onCompletedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::TabItem。
     */
    template <>
    struct convert<ECS::TabItem>
    {
        static Node encode(const ECS::TabItem& rhs)
        {
            Node node;
            node["title"] = rhs.title;
            node["contentGuid"] = rhs.contentGuid;
            node["isVisible"] = rhs.isVisible;
            node["isEnabled"] = rhs.isEnabled;
            return node;
        }

        static bool decode(const Node& node, ECS::TabItem& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.title = node["title"].as<std::string>(rhs.title);
            if (node["contentGuid"]) rhs.contentGuid = node["contentGuid"].as<Guid>(rhs.contentGuid);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.isEnabled = node["isEnabled"].as<bool>(rhs.isEnabled);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::TabControlComponent。
     */
    template <>
    struct convert<ECS::TabControlComponent>
    {
        static Node encode(const ECS::TabControlComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["tabs"] = rhs.tabs;
            node["activeTabIndex"] = rhs.activeTabIndex;
            node["isInteractable"] = rhs.isInteractable;
            node["allowReorder"] = rhs.allowReorder;
            node["allowCloseTabs"] = rhs.allowCloseTabs;
            node["backgroundImage"] = rhs.backgroundImage;
            node["tabBackgroundImage"] = rhs.tabBackgroundImage;
            node["tabHeight"] = rhs.tabHeight;
            node["tabSpacing"] = rhs.tabSpacing;
            node["backgroundColor"] = rhs.backgroundColor;
            node["tabColor"] = rhs.tabColor;
            node["activeTabColor"] = rhs.activeTabColor;
            node["hoverTabColor"] = rhs.hoverTabColor;
            node["disabledTabColor"] = rhs.disabledTabColor;
            node["onTabChangedTargets"] = rhs.onTabChangedTargets;
            node["onTabClosedTargets"] = rhs.onTabClosedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::TabControlComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.tabs = node["tabs"].as<std::vector<ECS::TabItem>>(rhs.tabs);
            rhs.activeTabIndex = node["activeTabIndex"].as<int>(rhs.activeTabIndex);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.allowReorder = node["allowReorder"].as<bool>(rhs.allowReorder);
            rhs.allowCloseTabs = node["allowCloseTabs"].as<bool>(rhs.allowCloseTabs);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            if (node["tabBackgroundImage"]) rhs.tabBackgroundImage = node["tabBackgroundImage"].as<AssetHandle>(rhs.tabBackgroundImage);
            rhs.tabHeight = node["tabHeight"].as<float>(rhs.tabHeight);
            rhs.tabSpacing = node["tabSpacing"].as<float>(rhs.tabSpacing);
            rhs.backgroundColor = node["backgroundColor"].as<ECS::Color>(rhs.backgroundColor);
            rhs.tabColor = node["tabColor"].as<ECS::Color>(rhs.tabColor);
            rhs.activeTabColor = node["activeTabColor"].as<ECS::Color>(rhs.activeTabColor);
            rhs.hoverTabColor = node["hoverTabColor"].as<ECS::Color>(rhs.hoverTabColor);
            rhs.disabledTabColor = node["disabledTabColor"].as<ECS::Color>(rhs.disabledTabColor);
            rhs.onTabChangedTargets = node["onTabChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onTabChangedTargets);
            rhs.onTabClosedTargets = node["onTabClosedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onTabClosedTargets);
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::ListBoxComponent。
     */
    template <>
    struct convert<ECS::ListBoxComponent>
    {
        static Node encode(const ECS::ListBoxComponent& rhs)
        {
            Node node;
            node["rect"] = rhs.rect;
            node["isVisible"] = rhs.isVisible;
            node["zIndex"] = rhs.zIndex;
            node["Enable"] = rhs.Enable;
            node["items"] = rhs.items;
            node["itemsContainerGuid"] = rhs.itemsContainerGuid;
            node["selectedIndices"] = rhs.selectedIndices;
            node["allowMultiSelect"] = rhs.allowMultiSelect;
            node["isInteractable"] = rhs.isInteractable;
            node["visibleItemCount"] = rhs.visibleItemCount;
            node["roundness"] = rhs.roundness;
            node["layout"] = static_cast<int>(rhs.layout);
            node["itemSpacing"] = rhs.itemSpacing;
            node["maxItemsPerRow"] = rhs.maxItemsPerRow;
            node["maxItemsPerColumn"] = rhs.maxItemsPerColumn;
            node["itemTemplate"] = rhs.itemTemplate;
            node["backgroundImage"] = rhs.backgroundImage;
            node["backgroundColor"] = rhs.backgroundColor;
            node["itemColor"] = rhs.itemColor;
            node["hoverColor"] = rhs.hoverColor;
            node["selectedColor"] = rhs.selectedColor;
            node["disabledColor"] = rhs.disabledColor;
            node["enableVerticalScrollbar"] = rhs.enableVerticalScrollbar;
            node["verticalScrollbarAutoHide"] = rhs.verticalScrollbarAutoHide;
            node["enableHorizontalScrollbar"] = rhs.enableHorizontalScrollbar;
            node["horizontalScrollbarAutoHide"] = rhs.horizontalScrollbarAutoHide;
            node["scrollbarThickness"] = rhs.scrollbarThickness;
            node["scrollbarTrackColor"] = rhs.scrollbarTrackColor;
            node["scrollbarThumbColor"] = rhs.scrollbarThumbColor;
            node["onSelectionChangedTargets"] = rhs.onSelectionChangedTargets;
            node["onItemActivatedTargets"] = rhs.onItemActivatedTargets;
            return node;
        }

        static bool decode(const Node& node, ECS::ListBoxComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.rect = node["rect"].as<ECS::RectF>(rhs.rect);
            rhs.isVisible = node["isVisible"].as<bool>(rhs.isVisible);
            rhs.zIndex = node["zIndex"].as<int>(rhs.zIndex);
            rhs.Enable = node["Enable"].as<bool>(rhs.Enable);
            rhs.items = node["items"].as<std::vector<std::string>>(rhs.items);
            rhs.itemsContainerGuid = node["itemsContainerGuid"].as<Guid>(rhs.itemsContainerGuid);
            rhs.selectedIndices = node["selectedIndices"].as<std::vector<int>>(rhs.selectedIndices);
            rhs.allowMultiSelect = node["allowMultiSelect"].as<bool>(rhs.allowMultiSelect);
            rhs.isInteractable = node["isInteractable"].as<bool>(rhs.isInteractable);
            rhs.visibleItemCount = node["visibleItemCount"].as<int>(rhs.visibleItemCount);
            rhs.roundness = node["roundness"].as<float>(rhs.roundness);
            if (node["layout"])
            {
                rhs.layout = static_cast<ECS::ListBoxLayout>(
                    node["layout"].as<int>(static_cast<int>(rhs.layout)));
            }
            rhs.itemSpacing = node["itemSpacing"].as<ECS::Vector2f>(rhs.itemSpacing);
            rhs.maxItemsPerRow = node["maxItemsPerRow"].as<int>(rhs.maxItemsPerRow);
            rhs.maxItemsPerColumn = node["maxItemsPerColumn"].as<int>(rhs.maxItemsPerColumn);
            rhs.itemTemplate = node["itemTemplate"].as<ECS::TextComponent>(rhs.itemTemplate);
            if (node["backgroundImage"]) rhs.backgroundImage = node["backgroundImage"].as<AssetHandle>(rhs.backgroundImage);
            rhs.backgroundColor = node["backgroundColor"].as<ECS::Color>(rhs.backgroundColor);
            rhs.itemColor = node["itemColor"].as<ECS::Color>(rhs.itemColor);
            rhs.hoverColor = node["hoverColor"].as<ECS::Color>(rhs.hoverColor);
            rhs.selectedColor = node["selectedColor"].as<ECS::Color>(rhs.selectedColor);
            rhs.disabledColor = node["disabledColor"].as<ECS::Color>(rhs.disabledColor);
            rhs.enableVerticalScrollbar = node["enableVerticalScrollbar"].as<bool>(rhs.enableVerticalScrollbar);
            rhs.verticalScrollbarAutoHide = node["verticalScrollbarAutoHide"].as<bool>(rhs.verticalScrollbarAutoHide);
            rhs.enableHorizontalScrollbar = node["enableHorizontalScrollbar"].as<bool>(rhs.enableHorizontalScrollbar);
            rhs.horizontalScrollbarAutoHide = node["horizontalScrollbarAutoHide"].as<bool>(rhs.horizontalScrollbarAutoHide);
            rhs.scrollbarThickness = node["scrollbarThickness"].as<float>(rhs.scrollbarThickness);
            rhs.scrollbarTrackColor = node["scrollbarTrackColor"].as<ECS::Color>(rhs.scrollbarTrackColor);
            rhs.scrollbarThumbColor = node["scrollbarThumbColor"].as<ECS::Color>(rhs.scrollbarThumbColor);
            rhs.onSelectionChangedTargets = node["onSelectionChangedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onSelectionChangedTargets);
            rhs.onItemActivatedTargets = node["onItemActivatedTargets"].as<std::vector<ECS::SerializableEventTarget>>(rhs.onItemActivatedTargets);
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::ButtonComponent>("ButtonComponent")
        .property("rect", &ECS::ButtonComponent::rect)
        .property("isVisible", &ECS::ButtonComponent::isVisible)
        .property("isInteractable", &ECS::ButtonComponent::isInteractable)

        .property("backgroundImage", &ECS::ButtonComponent::backgroundImage)
        .property("roundness", &ECS::ButtonComponent::roundness)

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

    Registry_<ECS::ToggleButtonComponent>("ToggleButtonComponent")
        .property("rect", &ECS::ToggleButtonComponent::rect)
        .property("isVisible", &ECS::ToggleButtonComponent::isVisible)
        .property("isInteractable", &ECS::ToggleButtonComponent::isInteractable)
        .property("isToggled", &ECS::ToggleButtonComponent::isToggled)
        .property("allowToggleOff", &ECS::ToggleButtonComponent::allowToggleOff)
        .property("backgroundImage", &ECS::ToggleButtonComponent::backgroundImage)
        .property("roundness", &ECS::ToggleButtonComponent::roundness)
        .property("normalColor", &ECS::ToggleButtonComponent::normalColor)
        .property("hoverColor", &ECS::ToggleButtonComponent::hoverColor)
        .property("pressedColor", &ECS::ToggleButtonComponent::pressedColor)
        .property("toggledColor", &ECS::ToggleButtonComponent::toggledColor)
        .property("toggledHoverColor", &ECS::ToggleButtonComponent::toggledHoverColor)
        .property("toggledPressedColor", &ECS::ToggleButtonComponent::toggledPressedColor)
        .property("disabledColor", &ECS::ToggleButtonComponent::disabledColor)
        .property("onToggleOnTargets", &ECS::ToggleButtonComponent::onToggleOnTargets)
        .property("onToggleOffTargets", &ECS::ToggleButtonComponent::onToggleOffTargets);

    Registry_<ECS::RadioButtonComponent>("RadioButtonComponent")
        .property("rect", &ECS::RadioButtonComponent::rect)
        .property("isVisible", &ECS::RadioButtonComponent::isVisible)
        .property("label", &ECS::RadioButtonComponent::label)
        .property("groupId", &ECS::RadioButtonComponent::groupId)
        .property("isSelected", &ECS::RadioButtonComponent::isSelected)
        .property("isInteractable", &ECS::RadioButtonComponent::isInteractable)
        .property("backgroundImage", &ECS::RadioButtonComponent::backgroundImage)
        .property("selectionImage", &ECS::RadioButtonComponent::selectionImage)
        .property("roundness", &ECS::RadioButtonComponent::roundness)
        .property("normalColor", &ECS::RadioButtonComponent::normalColor)
        .property("hoverColor", &ECS::RadioButtonComponent::hoverColor)
        .property("selectedColor", &ECS::RadioButtonComponent::selectedColor)
        .property("disabledColor", &ECS::RadioButtonComponent::disabledColor)
        .property("indicatorColor", &ECS::RadioButtonComponent::indicatorColor)
        .property("onSelectedTargets", &ECS::RadioButtonComponent::onSelectedTargets)
        .property("onDeselectedTargets", &ECS::RadioButtonComponent::onDeselectedTargets);

    Registry_<ECS::CheckBoxComponent>("CheckBoxComponent")
        .property("rect", &ECS::CheckBoxComponent::rect)
        .property("isVisible", &ECS::CheckBoxComponent::isVisible)
        .property("label", &ECS::CheckBoxComponent::label)
        .property("isChecked", &ECS::CheckBoxComponent::isChecked)
        .property("allowIndeterminate", &ECS::CheckBoxComponent::allowIndeterminate)
        .property("isIndeterminate", &ECS::CheckBoxComponent::isIndeterminate)
        .property("isInteractable", &ECS::CheckBoxComponent::isInteractable)
        .property("backgroundImage", &ECS::CheckBoxComponent::backgroundImage)
        .property("checkmarkImage", &ECS::CheckBoxComponent::checkmarkImage)
        .property("roundness", &ECS::CheckBoxComponent::roundness)
        .property("normalColor", &ECS::CheckBoxComponent::normalColor)
        .property("hoverColor", &ECS::CheckBoxComponent::hoverColor)
        .property("checkedColor", &ECS::CheckBoxComponent::checkedColor)
        .property("indeterminateColor", &ECS::CheckBoxComponent::indeterminateColor)
        .property("disabledColor", &ECS::CheckBoxComponent::disabledColor)
        .property("checkmarkColor", &ECS::CheckBoxComponent::checkmarkColor)
        .property("onValueChangedTargets", &ECS::CheckBoxComponent::onValueChangedTargets);

    Registry_<ECS::SliderComponent>("SliderComponent")
        .property("rect", &ECS::SliderComponent::rect)
        .property("isVisible", &ECS::SliderComponent::isVisible)
        .property("minValue", &ECS::SliderComponent::minValue)
        .property("maxValue", &ECS::SliderComponent::maxValue)
        .property("value", &ECS::SliderComponent::value)
        .property("step", &ECS::SliderComponent::step)
        .property("isInteractable", &ECS::SliderComponent::isInteractable)
        .property("isVertical", &ECS::SliderComponent::isVertical)
        .property("trackImage", &ECS::SliderComponent::trackImage)
        .property("fillImage", &ECS::SliderComponent::fillImage)
        .property("thumbImage", &ECS::SliderComponent::thumbImage)
        .property("trackColor", &ECS::SliderComponent::trackColor)
        .property("fillColor", &ECS::SliderComponent::fillColor)
        .property("thumbColor", &ECS::SliderComponent::thumbColor)
        .property("disabledColor", &ECS::SliderComponent::disabledColor)
        .property("onValueChangedTargets", &ECS::SliderComponent::onValueChangedTargets)
        .property("onDragStartedTargets", &ECS::SliderComponent::onDragStartedTargets)
        .property("onDragEndedTargets", &ECS::SliderComponent::onDragEndedTargets);

    Registry_<ECS::ComboBoxComponent>("ComboBoxComponent")
        .property("rect", &ECS::ComboBoxComponent::rect)
        .property("isVisible", &ECS::ComboBoxComponent::isVisible)
        .property("items", &ECS::ComboBoxComponent::items)
        .property("selectedIndex", &ECS::ComboBoxComponent::selectedIndex)
        .property("isInteractable", &ECS::ComboBoxComponent::isInteractable)
        .property("allowCustomInput", &ECS::ComboBoxComponent::allowCustomInput)
        .property("displayText", &ECS::ComboBoxComponent::displayText)
        .property("backgroundImage", &ECS::ComboBoxComponent::backgroundImage)
        .property("dropdownIcon", &ECS::ComboBoxComponent::dropdownIcon)
        .property("roundness", &ECS::ComboBoxComponent::roundness)
        .property("normalColor", &ECS::ComboBoxComponent::normalColor)
        .property("hoverColor", &ECS::ComboBoxComponent::hoverColor)
        .property("pressedColor", &ECS::ComboBoxComponent::pressedColor)
        .property("disabledColor", &ECS::ComboBoxComponent::disabledColor)
        .property("dropdownBackgroundColor", &ECS::ComboBoxComponent::dropdownBackgroundColor)
        .property("onSelectionChangedTargets", &ECS::ComboBoxComponent::onSelectionChangedTargets);

    Registry_<ECS::ExpanderComponent>("ExpanderComponent")
        .property("rect", &ECS::ExpanderComponent::rect)
        .property("isVisible", &ECS::ExpanderComponent::isVisible)
        .property("header", &ECS::ExpanderComponent::header)
        .property("isExpanded", &ECS::ExpanderComponent::isExpanded)
        .property("isInteractable", &ECS::ExpanderComponent::isInteractable)
        .property("roundness", &ECS::ExpanderComponent::roundness)
        .property("backgroundImage", &ECS::ExpanderComponent::backgroundImage)
        .property("headerColor", &ECS::ExpanderComponent::headerColor)
        .property("expandedColor", &ECS::ExpanderComponent::expandedColor)
        .property("collapsedColor", &ECS::ExpanderComponent::collapsedColor)
        .property("disabledColor", &ECS::ExpanderComponent::disabledColor)
        .property("onExpandedTargets", &ECS::ExpanderComponent::onExpandedTargets)
        .property("onCollapsedTargets", &ECS::ExpanderComponent::onCollapsedTargets);

    Registry_<ECS::ProgressBarComponent>("ProgressBarComponent")
        .property("rect", &ECS::ProgressBarComponent::rect)
        .property("isVisible", &ECS::ProgressBarComponent::isVisible)
        .property("minValue", &ECS::ProgressBarComponent::minValue)
        .property("maxValue", &ECS::ProgressBarComponent::maxValue)
        .property("value", &ECS::ProgressBarComponent::value)
        .property("showPercentage", &ECS::ProgressBarComponent::showPercentage)
        .property("isIndeterminate", &ECS::ProgressBarComponent::isIndeterminate)
        .property("indeterminateSpeed", &ECS::ProgressBarComponent::indeterminateSpeed)
        .property("backgroundImage", &ECS::ProgressBarComponent::backgroundImage)
        .property("fillImage", &ECS::ProgressBarComponent::fillImage)
        .property("backgroundColor", &ECS::ProgressBarComponent::backgroundColor)
        .property("fillColor", &ECS::ProgressBarComponent::fillColor)
        .property("borderColor", &ECS::ProgressBarComponent::borderColor)
        .property("onValueChangedTargets", &ECS::ProgressBarComponent::onValueChangedTargets)
        .property("onCompletedTargets", &ECS::ProgressBarComponent::onCompletedTargets);

    Registry_<ECS::TabControlComponent>("TabControlComponent")
        .property("rect", &ECS::TabControlComponent::rect)
        .property("isVisible", &ECS::TabControlComponent::isVisible)
        .property("tabs", &ECS::TabControlComponent::tabs)
        .property("activeTabIndex", &ECS::TabControlComponent::activeTabIndex)
        .property("isInteractable", &ECS::TabControlComponent::isInteractable)
        .property("allowReorder", &ECS::TabControlComponent::allowReorder)
        .property("allowCloseTabs", &ECS::TabControlComponent::allowCloseTabs)
        .property("backgroundImage", &ECS::TabControlComponent::backgroundImage)
        .property("tabBackgroundImage", &ECS::TabControlComponent::tabBackgroundImage)
        .property("tabHeight", &ECS::TabControlComponent::tabHeight)
        .property("tabSpacing", &ECS::TabControlComponent::tabSpacing)
        .property("backgroundColor", &ECS::TabControlComponent::backgroundColor)
        .property("tabColor", &ECS::TabControlComponent::tabColor)
        .property("activeTabColor", &ECS::TabControlComponent::activeTabColor)
        .property("hoverTabColor", &ECS::TabControlComponent::hoverTabColor)
        .property("disabledTabColor", &ECS::TabControlComponent::disabledTabColor)
        .property("onTabChangedTargets", &ECS::TabControlComponent::onTabChangedTargets)
        .property("onTabClosedTargets", &ECS::TabControlComponent::onTabClosedTargets);

    Registry_<ECS::ListBoxComponent>("ListBoxComponent")
        .property("rect", &ECS::ListBoxComponent::rect)
        .property("isVisible", &ECS::ListBoxComponent::isVisible)
        .property("items", &ECS::ListBoxComponent::items)
        .property("itemsContainerGuid", &ECS::ListBoxComponent::itemsContainerGuid)
        .property("selectedIndices", &ECS::ListBoxComponent::selectedIndices)
        .property("allowMultiSelect", &ECS::ListBoxComponent::allowMultiSelect)
        .property("isInteractable", &ECS::ListBoxComponent::isInteractable)
        .property("visibleItemCount", &ECS::ListBoxComponent::visibleItemCount)
        .property("layout", &ECS::ListBoxComponent::layout)
        .property("itemSpacing", &ECS::ListBoxComponent::itemSpacing)
        .property("maxItemsPerRow", &ECS::ListBoxComponent::maxItemsPerRow)
        .property("maxItemsPerColumn", &ECS::ListBoxComponent::maxItemsPerColumn)
        .property("itemTemplate", &ECS::ListBoxComponent::itemTemplate)
        .property("backgroundImage", &ECS::ListBoxComponent::backgroundImage)
        .property("backgroundColor", &ECS::ListBoxComponent::backgroundColor)
        .property("itemColor", &ECS::ListBoxComponent::itemColor)
        .property("hoverColor", &ECS::ListBoxComponent::hoverColor)
        .property("selectedColor", &ECS::ListBoxComponent::selectedColor)
        .property("disabledColor", &ECS::ListBoxComponent::disabledColor)
        .property("roundness", &ECS::ListBoxComponent::roundness)
        .property("enableVerticalScrollbar", &ECS::ListBoxComponent::enableVerticalScrollbar)
        .property("verticalScrollbarAutoHide", &ECS::ListBoxComponent::verticalScrollbarAutoHide)
        .property("enableHorizontalScrollbar", &ECS::ListBoxComponent::enableHorizontalScrollbar)
        .property("horizontalScrollbarAutoHide", &ECS::ListBoxComponent::horizontalScrollbarAutoHide)
        .property("scrollbarThickness", &ECS::ListBoxComponent::scrollbarThickness)
        .property("scrollbarTrackColor", &ECS::ListBoxComponent::scrollbarTrackColor)
        .property("scrollbarThumbColor", &ECS::ListBoxComponent::scrollbarThumbColor)
        .property("onSelectionChangedTargets", &ECS::ListBoxComponent::onSelectionChangedTargets)
        .property("onItemActivatedTargets", &ECS::ListBoxComponent::onItemActivatedTargets);
}

#endif

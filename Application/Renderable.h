#pragma once
#include <entt/entt.hpp>
#include <variant>
#include <string>
#include <unordered_map>
#include <vector>
#include <vector>
#include <memory>
#include <include/core/SkImage.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkRect.h>
#include <include/core/SkColor.h>
#include "../Components/Transform.h"
#include "../Renderer/RenderComponent.h"
#include "../Components/UIComponents.h" 
namespace Nut { class TextureA; }
class RuntimeWGSLMaterial;
struct SpriteRenderData
{
    SkImage* image = nullptr;
    Material* material = nullptr;
    std::shared_ptr<Nut::TextureA> wgpuTexture = nullptr;
    RuntimeWGSLMaterial* wgpuMaterial = nullptr;
    SkRect sourceRect;
    ECS::Color color;
    int filterQuality;
    int wrapMode;
    float ppuScaleFactor;
    bool isUISprite = false; 
};
struct TextRenderData
{
    SkTypeface* typeface = nullptr;
    std::string text;
    float fontSize;
    ECS::Color color;
    int alignment;
};
struct RawButtonRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    ECS::Color normalColor, hoverColor, pressedColor, disabledColor;
    sk_sp<SkImage> backgroundImage;
    float roundness;
};
struct RawInputTextRenderData
{
    ECS::RectF rect;
    float roundness;
    ECS::Color normalBackgroundColor, focusedBackgroundColor, readOnlyBackgroundColor, cursorColor;
    ECS::TextComponent text;
    ECS::TextComponent placeholder;
    bool isReadOnly, isFocused, isPasswordField, isCursorVisible;
    size_t cursorPosition;
    sk_sp<SkImage> backgroundImage;
    std::string inputBuffer;
};
struct RawToggleButtonRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    bool isToggled;
    ECS::Color normalColor, hoverColor, pressedColor;
    ECS::Color toggledColor, toggledHoverColor, toggledPressedColor;
    ECS::Color disabledColor;
    sk_sp<SkImage> backgroundImage;
    float roundness;
};
struct RawRadioButtonRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    bool isSelected;
    ECS::Color normalColor, hoverColor, selectedColor, disabledColor, indicatorColor;
    ECS::TextComponent label;
    sk_sp<SkImage> backgroundImage;
    sk_sp<SkImage> selectionImage;
    float roundness;
};
struct RawCheckBoxRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    bool isChecked;
    bool isIndeterminate;
    ECS::Color normalColor, hoverColor, checkedColor, indeterminateColor, disabledColor, checkmarkColor;
    ECS::TextComponent label;
    sk_sp<SkImage> backgroundImage;
    sk_sp<SkImage> checkmarkImage;
    float roundness;
};
struct RawSliderRenderData
{
    ECS::RectF rect;
    bool isVertical;
    bool isDragging;
    bool isInteractable;
    float normalizedValue;
    ECS::Color trackColor, fillColor, thumbColor, disabledColor;
    sk_sp<SkImage> trackImage;
    sk_sp<SkImage> fillImage;
    sk_sp<SkImage> thumbImage;
};
struct RawComboBoxRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    bool isDropdownOpen;
    int selectedIndex;
    int hoveredIndex;
    ECS::TextComponent displayText;
    std::vector<std::string> items;
    ECS::Color normalColor, hoverColor, pressedColor, disabledColor;
    ECS::Color dropdownBackgroundColor;
    sk_sp<SkImage> backgroundImage;
    sk_sp<SkImage> dropdownIcon;
    float roundness;
};
struct RawExpanderRenderData
{
    ECS::RectF rect;
    bool isExpanded;
    ECS::TextComponent header;
    ECS::Color headerColor, expandedColor, collapsedColor, disabledColor;
    sk_sp<SkImage> backgroundImage;
    float roundness;
};
struct RawProgressBarRenderData
{
    ECS::RectF rect;
    float minValue;
    float maxValue;
    float value;
    bool showPercentage;
    bool isIndeterminate;
    float indeterminatePhase;
    ECS::Color backgroundColor, fillColor, borderColor;
    sk_sp<SkImage> backgroundImage;
    sk_sp<SkImage> fillImage;
};
struct RawTabControlRenderData
{
    ECS::RectF rect;
    std::vector<ECS::TabItem> tabs;
    int activeTabIndex;
    int hoveredTabIndex;
    float tabHeight;
    float tabSpacing;
    ECS::Color backgroundColor, tabColor, activeTabColor, hoverTabColor, disabledTabColor;
    sk_sp<SkImage> backgroundImage;
    sk_sp<SkImage> tabBackgroundImage;
};
struct RawListBoxRenderData
{
    ECS::RectF rect;
    float roundness;
    int itemCount = 0;
    std::vector<std::string> items;
    std::vector<int> selectedIndices;
    int hoveredIndex;
    int scrollOffset;
    int visibleItemCount;
    ECS::ListBoxLayout layout;
    ECS::Vector2f itemSpacing;
    int maxItemsPerRow;
    int maxItemsPerColumn;
    bool useContainer = false;
    bool enableVerticalScrollbar;
    bool verticalScrollbarAutoHide;
    bool enableHorizontalScrollbar;
    bool horizontalScrollbarAutoHide;
    float scrollbarThickness;
    ECS::TextComponent itemTemplate;
    ECS::Color backgroundColor, itemColor, hoverColor, selectedColor, disabledColor;
    ECS::Color scrollbarTrackColor, scrollbarThumbColor;
    sk_sp<SkImage> backgroundImage;
};
struct Renderable
{
    entt::entity entityId;
    int zIndex = 0;
    uint64_t sortKey = 0;
    ECS::TransformComponent transform;
    std::variant<
        SpriteRenderData,
        TextRenderData,
        RawButtonRenderData,
        RawInputTextRenderData,
        RawToggleButtonRenderData,
        RawRadioButtonRenderData,
        RawCheckBoxRenderData,
        RawSliderRenderData,
        RawComboBoxRenderData,
        RawExpanderRenderData,
        RawProgressBarRenderData,
        RawTabControlRenderData,
        RawListBoxRenderData
    > data;
};

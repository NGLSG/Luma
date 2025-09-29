#pragma once

#include <entt/entt.hpp>
#include <variant>
#include <string>
#include <unordered_map>

#include <include/core/SkImage.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkRect.h>
#include <include/core/SkColor.h>

#include "../Components/Transform.h"
#include "../Renderer/RenderComponent.h"
#include "../Components/UIComponents.h" // 包含 Button 和 InputText 的定义

/**
 * @brief 存储精灵渲染所需的所有原始数据。
 */
struct SpriteRenderData
{
    SkImage* image = nullptr;
    Material* material = nullptr;
    SkRect sourceRect;
    ECS::Color color;
    int filterQuality;
    int wrapMode;
    float ppuScaleFactor;
};

/**
 * @brief 存储文本渲染所需的所有原始数据。
 */
struct TextRenderData
{
    SkTypeface* typeface = nullptr;
    std::string text;
    float fontSize;
    ECS::Color color;
    int alignment;
};

/**
 * @brief 存储按钮组件渲染所需的所有原始数据。
 */
struct RawButtonRenderData
{
    ECS::RectF rect;
    ECS::ButtonState currentState;
    ECS::Color normalColor, hoverColor, pressedColor, disabledColor;
    sk_sp<SkImage> backgroundImage;
    float roundness;
};

/**
 * @brief 存储输入框组件渲染所需的所有原始数据。
 */
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

/**
 * @brief 代表一个可渲染单元，包含所有必要数据以便在渲染线程中进行插值和批处理。
 */
struct Renderable
{
    entt::entity entityId;
    int zIndex = 0;
    ECS::TransformComponent transform;

    std::variant<
        SpriteRenderData,
        TextRenderData,
        RawButtonRenderData,
        RawInputTextRenderData
    > data;
};
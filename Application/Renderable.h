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

struct TextRenderData
{
    SkTypeface* typeface = nullptr;
    std::string text;
    float fontSize;
    ECS::Color color;
    int alignment;
};

struct Renderable
{
    entt::entity entityId;
    int zIndex = 0;
    ECS::TransformComponent transform;

    std::variant<
        SpriteRenderData,
        TextRenderData
    > data;

};

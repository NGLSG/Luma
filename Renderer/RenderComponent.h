#pragma once

#include <unordered_map>
#include <variant>
#include <vector>
#include <include/core/SkImage.h>
#include <include/core/SkPoint.h>
#include <include/core/SkSize.h>
#include <include/core/SkColor.h>
#include <include/core/SkRect.h>
#include <include/effects/SkRuntimeEffect.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkM44.h"
#include "include/core/SkTypeface.h"
#include "Nut/Buffer.h"
#include <memory>

class RuntimeWGSLMaterial;

namespace Nut
{
    class TextureA;
}

/**
 * @brief 概念：判断类型是否为统一变量类型。
 */
template <class T>
concept IsUniformType = std::is_same_v<T, float> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, SkPoint> ||
    std::is_same_v<T, SkColor4f> ||
    std::is_same_v<T, sk_sp<SkShader>>;

/**
 * @brief 统一变量的变体类型。
 */
using UniformVariant = std::variant<
    float,
    int,
    SkPoint,
    SkColor4f,
    SkV3,
    sk_sp<SkShader>
>;

/**
 * @brief 材质结构体。
 */
struct Material : public SkRefCnt
{
    sk_sp<SkRuntimeEffect> effect = nullptr;
    std::unordered_map<std::string, UniformVariant> uniforms;
};

/**
 * @brief 统一变量设置器。
 */
struct UniformSetter
{
    SkRuntimeShaderBuilder& builder;
    const std::string& name;

    void operator()(int value) const { builder.uniform(name) = static_cast<float>(value); }
    void operator()(float value) const { builder.uniform(name) = value; }
    void operator()(const SkPoint& value) const { builder.uniform(name) = value; }
    void operator()(const SkColor4f& value) const { builder.uniform(name) = value; }
    void operator()(const SkV3& value) const { builder.uniform(name) = value; }
    void operator()(const sk_sp<SkShader>& value) const { builder.child(name) = value; }
};

/**
 * @brief 变换组件结构体。
 */
struct TransformComponent
{
    SkPoint position{0.0f, 0.0f};
    SkPoint scale{1.0f};
    float rotation{0.0f};

    bool operator==(const TransformComponent& other) const
    {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }
};

/**
 * @brief 可渲染变换结构体。
 */
struct RenderableTransform
{
    SkPoint position;
    float scaleX;
    float scaleY;
    float sinR;
    float cosR;

    RenderableTransform() = default;

    RenderableTransform(const TransformComponent& transform)
        : position(transform.position),
          scaleX(transform.scale.x()),
          scaleY(transform.scale.y()),
          sinR(sinf(transform.rotation)),
          cosR(cosf(transform.rotation))
    {
    }

    RenderableTransform(const SkPoint& pos, float scale, float rotation)
        : position(pos),
          scaleX(scale),
          scaleY(scale),
          sinR(sinf(rotation)),
          cosR(cosf(rotation))
    {
    }

    RenderableTransform(const SkPoint& pos, float scaleX, float scaleY, float rotation) :
        position(pos),
        scaleX(scaleX),
        scaleY(scaleY),
        sinR(sinf(rotation)),
        cosR(cosf(rotation))
    {
    }

    RenderableTransform(const SkPoint& pos, float scaleX, float scaleY, float sin, float cos) :
        position(pos),
        scaleX(scaleX),
        scaleY(scaleY),
        sinR(sin),
        cosR(cos)
    {
    }
};

/**
 * @brief 渲染空间类型枚举。
 *
 * 定义渲染对象所在的坐标空间。
 */
enum class RenderSpace
{
    World,  ///< 世界空间，使用激活的世界摄像机变换。
    Camera  ///< 指定相机空间，使用cameraId指定的相机。
};

/**
 * @brief 精灵批次结构体。
 */
struct SpriteBatch
{
    const Material* material = nullptr;
    sk_sp<SkImage> image;
    SkRect sourceRect;
    SkColor4f color = SkColors::kWhite;
    const RenderableTransform* transforms = nullptr;
    int filterQuality = 0;
    int wrapMode = 0;
    float ppuScaleFactor;
    size_t count;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;  ///< 当renderSpace为Camera时使用的相机ID
};

/**
 * @brief WGSL 渲染批次
 */
struct WGPUSpriteBatch
{
    RuntimeWGSLMaterial* material = nullptr;
    std::shared_ptr<Nut::TextureA> image;
    SkRect sourceRect;
    ECS::Color color = ECS::Colors::White;
    const RenderableTransform* transforms = nullptr;
    int filterQuality = 0;
    int wrapMode = 0;
    float ppuScaleFactor = 1.0f;
    size_t count = 0;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
    uint32_t lightLayer = 0xFFFFFFFF; ///< 光照层掩码
    
    // 自发光数据 (Feature: 2d-lighting-enhancement)
    ECS::Color emissionColor = ECS::Colors::White; ///< 自发光颜色
    float emissionIntensity = 0.0f;                ///< 自发光强度（0 = 无自发光）
};

/**
 * @brief 实例批次结构体。
 */
struct InstanceBatch
{
    sk_sp<SkImage> atlasImage;
    const SkRect* sourceRects;
    const RenderableTransform* transforms;
    SkColor4f color = SkColors::kWhite;
    int filterQuality = 0;
    int wrapMode = 0;
    size_t count;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 文本批次结构体。
 */
struct TextBatch
{
    sk_sp<SkTypeface> typeface;
    float fontSize;
    SkColor4f color = SkColors::kWhite;
    const std::string* texts;
    int alignment = 0;
    const RenderableTransform* transforms;
    size_t count;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 矩形批次结构体。
 */
struct RectBatch
{
    SkSize size;
    SkColor4f color;
    const RenderableTransform* transforms;
    size_t count;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 着色器批次结构体。
 */
struct ShaderBatch
{
    const Material* material = nullptr;
    TransformComponent transform;
    SkSize size;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 圆形批次结构体。
 */
struct CircleBatch
{
    float radius;
    SkColor4f color;
    const SkPoint* centers;
    size_t count;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 线段批次结构体。
 */
struct LineBatch
{
    float width;
    SkColor4f color;
    const SkPoint* points;
    size_t pointCount;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 原始绘制批次结构体。
 */
struct RawDrawBatch
{
    LumaEvent<SkCanvas*> drawFunc;
    int zIndex = 0;
    RenderSpace renderSpace = RenderSpace::World;
    std::string cameraId;
};

/**
 * @brief 渲染包结构体。
 */
struct RenderPacket
{
    int zIndex = 0;
    uint64_t sortKey = 0;

    std::variant<
        SpriteBatch,
        RectBatch,
        CircleBatch,
        LineBatch,
        InstanceBatch,
        ShaderBatch,
        TextBatch,
        RawDrawBatch,
        WGPUSpriteBatch
    > batchData;
};

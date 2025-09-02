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

#include "include/core/SkM44.h"
#include "include/core/SkTypeface.h"

/**
 * @brief 概念：判断类型是否为统一变量类型。
 *
 * 用于检查给定类型 `T` 是否是 Skia 运行时着色器中支持的统一变量类型之一。
 * 支持的类型包括 `float`, `int`, `SkPoint`, `SkColor4f`, `sk_sp<SkShader>`。
 *
 * @tparam T 要检查的类型。
 */
template <class T>
concept IsUniformType = std::is_same_v<T, float> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, SkPoint> ||
    std::is_same_v<T, SkColor4f> ||
    std::is_same_v<T, sk_sp<SkShader>>;


/**
 * @brief 统一变量的变体类型。
 *
 * 定义了一个 `std::variant`，可以存储多种不同类型的统一变量值，
 * 包括浮点数、整数、SkPoint、SkColor4f、SkV3 和 Skia 着色器智能指针。
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
 *
 * 继承自 SkRefCnt，用于管理 Skia 运行时效果及其相关的统一变量。
 */
struct Material : public SkRefCnt
{
    sk_sp<SkRuntimeEffect> effect = nullptr; ///< 运行时效果对象。

    std::unordered_map<std::string, UniformVariant> uniforms; ///< 统一变量的映射，键为变量名，值为其变体类型。
};

/**
 * @brief 统一变量设置器。
 *
 * 一个函数对象，用于根据统一变量的类型将其设置到 SkRuntimeShaderBuilder 中。
 */
struct UniformSetter
{
    SkRuntimeShaderBuilder& builder; ///< 运行时着色器构建器引用。
    const std::string& name; ///< 统一变量的名称。

    /**
     * @brief 设置整数类型的统一变量。
     * @param value 要设置的整数值。
     */
    void operator()(int value) const { builder.uniform(name) = static_cast<float>(value); }
    /**
     * @brief 设置浮点数类型的统一变量。
     * @param value 要设置的浮点数值。
     */
    void operator()(float value) const { builder.uniform(name) = value; }
    /**
     * @brief 设置 SkPoint 类型的统一变量。
     * @param value 要设置的 SkPoint 值。
     */
    void operator()(const SkPoint& value) const { builder.uniform(name) = value; }
    /**
     * @brief 设置 SkColor4f 类型的统一变量。
     * @param value 要设置的 SkColor4f 值。
     */
    void operator()(const SkColor4f& value) const { builder.uniform(name) = value; }
    /**
     * @brief 设置 SkV3 类型的统一变量。
     * @param value 要设置的 SkV3 值。
     */
    void operator()(const SkV3& value) const { builder.uniform(name) = value; }

    /**
     * @brief 设置 SkShader 智能指针类型的统一变量（作为子着色器）。
     * @param value 要设置的 SkShader 智能指针。
     */
    void operator()(const sk_sp<SkShader>& value) const { builder.child(name) = value; }
};


/**
 * @brief 变换组件结构体。
 *
 * 存储一个对象的二维位置、缩放和旋转信息。
 */
struct TransformComponent
{
    SkPoint position{0.0f, 0.0f}; ///< 对象的位置。
    SkPoint scale{1.0f}; ///< 对象的缩放比例。
    float rotation{0.0f}; ///< 对象的旋转角度（弧度）。

    /**
     * @brief 比较两个 TransformComponent 是否相等。
     * @param other 另一个 TransformComponent 对象。
     * @return 如果所有变换属性都相等则返回 true，否则返回 false。
     */
    bool operator==(const TransformComponent& other) const
    {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }
};


/**
 * @brief 可渲染变换结构体。
 *
 * 存储用于渲染的变换信息，包括位置、缩放因子以及旋转的正弦和余弦值，
 * 方便在渲染时直接使用。
 */
struct RenderableTransform
{
    SkPoint position; ///< 渲染对象的位置。
    float scaleX; ///< X轴缩放因子。
    float scaleY; ///< Y轴缩放因子。
    float sinR; ///< 旋转角度的正弦值。
    float cosR; ///< 旋转角度的余弦值。

    /**
     * @brief 默认构造函数。
     */
    RenderableTransform() = default;

    /**
     * @brief 从 TransformComponent 构造 RenderableTransform。
     * @param transform 变换组件。
     */
    RenderableTransform(const TransformComponent& transform)
        : position(transform.position),
          scaleX(transform.scale.x()),
          scaleY(transform.scale.y()),
          sinR(sinf(transform.rotation)),
          cosR(cosf(transform.rotation))
    {
    }

    /**
     * @brief 从位置、统一缩放和旋转角度构造 RenderableTransform。
     * @param pos 位置。
     * @param scale 统一缩放因子。
     * @param rotation 旋转角度（弧度）。
     */
    RenderableTransform(const SkPoint& pos, float scale, float rotation)
        : position(pos),
          scaleX(scale),
          scaleY(scale),
          sinR(sinf(rotation)),
          cosR(cosf(rotation))
    {
    }

    /**
     * @brief 从位置、独立缩放和旋转角度构造 RenderableTransform。
     * @param pos 位置。
     * @param scaleX X轴缩放因子。
     * @param scaleY Y轴缩放因子。
     * @param rotation 旋转角度（弧度）。
     */
    RenderableTransform(const SkPoint& pos, float scaleX, float scaleY, float rotation) :
        position(pos),
        scaleX(scaleX),
        scaleY(scaleY),
        sinR(sinf(rotation)),
        cosR(cosf(rotation))
    {
    }

    /**
     * @brief 从位置、独立缩放、旋转正弦值和余弦值构造 RenderableTransform。
     * @param pos 位置。
     * @param scaleX X轴缩放因子。
     * @param scaleY Y轴缩放因子。
     * @param sin 旋转角度的正弦值。
     * @param cos 旋转角度的余弦值。
     */
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
 * @brief 精灵批次结构体。
 *
 * 包含渲染一个或多个精灵所需的所有信息。
 */
struct SpriteBatch
{
    const Material* material = nullptr; ///< 材质指针，可为空。
    sk_sp<SkImage> image; ///< 要渲染的图像。
    SkRect sourceRect; ///< 图像中要渲染的源矩形。
    SkColor4f color = SkColors::kWhite; ///< 精灵的颜色。
    const RenderableTransform* transforms = nullptr; ///< 变换数组的指针。
    int filterQuality = 0; ///< 图像过滤质量。
    int wrapMode = 0; ///< 图像环绕模式。
    float ppuScaleFactor; ///< 每像素单位缩放因子。
    size_t count; ///< 批次中精灵的数量。
};


/**
 * @brief 实例批次结构体。
 *
 * 包含渲染多个实例所需的所有信息，通常用于图集。
 */
struct InstanceBatch
{
    sk_sp<SkImage> atlasImage; ///< 图集图像。
    const SkRect* sourceRects; ///< 源矩形数组的指针。
    const RenderableTransform* transforms; ///< 变换数组的指针。
    SkColor4f color = SkColors::kWhite; ///< 实例的颜色。
    int filterQuality = 0; ///< 图像过滤质量。
    int wrapMode = 0; ///< 图像环绕模式。
    size_t count; ///< 批次中实例的数量。
};

/**
 * @brief 文本批次结构体。
 *
 * 包含渲染一个或多个文本字符串所需的所有信息。
 */
struct TextBatch
{
    sk_sp<SkTypeface> typeface; ///< 字体类型。
    float fontSize; ///< 字体大小。
    SkColor4f color = SkColors::kWhite; ///< 文本颜色。
    const std::string* texts; ///< 文本字符串数组的指针。
    int alignment = 0; ///< 文本对齐方式。
    const RenderableTransform* transforms; ///< 变换数组的指针。
    size_t count; ///< 批次中文本字符串的数量。
};


/**
 * @brief 矩形批次结构体。
 *
 * 包含渲染一个或多个矩形所需的所有信息。
 */
struct RectBatch
{
    SkSize size; ///< 矩形的大小。
    SkColor4f color; ///< 矩形的颜色。
    const RenderableTransform* transforms; ///< 变换数组的指针。
    size_t count; ///< 批次中矩形的数量。
};


/**
 * @brief 着色器批次结构体。
 *
 * 包含使用自定义着色器渲染一个对象所需的所有信息。
 */
struct ShaderBatch
{
    const Material* material = nullptr; ///< 材质指针。
    TransformComponent transform; ///< 对象的变换组件。
    SkSize size; ///< 对象的大小。
};


/**
 * @brief 圆形批次结构体。
 *
 * 包含渲染一个或多个圆形所需的所有信息。
 */
struct CircleBatch
{
    float radius; ///< 圆形的半径。
    SkColor4f color; ///< 圆形的颜色。
    const SkPoint* centers; ///< 圆心位置数组的指针。
    size_t count; ///< 批次中圆形的数量。
};


/**
 * @brief 线段批次结构体。
 *
 * 包含渲染一条或多条线段所需的所有信息。
 */
struct LineBatch
{
    float width; ///< 线段的宽度。
    SkColor4f color; ///< 线段的颜色。
    const SkPoint* points; ///< 线段顶点数组的指针。
    size_t pointCount; ///< 批次中线段顶点的数量。
};


/**
 * @brief 渲染包结构体。
 *
 * 包含一个渲染批次及其渲染顺序（Z-index）信息。
 */
struct RenderPacket
{
    int zIndex = 0; ///< 渲染顺序的Z-index值。

    std::variant<
        SpriteBatch,
        RectBatch,
        CircleBatch,
        LineBatch,
        InstanceBatch,
        ShaderBatch,
        TextBatch
    > batchData; ///< 包含具体批次数据的变体。
};
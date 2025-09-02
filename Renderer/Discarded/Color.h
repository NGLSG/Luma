#ifndef COLOR_H
#define COLOR_H
#include <cstdint>

#include "include/core/SkColor.h"


/**
 * @brief 表示一个颜色，包含红、绿、蓝和透明度分量。
 *
 * 颜色分量通常以浮点数表示，范围从 0.0f 到 1.0f。
 */
class Color
{
public:
    float r; ///< 红色分量，范围 [0.0f, 1.0f]。
    float g; ///< 绿色分量，范围 [0.0f, 1.0f]。
    float b; ///< 蓝色分量，范围 [0.0f, 1.0f]。
    float a; ///< 透明度分量，范围 [0.0f, 1.0f]。

    /**
     * @brief 构造一个颜色对象，使用浮点数分量初始化。
     * @param red 红色分量，范围 [0.0f, 1.0f]。
     * @param green 绿色分量，范围 [0.0f, 1.0f]。
     * @param blue 蓝色分量，范围 [0.0f, 1.0f]。
     * @param alpha 透明度分量，范围 [0.0f, 1.0f]。
     */
    Color(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha)
    {
    }

    /**
     * @brief 构造一个颜色对象，使用一个32位无符号整数（通常为ARGB格式）初始化。
     * @param color 32位颜色值，高8位为透明度，接着是红、绿、蓝分量。
     */
    Color(uint32_t color)
    {
        r = ((color >> 16) & 0xFF) / 255.0f;
        g = ((color >> 8) & 0xFF) / 255.0f;
        b = (color & 0xFF) / 255.0f;
        a = ((color >> 24) & 0xFF) / 255.0f;
    }

    /**
     * @brief 设置颜色对象的红、绿、蓝和透明度分量。
     * @param red 红色分量，范围 [0.0f, 1.0f]。
     * @param green 绿色分量，范围 [0.0f, 1.0f]。
     * @param blue 蓝色分量，范围 [0.0f, 1.0f]。
     * @param alpha 透明度分量，范围 [0.0f, 1.0f]。
     */
    void Set(float red, float green, float blue, float alpha = 1.0f)
    {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    /**
     * @brief 获取颜色对象的红、绿、蓝和透明度分量。
     * @param red 用于接收红色分量的引用。
     * @param green 用于接收绿色分量的引用。
     * @param blue 用于接收蓝色分量的引用。
     * @param alpha 用于接收透明度分量的引用。
     */
    void Get(float& red, float& green, float& blue, float& alpha) const
    {
        red = r;
        green = g;
        blue = b;
        alpha = a;
    }

    /**
     * @brief 将当前颜色对象隐式转换为Skia库的SkColor类型。
     * @return 对应的SkColor值。
     */
    operator SkColor() const
    {
        return SkColorSetARGB(static_cast<uint8_t>(a * 255),
                              static_cast<uint8_t>(r * 255),
                              static_cast<uint8_t>(g * 255),
                              static_cast<uint8_t>(b * 255));
    }

    /**
     * @brief 将当前颜色对象隐式转换为Skia库的SkColor4f类型。
     * @return 对应的SkColor4f值。
     */
    operator SkColor4f() const
    {
        return SkColor4f{r, g, b, a};
    }
};


#endif
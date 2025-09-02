#ifndef VECTOR2D_H
#define VECTOR2D_H

#include <include/core/SkPoint.h>
#include <include/core/SkColor.h>

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "include/core/SkRect.h"
#include "yaml-cpp/node/node.h"

namespace ECS
{
    /**
     * @brief 滤镜质量枚举
     */
    enum class FilterQuality
    {
        Nearest = 0, ///< 最近邻
        Bilinear = 1, ///< 双线性
        Mipmap = 2 ///< mipmap
    };


    /**
     * @brief 纹理包裹模式枚举
     */
    enum class WrapMode
    {
        Clamp = 0, ///< Clamp
        Repeat = 1, ///< Repeat
        Mirror = 2 ///< Mirror
    };


    /**
     * @brief 二维浮点向量结构体
     *
     * 继承自 glm::vec2，提供额外的转换和操作。
     */
    struct Vector2f : public glm::vec2
    {
        using glm::vec2::vec2;


        /**
         * @brief 默认构造函数，初始化为(0.0f, 0.0f)
         */
        Vector2f() : glm::vec2(0.0f)
        {
        }

        /**
         * @brief 使用单个浮点数初始化向量
         * @param uni 初始化值
         */
        Vector2f(float uni) : glm::vec2(uni)
        {
        }

        /**
         * @brief 使用两个浮点数初始化向量
         * @param x x 坐标
         * @param y y 坐标
         */
        Vector2f(float x, float y) : glm::vec2(x, y)
        {
        }


        /**
         * @brief 将 Vector2f 转换为 SkPoint
         * @return 转换后的 SkPoint
         */
        operator SkPoint() const { return {x, y}; }

        /**
         * @brief 向量取反
         * @return 取反后的向量
         */
        Vector2f operator-() const
        {
            return Vector2f(-x, -y);
        }

        /**
         * @brief 计算与另一个向量的点积
         * @param other 另一个向量
         * @return 点积结果
         */
        float Dot(const Vector2f& other) const
        {
            return x * other.x + y * other.y;
        }

        /**
         * @brief 将向量标准化
         * @return 标准化后的向量
         */
        Vector2f Normalize() const
        {
            float len = Length();
            if (len > 1e-6f)
                return Vector2f(x / len, y / len);
            else
                return Vector2f(0.0f, 0.0f);
        }

        /**
         * @brief 计算向量的长度
         * @return 向量的长度
         */
        float Length() const
        {
            return sqrtf(x * x + y * y);
        }

        /**
         * @brief 向量加法赋值运算符
         * @param other 要加的向量
         * @return 加法后的向量引用
         */
        Vector2f& operator+=(const Vector2f& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        /**
         * @brief 向量减法赋值运算符
         * @param other 要减的向量
         * @return 减法后的向量引用
         */
        Vector2f& operator-=(const Vector2f& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        /**
         * @brief 标量乘法赋值运算符
         * @param scalar 标量值
         * @return 乘法后的向量引用
         */
        Vector2f& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        /**
         * @brief 向量减法运算符
         * @param other 要减的向量
         * @return 减法后的向量
         */
        Vector2f operator-(const Vector2f& other) const
        {
            return Vector2f(x - other.x, y - other.y);
        }

        /**
         * @brief 向量加法运算符
         * @param other 要加的向量
         * @return 加法后的向量
         */
        Vector2f operator+(const Vector2f& other) const
        {
            return Vector2f(x + other.x, y + other.y);
        }

        /**
         * @brief 标量乘法运算符
         * @param scalar 标量值
         * @return 乘法后的向量
         */
        Vector2f operator*(float scalar) const
        {
            return Vector2f(x * scalar, y * scalar);
        }


        /**
         * @brief 从 SkPoint 创建 Vector2f
         * @param point SkPoint 对象
         * @return 新创建的 Vector2f 对象
         */
        static Vector2f FromSkPoint(const SkPoint& point)
        {
            return Vector2f(point.x(), point.y());
        }

        /**
         * @brief 获取零向量
         * @return 零向量 (0.0f, 0.0f)
         */
        inline static Vector2f Zero() { return Vector2f(0.0f, 0.0f); }
    };

    /**
     * @brief 二维整型向量结构体
     *
     * 继承自 glm::ivec2，提供额外的转换和操作。
     */
    struct Vector2i : public glm::ivec2
    {
        using glm::ivec2::ivec2;


        /**
         * @brief 默认构造函数，初始化为(0, 0)
         */
        Vector2i() : glm::ivec2(0)
        {
        }

        /**
         * @brief 将 Vector2i 转换为 SkPoint
         * @return 转换后的 SkPoint
         */
        SkPoint ToSkPoint() const
        {
            return SkPoint::Make(static_cast<float>(x), static_cast<float>(y));
        }

        /**
         * @brief 获取零向量
         * @return 零向量 (0, 0)
         */
        inline static Vector2i Zero() { return Vector2i(0, 0); }
    };


    /**
     * @brief 颜色结构体
     *
     * 继承自 glm::vec4，表示 RGBA 颜色值。
     */
    struct Color : public glm::vec4
    {
        using glm::vec4::vec4;


        /**
         * @brief 默认构造函数，初始化为白色(1.0f, 1.0f, 1.0f, 1.0f)
         */
        Color() : glm::vec4(1.0f)
        {
        }

        /**
         * @brief 使用RGBA值初始化颜色
         * @param r 红色分量
         * @param g 绿色分量
         * @param b 蓝色分量
         * @param a alpha分量，默认为1.0f
         */
        Color(float r, float g, float b, float a = 1.0f) : glm::vec4(r, g, b, a)
        {
        }


        /**
         * @brief 将 Color 转换为 SkColor4f
         * @return 转换后的 SkColor4f
         */
        operator SkColor4f() const { return {r, g, b, a}; }

        /**
         * @brief 将颜色转换为 uint32_t (ARGB格式)
         * @return uint32_t 表示的颜色值
         */
        uint32_t ToU32() const
        {
            auto r_ = static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
            auto g_ = static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
            auto b_ = static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
            auto a_ = static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
            return (a_ << 24) | (b_ << 16) | (g_ << 8) | r_;
        }
    };


    /**
     * @brief 矩形结构体
     *
     * 继承自 glm::vec4，表示矩形的 x, y, width, height。
     */
    struct RectF : public glm::vec4
    {
        using glm::vec4::vec4;


        /**
         * @brief 默认构造函数，初始化为(0.0f, 0.0f, 0.0f, 0.0f)
         */
        RectF() : glm::vec4(0.0f)
        {
        }

        /**
         * @brief 使用x, y, width, height 初始化矩形
         * @param x x坐标
         * @param y y坐标
         * @param w 宽度
         * @param h 高度
         */
        RectF(float x, float y, float w, float h) : glm::vec4(x, y, w, h)
        {
        }


        /**
         * @brief 将 RectF 转换为 SkRect
         * @return 转换后的 SkRect
         */
        operator SkRect() const { return SkRect::MakeXYWH(x, y, z, w); }
        /**
         * @brief 将 RectF 转换为 SkSize
         * @return 转换后的 SkSize
         */
        operator SkSize() const { return SkSize::Make(z, w); }


        /**
         * @brief 获取并修改矩形的x坐标
         * @return x坐标的引用
         */
        float& X() { return x; }
        /**
         * @brief 获取并修改矩形的y坐标
         * @return y坐标的引用
         */
        float& Y() { return y; }
        /**
         * @brief 获取并修改矩形的宽度
         * @return 宽度的引用
         */
        float& Width() { return z; }
        /**
         * @brief 获取并修改矩形的高度
         * @return 高度的引用
         */
        float& Height() { return w; }

        /**
         * @brief 获取矩形的x坐标
         * @return x坐标
         */
        float X() const { return x; }
        /**
         * @brief 获取矩形的y坐标
         * @return y坐标
         */
        float Y() const { return y; }
        /**
         * @brief 获取矩形的宽度
         * @return 宽度
         */
        float Width() const { return z; }
        /**
         * @brief 获取矩形的高度
         * @return 高度
         */
        float Height() const { return w; }
    };


    /**
     * @brief 预定义颜色集合
     */
    namespace Colors
    {
        inline const Color White(1.0f, 1.0f, 1.0f, 1.0f); ///< 白色
        inline const Color Black(0.0f, 0.0f, 0.0f, 1.0f); ///< 黑色
        inline const Color Red(1.0f, 0.0f, 0.0f, 1.0f); ///< 红色
        inline const Color Green(0.0f, 1.0f, 0.0f, 1.0f); ///< 绿色
        inline const Color Blue(0.0f, 0.0f, 1.0f, 1.0f); ///< 蓝色
        inline const Color Yellow(1.0f, 1.0f, 0.0f, 1.0f); ///< 黄色
        inline const Color Cyan(0.0f, 1.0f, 1.0f, 1.0f); ///< 青色
        inline const Color Magenta(1.0f, 0.0f, 1.0f, 1.0f); ///< 品红色
        inline const Color Orange(1.0f, 0.5f, 0.0f, 1.0f); ///< 橙色
        inline const Color Purple(0.5f, 0.0f, 0.5f, 1.0f); ///< 紫色
        inline const Color Pink(1.0f, 0.75f, 0.8f, 1.0f); ///< 粉色
        inline const Color Gray(0.5f, 0.5f, 0.5f, 1.0f); ///< 灰色
        inline const Color LightGray(0.75f, 0.75f, 0.75f, 1.0f); ///< 浅灰色
        inline const Color DarkGray(0.25f, 0.25f, 0.25f, 1.0f); ///< 深灰色
        inline const Color Brown(0.6f, 0.4f, 0.2f, 1.0f); ///< 棕色
        inline const Color Transparent(0.0f, 0.0f, 0.0f, 0.0f); ///< 透明色
        inline const Color CornflowerBlue(0.39f, 0.58f, 0.93f, 1.0f); ///< 矢车菊蓝
        inline const Color Lime(0.0f, 1.0f, 0.5f, 1.0f); ///< 酸橙色
        inline const Color Gold(1.0f, 0.84f, 0.0f, 1.0f); ///< 金色
        inline const Color Silver(0.75f, 0.75f, 0.75f, 1.0f); ///< 银色
        inline const Color SkyBlue(0.53f, 0.81f, 0.92f, 1.0f); ///< 天蓝色
        inline const Color Olive(0.5f, 0.5f, 0.0f, 1.0f); ///< 橄榄色
        inline const Color Teal(0.0f, 0.5f, 0.5f, 1.0f); ///< 青绿色
        inline const Color Maroon(0.5f, 0.0f, 0.0f, 1.0f); ///< 栗色
        inline const Color Navy(0.0f, 0.0f, 0.5f, 1.0f); ///< 海军蓝
    }
}

namespace std
{
    /**
     * @brief Color 的哈希函数特化
     *
     * 允许 ECS::Color 类型在标准库容器（如 std::unordered_map）中使用。
     */
    template <>
    struct hash<ECS::Color>
    {
        /**
         * @brief 计算 Color 的哈希值
         * @param color 颜色值
         * @return 颜色值的哈希值
         */
        size_t operator()(const ECS::Color& color) const
        {
            return hash<uint32_t>{}(color.ToU32());
        }
    };
}

namespace YAML
{
    /**
     * @brief Vector2f 的 YAML 转换器特化
     *
     * 允许 ECS::Vector2f 类型在 YAML 序列化和反序列化中使用。
     */
    template <>
    struct convert<ECS::Vector2f>
    {
        /**
         * @brief 将 Vector2f 编码为 YAML 节点
         * @param vec Vector2f 向量
         * @return YAML 节点
         */
        static Node encode(const ECS::Vector2f& vec)
        {
            Node node;
            node["x"] = vec.x;
            node["y"] = vec.y;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Vector2f
         * @param node YAML 节点
         * @param vec Vector2f 向量
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::Vector2f& vec)
        {
            if (!node.IsMap() || !node["x"] || !node["y"])
                return false;

            vec.x = node["x"].as<float>();
            vec.y = node["y"].as<float>();
            return true;
        }
    };

    /**
     * @brief Vector2i 的 YAML 转换器特化
     *
     * 允许 ECS::Vector2i 类型在 YAML 序列化和反序列化中使用。
     */
    template <>
    struct convert<ECS::Vector2i>
    {
        /**
         * @brief 将 Vector2i 编码为 YAML 节点
         * @param vec Vector2i 向量
         * @return YAML 节点
         */
        static Node encode(const ECS::Vector2i& vec)
        {
            Node node;
            node["x"] = vec.x;
            node["y"] = vec.y;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Vector2i
         * @param node YAML 节点
         * @param vec Vector2i 向量
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::Vector2i& vec)
        {
            if (!node.IsMap() || !node["x"] || !node["y"])
                return false;

            vec.x = node["x"].as<int>();
            vec.y = node["y"].as<int>();
            return true;
        }
    };


    /**
     * @brief Color 的 YAML 转换器特化
     *
     * 允许 ECS::Color 类型在 YAML 序列化和反序列化中使用。
     */
    template <>
    struct convert<ECS::Color>
    {
        /**
         * @brief 将 Color 编码为 YAML 节点
         * @param color 颜色值
         * @return YAML 节点
         */
        static Node encode(const ECS::Color& color)
        {
            Node node;
            node["r"] = color.r;
            node["g"] = color.g;
            node["b"] = color.b;
            node["a"] = color.a;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Color
         * @param node YAML 节点
         * @param color 颜色值
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::Color& color)
        {
            if (!node.IsMap() || !node["r"] || !node["g"] || !node["b"] || !node["a"])
                return false;

            color.r = node["r"].as<float>();
            color.g = node["g"].as<float>();
            color.b = node["b"].as<float>();
            color.a = node["a"].as<float>();
            return true;
        }
    };

    /**
     * @brief RectF 的 YAML 转换器特化
     *
     * 允许 ECS::RectF 类型在 YAML 序列化和反序列化中使用。
     */
    template <>
    struct convert<ECS::RectF>
    {
        /**
         * @brief 将 RectF 编码为 YAML 节点
         * @param rect 矩形
         * @return YAML 节点
         */
        static Node encode(const ECS::RectF& rect)
        {
            Node node;
            node["x"] = rect.x;
            node["y"] = rect.y;
            node["width"] = rect.z;
            node["height"] = rect.w;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 RectF
         * @param node YAML 节点
         * @param rect 矩形
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::RectF& rect)
        {
            if (!node.IsMap() || !node["x"] || !node["y"] || !node["width"] || !node["height"])
                return false;

            rect.x = node["x"].as<float>();
            rect.y = node["y"].as<float>();
            rect.z = node["width"].as<float>();
            rect.w = node["height"].as<float>();
            return true;
        }
    };
}

namespace std
{
    /**
     * @brief Vector2i 的哈希函数特化
     *
     * 允许 ECS::Vector2i 类型在标准库容器（如 std::unordered_map）中使用。
     */
    template <>
    struct hash<ECS::Vector2i>
    {
        /**
         * @brief 计算 Vector2i 的哈希值
         * @param v Vector2i 向量
         * @return 向量的哈希值
         */
        size_t operator()(const ECS::Vector2i& v) const
        {
            return hash<int>()(v.x) ^ (hash<int>()(v.y) << 1);
        }
    };
}
#endif
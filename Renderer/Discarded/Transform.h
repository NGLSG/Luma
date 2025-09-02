#pragma once
#include <cmath>
#include <concepts>
#include <numbers>
#include "include/core/SkMatrix.h"
#include "include/core/SkRSXform.h"

/**
 * @brief 表示一个二维向量，支持整数或浮点类型。
 * @tparam T 向量分量的类型，必须是整数或浮点类型。
 */
template <typename T>
    requires std::integral<T> || std::floating_point<T>
struct Vector2D
{
    T x; ///< 向量的X分量。
    T y; ///< 向量的Y分量。

    /**
     * @brief 计算向量的长度（模）。
     * @return 向量的浮点长度。
     */
    float length() const
    {
        return std::sqrt(static_cast<float>(x * x + y * y));
    }

    /**
     * @brief 实现向量加法。
     * @param other 另一个要相加的二维向量。
     * @return 两个向量相加后的新向量。
     */
    Vector2D operator+(const Vector2D& other) const
    {
        return {x + other.x, y + other.y};
    }

    /**
     * @brief 实现向量减法。
     * @param other 另一个要相减的二维向量。
     * @return 两个向量相减后的新向量。
     */
    Vector2D operator-(const Vector2D& other) const
    {
        return {x - other.x, y - other.y};
    }

    /**
     * @brief 实现向量与标量的乘法。
     * @param scalar 乘数标量。
     * @return 向量与标量相乘后的新向量。
     */
    Vector2D operator*(float scalar) const
    {
        return {static_cast<T>(x * scalar), static_cast<T>(y * scalar)};
    }

    /**
     * @brief 实现向量与标量的除法。
     * @param scalar 除数标量。
     * @return 向量与标量相除后的新向量。
     */
    Vector2D operator/(float scalar) const
    {
        return {static_cast<T>(x / scalar), static_cast<T>(y / scalar)};
    }

    /**
     * @brief 将向量归一化，使其长度为1。
     * @return 归一化后的新向量。如果原向量长度为0，则返回零向量。
     */
    Vector2D normalize() const
    {
        float len = length();
        if (len == 0) return {0, 0};
        return *this / len;
    }

    /**
     * @brief 计算当前向量与另一个向量的点积。
     * @param other 另一个二维向量。
     * @return 两个向量的点积。
     */
    float dot(const Vector2D& other) const
    {
        return static_cast<float>(x * other.x + y * other.y);
    }

    /**
     * @brief 计算当前向量与另一个向量的二维叉积（Z分量）。
     * @param other 另一个二维向量。
     * @return 两个向量的二维叉积。
     */
    float cross(const Vector2D& other) const
    {
        return static_cast<float>(x * other.y - y * other.x);
    }

    /**
     * @brief 计算当前向量与另一个向量之间的夹角（弧度）。
     * @param other 另一个二维向量。
     * @return 两个向量之间的夹角（弧度）。如果任一向量长度为0，则返回0。
     */
    float angle(const Vector2D& other) const
    {
        float dotProduct = dot(other);
        float lengthsProduct = length() * other.length();
        if (lengthsProduct == 0) return 0;
        return std::acos(dotProduct / lengthsProduct);
    }

    /**
     * @brief 将向量绕原点旋转指定角度。
     * @param degrees 旋转角度（度）。
     * @return 旋转后的新向量。
     */
    Vector2D rotate(float degrees) const
    {
        float rad = degrees * (std::numbers::pi_v<float> / 180.0f);
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        return {
            static_cast<T>(x * cosA - y * sinA),
            static_cast<T>(x * sinA + y * cosA)
        };
    }

    /**
     * @brief 比较两个向量是否相等。
     * @param other 另一个二维向量。
     * @return 如果两个向量的X和Y分量都相等，则返回true；否则返回false。
     */
    bool operator==(const Vector2D& other) const = default;

    /**
     * @brief 计算向量的负向量。
     * @return 当前向量的负向量。
     */
    Vector2D operator-() const
    {
        return {-x, -y};
    }

    /**
     * @brief 实现向量的复合加法赋值操作。
     * @param other 另一个要相加的二维向量。
     * @return 对当前向量的引用。
     */
    Vector2D& operator+=(const Vector2D& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /**
     * @brief 实现向量的复合减法赋值操作。
     * @param other 另一个要相减的二维向量。
     * @return 对当前向量的引用。
     */
    Vector2D& operator-=(const Vector2D& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /**
     * @brief 实现向量的复合标量乘法赋值操作。
     * @param scalar 乘数标量。
     * @return 对当前向量的引用。
     */
    Vector2D& operator*=(float scalar)
    {
        x = static_cast<T>(x * scalar);
        y = static_cast<T>(y * scalar);
        return *this;
    }

    /**
     * @brief 实现向量的复合标量除法赋值操作。
     * @param scalar 除数标量。
     * @return 对当前向量的引用。
     */
    Vector2D& operator/=(float scalar)
    {
        if (scalar != 0)
        {
            x = static_cast<T>(x / scalar);
            y = static_cast<T>(y / scalar);
        }
        return *this;
    }
};

/// 浮点类型的二维向量别名。
using Vector2Df = Vector2D<float>;
/// 整数类型的二维向量别名。
using Vector2Di = Vector2D<int>;


/**
 * @brief 表示一个二维变换，包含位置、缩放、旋转和枢轴点。
 */
class Transform
{
private:
    Vector2Df position; ///< 变换的位置向量。
    Vector2Df scale;    ///< 变换的缩放向量。
    float rotation;     ///< 变换的旋转角度（度）。
    Vector2Df pivot;    ///< 变换的枢轴点。
    SkMatrix data;      ///< 内部维护的Skia矩阵数据。

    /**
     * @brief 更新内部的Skia矩阵数据。
     * @note 这是一个私有方法，用于在变换属性改变时更新矩阵。
     */
    void updateMatrix();

public:
    /**
     * @brief 默认构造函数，初始化一个单位变换。
     */
    Transform();

    /**
     * @brief 构造函数，使用指定的位置、缩放、旋转和枢轴点初始化变换。
     * @param pos 变换的位置向量。
     * @param scl 变换的缩放向量。
     * @param rot 变换的旋转角度（度）。
     * @param pvt 变换的枢轴点。
     */
    Transform(const Vector2Df& pos, const Vector2Df& scl, float rot, const Vector2Df& pvt);

    /**
     * @brief 拷贝构造函数。
     * @param other 要拷贝的Transform对象。
     */
    Transform(const Transform&);
    /**
     * @brief 拷贝赋值运算符。
     * @param other 要拷贝的Transform对象。
     * @return 对当前Transform对象的引用。
     */
    Transform& operator=(const Transform&);
    /**
     * @brief 移动构造函数。
     * @param other 要移动的Transform对象。
     */
    Transform(Transform&&) noexcept;
    /**
     * @brief 移动赋值运算符。
     * @param other 要移动的Transform对象。
     * @return 对当前Transform对象的引用。
     */
    Transform& operator=(Transform&&) noexcept;

    /**
     * @brief 设置变换的位置。
     * @param pos 新的位置向量。
     */
    void SetPosition(const Vector2Df& pos);

    /**
     * @brief 设置变换的缩放。
     * @param scl 新的缩放向量。
     */
    void SetScale(const Vector2Df& scl);

    /**
     * @brief 设置变换的旋转角度。
     * @param rot 新的旋转角度（度）。
     */
    void SetRotation(float rot);

    /**
     * @brief 设置变换的枢轴点。
     * @param pvt 新的枢轴点向量。
     */
    void SetPivot(const Vector2Df& pvt);

    /**
     * @brief 获取变换的当前位置。
     * @return 当前位置向量。
     */
    Vector2Df GetPosition() const;
    /**
     * @brief 获取变换的当前缩放。
     * @return 当前缩放向量。
     */
    Vector2Df GetScale() const;
    /**
     * @brief 获取变换的当前旋转角度。
     * @return 当前旋转角度（度）。
     */
    float GetRotation() const;
    /**
     * @brief 获取变换的当前枢轴点。
     * @return 当前枢轴点向量。
     */
    Vector2Df GetPivot() const;

    /**
     * @brief 将Transform对象隐式转换为SkMatrix。
     * @return 对应的Skia SkMatrix对象。
     */
    operator SkMatrix() const;

    /**
     * @brief 将Transform对象隐式转换为SkRSXform。
     * @return 对应的Skia SkRSXform对象。
     */
    operator SkRSXform() const;
};
#ifndef DRAWCOMMAND_H
#define DRAWCOMMAND_H
#include <string>

#include "Color.h"
#include "Transform.h"
#include "../Utils/Guid.h"

/** @brief 精灵类的向前声明。 */
class Sprite;

/**
 * @brief 绘制模式枚举。
 * 用于指定图形的绘制方式，例如填充、描边或两者兼有。
 */
enum class DrawMode
{
    Fill, ///< 填充模式。
    Stroke, ///< 描边模式。
    StrokeAndFill ///< 描边并填充模式。
};

/**
 * @brief 绘制类型枚举。
 * 用于指定要绘制的图形的几何类型。
 */
enum class DrawType
{
    Point, ///< 点。
    Line, ///< 线。
    Triangle, ///< 三角形。
    Quad, ///< 四边形。
    Polygon, ///< 多边形。
    Sprite, ///< 精灵。
};

/**
 * @brief 绘制命令结构体。
 * 用于封装一个通用的绘制操作所需的数据，如类型、模式、变换、颜色和关联的游戏对象UID。
 */
struct DrawCommand
{
public:
    DrawType type; ///< 绘制类型。
    DrawMode mode; ///< 绘制模式。
    Transform transform; ///< 变换信息。
    Color color; ///< 颜色。
    Guid GameObjectUID; ///< 关联的游戏对象唯一标识符。
};

/**
 * @brief 精灵绘制命令结构体。
 * 继承自 DrawCommand，并添加了精灵特有的数据，即指向要绘制的精灵对象。
 */
struct SpriteDrawCommand : public DrawCommand
{
    Sprite* sprite; ///< 指向要绘制的精灵对象。
};

#endif
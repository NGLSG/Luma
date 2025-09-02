/**
 * @file Renderer.h
 * @brief 渲染器相关的定义。
 */
#ifndef RENDERER_H
#define RENDERER_H
#include <mutex>
#include <queue>

#include "DrawCommand.h"
#include "../GraphicsBackend.h"
#include "../Utils/LazySingleton.h"

/**
 * @brief 渲染器基类（或占位符）。
 * 这是一个空的类定义，可能作为未来渲染器体系的起点。
 */
class Renderer
{
private:
};


/**
 * @brief 绘制命令渲染器接口。
 * 定义了处理和渲染绘制命令的抽象接口。
 */
class DrawCommandRenderer
{
public:
    /**
     * @brief 虚析构函数。
     * 确保派生类对象能正确销毁。
     */
    virtual ~DrawCommandRenderer() = default;

    /**
     * @brief 添加绘制命令到渲染器。
     * @param commands 要添加的绘制命令列表。
     */
    virtual void AddCommands(const std::vector<DrawCommand*>& commands) = 0;

    /**
     * @brief 准备渲染器进行渲染。
     * 执行渲染前的必要设置或数据处理。
     */
    virtual void Prepare() = 0;

    /**
     * @brief 执行渲染操作。
     * 将准备好的命令渲染到目标。
     */
    virtual void Render() = 0;

    /**
     * @brief 清理渲染器资源。
     * 释放渲染周期结束后不再需要的资源。
     */
    virtual void Cleanup() = 0;

    /**
     * @brief 执行命中测试。
     * 根据给定的世界坐标，判断是否有可点击的元素被命中。
     * @param worldPos 进行命中测试的世界坐标。
     * @return 被命中的元素的全局唯一标识符（Guid），如果没有命中则返回无效Guid。
     */
    virtual Guid HitTest(const Vector2Df& worldPos) = 0;
};


/**
 * @brief 精灵批处理渲染器。
 * 这是一个单例模式的渲染器，用于高效地批处理和渲染精灵绘制命令。
 */
class SpriteBatchRenderer final : public LazySingleton<SpriteBatchRenderer>, public DrawCommandRenderer
{
    friend class LazySingleton<SpriteBatchRenderer>;

private:
    GraphicsBackend* m_backend = nullptr; ///< 图形后端接口指针。

    std::vector<SpriteDrawCommand> m_incomingCommands; ///< 待处理的传入绘制命令。

    std::vector<SpriteDrawCommand> m_renderQueue; ///< 准备好进行渲染的命令队列。

    mutable std::mutex m_mutex; ///< 用于保护共享数据的互斥锁。

    std::vector<SkPoint> m_positions; ///< 顶点位置数据。
    std::vector<SkPoint> m_texCoords; ///< 纹理坐标数据。
    std::vector<SkColor> m_colors; ///< 顶点颜色数据。
    std::vector<uint16_t> m_indices; ///< 顶点索引数据。

    /**
     * @brief 默认构造函数。
     * 私有构造函数，遵循单例模式。
     */
    SpriteBatchRenderer() = default;
    /**
     * @brief 默认析构函数。
     * 私有析构函数，遵循单例模式。
     */
    ~SpriteBatchRenderer() override = default;

    /**
     * @brief 计算精灵的世界坐标顶点。
     * @param cmd 精灵绘制命令。
     * @param worldVerts 用于存储计算出的四个世界坐标顶点的数组。
     */
    void CalculateWorldVertices(const SpriteDrawCommand& cmd, SkPoint worldVerts[4]) const;

public:
    /**
     * @brief 初始化渲染器。
     * @param backend 图形后端接口。
     */
    void Initialize(GraphicsBackend* backend);

    /**
     * @brief 添加绘制命令到渲染器。
     * @param commands 要添加的绘制命令列表。
     */
    void AddCommands(const std::vector<DrawCommand*>& commands) override;
    /**
     * @brief 准备渲染器进行渲染。
     * 执行渲染前的必要设置或数据处理。
     */
    void Prepare() override;
    /**
     * @brief 执行渲染操作。
     * 将准备好的命令渲染到目标。
     */
    void Render() override;
    /**
     * @brief 清理渲染器资源。
     * 释放渲染周期结束后不再需要的资源。
     */
    void Cleanup() override;
    /**
     * @brief 执行命中测试。
     * 根据给定的世界坐标，判断是否有可点击的元素被命中。
     * @param worldPos 进行命中测试的世界坐标。
     * @return 被命中的元素的全局唯一标识符（Guid），如果没有命中则返回无效Guid。
     */
    Guid HitTest(const Vector2Df& worldPos) override;
};
#endif
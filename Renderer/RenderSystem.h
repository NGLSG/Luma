#pragma once

#include "RenderComponent.h"
#include <include/core/SkColor.h>
#include <vector>
#include <memory>


class GraphicsBackend;


/**
 * @brief 每批次内部处理的最大图元数量。
 */
constexpr int MAX_PRIMITIVES_PER_INTERNAL_BATCH = 16384;


/**
 * @brief 渲染系统，负责管理和提交所有渲染操作。
 *
 * 这是一个高层级的渲染接口，用于将各种渲染批次和渲染包提交给图形后端进行绘制。
 */
class LUMA_API RenderSystem
{
public:
    /**
     * @brief 销毁渲染系统实例。
     */
    ~RenderSystem();

    // 禁用拷贝构造函数和赋值运算符，确保RenderSystem实例的唯一性。
    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;


    /**
     * @brief 创建一个RenderSystem实例。
     * @param backend 图形后端引用，用于实际的渲染操作。
     * @param maxPrimitivesPerBatch 每个批次处理的最大图元数量。
     * @return 指向新创建的RenderSystem实例的唯一指针。
     */
    static std::unique_ptr<RenderSystem> Create(
        GraphicsBackend& backend,
        size_t maxPrimitivesPerBatch = MAX_PRIMITIVES_PER_INTERNAL_BATCH);


    /**
     * @brief 提交一个精灵批次进行渲染。
     * @param batch 要提交的精灵批次。
     */
    void Submit(const SpriteBatch& batch);

    /**
     * @brief 提交一个文本批次进行渲染。
     * @param batch 要提交的文本批次。
     */
    void Submit(const TextBatch& batch);


    /**
     * @brief 提交一个矩形批次进行渲染。
     * @param batch 要提交的矩形批次。
     */
    void Submit(const RectBatch& batch);


    /**
     * @brief 提交一个圆形批次进行渲染。
     * @param batch 要提交的圆形批次。
     */
    void Submit(const CircleBatch& batch);


    /**
     * @brief 提交一个线条批次进行渲染。
     * @param batch 要提交的线条批次。
     */
    void Submit(const LineBatch& batch);


    /**
     * @brief 提交一个实例批次进行渲染。
     * @param batch 要提交的实例批次。
     */
    void Submit(const InstanceBatch& batch);


    /**
     * @brief 提交一个着色器批次进行渲染。
     * @param batch 要提交的着色器批次。
     */
    void Submit(const ShaderBatch& batch);

    /**
     * @brief 提交一个原始绘制批次进行渲染。
     * @param batch 要提交的原始绘制批次。
     */
    void Submit(const RawDrawBatch& batch);


    /**
     * @brief 提交一个渲染包进行渲染。
     * @param packet 要提交的渲染包。
     */
    void Submit(const RenderPacket& packet);


    /**
     * @brief 使用指定的颜色清除渲染目标。
     * @param color 清除颜色，默认为透明。
     */
    void Clear(const SkColor4f& color = SkColors::kTransparent);


    /**
     * @brief 使用指定的颜色清除渲染目标。
     * @param color 清除颜色，默认为透明。
     */
    void Clear(const SkColor& color = SkColors::kTransparent.toSkColor());


    /**
     * @brief 强制立即执行所有待处理的渲染命令。
     */
    void Flush();


    /**
     * @brief 设置渲染裁剪矩形。
     * @param rect 裁剪矩形区域。
     */
    void SetClipRect(const SkRect& rect);


    /**
     * @brief 清除当前设置的裁剪矩形。
     */
    void ClearClipRect();

    /**
     * @brief 绘制一个自定义光标。
     * @param position 光标位置。
     * @param size 光标大小。
     * @param color 光标颜色。
     */
    void DrawCursor(const SkPoint& position, float size, const SkColor4f& color);

private:
    // 私有构造函数，通过Create静态方法创建实例。
    RenderSystem(GraphicsBackend& backend, size_t maxPrimitivesPerBatch);


    class RenderSystemImpl;
    std::unique_ptr<RenderSystemImpl> pImpl; ///< 渲染系统的私有实现指针。
};

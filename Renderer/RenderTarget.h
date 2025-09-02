#ifndef RENDERTARGET_H
#define RENDERTARGET_H
#include "dawn/native/DawnNative.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImage.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/Recorder.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/dawn/DawnBackendContext.h"
#include "include/gpu/graphite/dawn/DawnGraphiteTypes.h"
#include "include/gpu/graphite/dawn/DawnUtils.h"

/**
 * @brief 渲染目标类，封装了一个WGPU纹理及其尺寸信息。
 *
 * 此类表示一个可用于渲染操作的目标，通常是一个纹理。
 * 它提供了访问底层WGPU纹理和其尺寸的方法。
 */
class RenderTarget
{
public:
    /**
     * @brief 禁用拷贝构造函数。
     *
     * RenderTarget对象不可拷贝。
     */
    RenderTarget(const RenderTarget&) = delete;
    /**
     * @brief 禁用拷贝赋值运算符。
     *
     * RenderTarget对象不可赋值。
     */
    RenderTarget& operator=(const RenderTarget&) = delete;

    /**
     * @brief 获取内部封装的WGPU纹理对象。
     *
     * @return WGPU纹理对象。
     */
    wgpu::Texture GetTexture() const;
    /**
     * @brief 获取渲染目标的宽度。
     *
     * @return 渲染目标的宽度。
     */
    uint16_t GetWidth() const;
    /**
     * @brief 获取渲染目标的高度。
     *
     * @return 渲染目标的高度。
     */
    uint16_t GetHeight() const;
    /**
     * @brief 构造函数，使用给定的WGPU纹理、宽度和高度初始化渲染目标。
     *
     * @param texture WGPU纹理对象。
     * @param width 渲染目标的宽度。
     * @param height 渲染目标的高度。
     */
    RenderTarget(wgpu::Texture texture, uint16_t width, uint16_t height);

private:
    wgpu::Texture m_texture; ///< 内部封装的WGPU纹理对象。
    uint16_t m_width;       ///< 渲染目标的宽度。
    uint16_t m_height;      ///< 渲染目标的高度。
};

#endif
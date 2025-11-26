#ifndef CAMERA_H
#define CAMERA_H

#include "../Utils/LazySingleton.h"
#include <shared_mutex>
#include <include/core/SkPoint.h>
#include <include/core/SkRect.h>
#include <include/core/SkColor.h>
#include "include/core/SkM44.h"
#include "Nut/ShaderStruct.h"

class SkCanvas;

/**
 * @brief 摄像机类，用于管理场景的视图和投影。
 *
 * 这是一个单例类，通过LazySingleton模式确保全局只有一个摄像机实例。
 * 它负责将世界坐标转换为屏幕坐标，反之亦然，并应用视图变换到SkCanvas。
 */
class LUMA_API Camera : public LazySingleton<Camera>
{
public:
    friend class LazySingleton<Camera>;

    /**
     * @brief 摄像机属性结构体。
     *
     * 包含摄像机的位置、视口、旋转、缩放和背景清除颜色等属性。
     */
    struct CamProperties
    {
        SkPoint position{0.0f, 0.0f}; ///< 摄像机在世界坐标系中的位置。
        SkRect viewport{0, 0, 1, 1}; ///< 摄像机的视口矩形，通常表示屏幕或渲染区域。
        float rotation = 0.0f; ///< 摄像机的旋转角度（以弧度为单位）。
        SkPoint zoom{1.0f, 1.0f}; ///< 摄像机的缩放级别 (x, y)。
        SkPoint zoomFactor{1.0f, 1.0f}; ///< 布局系统设置的缩放因子，最终缩放 = zoom * zoomFactor。
        SkColor4f clearColor = SkColor4f{0.15f, 0.16f, 0.18f, 1.0f}; ///< 渲染前用于清除背景的颜色。
        
        /// 获取最终缩放值 (zoom * zoomFactor)
        SkPoint GetEffectiveZoom() const { return {zoom.x() * zoomFactor.x(), zoom.y() * zoomFactor.y()}; }
    } m_properties; ///< 当前摄像机的属性实例。
    mutable std::shared_mutex m_mutex; ///< 保护摄像机属性的读写锁。

    /**
     * @brief 将摄像机的变换应用到给定的SkCanvas。
     *
     * 此方法会根据摄像机的当前位置、旋转和缩放来变换画布，
     * 以便后续的绘制操作在摄像机视图下进行。
     * @param canvas 要应用变换的SkCanvas指针。
     */
    void ApplyTo(SkCanvas* canvas) const;

    /**
     * @brief 将屏幕坐标点转换为世界坐标点。
     *
     * @param screenPoint 要转换的屏幕坐标点。
     * @return 转换后的世界坐标点。
     */
    SkPoint ScreenToWorld(const SkPoint& screenPoint) const;

    /**
     * @brief 将世界坐标点转换为屏幕坐标点。
     *
     * @param worldPoint 要转换的世界坐标点。
     * @return 转换后的屏幕坐标点。
     */
    SkPoint WorldToScreen(const SkPoint& worldPoint) const;

    /**
     * @brief 设置摄像机的属性。
     *
     * @param cam_properties 要设置的新的摄像机属性。
     */
    void SetProperties(const CamProperties& cam_properties);

    /**
     * @brief 构建并返回摄像机的视图矩阵。
     *
     * 视图矩阵用于将世界坐标转换为摄像机坐标。
     * @return 构建好的SkM44视图矩阵。
     */
    SkM44 BuildViewMatrix();

    /**
     * @brief 获取当前摄像机的属性。
     *
     * @return 当前摄像机的CamProperties结构体。
     */
    CamProperties GetProperties() const;
    SkM44 GetViewProjectionMatrix();

    void FillEngineData(EngineData& data) const;
};

#endif

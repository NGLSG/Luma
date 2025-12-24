#ifndef UI_CAMERA_H
#define UI_CAMERA_H

#include "../Utils/LazySingleton.h"
#include <shared_mutex>
#include <include/core/SkPoint.h>
#include <include/core/SkRect.h>
#include "include/core/SkM44.h"

class SkCanvas;

/**
 * @brief UI摄像机类，专门用于UI元素的渲染。
 *
 * 这是一个单例类，通过LazySingleton模式确保全局只有一个UI摄像机实例。
 * UI摄像机使用屏幕空间坐标系，不受世界摄像机变换的影响。
 * 这确保了UI元素始终固定在屏幕上，不会随世界摄像机移动、缩放或旋转。
 */
class LUMA_API UICamera : public LazySingleton<UICamera>
{
public:
    friend class LazySingleton<UICamera>;

    /**
     * @brief UI摄像机属性结构体。
     *
     * 包含UI摄像机的视口和缩放属性。
     * UI摄像机不支持位置偏移和旋转，因为UI应该始终固定在屏幕上。
     */
    struct UICamProperties
    {
        SkRect viewport{0, 0, 1, 1}; ///< UI摄像机的视口矩形，通常与屏幕大小一致。
        SkPoint scale{1.0f, 1.0f};  ///< UI缩放因子，用于支持不同DPI的屏幕。
    } m_properties; ///< 当前UI摄像机的属性实例。
    
    mutable std::shared_mutex m_mutex; ///< 保护UI摄像机属性的读写锁。

    /**
     * @brief 将UI摄像机的变换应用到给定的SkCanvas。
     *
     * UI摄像机使用简单的正交投影，原点在左上角。
     * @param canvas 要应用变换的SkCanvas指针。
     */
    void ApplyTo(SkCanvas* canvas) const;

    /**
     * @brief 将屏幕坐标点转换为UI坐标点。
     *
     * 对于UI摄像机，屏幕坐标和UI坐标通常是相同的（考虑缩放因子）。
     * @param screenPoint 要转换的屏幕坐标点。
     * @return 转换后的UI坐标点。
     */
    SkPoint ScreenToUI(const SkPoint& screenPoint) const;

    /**
     * @brief 将UI坐标点转换为屏幕坐标点。
     *
     * @param uiPoint 要转换的UI坐标点。
     * @return 转换后的屏幕坐标点。
     */
    SkPoint UIToScreen(const SkPoint& uiPoint) const;

    /**
     * @brief 设置UI摄像机的属性。
     *
     * @param properties 要设置的新的UI摄像机属性。
     */
    void SetProperties(const UICamProperties& properties);

    /**
     * @brief 获取当前UI摄像机的属性。
     *
     * @return 当前UI摄像机的UICamProperties结构体。
     */
    UICamProperties GetProperties() const;

    /**
     * @brief 设置UI摄像机的视口。
     *
     * 通常在窗口大小改变时调用。
     * @param viewport 新的视口矩形。
     */
    void SetViewport(const SkRect& viewport);

    /**
     * @brief 设置UI缩放因子。
     *
     * 用于支持高DPI显示器或用户自定义UI缩放。
     * @param scale 缩放因子。
     */
    void SetScale(const SkPoint& scale);

    /**
     * @brief 构建并返回UI摄像机的视图矩阵。
     *
     * @return 构建好的SkM44视图矩阵。
     */
    SkM44 BuildViewMatrix() const;
};

#endif // UI_CAMERA_H

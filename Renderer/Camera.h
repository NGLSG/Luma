#ifndef CAMERA_H
#define CAMERA_H

#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <string>
#include <include/core/SkPoint.h>
#include <include/core/SkRect.h>
#include <include/core/SkColor.h>
#include "include/core/SkM44.h"
#include "Nut/ShaderStruct.h"

class SkCanvas;

/**
 * @brief 摄像机属性结构体。
 *
 * 包含摄像机的位置、视口、旋转、缩放和背景清除颜色等属性。
 */
struct LUMA_API CameraProperties
{
    SkPoint position{0.0f, 0.0f};                                    ///< 摄像机在世界坐标系中的位置。
    SkRect viewport{0, 0, 1, 1};                                     ///< 摄像机的视口矩形。
    float rotation = 0.0f;                                           ///< 摄像机的旋转角度（弧度）。
    SkPoint zoom{1.0f, 1.0f};                                        ///< 摄像机的缩放级别。
    SkPoint zoomFactor{1.0f, 1.0f};                                  ///< 布局系统设置的缩放因子。
    SkColor4f clearColor = SkColor4f{0.15f, 0.16f, 0.18f, 1.0f};     ///< 背景清除颜色。

    /// 获取最终缩放值 (zoom * zoomFactor)
    SkPoint GetEffectiveZoom() const { return {zoom.x() * zoomFactor.x(), zoom.y() * zoomFactor.y()}; }
};

/**
 * @brief 摄像机类，用于管理场景的视图和投影。
 *
 * 支持多个摄像机实例，可以通过ID创建、获取和切换摄像机。
 */
class LUMA_API Camera
{
public:
    // 为了兼容性保留的类型别名
    using CamProperties = CameraProperties;

    Camera();
    ~Camera() = default;

    // 禁用拷贝
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    // 允许移动
    Camera(Camera&&) = default;
    Camera& operator=(Camera&&) = default;

    /**
     * @brief 将摄像机的变换应用到给定的SkCanvas。
     */
    void ApplyTo(SkCanvas* canvas) const;

    /**
     * @brief 将屏幕坐标点转换为世界坐标点。
     */
    SkPoint ScreenToWorld(const SkPoint& screenPoint) const;

    /**
     * @brief 将世界坐标点转换为屏幕坐标点。
     */
    SkPoint WorldToScreen(const SkPoint& worldPoint) const;

    /**
     * @brief 设置摄像机的属性。
     */
    void SetProperties(const CameraProperties& properties);

    /**
     * @brief 获取当前摄像机的属性。
     */
    CameraProperties GetProperties() const;

    /**
     * @brief 构建并返回摄像机的视图矩阵。
     */
    SkM44 BuildViewMatrix() const;

    /**
     * @brief 获取视图投影矩阵。
     */
    SkM44 GetViewProjectionMatrix() const;

    /**
     * @brief 填充引擎数据。
     */
    void FillEngineData(EngineData& data) const;

    CameraProperties m_properties;          ///< 摄像机属性
    mutable std::shared_mutex m_mutex;      ///< 保护属性的读写锁
};

/// 默认主相机ID
constexpr const char* DEFAULT_CAMERA_ID = "Main";
/// UI相机ID
constexpr const char* UI_CAMERA_ID = "UI";

/**
 * @brief 摄像机管理器，管理多个摄像机实例并提供全局访问。
 *
 * 通过字符串ID创建和管理多个摄像机，支持切换当前激活的摄像机。
 * 默认创建"Main"主摄像机和"UI"相机。
 */
class LUMA_API CameraManager
{
public:
    static CameraManager& GetInstance();

    /**
     * @brief 创建一个新的摄像机。
     * @param id 摄像机的唯一标识符（字符串）。
     * @return 如果创建成功返回true，如果ID已存在返回false。
     */
    bool CreateCamera(const std::string& id);

    /**
     * @brief 删除指定ID的摄像机。
     * @param id 要删除的摄像机ID。
     * @return 如果删除成功返回true，如果ID不存在或是当前激活的摄像机返回false。
     * @note 不能删除当前激活的摄像机和默认相机（Main/UI）。
     */
    bool DestroyCamera(const std::string& id);

    /**
     * @brief 获取指定ID的摄像机。
     * @param id 摄像机ID。
     * @return 摄像机指针，如果不存在返回nullptr。
     */
    Camera* GetCamera(const std::string& id);

    /**
     * @brief 获取当前激活的摄像机。
     * @return 当前激活的摄像机引用。
     */
    Camera& GetActiveCamera();

    /**
     * @brief 获取当前激活的摄像机（const版本）。
     */
    const Camera& GetActiveCamera() const;

    /**
     * @brief 切换当前激活的摄像机。
     * @param id 要激活的摄像机ID。
     * @return 如果切换成功返回true，如果ID不存在返回false。
     */
    bool SetActiveCamera(const std::string& id);

    /**
     * @brief 获取当前激活的摄像机ID。
     */
    const std::string& GetActiveCameraId() const;

    /**
     * @brief 检查指定ID的摄像机是否存在。
     */
    bool HasCamera(const std::string& id) const;

    /**
     * @brief 获取所有摄像机的ID列表。
     */
    std::vector<std::string> GetAllCameraIds() const;

    /**
     * @brief 获取UI相机。
     * @return UI相机引用。
     */
    Camera& GetUICamera();

private:
    CameraManager();
    ~CameraManager() = default;

    std::unordered_map<std::string, std::unique_ptr<Camera>> m_cameras;  ///< 摄像机映射表
    std::string m_activeCameraId = DEFAULT_CAMERA_ID;                     ///< 当前激活的摄像机ID
    mutable std::shared_mutex m_managerMutex;                             ///< 保护管理器的读写锁
};

// 便捷的全局访问函数，获取当前激活的摄像机
inline Camera& GetActiveCamera()
{
    return CameraManager::GetInstance().GetActiveCamera();
}

#endif

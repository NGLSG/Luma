#ifndef TOUCH_GESTURE_HANDLER_H
#define TOUCH_GESTURE_HANDLER_H

#include <SDL3/SDL.h>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

/**
 * @brief 触摸手势处理器 - 用于处理Android Pad等触摸设备的手势操作
 * 
 * 支持的手势:
 * - 单指点击/拖拽: 对象选择和移动
 * - 双指平移: 相机平移
 * - 双指捏合: 相机缩放
 */
class TouchGestureHandler
{
public:
    struct TouchPoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float startX = 0.0f;
        float startY = 0.0f;
        float pressure = 1.0f;
        bool active = false;
    };

    struct GestureState
    {
        // 单指操作
        bool isSingleTouch = false;
        bool isSingleDragging = false;
        glm::vec2 singleTouchPos{0.0f};
        glm::vec2 singleTouchDelta{0.0f};
        
        // 双指操作
        bool isTwoFingerTouch = false;
        bool isPanning = false;
        bool isPinching = false;
        glm::vec2 panDelta{0.0f};
        float pinchScale = 1.0f;
        glm::vec2 pinchCenter{0.0f};
        
        // 点击检测
        bool justTapped = false;
        glm::vec2 tapPosition{0.0f};
    };

    using PanCallback = std::function<void(float dx, float dy)>;
    using ZoomCallback = std::function<void(float scale, float centerX, float centerY)>;
    using TapCallback = std::function<void(float x, float y)>;
    using DragCallback = std::function<void(float x, float y, float dx, float dy)>;
    using DragStartCallback = std::function<void(float x, float y)>;
    using DragEndCallback = std::function<void(float x, float y)>;

    TouchGestureHandler() = default;
    ~TouchGestureHandler() = default;

    /**
     * @brief 处理触摸按下事件
     */
    void OnTouchDown(SDL_FingerID fingerId, float x, float y, float pressure);

    /**
     * @brief 处理触摸移动事件
     */
    void OnTouchMove(SDL_FingerID fingerId, float x, float y, float dx, float dy, float pressure);

    /**
     * @brief 处理触摸抬起事件
     */
    void OnTouchUp(SDL_FingerID fingerId, float x, float y);

    /**
     * @brief 每帧更新,处理手势识别
     */
    void Update(float deltaTime);

    /**
     * @brief 重置所有状态
     */
    void Reset();

    /**
     * @brief 获取当前手势状态
     */
    const GestureState& GetState() const { return m_state; }

    /**
     * @brief 设置屏幕尺寸(用于归一化坐标转换)
     */
    void SetScreenSize(float width, float height);

    /**
     * @brief 设置双指平移回调
     */
    void SetPanCallback(PanCallback callback) { m_onPan = std::move(callback); }

    /**
     * @brief 设置缩放回调
     */
    void SetZoomCallback(ZoomCallback callback) { m_onZoom = std::move(callback); }

    /**
     * @brief 设置点击回调
     */
    void SetTapCallback(TapCallback callback) { m_onTap = std::move(callback); }

    /**
     * @brief 设置单指拖拽回调
     */
    void SetDragCallback(DragCallback callback) { m_onDrag = std::move(callback); }
    void SetDragStartCallback(DragStartCallback callback) { m_onDragStart = std::move(callback); }
    void SetDragEndCallback(DragEndCallback callback) { m_onDragEnd = std::move(callback); }

    /**
     * @brief 获取活跃的触摸点数量
     */
    int GetActiveTouchCount() const;

    /**
     * @brief 检查是否正在进行双指操作
     */
    bool IsTwoFingerGesture() const { return m_state.isTwoFingerTouch; }

    /**
     * @brief 检查是否正在进行单指拖拽
     */
    bool IsSingleFingerDragging() const { return m_state.isSingleDragging; }

private:
    void UpdateGestureState();
    glm::vec2 NormalizedToScreen(float nx, float ny) const;

private:
    std::unordered_map<SDL_FingerID, TouchPoint> m_touches;
    GestureState m_state;
    
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;
    
    // 手势识别参数
    float m_tapThreshold = 15.0f;      // 点击判定的最大移动距离(像素)
    float m_tapTimeThreshold = 0.3f;   // 点击判定的最大时间(秒)
    float m_dragThreshold = 10.0f;     // 拖拽判定的最小移动距离(像素)
    
    // 上一帧双指状态
    float m_lastPinchDistance = 0.0f;
    glm::vec2 m_lastTwoFingerCenter{0.0f};
    
    // 单指点击检测
    float m_touchDownTime = 0.0f;
    glm::vec2 m_touchDownPos{0.0f};
    bool m_touchMoved = false;
    
    // 回调
    PanCallback m_onPan;
    ZoomCallback m_onZoom;
    TapCallback m_onTap;
    DragCallback m_onDrag;
    DragStartCallback m_onDragStart;
    DragEndCallback m_onDragEnd;
};

#endif // TOUCH_GESTURE_HANDLER_H

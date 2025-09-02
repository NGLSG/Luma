#ifndef CURSOR_H
#define CURSOR_H

#include "../Utils/LazySingleton.h"
#include "../Components/Core.h"
#include <SDL3/SDL_events.h>
#include <array>


/**
 * @brief 表示一个鼠标按钮的状态。
 */
struct MouseButton
{
    /**
     * @brief 检查鼠标按钮当前是否被按下。
     * @return 如果按钮当前被按下，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsPressed() const { return isDown; }
    /**
     * @brief 检查鼠标按钮是否在当前帧被按下（从抬起到按下）。
     * @return 如果按钮在当前帧被按下，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsDown() const { return isDown && !wasDown; }
    /**
     * @brief 检查鼠标按钮是否在当前帧被释放（从按到抬起）。
     * @return 如果按钮在当前帧被释放，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsUp() const { return !isDown && wasDown; }


    /**
     * @brief 获取鼠标按钮被点击时的位置。
     * @return 鼠标按钮被点击时的二维整数坐标。
     */
    [[nodiscard]] const ECS::Vector2i& GetClickPosition() const { return clickPosition; }

private:
    friend class LumaCursor;
    bool isDown = false; ///< 按钮当前是否被按下。
    bool wasDown = false; ///< 按钮上一帧是否被按下。
    ECS::Vector2i clickPosition = {0, 0}; ///< 按钮被点击时的屏幕位置。
};


/**
 * @brief 管理鼠标光标的状态和输入。
 *
 * 这是一个懒加载的单例类，用于跟踪鼠标的位置、移动、滚轮滚动以及按钮状态。
 */
class LumaCursor : public LazySingleton<LumaCursor>
{
public:
    friend class LazySingleton<LumaCursor>;


    /**
     * @brief 更新光标状态，例如重置滚轮增量和更新按钮的“上一帧状态”。
     */
    void Update();


    /**
     * @brief 处理 SDL 事件以更新光标状态。
     * @param event 要处理的 SDL 事件。
     */
    void ProcessEvent(const SDL_Event& event);


    /**
     * @brief 获取当前光标的屏幕位置。
     * @return 光标的二维整数坐标。
     */
    static ECS::Vector2i GetPosition();
    /**
     * @brief 获取光标自上一帧以来的移动增量。
     * @return 光标的二维整数移动增量。
     */
    static ECS::Vector2i GetDelta();
    /**
     * @brief 获取鼠标滚轮自上一帧以来的滚动增量。
     * @return 鼠标滚轮的二维浮点数滚动增量。
     */
    static ECS::Vector2f GetScrollDelta();


    static const MouseButton& Left; ///< 左键鼠标按钮状态。
    static const MouseButton& Right; ///< 右键鼠标按钮状态。
    static const MouseButton& Middle; ///< 中键鼠标按钮状态。

private:
    /**
     * @brief 默认构造函数，私有以强制单例模式。
     */
    LumaCursor() = default;

    ECS::Vector2i m_position = {0, 0}; ///< 光标当前的屏幕位置。
    ECS::Vector2i m_delta = {0, 0}; ///< 光标自上一帧以来的移动增量。
    ECS::Vector2f m_scrollDelta = {0.0f, 0.0f}; ///< 鼠标滚轮自上一帧以来的滚动增量。


    std::array<MouseButton, 6> m_buttons{}; ///< 存储所有鼠标按钮的状态。
};

#endif
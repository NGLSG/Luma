#pragma once

#include <string>
#include <functional>
#include <SDL3/SDL.h>
#include "../Renderer/GraphicsBackend.h"
#include "../Data/EngineContext.h"
#include <unordered_map>

/**
 * @brief 表示一个应用程序窗口。
 *
 * 该类封装了SDL窗口的创建、管理和事件处理。
 */
class LUMA_API PlatformWindow
{
public:
    /**
     * @brief 消息框的通知级别。
     */
    enum NoticeLevel
    {
        Info, ///< 信息级别。
        Warning, ///< 警告级别。
        Error ///< 错误级别。
    };

    /**
     * @brief 触摸点信息结构。
     */
    struct TouchPoint
    {
        SDL_FingerID fingerId; ///< 触摸点ID。
        float x; ///< 触摸点X坐标（归一化）。
        float y; ///< 触摸点Y坐标（归一化）。
        float pressure; ///< 触摸压力。
    };

    /// 通用事件回调函数类型。
    using EventCallback = std::function<void(const SDL_Event&)>;
    /// 窗口大小改变回调函数类型。
    using ResizeCallback = std::function<void(int width, int height)>;
    /// 窗口关闭请求回调函数类型。
    using CloseCallback = std::function<void()>;
    /// 鼠标移动回调函数类型。
    using MouseMoveCallback = std::function<void(int x, int y)>;
    /// 鼠标按键回调函数类型。
    using MouseButtonCallback = std::function<void(int button, int x, int y)>;
    /// 鼠标滚轮回调函数类型。
    using MouseWheelCallback = std::function<void(float x, float y)>;
    /// 键盘按键回调函数类型。
    using KeyCallback = std::function<void(SDL_Keycode key, SDL_Scancode scancode, bool isRepeat)>;
    /// 文本输入回调函数类型。
    using TextInputCallback = std::function<void(const char* text)>;
    /// 文件拖放回调函数类型。
    using FileDropCallback = std::function<void(const std::vector<std::string>& files)>;
    /// 触摸点按下回调函数类型。
    using TouchDownCallback = std::function<void(SDL_FingerID fingerId, float x, float y, float pressure)>;
    /// 触摸点移动回调函数类型。
    using TouchMoveCallback = std::function<void(SDL_FingerID fingerId, float x, float y, float dx, float dy,
                                                 float pressure)>;
    /// 触摸点抬起回调函数类型。
    using TouchUpCallback = std::function<void(SDL_FingerID fingerId, float x, float y)>;

public:
    /**
     * @brief 构造一个新的Window对象。
     * @param title 窗口的标题。
     * @param width 窗口的宽度。
     * @param height 窗口的高度。
     */
    PlatformWindow(const std::string& title, int width, int height);

    /**
     * @brief 销毁Window对象。
     */
    ~PlatformWindow();

    /**
     * @brief 获取当前的输入状态。
     * @return 当前的输入状态。
     */
    InputState GetInputState() const;

    /**
     * @brief 禁用拷贝构造函数。
     */
    PlatformWindow(const PlatformWindow&) = delete;

    /**
     * @brief 禁用赋值运算符。
     */
    PlatformWindow& operator=(const PlatformWindow&) = delete;

    /**
     * @brief 轮询并处理所有待处理的窗口事件。
     */
    void PollEvents();

    /**
     * @brief 检查窗口是否应该关闭。
     * @return 如果窗口收到关闭请求，则返回true；否则返回false。
     */
    bool ShouldClose() const;

    /**
     * @brief 设置窗口的标题。
     * @param title 新的窗口标题。
     */
    void SetTitle(const std::string& title);

    /**
     * @brief 获取底层的SDL_Window指针。
     * @return 指向SDL_Window结构的指针。
     */
    SDL_Window* GetSdlWindow() const;

    /**
     * @brief 获取窗口的本地句柄。
     * @return 窗口的本地句柄。
     */
    NativeWindowHandle GetNativeWindowHandle() const;

    /**
     * @brief 获取窗口的当前大小（宽度和高度）。
     * @param width 用于接收窗口宽度的引用。
     * @param height 用于接收窗口高度的引用。
     */
    void GetSize(uint16_t& width, uint16_t& height) const;

    /**
     * @brief 获取窗口的当前大小（宽度和高度）。
     * @param width 用于接收窗口宽度的引用。
     * @param height 用于接收窗口高度的引用。
     */
    void GetSize(int& width, int& height) const;

    /**
     * @brief 获取窗口的宽度。
     * @return 窗口的宽度。
     */
    float GetWidth() const;

    /**
     * @brief 获取窗口的高度。
     * @return 窗口的高度。
     */
    float GetHeight() const;

    /**
     * @brief 启动文本输入模式。
     */
    void StartTextInput();

    /**
     * @brief 停止文本输入模式。
     */
    void StopTextInput();

    /**
     * @brief 设置文本输入区域和光标位置。
     * @param rect 文本输入区域的矩形。
     * @param cursor 光标在文本输入区域中的位置。
     */
    void SetTextInputArea(const SDL_Rect& rect, int cursor);

    /**
     * @brief 检查文本输入是否处于活动状态。
     * @return 如果文本输入处于活动状态，则返回true；否则返回false。
     */
    bool IsTextInputActive() const;

    /**
     * @brief 显示一个消息框。
     * @param level 消息框的通知级别。
     * @param title 消息框的标题。
     * @param message 消息框显示的消息内容。
     */
    void ShowMessageBox(NoticeLevel level, std::string_view title, std::string_view message) const;

    /**
     * @brief 销毁窗口并释放相关资源。
     */
    void Destroy();

    /**
     * @brief 创建一个Window对象的静态工厂方法。
     * @param title 窗口的标题。
     * @param width 窗口的宽度。
     * @param height 窗口的高度。
     * @return 指向新创建的Window对象的unique_ptr。
     */
    static std::unique_ptr<PlatformWindow> Create(const std::string& title, int width, int height);

#if defined(SDL_PLATFORM_ANDROID)
    /**
     * @brief Android平台允许从Java/Kotlin层注入现有的ANativeWindow指针。
     * @param nativeWindow Android原生窗口指针。
     */
    static void SetAndroidNativeWindow(void* nativeWindow);

    /**
     * @brief 获取所有活动的触摸点。
     * @return 触摸点ID到触摸点信息的映射。
     */
    const std::unordered_map<SDL_FingerID, TouchPoint>& GetActiveTouches() const { return activeTouches; }
#endif

    /**
     * @brief 设置或取消窗口的全屏模式。
     * @param fullscreen 如果为true，则进入全屏模式；否则退出全屏模式。
     */
    void FullScreen(bool fullscreen);

    /**
     * @brief 设置或取消窗口的无边框模式。
     * @param broaderLess 如果为true，则进入无边框模式；否则退出无边框模式。
     */
    void BroaderLess(bool broaderLess);

    /**
     * @brief 设置窗口的图标。
     * @param iconPath 图标文件的路径。
     */
    void SetIcon(const std::string& iconPath);

    /// 当任何SDL事件发生时触发。
    LumaEvent<const SDL_Event&> OnAnyEvent;
    /// 当窗口大小改变时触发。
    LumaEvent<int, int> OnResize;
    /// 当窗口收到关闭请求时触发。
    LumaEvent<> OnCloseRequest;
    /// 当鼠标移动时触发。
    LumaEvent<int, int> OnMouseMove;
    /// 当鼠标按键按下时触发。
    LumaEvent<int, int, int> OnMouseButtonDown;
    /// 当鼠标按键释放时触发。
    LumaEvent<int, int, int> OnMouseButtonUp;
    /// 当鼠标滚轮滚动时触发。
    LumaEvent<float, float> OnMouseWheel;
    /// 当键盘按键按下时触发。
    LumaEvent<SDL_Keycode, SDL_Scancode, bool> OnKeyPress;
    /// 当键盘按键释放时触发。
    LumaEvent<SDL_Keycode, SDL_Scancode, bool> OnKeyRelease;
    /// 当文本输入发生时触发。
    LumaEvent<const char*> OnTextInput;
    /// 当触摸点按下时触发。
    LumaEvent<SDL_FingerID, float, float, float> OnTouchDown;
    /// 当触摸点移动时触发。
    LumaEvent<SDL_FingerID, float, float, float, float, float> OnTouchMove;
    /// 当触摸点抬起时触发。
    LumaEvent<SDL_FingerID, float, float> OnTouchUp;

private:
    /// 处理单个SDL事件。
    void handleEvent(const SDL_Event& event);

    /// 处理触摸事件。
    void handleTouchEvent(const SDL_Event& event);

private:
    SDL_Window* sdlWindow = nullptr; ///< 底层的SDL窗口指针。
    bool shouldCloseFlag = false; ///< 标记窗口是否应该关闭。
    InputState inputState; ///< 窗口的输入状态。

#if defined(SDL_PLATFORM_ANDROID)
    std::unordered_map<SDL_FingerID, TouchPoint> activeTouches; ///< 活动的触摸点映射。
#endif

    EventCallback onAnyEvent; ///< 任何事件的回调。
    ResizeCallback onResize; ///< 窗口大小改变的回调。
    CloseCallback onCloseRequest; ///< 窗口关闭请求的回调。
    MouseMoveCallback onMouseMove; ///< 鼠标移动的回调。
    MouseButtonCallback onMouseButtonDown; ///< 鼠标按键按下的回调。
    MouseButtonCallback onMouseButtonUp; ///< 鼠标按键释放的回调。
    MouseWheelCallback onMouseWheel; ///< 鼠标滚轮滚动的回调。
    KeyCallback onKeyPress; ///< 键盘按键按下的回调。
    KeyCallback onKeyRelease; ///< 键盘按键释放的回调。
    TextInputCallback onTextInput; ///< 文本输入的回调。
    FileDropCallback onFileDrop; ///< 文件拖放的回调。
    TouchDownCallback onTouchDown; ///< 触摸点按下的回调。
    TouchMoveCallback onTouchMove; ///< 触摸点移动的回调。
    TouchUpCallback onTouchUp; ///< 触摸点抬起的回调。
};

#pragma once
#include <string>
#include <functional>
#include <SDL3/SDL.h>
#include "../Renderer/GraphicsBackend.h"
#include "../Data/EngineContext.h"
#include <unordered_map>
class LUMA_API PlatformWindow
{
public:
    enum NoticeLevel
    {
        Info, 
        Warning, 
        Error 
    };
    struct TouchPoint
    {
        SDL_FingerID fingerId; 
        float x; 
        float y; 
        float pressure; 
    };
    using EventCallback = std::function<void(const SDL_Event&)>;
    using ResizeCallback = std::function<void(int width, int height)>;
    using CloseCallback = std::function<void()>;
    using MouseMoveCallback = std::function<void(int x, int y)>;
    using MouseButtonCallback = std::function<void(int button, int x, int y)>;
    using MouseWheelCallback = std::function<void(float x, float y)>;
    using KeyCallback = std::function<void(SDL_Keycode key, SDL_Scancode scancode, bool isRepeat)>;
    using TextInputCallback = std::function<void(const char* text)>;
    using FileDropCallback = std::function<void(const std::vector<std::string>& files)>;
    using TouchDownCallback = std::function<void(SDL_FingerID fingerId, float x, float y, float pressure)>;
    using TouchMoveCallback = std::function<void(SDL_FingerID fingerId, float x, float y, float dx, float dy,
                                                 float pressure)>;
    using TouchUpCallback = std::function<void(SDL_FingerID fingerId, float x, float y)>;
public:
    PlatformWindow(const std::string& title, int width, int height);
    ~PlatformWindow();
    InputState GetInputState() const;
    PlatformWindow(const PlatformWindow&) = delete;
    PlatformWindow& operator=(const PlatformWindow&) = delete;
    void PollEvents();
    bool ShouldClose() const;
    void SetTitle(const std::string& title);
    SDL_Window* GetSdlWindow() const;
    NativeWindowHandle GetNativeWindowHandle() const;
    void GetSize(uint16_t& width, uint16_t& height) const;
    void GetSize(int& width, int& height) const;
    float GetWidth() const;
    float GetHeight() const;
    void StartTextInput();
    void StopTextInput();
    void SetTextInputArea(const SDL_Rect& rect, int cursor);
    bool IsTextInputActive() const;
    void ShowMessageBox(NoticeLevel level, std::string_view title, std::string_view message) const;
    void Destroy();
    static std::unique_ptr<PlatformWindow> Create(const std::string& title, int width, int height);
#if defined(SDL_PLATFORM_ANDROID)
    static void SetAndroidNativeWindow(void* nativeWindow);
    const std::unordered_map<SDL_FingerID, TouchPoint>& GetActiveTouches() const { return activeTouches; }
#endif
    void FullScreen(bool fullscreen);
    void BroaderLess(bool broaderLess);
    void SetIcon(const std::string& iconPath);
    LumaEvent<const SDL_Event&> OnAnyEvent;
    LumaEvent<int, int> OnResize;
    LumaEvent<> OnCloseRequest;
    LumaEvent<int, int> OnMouseMove;
    LumaEvent<int, int, int> OnMouseButtonDown;
    LumaEvent<int, int, int> OnMouseButtonUp;
    LumaEvent<float, float> OnMouseWheel;
    LumaEvent<SDL_Keycode, SDL_Scancode, bool> OnKeyPress;
    LumaEvent<SDL_Keycode, SDL_Scancode, bool> OnKeyRelease;
    LumaEvent<const char*> OnTextInput;
    LumaEvent<SDL_FingerID, float, float, float> OnTouchDown;
    LumaEvent<SDL_FingerID, float, float, float, float, float> OnTouchMove;
    LumaEvent<SDL_FingerID, float, float> OnTouchUp;
private:
    void handleEvent(const SDL_Event& event);
    void handleTouchEvent(const SDL_Event& event);
private:
    SDL_Window* sdlWindow = nullptr; 
    bool shouldCloseFlag = false; 
    InputState inputState; 
#if defined(SDL_PLATFORM_ANDROID)
    std::unordered_map<SDL_FingerID, TouchPoint> activeTouches; 
#endif
    EventCallback onAnyEvent; 
    ResizeCallback onResize; 
    CloseCallback onCloseRequest; 
    MouseMoveCallback onMouseMove; 
    MouseButtonCallback onMouseButtonDown; 
    MouseButtonCallback onMouseButtonUp; 
    MouseWheelCallback onMouseWheel; 
    KeyCallback onKeyPress; 
    KeyCallback onKeyRelease; 
    TextInputCallback onTextInput; 
    FileDropCallback onFileDrop; 
    TouchDownCallback onTouchDown; 
    TouchMoveCallback onTouchMove; 
    TouchUpCallback onTouchUp; 
};

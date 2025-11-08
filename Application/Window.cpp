#include "Window.h"
#include <stdexcept>
#include <vector>

#include "stb_image.h"
#include "Event/EventBus.h"
#include "Event/Events.h"


#if defined(SDL_PLATFORM_WINDOWS)
#include <windows.h>
#include <shellapi.h>
#endif

#if defined(SDL_PLATFORM_ANDROID)
static void* g_androidNativeWindow = nullptr;
#endif


PlatformWindow::PlatformWindow(const std::string& title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    
#if defined(SDL_PLATFORM_ANDROID)
    if (g_androidNativeWindow)
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, g_androidNativeWindow);
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title.c_str());
        
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 0);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 0);
        
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
        sdlWindow = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
        g_androidNativeWindow = nullptr;
    }
    else
    {
        
        
        sdlWindow = SDL_CreateWindow(
            title.c_str(),
            0,
            0,
            SDL_WINDOW_FULLSCREEN
        );
    }
#else
    
    sdlWindow = SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );
#endif

    if (!sdlWindow)
    {
        SDL_Quit();
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }


    SDL_SetEventEnabled(SDL_EVENT_DROP_BEGIN, true);
    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
    SDL_SetEventEnabled(SDL_EVENT_DROP_COMPLETE, true);

#if defined(SDL_PLATFORM_WINDOWS)

    {
        BOOL elevated = FALSE;
        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        {
            TOKEN_ELEVATION te{};
            DWORD len = 0;
            if (GetTokenInformation(token, TokenElevation, &te, sizeof(te), &len))
                elevated = te.TokenIsElevated;
            CloseHandle(token);
        }
        if (elevated)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Process is running elevated (Administrator). Windows will block file drag-and-drop from a non-elevated Explorer.");
        }
    }


    {
        SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
        void* hwndPtr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwndPtr)
            DragAcceptFiles(static_cast<HWND>(hwndPtr), TRUE);
    }
#endif
}

#if defined(SDL_PLATFORM_ANDROID)
void PlatformWindow::SetAndroidNativeWindow(void* nativeWindow)
{
    g_androidNativeWindow = nativeWindow;
}
#endif

PlatformWindow::~PlatformWindow()
{
    if (sdlWindow)
    {
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
    }
    SDL_Quit();
}

InputState PlatformWindow::GetInputState() const
{
    return inputState;
}

std::unique_ptr<PlatformWindow> PlatformWindow::Create(const std::string& title, int width, int height)
{
    return std::make_unique<PlatformWindow>(title, width, height);
}

void PlatformWindow::FullScreen(bool fullscreen)
{
    SDL_SetWindowFullscreen(sdlWindow, fullscreen);
}

void PlatformWindow::BroaderLess(bool broaderLess)
{
    if (broaderLess)
    {
        SDL_SetWindowBordered(sdlWindow, false);
        SDL_SetWindowResizable(sdlWindow, false);
    }
    else
    {
        SDL_SetWindowBordered(sdlWindow, true);
        SDL_SetWindowResizable(sdlWindow, true);
    }
}

void PlatformWindow::SetIcon(const std::string& iconPath)
{
    int width, height, channels;

    unsigned char* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);

    if (!pixels)
    {
        LogError("使用 stb_image 加载图标失败 '{}': {}", iconPath, stbi_failure_reason());
        return;
    }


    SDL_Surface* iconSurface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        pixels,
        width * 4
    );

    if (!iconSurface)
    {
        LogError("从像素数据创建 SDL_Surface 失败: {}", SDL_GetError());
        stbi_image_free(pixels);
        return;
    }


    SDL_SetWindowIcon(sdlWindow, iconSurface);


    SDL_DestroySurface(iconSurface);
    stbi_image_free(pixels);
}


void PlatformWindow::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        handleEvent(event);
    }
}

void PlatformWindow::handleEvent(const SDL_Event& event)
{
    OnAnyEvent.Invoke(event);


    static bool s_dropInProgress = false;
    static std::vector<std::string> s_dropBatchPaths;

    switch (event.type)
    {
    case SDL_EVENT_QUIT:
        shouldCloseFlag = true;
        OnCloseRequest.Invoke();
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        OnResize.Invoke(event.window.data1, event.window.data2);
        break;

    case SDL_EVENT_MOUSE_MOTION:
        inputState.mousePosition = {event.motion.x, event.motion.y};
        OnMouseMove.Invoke(event.motion.x, event.motion.y);
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_LEFT) inputState.isLeftMouseDown = true;
        else if (event.button.button == SDL_BUTTON_RIGHT) inputState.isRightMouseDown = true;
        OnMouseButtonDown.Invoke(event.button.button, event.button.x, event.button.y);
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == SDL_BUTTON_LEFT) inputState.isLeftMouseDown = false;
        else if (event.button.button == SDL_BUTTON_RIGHT) inputState.isRightMouseDown = false;
        OnMouseButtonUp.Invoke(event.button.button, event.button.x, event.button.y);
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        OnMouseWheel.Invoke(event.wheel.x, event.wheel.y);
        break;

    case SDL_EVENT_KEY_DOWN:
        OnKeyPress.Invoke(event.key.key, event.key.scancode, event.key.repeat != 0);
        break;

    case SDL_EVENT_KEY_UP:
        OnKeyRelease.Invoke(event.key.key, event.key.scancode, false);
        break;

    case SDL_EVENT_TEXT_INPUT:
        OnTextInput.Invoke(event.text.text);
        break;


    case SDL_EVENT_DROP_BEGIN:
        s_dropInProgress = true;
        s_dropBatchPaths.clear();
        SDL_Log("DROP_BEGIN");
        break;

    case SDL_EVENT_DROP_FILE:
        if (event.drop.data)
        {
            s_dropBatchPaths.emplace_back(event.drop.data);
        }
        SDL_Log("DROP_FILE (count=%d)", (int)s_dropBatchPaths.size());


        if (!s_dropInProgress && !s_dropBatchPaths.empty())
        {
            EventBus::GetInstance().Publish(DragDorpFileEvent{s_dropBatchPaths});
            s_dropBatchPaths.clear();
        }
        break;

    case SDL_EVENT_DROP_COMPLETE:
        SDL_Log("DROP_COMPLETE (count=%d)", (int)s_dropBatchPaths.size());
        if (!s_dropBatchPaths.empty())
        {
            EventBus::GetInstance().Publish(DragDorpFileEvent{s_dropBatchPaths});
            s_dropBatchPaths.clear();
        }
        s_dropInProgress = false;
        break;

    default:
        break;
    }
}

bool PlatformWindow::ShouldClose() const
{
    return shouldCloseFlag;
}

void PlatformWindow::SetTitle(const std::string& title)
{
    SDL_SetWindowTitle(sdlWindow, title.c_str());
}

SDL_Window* PlatformWindow::GetSdlWindow() const
{
    return sdlWindow;
}

NativeWindowHandle PlatformWindow::GetNativeWindowHandle() const
{
    NativeWindowHandle handle = {};
    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);

#if defined(SDL_PLATFORM_WINDOWS)
    handle.hWnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (handle.hWnd)
    {
        handle.hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(static_cast<HWND>(handle.hWnd), GWLP_HINSTANCE));
    }

#elif defined(SDL_PLATFORM_MACOS)

    handle.metalLayer = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);

#elif defined(SDL_PLATFORM_LINUX) && !defined(SDL_PLATFORM_ANDROID)

    handle.x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    handle.x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#elif defined(SDL_PLATFORM_ANDROID)
    handle.aNativeWindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, NULL);

#endif

    return handle;
}

void PlatformWindow::GetSize(uint16_t& width, uint16_t& height) const
{
    int w, h;
    SDL_GetWindowSizeInPixels(sdlWindow, &w, &h);
    width = static_cast<uint16_t>(w);
    height = static_cast<uint16_t>(h);
}

void PlatformWindow::GetSize(int& width, int& height) const
{
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
}

float PlatformWindow::GetWidth() const
{
    int w, h;
    SDL_GetWindowSizeInPixels(sdlWindow, &w, &h);
    return static_cast<float>(w);
}

float PlatformWindow::GetHeight() const
{
    int w, h;
    SDL_GetWindowSizeInPixels(sdlWindow, &w, &h);
    return static_cast<float>(h);
}

void PlatformWindow::StartTextInput()
{
    SDL_StartTextInput(sdlWindow);
}

void PlatformWindow::StopTextInput()
{
    SDL_StopTextInput(sdlWindow);
}

void PlatformWindow::SetTextInputArea(const SDL_Rect& rect, int cursor)
{
    SDL_SetTextInputArea(sdlWindow, &rect, cursor);
}

bool PlatformWindow::IsTextInputActive() const
{
    return SDL_TextInputActive(sdlWindow);
}

void PlatformWindow::ShowMessageBox(NoticeLevel level, std::string_view title, std::string_view message) const
{
    SDL_MessageBoxFlags flags = SDL_MESSAGEBOX_INFORMATION;
    switch (level)
    {
    case Info:
        flags = SDL_MESSAGEBOX_INFORMATION;
        break;
    case Warning:
        flags = SDL_MESSAGEBOX_WARNING;
        break;
    case Error:
        flags = SDL_MESSAGEBOX_ERROR;
        break;
    }
    SDL_ShowSimpleMessageBox(flags, title.data(), message.data(), sdlWindow);
}

void PlatformWindow::Destroy()
{
    if (sdlWindow)
    {
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
    }
}

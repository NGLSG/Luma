#include "ApplicationBase.h"
#include "Window.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/RenderSystem.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <stdexcept>

#include "imgui_node_editor.h"
#include "JobSystem.h"
#include "../Utils/PCH.h"
#include "Input/Cursor.h"
#include "Input/Keyboards.h"

ApplicationBase::ApplicationBase(ApplicationConfig config)
    : m_title(config.title), m_isRunning(true)
      , m_window(nullptr), m_graphicsBackend(nullptr), m_renderSystem(nullptr)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
    }
    JobSystem::GetInstance().Initialize();
}

ApplicationBase::~ApplicationBase()
{
    SDL_Quit();
}

void ApplicationBase::Run()
{
    InitializeCoreSystems();


    InitializeDerived();

    auto lastTime = std::chrono::high_resolution_clock::now();
    m_window->OnAnyEvent.AddListener([&](const SDL_Event& event)
    {
        Keyboard::GetInstance().ProcessEvent(event);

        LumaCursor::GetInstance().ProcessEvent(event);
        m_context.frameEvents.push_back(event);
    });
    m_context.window = m_window.get();
    while (m_isRunning)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;
        Keyboard::GetInstance().Update();
        LumaCursor::GetInstance().Update();
        m_context.frameEvents.clear();
        m_window->PollEvents();
        if (m_window->ShouldClose())
        {
            m_isRunning = false;
        }
        m_context.inputState = m_window->GetInputState();
        Update(deltaTime);
        Render();
    }


    ShutdownDerived();


    ShutdownCoreSystems();
}

void ApplicationBase::InitializeCoreSystems()
{
    m_window = PlatformWindow::Create(m_title, m_config.width, m_config.height);
    if (!m_window || !m_window->GetSdlWindow())
    {
        throw std::runtime_error("Failed to create Window.");
    }


    GraphicsBackendOptions options;
    options.windowHandle = m_window->GetNativeWindowHandle();
    m_window->GetSize(options.width, options.height);
#ifdef _WIN32
    options.backendTypePriority = {BackendType::D3D12, BackendType::D3D11, BackendType::Vulkan, BackendType::OpenGL};
#elif __APPLE__
    options.backendTypePriority = {BackendType::Metal, BackendType::OpenGL};
#elif __linux__
    options.backendTypePriority = {BackendType::Vulkan, BackendType::OpenGL, BackendType::OpenGLES};
#elif __ANDROID__
    options.backendTypePriority = {BackendType::OpenGLES, BackendType::Vulkan};
#else
#error "Unsupported platform"
#endif
    options.enableVSync = m_config.vsync;

    m_graphicsBackend = GraphicsBackend::Create(options);
    if (!m_graphicsBackend)
    {
        throw std::runtime_error("Failed to create GraphicsBackend.");
    }

    m_window->OnResize.AddListener([&](int width, int height)
    {
        if (m_graphicsBackend)
        {
            m_graphicsBackend->Resize(width, height);
        }
    });


    m_renderSystem = RenderSystem::Create(*m_graphicsBackend);
    if (!m_renderSystem)
    {
        throw std::runtime_error("Failed to create RenderSystem.");
    }
}

void ApplicationBase::ShutdownCoreSystems()
{
    m_renderSystem.reset();
    m_graphicsBackend.reset();
    m_window.reset();
}

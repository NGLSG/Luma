#include "ApplicationBase.h"
#include "Window.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/RenderSystem.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <stdexcept>

#include "imgui_node_editor.h"
#include "JobSystem.h"
#include "Profiler.h"
#include "../Utils/PCH.h"
#include "Input/Cursor.h"
#include "Input/Keyboards.h"

ApplicationBase::ApplicationBase(ApplicationConfig config)
    : m_config(config), m_title(config.title), m_isRunning(true)
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

    m_window->OnAnyEvent.AddListener([&](const SDL_Event& event)
    {
        Keyboard::GetInstance().ProcessEvent(event);
        LumaCursor::GetInstance().ProcessEvent(event);
        m_context.frameEvents.Modify([&](auto& events)
        {
            events.PushBack(event);
        });
    });
    m_context.window = m_window.get();


    m_simulationThread = std::thread(&ApplicationBase::simulationLoop, this);


    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    while (m_isRunning)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        Keyboard::GetInstance().Update();
        LumaCursor::GetInstance().Update();
        m_context.frameEvents.Clear();
        m_window->PollEvents();
        if (m_window->ShouldClose())
        {
            m_isRunning = false;
        }
        m_context.inputState = m_window->GetInputState();


        const float fixedDeltaTime = 1.0f / static_cast<float>(m_config.SimulationFPS);
        accumulator += frameTime;
        m_context.interpolationAlpha = static_cast<float>(accumulator / fixedDeltaTime);
        if (accumulator >= fixedDeltaTime)
        {
            accumulator -= fixedDeltaTime;
        }

        Render();
        Profiler::GetInstance().Update();
    }


    if (m_simulationThread.joinable())
    {
        m_simulationThread.join();
    }

    ShutdownDerived();
    ShutdownCoreSystems();
}

void ApplicationBase::simulationLoop()
{
    const std::chrono::duration<double> fixedDeltaTime(1.0 / m_config.SimulationFPS);
    auto nextFrameTime = std::chrono::high_resolution_clock::now();
    while (m_isRunning)
    {
        nextFrameTime += std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(fixedDeltaTime);
        m_context.deferredCommands.Execute();
        Update(static_cast<float>(fixedDeltaTime.count()));
        std::this_thread::sleep_until(nextFrameTime);
    }
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

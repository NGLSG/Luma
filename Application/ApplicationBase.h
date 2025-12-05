#ifndef APPLICATIONBASE_H
#define APPLICATIONBASE_H
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include "../Data/EngineContext.h"
struct LUMA_API ApplicationConfig
{
    std::string title = "Luma Engine"; 
    int width = 1280; 
    int height = 720; 
    bool vsync = false; 
    int SimulationFPS = 120; 
    Guid StartSceneGuid = Guid::Invalid(); 
    std::string LastProjectPath = ""; 
};
class LUMA_API ApplicationBase
{
public:
    explicit ApplicationBase(ApplicationConfig config = ApplicationConfig());
    virtual ~ApplicationBase();
    ApplicationBase(const ApplicationBase&) = delete;
    ApplicationBase& operator=(const ApplicationBase&) = delete;
    void Run();
    inline static ApplicationMode CURRENT_MODE; 
protected:
    virtual void InitializeDerived() = 0;
    virtual void ShutdownDerived() = 0;
    virtual void Update(float fixedDeltaTime) = 0;
    virtual void Render() = 0;
private:
    void InitializeCoreSystems();
    void ShutdownCoreSystems();
    void simulationLoop();
protected:
    std::atomic<bool> m_isRunning;
    std::thread m_simulationThread; 
    std::unique_ptr<PlatformWindow> m_window; 
    std::unique_ptr<GraphicsBackend> m_graphicsBackend; 
    std::unique_ptr<RenderSystem> m_renderSystem; 
    EngineContext m_context; 
    std::string m_title; 
    ApplicationConfig m_config; 
};
#endif 

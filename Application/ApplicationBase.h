#ifndef APPLICATIONBASE_H
#define APPLICATIONBASE_H

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include "../Data/EngineContext.h"

/**
 * @brief 应用程序配置结构体。
 * 包含应用程序启动和运行所需的基本设置。
 */
struct LUMA_API ApplicationConfig
{
    std::string title = "Luma Engine"; ///< 应用程序窗口的标题。
    int width = 1280; ///< 应用程序窗口的宽度。
    int height = 720; ///< 应用程序窗口的高度。
    bool vsync = false; ///< 是否启用垂直同步。
    int SimulationFPS = 60; ///< 目标模拟更新频率 (Fixed Timestep)。
    Guid StartSceneGuid = Guid::Invalid(); ///< 启动场景的全局唯一标识符。
    std::string LastProjectPath = ""; ///< 上次打开的项目路径。
};


/**
 * @brief 应用程序基类。
 * 提供应用程序生命周期管理和核心系统初始化/关闭的抽象。
 * 采用渲染/模拟分离的多线程架构。
 */
class LUMA_API ApplicationBase
{
public:
    /**
     * @brief 构造函数。
     * 使用给定的配置初始化应用程序基类。
     * @param config 应用程序的配置设置。
     */
    explicit ApplicationBase(ApplicationConfig config = ApplicationConfig());

    /**
     * @brief 虚析构函数。
     * 确保派生类资源能够正确释放。
     */
    virtual ~ApplicationBase();

    ApplicationBase(const ApplicationBase&) = delete;
    ApplicationBase& operator=(const ApplicationBase&) = delete;

    /**
     * @brief 运行应用程序。
     * 启动渲染和模拟线程，并进入主循环。
     */
    void Run();

    inline static ApplicationMode CURRENT_MODE; ///< 当前应用程序的运行模式。

protected:
    /**
     * @brief 派生类初始化方法。
     * 这是一个纯虚函数，要求派生类实现其特有的初始化逻辑。
     */
    virtual void InitializeDerived() = 0;

    /**
     * @brief 派生类关闭方法。
     * 这是一个纯虚函数，要求派生类实现其特有的关闭逻辑。
     */
    virtual void ShutdownDerived() = 0;

    /**
     * @brief 以固定步长更新应用程序状态。
     * 此函数由独立的模拟线程以固定频率调用。
     * @param fixedDeltaTime 固定的时间间隔（秒），用于保证模拟的确定性。
     */
    virtual void Update(float fixedDeltaTime) = 0;

    /**
     * @brief 渲染应用程序内容。
     * 此函数由主线程上的渲染循环尽可能快地调用。
     */
    virtual void Render() = 0;

private:
    /**
     * @brief 初始化核心系统。
     * 负责设置应用程序运行所需的核心组件。
     */
    void InitializeCoreSystems();

    /**
     * @brief 关闭核心系统。
     * 负责清理应用程序核心组件的资源。
     */
    void ShutdownCoreSystems();

    //TODO 渲染和模拟分离
    /**
     * @brief 模拟循环（固定步长）。
     * 在独立的线程上运行，负责所有逻辑和物理更新。
     */
    void SimulationLoop();

    /**
     * @brief 渲染循环（可变步长）。
     * 在主线程上运行，负责调用渲染指令，会尽可能快地执行。
     */
    void RenderLoop();

protected:
    std::atomic<bool> m_isRunning;
    std::unique_ptr<PlatformWindow> m_window; ///< 应用程序窗口的智能指针。
    std::unique_ptr<GraphicsBackend> m_graphicsBackend; ///< 图形后端的智能指针。
    std::unique_ptr<RenderSystem> m_renderSystem; ///< 渲染系统的智能指针。
    EngineContext m_context; ///< 引擎上下文，包含全局可访问的数据。
    std::string m_title; ///< 应用程序窗口的标题。
    ApplicationConfig m_config; ///< 应用程序的配置设置。
};

#endif // APPLICATIONBASE_H

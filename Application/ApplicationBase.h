#ifndef APPLICATIONBASE_H
#define APPLICATIONBASE_H

#include <memory>
#include <string>
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
    int TargetFPS = 60; ///< 目标帧率。
    Guid StartSceneGuid = Guid::Invalid(); ///< 启动场景的全局唯一标识符。
    std::string LastProjectPath = ""; ///< 上次打开的项目路径。
};

/**
 * @brief 应用程序基类。
 * 提供应用程序生命周期管理和核心系统初始化/关闭的抽象。
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
     * 启动应用程序的主循环。
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
     * @brief 更新应用程序状态。
     * 这是一个纯虚函数，要求派生类实现其游戏逻辑或状态更新。
     * @param deltaTime 自上一帧以来的时间间隔（秒）。
     */
    virtual void Update(float deltaTime) = 0;

    /**
     * @brief 渲染应用程序内容。
     * 这是一个纯虚函数，要求派生类实现其渲染逻辑。
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

protected:
    std::unique_ptr<PlatformWindow> m_window; ///< 应用程序窗口的智能指针。
    std::unique_ptr<GraphicsBackend> m_graphicsBackend; ///< 图形后端的智能指针。
    std::unique_ptr<RenderSystem> m_renderSystem; ///< 渲染系统的智能指针。
    EngineContext m_context; ///< 引擎上下文，包含全局可访问的数据。
    std::string m_title; ///< 应用程序窗口的标题。
    bool m_isRunning = true; ///< 应用程序是否正在运行的标志。
    ApplicationConfig m_config; ///< 应用程序的配置设置。
};

#endif
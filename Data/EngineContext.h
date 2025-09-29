#ifndef ENGINECONTEXT_H
#define ENGINECONTEXT_H
#include "CommandQueue.h"
#include "DynamicArray.h"
#include "SDL3/SDL_events.h"
class SceneRenderer;
class RuntimeTextureManager;
class RuntimeMaterialManager;
class SceneManager;
class AssetManager;

class PlatformWindow;
class GraphicsBackend;
class RenderSystem;
#include "../Components/Core.h"

/**
 * @brief 输入状态结构体，用于存储用户输入信息。
 */
struct InputState
{
    ECS::Vector2i mousePosition; ///< 鼠标当前位置。
    bool isLeftMouseDown = false; ///< 左键是否按下。
    bool isRightMouseDown = false; ///< 右键是否按下。
};

/**
 * @brief 应用程序运行模式枚举。
 */
enum class ApplicationMode
{
    Editor, ///< 编辑器模式。
    PIE, ///< 运行时预览模式 (Play In Editor)。
    Runtime ///< 独立运行时模式。
};

/**
 * @brief 引擎上下文结构体，包含引擎运行时的核心组件和状态。
 */
struct EngineContext
{
    GraphicsBackend* graphicsBackend = nullptr; ///< 图形后端接口指针。
    RenderSystem* renderSystem = nullptr; ///< 渲染系统指针。
    PlatformWindow* window = nullptr; ///< 窗口系统指针。
    SceneRenderer* sceneRenderer = nullptr; ///< 场景渲染器指针。
    InputState inputState; ///< 当前输入状态。
    float currentFps = 60.0f; ///< 当前帧率。
    ECS::RectF sceneViewRect; ///< 场景视图的矩形区域。
    bool isSceneViewFocused = false; ///< 场景视图是否获得焦点。
    ApplicationMode appMode = ApplicationMode::Editor; ///< 应用程序当前运行模式。
    DynamicArray<SDL_Event> frameEvents; ///< 上一帧捕获的事件列表。
    float interpolationAlpha = 1.0f; /// < 用于插值计算的alpha值。
    CommandQueue commandsForSim; ///< 延迟命令队列，用于线程安全的命令执行。
    CommandQueue commandsForRender;
};
#endif

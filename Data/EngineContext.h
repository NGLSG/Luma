#ifndef ENGINECONTEXT_H
#define ENGINECONTEXT_H

#include "CommandQueue.h"
#include "DynamicArray.h"
#include "SDL3/SDL_events.h"
#include <vector> // 确保包含 <vector>
#include <mutex>  // 确保包含 <mutex>

class SceneRenderer;
class RuntimeTextureManager;
class RuntimeMaterialManager;
class SceneManager;
class AssetManager;

class PlatformWindow;
class GraphicsBackend;
class RenderSystem;
#include "../Components/Core.h"
#include "../Components/LightingTypes.h"

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
    ApplicationMode *appMode; ///< 应用程序当前运行模式。
    float interpolationAlpha = 1.0f; /// < 用于插值计算的alpha值。
    CommandQueue commandsForSim; ///< 延迟命令队列，用于线程安全的命令执行。
    CommandQueue commandsForRender;

    // 线程安全的事件处理：
    // eventsWriting 由主线程（渲染线程）在 PollEvents 期间填充。
    // eventsForSim 由模拟线程在执行 'commandsForSim' 队列时更新，并在 Update 期间读取。
    std::vector<SDL_Event> eventsForSim; ///< 供模拟线程安全读取的事件列表。
    std::vector<SDL_Event> eventsWriting; ///< 供主线程（轮询）写入的事件列表。
    
    // 后处理效果预览开关 (Requirements: 13.4)
    bool postProcessBloomEnabled = true;           ///< Bloom 效果开关
    bool postProcessLightShaftsEnabled = true;     ///< 光束效果开关
    bool postProcessFogEnabled = true;             ///< 雾效开关
    bool postProcessToneMappingEnabled = true;     ///< 色调映射开关
    bool postProcessColorGradingEnabled = true;    ///< 颜色分级开关
    
    // 质量等级快速切换 (Requirements: 13.5)
    ECS::QualityLevel currentQualityLevel = ECS::QualityLevel::High; ///< 当前质量等级
};

#endif // ENGINECONTEXT_H
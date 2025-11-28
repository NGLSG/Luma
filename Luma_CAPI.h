#ifndef LUMA_CAPI_H
#define LUMA_CAPI_H

#include <cstdint>
#include <cstddef>

#if ! defined(LUMA_DISABLE_SCRIPTING)
#include "Scripting/ManagedHost.h"
#endif
#include "Components/AssetHandle.h"
#include "RuntimeAsset/RuntimeScene.h"

/**
 * @brief Luma 场景的句柄类型。
 */
typedef struct LumaScene_t* LumaSceneHandle;

/**
 * @brief Luma 实体的句柄类型。
 */
typedef uint32_t LumaEntityHandle;

/**
 * @brief 表示一个二维整数向量的 C API 结构体。
 */
struct Vector2i_CAPI
{
    int x; /**< X 坐标。 */
    int y; /**< Y 坐标。 */
};

/**
 * @brief 表示一个二维浮点向量的 C API 结构体。
 */
struct Vector2f_CAPI
{
    float x; /**< X 坐标。 */
    float y; /**< Y 坐标。 */
};

/**
 * @brief GUID 的 C API 表示。
 */
struct Guid_CAPI
{
    uint64_t data1; /**< GUID 高位。 */
    uint64_t data2; /**< GUID 低位。 */
};

/**
 * @brief 射线检测结果的 C API 结构体。
 */
struct RayCastResult_CAPI
{
    LumaEntityHandle hitEntity; /**< 命中的实体句柄。 */
    Vector2f_CAPI point; /**< 命中点的世界坐标。 */
    Vector2f_CAPI normal; /**< 命中点处的法线向量。 */
    float fraction; /**< 命中点在射线上的分数位置 (0 到 1)。 */
};

/**
 * @brief 力模式枚举的 C API 版本。
 */
typedef enum
{
    ForceMode_Force, /**< 持续力模式。 */
    ForceMode_Impulse /**< 冲量模式。 */
} ForceMode_CAPI;

/**
 * @brief 可序列化事件目标的 C API 表示。
 */
struct SerializableEventTarget_CAPI
{
    Guid_CAPI targetEntityGuid; /**< 目标实体 GUID。 */
    const char* targetComponentName; /**< 目标组件名称。 */
    const char* targetMethodName; /**< 目标方法名。 */
};

/**
 * @brief 音频播放描述的 C API 兼容版本。
 */
struct PlayDesc_CAPI
{
    AssetHandle audioHandle; /**< 音频资源的句柄。 */
    bool loop; /**< 是否循环。 */
    float volume; /**< 音量。 */
    bool spatial; /**< 是否启用空间音频。 */
    float sourceX, sourceY, sourceZ; /**< 音源世界坐标。 */
    float minDistance; /**< 最小衰减距离。 */
    float maxDistance; /**< 最大衰减距离。 */
};

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 平台相关 API
// =============================================================================

/**
 * @brief 检查是否拥有指定的权限。
 * @param[in] permissions 权限名称数组。
 * @param[in] count 权限数量。
 * @return 如果拥有所有权限则返回 true，否则返回 false。
 */
LUMA_API bool Platform_HasPermissions(const char** permissions, int count);

/**
 * @brief 请求指定的权限。
 * @param[in] permissions 权限名称数组。
 * @param[in] count 权限数量。
 * @return 如果权限请求成功则返回 true，否则返回 false。
 */
LUMA_API bool Platform_RequestPermissions(const char** permissions, int count);

// =============================================================================
// 实体组件 API
// =============================================================================

/**
 * @brief 检查实体是否拥有指定名称的组件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 组件的名称。
 * @return 如果实体拥有该组件则返回 true，否则返回 false。
 */
LUMA_API bool Entity_HasComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 为实体添加一个指定名称的组件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 要添加的组件的名称。
 * @return 指向新添加组件数据的指针，如果失败则返回 nullptr。
 */
LUMA_API void* Entity_AddComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 获取实体上指定名称组件的数据指针。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 要获取的组件的名称。
 * @return 指向组件数据的指针，如果实体没有该组件则返回 nullptr。
 */
LUMA_API void* Entity_GetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 从实体中移除指定名称的组件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 要移除的组件的名称。
 */
LUMA_API void Entity_RemoveComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 设置实体上指定名称组件的数据。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 要设置的组件的名称。
 * @param[in] componentData 指向组件新数据的指针。
 * @param[in] dataSize 组件数据的大小。
 */
LUMA_API void Entity_SetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName,
                                  void* componentData, size_t dataSize);

/**
 * @brief 设置组件的特定属性值。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 组件名称。
 * @param[in] propertyName 属性名称。
 * @param[in] valueData 指向属性值数据的指针。
 */
LUMA_API void Entity_SetComponentProperty(LumaSceneHandle scene, LumaEntityHandle entity,
                                          const char* componentName, const char* propertyName, void* valueData);

/**
 * @brief 获取组件的特定属性值。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] componentName 组件名称。
 * @param[in] propertyName 属性名称。
 * @param[out] valueData 用于接收属性值的指针。
 */
LUMA_API void Entity_GetComponentProperty(LumaSceneHandle scene, LumaEntityHandle entity,
                                          const char* componentName, const char* propertyName, void* valueData);

// =============================================================================
// 日志 API
// =============================================================================

/**
 * @brief 定义 Luma 日志的级别。
 */
typedef enum
{
    LumaLogLevel_Trace, /**< 跟踪级别日志。 */
    LumaLogLevel_Info, /**< 信息级别日志。 */
    LumaLogLevel_Warning, /**< 警告级别日志。 */
    LumaLogLevel_Error, /**< 错误级别日志。 */
    LumaLogLevel_Critical /**< 严重级别日志。 */
} LumaLogLevel;

/**
 * @brief 记录一条日志消息。
 * @param[in] level 日志级别。
 * @param[in] message 要记录的消息。
 */
LUMA_API void Logger_Log(LumaLogLevel level, const char* message);

// =============================================================================
// 脚本 API
// =============================================================================

/**
 * @brief 调用一个托管方法。
 * @param[in] handle 托管对象的垃圾回收句柄。
 * @param[in] methodName 要调用的方法名称。
 * @param[in] argsAsYaml 以 YAML 格式表示的方法参数。
 */
LUMA_API void Scripting_InvokeMethod(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml);

// =============================================================================
// 事件 API
// =============================================================================

/**
 * @brief 触发一个不带参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 */
LUMA_API void Event_Invoke_Void(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName);

/**
 * @brief 触发一个带字符串参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 * @param[in] arg1 字符串参数。
 */
LUMA_API void Event_Invoke_String(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                  const char* arg1);

/**
 * @brief 触发一个带整数参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 * @param[in] arg1 整数参数。
 */
LUMA_API void Event_Invoke_Int(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName, int arg1);

/**
 * @brief 触发一个带浮点参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 * @param[in] arg1 浮点参数。
 */
LUMA_API void Event_Invoke_Float(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                 float arg1);

/**
 * @brief 触发一个带布尔参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 * @param[in] arg1 布尔参数。
 */
LUMA_API void Event_Invoke_Bool(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                bool arg1);

/**
 * @brief 触发一个带 YAML 格式参数的事件。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] eventName 事件的名称。
 * @param[in] argsAsYaml 以 YAML 格式表示的事件参数。
 */
LUMA_API void Event_InvokeWithArgs(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                   const char* argsAsYaml);

// =============================================================================
// 引擎运行时 API
// =============================================================================

/**
 * @brief 使用原生窗口初始化渲染后端。
 * @param[in] aNativeWindow 原生窗口指针（在 Android 上传入 ANativeWindow*）。
 * @param[in] width 窗口宽度。
 * @param[in] height 窗口高度。
 * @return 初始化成功返回 true，否则返回 false。
 */
LUMA_API bool Engine_InitWithANativeWindow(void* aNativeWindow, int width, int height);

/**
 * @brief 关闭引擎运行时。
 */
LUMA_API void Engine_Shutdown();

/**
 * @brief 执行一帧渲染。
 * @param[in] dt 增量时间（秒）。
 */
LUMA_API void Engine_Frame(float dt);

// =============================================================================
// 场景管理 API
// =============================================================================

/**
 * @brief 获取当前场景的句柄。
 * @return 当前场景的句柄。
 */
LUMA_API LumaSceneHandle SceneManager_GetCurrentScene();

/**
 * @brief 同步加载一个场景。
 * @param[in] sceneGuid 场景的全局唯一标识符 (GUID)。
 * @return 如果场景加载成功则返回 true，否则返回 false。
 */
LUMA_API bool SceneManager_LoadScene(const char* sceneGuid);

/**
 * @brief 异步加载一个场景。
 * @param[in] sceneGuid 场景的全局唯一标识符 (GUID)。
 */
LUMA_API void SceneManager_LoadSceneAsync(const char* sceneGuid);

/**
 * @brief 获取当前场景的 GUID。
 * @return 当前场景的 GUID 字符串。
 */
LUMA_API const char* SceneManager_GetCurrentSceneGuid();

/**
 * @brief 在指定场景中通过 GUID 查找游戏对象。
 * @param[in] scene 场景句柄。
 * @param[in] guid 游戏对象的全局唯一标识符 (GUID)。
 * @return 找到的游戏对象的句柄，如果未找到则返回无效句柄。
 */
LUMA_API LumaEntityHandle Scene_FindGameObjectByGuid(LumaSceneHandle scene, const char* guid);

/**
 * @brief 在指定场景中创建一个新的游戏对象。
 * @param[in] scene 场景句柄。
 * @param[in] name 新游戏对象的名称。
 * @return 新创建的游戏对象的句柄。
 */
LUMA_API LumaEntityHandle Scene_CreateGameObject(LumaSceneHandle scene, const char* name);

// =============================================================================
// 游戏对象 API
// =============================================================================

/**
 * @brief 获取游戏对象的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 游戏实体句柄。
 * @return 游戏对象的名称字符串。
 */
LUMA_API const char* GameObject_GetName(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置游戏对象的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 游戏实体句柄。
 * @param[in] name 新的名称。
 */
LUMA_API void GameObject_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);

/**
 * @brief 检查游戏对象是否处于激活状态。
 * @param[in] scene 场景句柄。
 * @param[in] entity 游戏实体句柄。
 * @return 如果游戏对象激活则返回 true，否则返回 false。
 */
LUMA_API bool GameObject_IsActive(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置游戏对象的激活状态。
 * @param[in] scene 场景句柄。
 * @param[in] entity 游戏实体句柄。
 * @param[in] active 要设置的激活状态。
 */
LUMA_API void GameObject_SetActive(LumaSceneHandle scene, LumaEntityHandle entity, bool active);

// =============================================================================
// 脚本组件 API
// =============================================================================

/**
 * @brief 获取脚本组件的托管垃圾回收句柄。
 * @param[in] componentPtr 指向脚本组件数据的指针。
 * @param[in] typeName 类型名称。
 * @return 托管垃圾回收句柄。
 */
LUMA_API intptr_t ScriptComponent_GetGCHandle(void* componentPtr, const char* typeName);

/**
 * @brief 获取实体上所有脚本组件的垃圾回收句柄。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[out] outHandles 用于接收句柄的数组。
 * @param[in] count 数组容量。
 */
LUMA_API void ScriptComponent_GetAllGCHandles(LumaSceneHandle scene, uint32_t entity, intptr_t* outHandles, int count);

/**
 * @brief 获取实体上脚本组件的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[out] outCount 输出脚本组件数量。
 */
LUMA_API void ScriptComponent_GetAllGCHandlesCount(LumaSceneHandle scene, uint32_t entity, int* outCount);

// =============================================================================
// 动画控制器 API
// =============================================================================

/**
 * @brief 播放指定实体的动画。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] animationName 动画的名称。
 * @param[in] speed 动画播放速度。
 * @param[in] transitionDuration 动画过渡持续时间。
 */
LUMA_API void AnimationController_Play(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName,
                                       float speed, float transitionDuration);

/**
 * @brief 停止指定实体的所有动画。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void AnimationController_Stop(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 检查指定实体的某个动画是否正在播放。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] animationName 动画的名称。
 * @return 如果动画正在播放则返回 true，否则返回 false。
 */
LUMA_API bool AnimationController_IsPlaying(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName);

/**
 * @brief 设置动画控制器中的浮点参数。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 参数的名称。
 * @param[in] value 要设置的浮点值。
 */
LUMA_API void AnimationController_SetFloat(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                           float value);

/**
 * @brief 设置动画控制器中的布尔参数。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 参数的名称。
 * @param[in] value 要设置的布尔值。
 */
LUMA_API void AnimationController_SetBool(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                          bool value);

/**
 * @brief 设置动画控制器中的整数参数。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 参数的名称。
 * @param[in] value 要设置的整数值。
 */
LUMA_API void AnimationController_SetInt(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                         int value);

/**
 * @brief 设置动画控制器中的触发器。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 触发器的名称。
 */
LUMA_API void AnimationController_SetTrigger(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);

/**
 * @brief 设置动画控制器的帧率。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] frameRate 要设置的帧率。
 */
LUMA_API void AnimationController_SetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity, float frameRate);

/**
 * @brief 获取动画控制器的帧率。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 当前的帧率。
 */
LUMA_API float AnimationController_GetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity);

// =============================================================================
// 光标输入 API
// =============================================================================

/**
 * @brief 获取光标的当前位置。
 * @return 光标的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetPosition();

/**
 * @brief 获取光标的移动增量。
 * @return 光标的二维整数移动增量。
 */
LUMA_API Vector2i_CAPI Cursor_GetDelta();

/**
 * @brief 获取滚轮的滚动增量。
 * @return 滚轮的二维浮点滚动增量。
 */
LUMA_API Vector2f_CAPI Cursor_GetScrollDelta();

/**
 * @brief 检查鼠标左键是否被按下（仅在按下瞬间为 true）。
 * @return 如果鼠标左键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonPressed();

/**
 * @brief 检查鼠标左键是否处于按住状态。
 * @return 如果鼠标左键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonDown();

/**
 * @brief 检查鼠标左键是否被释放（仅在释放瞬间为 true）。
 * @return 如果鼠标左键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonUp();

/**
 * @brief 获取鼠标左键点击时的位置。
 * @return 鼠标左键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetLeftClickPosition();

/**
 * @brief 检查鼠标右键是否被按下（仅在按下瞬间为 true）。
 * @return 如果鼠标右键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonPressed();

/**
 * @brief 检查鼠标右键是否处于按住状态。
 * @return 如果鼠标右键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonDown();

/**
 * @brief 检查鼠标右键是否被释放（仅在释放瞬间为 true）。
 * @return 如果鼠标右键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonUp();

/**
 * @brief 获取鼠标右键点击时的位置。
 * @return 鼠标右键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetRightClickPosition();

/**
 * @brief 检查鼠标中键是否被按下（仅在按下瞬间为 true）。
 * @return 如果鼠标中键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonPressed();

/**
 * @brief 检查鼠标中键是否处于按住状态。
 * @return 如果鼠标中键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonDown();

/**
 * @brief 检查鼠标中键是否被释放（仅在释放瞬间为 true）。
 * @return 如果鼠标中键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonUp();

/**
 * @brief 获取鼠标中键点击时的位置。
 * @return 鼠标中键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetMiddleClickPosition();

// =============================================================================
// 键盘输入 API
// =============================================================================

/**
 * @brief 检查键盘上的某个键是否被按下（仅在按下瞬间为 true）。
 * @param[in] scancode 键的扫描码。
 * @return 如果键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyPressed(int scancode);

/**
 * @brief 检查键盘上的某个键是否处于按住状态。
 * @param[in] scancode 键的扫描码。
 * @return 如果键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyDown(int scancode);

/**
 * @brief 检查键盘上的某个键是否被释放（仅在释放瞬间为 true）。
 * @param[in] scancode 键的扫描码。
 * @return 如果键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyUp(int scancode);

// =============================================================================
// 作业系统 API
// =============================================================================

/**
 * @brief 作业句柄的 C API 类型。
 */
typedef struct JobHandle_t* JobHandle_CAPI;

/**
 * @brief 托管作业回调函数的类型定义。
 * @param[in] context 用户提供的上下文数据。
 */
typedef void (*ManagedJobCallback)(void* context);

/**
 * @brief 注册一个用于释放托管垃圾回收句柄的回调函数。
 * @param[in] freeCallback 释放回调函数。
 */
LUMA_API void JobSystem_RegisterGCHandleFreeCallback(ManagedJobCallback freeCallback);

/**
 * @brief 调度一个作业。
 * @param[in] callback 作业执行的回调函数。
 * @param[in] context 传递给回调函数的上下文数据。
 * @return 作业的句柄。
 */
LUMA_API JobHandle_CAPI JobSystem_Schedule(ManagedJobCallback callback, void* context);

/**
 * @brief 完成一个指定的作业。
 * @param[in] handle 要完成的作业句柄。
 */
LUMA_API void JobSystem_Complete(JobHandle_CAPI handle);

/**
 * @brief 完成一组指定的作业。
 * @param[in] handles 作业句柄数组。
 * @param[in] count 数组中作业句柄的数量。
 */
LUMA_API void JobSystem_CompleteAll(JobHandle_CAPI* handles, int count);

/**
 * @brief 获取作业系统中的线程数量。
 * @return 作业系统使用的线程数量。
 */
LUMA_API int JobSystem_GetThreadCount();

// =============================================================================
// SIMD 向量运算 API
// =============================================================================

/**
 * @brief 使用 SIMD 进行向量加法。
 * @param[in] a 输入向量 A。
 * @param[in] b 输入向量 B。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorAdd(const float* a, const float* b, float* result, size_t count);

/**
 * @brief 使用 SIMD 进行向量乘法。
 * @param[in] a 输入向量 A。
 * @param[in] b 输入向量 B。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorMultiply(const float* a, const float* b, float* result, size_t count);

/**
 * @brief 使用 SIMD 计算向量点积。
 * @param[in] a 输入向量 A。
 * @param[in] b 输入向量 B。
 * @param[in] count 元素数量。
 * @return 点积结果。
 */
LUMA_API float SIMDVectorDotProduct(const float* a, const float* b, size_t count);

/**
 * @brief 使用 SIMD 进行向量乘加运算 (a * b + c)。
 * @param[in] a 输入向量 A。
 * @param[in] b 输入向量 B。
 * @param[in] c 输入向量 C。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count);

/**
 * @brief 使用 SIMD 计算向量平方根。
 * @param[in] input 输入向量。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorSqrt(const float* input, float* result, size_t count);

/**
 * @brief 使用 SIMD 计算向量倒数。
 * @param[in] input 输入向量。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorReciprocal(const float* input, float* result, size_t count);

/**
 * @brief 使用 SIMD 获取向量中的最大值。
 * @param[in] input 输入向量。
 * @param[in] count 元素数量。
 * @return 最大值。
 */
LUMA_API float SIMDVectorMax(const float* input, size_t count);

/**
 * @brief 使用 SIMD 获取向量中的最小值。
 * @param[in] input 输入向量。
 * @param[in] count 元素数量。
 * @return 最小值。
 */
LUMA_API float SIMDVectorMin(const float* input, size_t count);

/**
 * @brief 使用 SIMD 计算向量绝对值。
 * @param[in] input 输入向量。
 * @param[out] result 输出结果向量。
 * @param[in] count 元素数量。
 */
LUMA_API void SIMDVectorAbs(const float* input, float* result, size_t count);

/**
 * @brief 使用 SIMD 旋转点集。
 * @param[in] points_x 输入点的 X 坐标数组。
 * @param[in] points_y 输入点的 Y 坐标数组。
 * @param[in] sin_vals 正弦值数组。
 * @param[in] cos_vals 余弦值数组。
 * @param[out] result_x 输出点的 X 坐标数组。
 * @param[out] result_y 输出点的 Y 坐标数组。
 * @param[in] count 点的数量。
 */
LUMA_API void SIMDVectorRotatePoints(const float* points_x, const float* points_y,
                                     const float* sin_vals, const float* cos_vals,
                                     float* result_x, float* result_y, size_t count);

// =============================================================================
// 物理系统 API
// =============================================================================

/**
 * @brief 对指定实体施加力或冲量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] force 施加的力向量。
 * @param[in] mode 力的应用模式 (Force 或 Impulse)。
 */
LUMA_API void Physics_ApplyForce(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI force,
                                 ForceMode_CAPI mode);

/**
 * @brief 执行射线检测。
 * @param[in] scene 场景句柄。
 * @param[in] start 射线起点（像素坐标）。
 * @param[in] end 射线终点（像素坐标）。
 * @param[in] penetrate 是否穿透。
 * @param[out] outHits 用于接收命中结果的数组指针。
 * @param[in] maxHits outHits 数组的最大容量。
 * @param[out] outHitCount 返回实际命中的数量。
 * @return 如果有任何命中，则返回 true。
 */
LUMA_API bool Physics_RayCast(LumaSceneHandle scene, Vector2f_CAPI start, Vector2f_CAPI end, bool penetrate,
                              RayCastResult_CAPI* outHits, int maxHits, int* outHitCount);

/**
 * @brief 执行圆形区域检测。
 * @param[in] scene 场景句柄。
 * @param[in] center 圆形中心（像素坐标）。
 * @param[in] radius 圆形半径（像素）。
 * @param[in] tags 用于过滤的标签数组。
 * @param[in] tagCount 标签数量。
 * @param[out] outHit 用于接收最近命中结果的指针。
 * @return 如果有任何命中，则返回 true。
 */
LUMA_API bool Physics_CircleCheck(LumaSceneHandle scene, Vector2f_CAPI center, float radius,
                                  const char* tags[], int tagCount, RayCastResult_CAPI* outHit);

/**
 * @brief 执行圆形区域检测，返回所有碰撞的实体。
 * @param[in] scene 场景句柄。
 * @param[in] center 圆形中心（像素坐标）。
 * @param[in] radius 圆形半径（像素）。
 * @param[in] tags 用于过滤的标签数组（可为空）。
 * @param[in] tagCount 标签数量。
 * @param[out] outEntities 用于接收命中实体的数组。
 * @param[in] maxEntities 数组最大容量。
 * @param[out] outEntityCount 输出：实际命中的实体数量。
 * @return 如果有任何命中，则返回 true。
 */
LUMA_API bool Physics_OverlapCircle(LumaSceneHandle scene, Vector2f_CAPI center, float radius,
                                    const char* tags[], int tagCount,
                                    LumaEntityHandle* outEntities, int maxEntities, int* outEntityCount);

// =============================================================================
// 音频管理 API
// =============================================================================

/**
 * @brief 根据描述播放一个音频。
 * @param[in] desc 音频播放的描述。
 * @return 返回一个唯一的 voiceId，失败则返回 0。
 */
LUMA_API uint32_t AudioManager_Play(PlayDesc_CAPI desc);

/**
 * @brief 停止指定 voiceId 的音频。
 * @param[in] voiceId 要停止的音频 ID。
 */
LUMA_API void AudioManager_Stop(uint32_t voiceId);

/**
 * @brief 停止所有正在播放的音频。
 */
LUMA_API void AudioManager_StopAll();

/**
 * @brief 检查指定 voiceId 的音频是否播放完毕。
 * @param[in] voiceId 要检查的音频 ID。
 * @return 如果已结束或无效，返回 true。
 */
LUMA_API bool AudioManager_IsFinished(uint32_t voiceId);

/**
 * @brief 设置指定 voiceId 音频的音量。
 * @param[in] voiceId 音频 ID。
 * @param[in] volume 新音量 (0.0f - 1.0f)。
 */
LUMA_API void AudioManager_SetVolume(uint32_t voiceId, float volume);

/**
 * @brief 设置指定 voiceId 音频的循环状态。
 * @param[in] voiceId 音频 ID。
 * @param[in] loop 是否循环。
 */
LUMA_API void AudioManager_SetLoop(uint32_t voiceId, bool loop);

/**
 * @brief 设置指定 voiceId 音频的位置。
 * @param[in] voiceId 音频 ID。
 * @param[in] x 新的世界坐标 X。
 * @param[in] y 新的世界坐标 Y。
 * @param[in] z 新的世界坐标 Z。
 */
LUMA_API void AudioManager_SetVoicePosition(uint32_t voiceId, float x, float y, float z);

// =============================================================================
// 文本组件 API
// =============================================================================

/**
 * @brief 设置文本组件的文本内容。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] text 文本内容。
 */
LUMA_API void TextComponent_SetText(LumaSceneHandle scene, LumaEntityHandle entity, const char* text);

/**
 * @brief 设置文本组件的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 名称。
 */
LUMA_API void TextComponent_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);

/**
 * @brief 获取文本组件的文本内容。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 文本内容字符串。
 * @note 返回的指针生命周期很短，调用方必须立即使用并复制，不能长期持有。
 */
LUMA_API const char* TextComponent_GetText(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取文本组件的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 名称字符串。
 * @note 返回的指针生命周期很短，调用方必须立即使用并复制，不能长期持有。
 */
LUMA_API const char* TextComponent_GetName(LumaSceneHandle scene, LumaEntityHandle entity);

// =============================================================================
// 标签组件 API
// =============================================================================

/**
 * @brief 获取标签组件的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 标签名称字符串。
 */
LUMA_API const char* TagComponent_GetName(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置标签组件的名称。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] name 标签名称。
 */
LUMA_API void TagComponent_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);

// =============================================================================
// 输入文本组件 API
// =============================================================================

/**
 * @brief 获取输入文本组件的文本内容。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 文本内容字符串。
 */
LUMA_API const char* InputTextComponent_GetText(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置输入文本组件的文本内容。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] text 文本内容。
 */
LUMA_API void InputTextComponent_SetText(LumaSceneHandle scene, LumaEntityHandle entity, const char* text);

/**
 * @brief 获取输入文本组件的占位符文本。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 占位符文本字符串。
 */
LUMA_API const char* InputTextComponent_GetPlaceholder(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置输入文本组件的占位符文本。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] text 占位符文本。
 */
LUMA_API void InputTextComponent_SetPlaceholder(LumaSceneHandle scene, LumaEntityHandle entity, const char* text);

// =============================================================================
// 按钮组件事件目标 API
// =============================================================================

/**
 * @brief 获取按钮组件 OnClick 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ButtonComponent_GetOnClickTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取按钮组件指定索引的 OnClick 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ButtonComponent_GetOnClickTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                               SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除按钮组件的所有 OnClick 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ButtonComponent_ClearOnClickTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为按钮组件添加 OnClick 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ButtonComponent_AddOnClickTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                               const char* componentName, const char* methodName);

// =============================================================================
// 输入文本组件事件目标 API
// =============================================================================

/**
 * @brief 获取输入文本组件 OnTextChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int InputTextComponent_GetOnTextChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取输入文本组件指定索引的 OnTextChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool InputTextComponent_GetOnTextChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除输入文本组件的所有 OnTextChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void InputTextComponent_ClearOnTextChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为输入文本组件添加 OnTextChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void InputTextComponent_AddOnTextChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid, const char* componentName,
                                                        const char* methodName);

/**
 * @brief 获取输入文本组件 OnSubmit 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int InputTextComponent_GetOnSubmitTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取输入文本组件指定索引的 OnSubmit 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool InputTextComponent_GetOnSubmitTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                   SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除输入文本组件的所有 OnSubmit 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void InputTextComponent_ClearOnSubmitTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为输入文本组件添加 OnSubmit 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void InputTextComponent_AddOnSubmitTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                                   const char* componentName, const char* methodName);

// =============================================================================
// 切换按钮组件事件目标 API
// =============================================================================

/**
 * @brief 获取切换按钮组件 OnToggleOn 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ToggleButtonComponent_GetOnToggleOnTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取切换按钮组件指定索引的 OnToggleOn 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ToggleButtonComponent_GetOnToggleOnTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除切换按钮组件的所有 OnToggleOn 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ToggleButtonComponent_ClearOnToggleOnTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为切换按钮组件添加 OnToggleOn 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ToggleButtonComponent_AddOnToggleOnTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName);

/**
 * @brief 获取切换按钮组件 OnToggleOff 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ToggleButtonComponent_GetOnToggleOffTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取切换按钮组件指定索引的 OnToggleOff 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ToggleButtonComponent_GetOnToggleOffTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                         SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除切换按钮组件的所有 OnToggleOff 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ToggleButtonComponent_ClearOnToggleOffTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为切换按钮组件添加 OnToggleOff 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ToggleButtonComponent_AddOnToggleOffTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                         Guid_CAPI targetGuid,
                                                         const char* componentName, const char* methodName);

// =============================================================================
// 单选按钮组件事件目标 API
// =============================================================================

/**
 * @brief 获取单选按钮组件 OnSelected 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int RadioButtonComponent_GetOnSelectedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取单选按钮组件指定索引的 OnSelected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool RadioButtonComponent_GetOnSelectedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                       SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除单选按钮组件的所有 OnSelected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void RadioButtonComponent_ClearOnSelectedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为单选按钮组件添加 OnSelected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void RadioButtonComponent_AddOnSelectedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                       Guid_CAPI targetGuid,
                                                       const char* componentName, const char* methodName);

/**
 * @brief 获取单选按钮组件 OnDeselected 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int RadioButtonComponent_GetOnDeselectedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取单选按钮组件指定索引的 OnDeselected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool RadioButtonComponent_GetOnDeselectedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                         SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除单选按钮组件的所有 OnDeselected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void RadioButtonComponent_ClearOnDeselectedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为单选按钮组件添加 OnDeselected 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void RadioButtonComponent_AddOnDeselectedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                         Guid_CAPI targetGuid,
                                                         const char* componentName, const char* methodName);

// =============================================================================
// 复选框组件事件目标 API
// =============================================================================

/**
 * @brief 获取复选框组件 OnValueChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int CheckBoxComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取复选框组件指定索引的 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool CheckBoxComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除复选框组件的所有 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void CheckBoxComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为复选框组件添加 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void CheckBoxComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName);

// =============================================================================
// 滑块组件事件目标 API
// =============================================================================

/**
 * @brief 获取滑块组件 OnValueChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int SliderComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取滑块组件指定索引的 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool SliderComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                      SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除滑块组件的所有 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void SliderComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为滑块组件添加 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void SliderComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                      Guid_CAPI targetGuid,
                                                      const char* componentName, const char* methodName);

/**
 * @brief 获取滑块组件 OnDragStarted 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int SliderComponent_GetOnDragStartedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取滑块组件指定索引的 OnDragStarted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool SliderComponent_GetOnDragStartedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                     SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除滑块组件的所有 OnDragStarted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void SliderComponent_ClearOnDragStartedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为滑块组件添加 OnDragStarted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void SliderComponent_AddOnDragStartedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                     Guid_CAPI targetGuid,
                                                     const char* componentName, const char* methodName);

/**
 * @brief 获取滑块组件 OnDragEnded 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int SliderComponent_GetOnDragEndedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取滑块组件指定索引的 OnDragEnded 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool SliderComponent_GetOnDragEndedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                   SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除滑块组件的所有 OnDragEnded 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void SliderComponent_ClearOnDragEndedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为滑块组件添加 OnDragEnded 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void SliderComponent_AddOnDragEndedTarget(LumaSceneHandle scene, LumaEntityHandle entity, Guid_CAPI targetGuid,
                                                   const char* componentName, const char* methodName);

// =============================================================================
// 下拉框组件事件目标 API
// =============================================================================

/**
 * @brief 获取下拉框组件 OnSelectionChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ComboBoxComponent_GetOnSelectionChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取下拉框组件指定索引的 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ComboBoxComponent_GetOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                            SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除下拉框组件的所有 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ComboBoxComponent_ClearOnSelectionChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为下拉框组件添加 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ComboBoxComponent_AddOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                            Guid_CAPI targetGuid,
                                                            const char* componentName, const char* methodName);

// =============================================================================
// 进度条组件事件目标 API
// =============================================================================

/**
 * @brief 获取进度条组件 OnValueChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ProgressBarComponent_GetOnValueChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取进度条组件指定索引的 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ProgressBarComponent_GetOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                           SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除进度条组件的所有 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ProgressBarComponent_ClearOnValueChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为进度条组件添加 OnValueChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ProgressBarComponent_AddOnValueChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                           Guid_CAPI targetGuid,
                                                           const char* componentName, const char* methodName);

/**
 * @brief 获取进度条组件 OnCompleted 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ProgressBarComponent_GetOnCompletedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取进度条组件指定索引的 OnCompleted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ProgressBarComponent_GetOnCompletedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除进度条组件的所有 OnCompleted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ProgressBarComponent_ClearOnCompletedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为进度条组件添加 OnCompleted 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ProgressBarComponent_AddOnCompletedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName);

// =============================================================================
// 标签页控件组件事件目标 API
// =============================================================================

/**
 * @brief 获取标签页控件组件 OnTabChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int TabControlComponent_GetOnTabChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取标签页控件组件指定索引的 OnTabChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool TabControlComponent_GetOnTabChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除标签页控件组件的所有 OnTabChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void TabControlComponent_ClearOnTabChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为标签页控件组件添加 OnTabChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void TabControlComponent_AddOnTabChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName);

/**
 * @brief 获取标签页控件组件 OnTabClosed 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int TabControlComponent_GetOnTabClosedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取标签页控件组件指定索引的 OnTabClosed 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool TabControlComponent_GetOnTabClosedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                       SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除标签页控件组件的所有 OnTabClosed 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void TabControlComponent_ClearOnTabClosedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为标签页控件组件添加 OnTabClosed 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void TabControlComponent_AddOnTabClosedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                       Guid_CAPI targetGuid,
                                                       const char* componentName, const char* methodName);

// =============================================================================
// 列表框组件事件目标 API
// =============================================================================

/**
 * @brief 获取列表框组件 OnSelectionChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ListBoxComponent_GetOnSelectionChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取列表框组件指定索引的 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ListBoxComponent_GetOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                           SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除列表框组件的所有 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ListBoxComponent_ClearOnSelectionChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为列表框组件添加 OnSelectionChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ListBoxComponent_AddOnSelectionChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                           Guid_CAPI targetGuid,
                                                           const char* componentName, const char* methodName);

/**
 * @brief 获取列表框组件 OnItemActivated 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ListBoxComponent_GetOnItemActivatedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取列表框组件指定索引的 OnItemActivated 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ListBoxComponent_GetOnItemActivatedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                        SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除列表框组件的所有 OnItemActivated 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ListBoxComponent_ClearOnItemActivatedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为列表框组件添加 OnItemActivated 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ListBoxComponent_AddOnItemActivatedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                        Guid_CAPI targetGuid,
                                                        const char* componentName, const char* methodName);

// =============================================================================
// 滚动视图组件事件目标 API
// =============================================================================

/**
 * @brief 获取滚动视图组件 OnScrollChanged 事件目标的数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 事件目标数量。
 */
LUMA_API int ScrollViewComponent_GetOnScrollChangedTargetCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取滚动视图组件指定索引的 OnScrollChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] index 目标索引。
 * @param[out] outTarget 输出事件目标结构体。
 * @return 如果成功获取则返回 true。
 */
LUMA_API bool ScrollViewComponent_GetOnScrollChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity, int index,
                                                           SerializableEventTarget_CAPI* outTarget);

/**
 * @brief 清除滚动视图组件的所有 OnScrollChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 */
LUMA_API void ScrollViewComponent_ClearOnScrollChangedTargets(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 为滚动视图组件添加 OnScrollChanged 事件目标。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] targetGuid 目标实体 GUID。
 * @param[in] componentName 目标组件名称。
 * @param[in] methodName 目标方法名称。
 */
LUMA_API void ScrollViewComponent_AddOnScrollChangedTarget(LumaSceneHandle scene, LumaEntityHandle entity,
                                                           Guid_CAPI targetGuid, const char* componentName,
                                                           const char* methodName);

// =============================================================================
// 多边形碰撞体组件 API
// =============================================================================

/**
 * @brief 获取多边形碰撞体的顶点数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 顶点数量。
 */
LUMA_API int PolygonCollider_GetVertexCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取多边形碰撞体的所有顶点。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[out] outVertices 用于接收顶点数据的数组。
 */
LUMA_API void PolygonCollider_GetVertices(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI* outVertices);

/**
 * @brief 设置多边形碰撞体的顶点。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] vertices 顶点数据数组。
 * @param[in] count 顶点数量。
 */
LUMA_API void PolygonCollider_SetVertices(LumaSceneHandle scene, LumaEntityHandle entity, const Vector2f_CAPI* vertices,
                                          int count);

// =============================================================================
// 边缘碰撞体组件 API
// =============================================================================

/**
 * @brief 获取边缘碰撞体的顶点数量。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 顶点数量。
 */
LUMA_API int EdgeCollider_GetVertexCount(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 获取边缘碰撞体的所有顶点。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[out] outVertices 用于接收顶点数据的数组。
 */
LUMA_API void EdgeCollider_GetVertices(LumaSceneHandle scene, LumaEntityHandle entity, Vector2f_CAPI* outVertices);

/**
 * @brief 设置边缘碰撞体的顶点。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @param[in] vertices 顶点数据数组。
 * @param[in] count 顶点数量。
 */
LUMA_API void EdgeCollider_SetVertices(LumaSceneHandle scene, LumaEntityHandle entity, const Vector2f_CAPI* vertices,
                                       int count);

// =============================================================================
// 资源管理 API
// =============================================================================

/**
 * @brief 开始资源预加载。
 * @return 如果成功开始预加载则返回 true。
 */
LUMA_API bool AssetManager_StartPreload();

/**
 * @brief 停止资源预加载。
 */
LUMA_API void AssetManager_StopPreload();

/**
 * @brief 检查资源预加载是否已完成。
 * @return 如果预加载完成则返回 true。
 */
LUMA_API bool AssetManager_IsPreloadComplete();

/**
 * @brief 检查资源预加载是否正在运行。
 * @return 如果预加载正在运行则返回 true。
 */
LUMA_API bool AssetManager_IsPreloadRunning();

/**
 * @brief 获取资源预加载的进度。
 * @param[out] outTotalCount 输出：总资源数量。
 * @param[out] outLoadedCount 输出：已加载的资源数量。
 */
LUMA_API void AssetManager_GetPreloadProgress(int* outTotalCount, int* outLoadedCount);

/**
 * @brief 根据资源路径加载资源。
 * @param[in] assetPath 资源路径。
 * @return 资源的 GUID。
 */
LUMA_API Guid_CAPI LoadAsset(const char* assetPath);

// =============================================================================
// 路径工具 API
// =============================================================================

/**
 * @brief 获取可执行文件所在目录。
 * @return 可执行文件目录路径字符串。
 */
LUMA_API const char* PathUtils_GetExecutableDir();

/**
 * @brief 获取持久化数据目录。
 * @return 持久化数据目录路径字符串。
 */
LUMA_API const char* PathUtils_GetPersistentDataDir();

/**
 * @brief 获取缓存目录。
 * @return 缓存目录路径字符串。
 */
LUMA_API const char* PathUtils_GetCacheDir();

/**
 * @brief 获取内容目录。
 * @return 内容目录路径字符串。
 */
LUMA_API const char* PathUtils_GetContentDir();

/**
 * @brief 获取 Android 内部数据目录。
 * @return Android 内部数据目录路径字符串。
 */
LUMA_API const char* PathUtils_GetAndroidInternalDataDir();

/**
 * @brief 获取 Android 外部数据目录。
 * @return Android 外部数据目录路径字符串。
 */
LUMA_API const char* PathUtils_GetAndroidExternalDataDir();

// =============================================================================
// 图形 - WGSL 材质 API
// =============================================================================

/**
 * @brief WGSL 材质句柄类型。
 */
typedef void* WGSLMaterialHandle;

/**
 * @brief 根据资源 GUID 加载 WGSL 材质。
 * @param[in] assetGuid 资源 GUID。
 * @return 材质句柄，如果失败则返回 nullptr。
 */
LUMA_API WGSLMaterialHandle WGSLMaterial_Load(Guid_CAPI assetGuid);

/**
 * @brief 从实体的精灵组件获取 WGSL 材质。
 * @param[in] scene 场景句柄。
 * @param[in] entity 实体句柄。
 * @return 材质句柄。
 */
LUMA_API WGSLMaterialHandle WGSLMaterial_GetFromSprite(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 检查 WGSL 材质是否有效。
 * @param[in] material 材质句柄。
 * @return 如果材质有效则返回 true。
 */
LUMA_API bool WGSLMaterial_IsValid(WGSLMaterialHandle material);

/**
 * @brief 设置材质的浮点 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] value 浮点值。
 */
LUMA_API void WGSLMaterial_SetFloat(WGSLMaterialHandle material, const char* name, float value);

/**
 * @brief 设置材质的整数 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] value 整数值。
 */
LUMA_API void WGSLMaterial_SetInt(WGSLMaterialHandle material, const char* name, int value);

/**
 * @brief 设置材质的 vec2 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] x X 分量。
 * @param[in] y Y 分量。
 */
LUMA_API void WGSLMaterial_SetVec2(WGSLMaterialHandle material, const char* name, float x, float y);

/**
 * @brief 设置材质的 vec3 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] x X 分量。
 * @param[in] y Y 分量。
 * @param[in] z Z 分量。
 */
LUMA_API void WGSLMaterial_SetVec3(WGSLMaterialHandle material, const char* name, float x, float y, float z);

/**
 * @brief 设置材质的 vec4 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] r R 分量（或 X）。
 * @param[in] g G 分量（或 Y）。
 * @param[in] b B 分量（或 Z）。
 * @param[in] a A 分量（或 W）。
 */
LUMA_API void WGSLMaterial_SetVec4(WGSLMaterialHandle material, const char* name, float r, float g, float b, float a);

/**
 * @brief 设置材质的结构体 uniform 值。
 * @param[in] material 材质句柄。
 * @param[in] name uniform 名称。
 * @param[in] data 指向结构体数据的指针。
 * @param[in] size 数据大小（字节）。
 */
LUMA_API void WGSLMaterial_SetUniformStruct(WGSLMaterialHandle material, const char* name, const void* data, int size);

/**
 * @brief 设置材质的纹理绑定。
 * @param[in] material 材质句柄。
 * @param[in] name 纹理名称。
 * @param[in] texture 纹理句柄。
 * @param[in] binding 绑定槽位（默认为 1）。
 * @param[in] group 绑定组（默认为 0）。
 */
LUMA_API void WGSLMaterial_SetTexture(WGSLMaterialHandle material, const char* name, void* texture, uint32_t binding,
                                      uint32_t group);

/**
 * @brief 将所有 uniform 缓冲区更新到 GPU。
 * @param[in] material 材质句柄。
 */
LUMA_API void WGSLMaterial_UpdateUniformBuffer(WGSLMaterialHandle material);

// =============================================================================
// 图形 - 纹理 API
// =============================================================================

/**
 * @brief 纹理句柄类型。
 */
typedef void* TextureAHandle;

/**
 * @brief 根据资源 GUID 加载纹理。
 * @param[in] assetGuid 资源 GUID。
 * @return 纹理句柄。
 */
LUMA_API TextureAHandle TextureA_Load(Guid_CAPI assetGuid);

/**
 * @brief 创建一个指定尺寸的空纹理。
 * @param[in] width 纹理宽度。
 * @param[in] height 纹理高度。
 * @return 纹理句柄。
 */
LUMA_API TextureAHandle TextureA_Create(uint32_t width, uint32_t height);

/**
 * @brief 检查纹理是否有效。
 * @param[in] texture 纹理句柄。
 * @return 如果纹理有效则返回 true。
 */
LUMA_API bool TextureA_IsValid(TextureAHandle texture);

/**
 * @brief 释放通过 TextureA_Create 创建的纹理。
 * @param[in] texture 纹理句柄。
 */
LUMA_API void TextureA_Release(TextureAHandle texture);

/**
 * @brief 获取纹理宽度。
 * @param[in] texture 纹理句柄。
 * @return 纹理宽度。
 */
LUMA_API uint32_t TextureA_GetWidth(TextureAHandle texture);

/**
 * @brief 获取纹理高度。
 * @param[in] texture 纹理句柄。
 * @return 纹理高度。
 */
LUMA_API uint32_t TextureA_GetHeight(TextureAHandle texture);

/**
 * @brief 获取纹理深度（用于 3D 纹理）或数组层数。
 * @param[in] texture 纹理句柄。
 * @return 纹理深度或数组层数。
 */
LUMA_API uint32_t TextureA_GetDepth(TextureAHandle texture);

/**
 * @brief 获取 mipmap 级别数量。
 * @param[in] texture 纹理句柄。
 * @return mipmap 级别数量。
 */
LUMA_API uint32_t TextureA_GetMipLevelCount(TextureAHandle texture);

/**
 * @brief 获取采样数（用于 MSAA）。
 * @param[in] texture 纹理句柄。
 * @return 采样数。
 */
LUMA_API uint32_t TextureA_GetSampleCount(TextureAHandle texture);

/**
 * @brief 检查是否为 3D 纹理。
 * @param[in] texture 纹理句柄。
 * @return 如果是 3D 纹理则返回 true。
 */
LUMA_API bool TextureA_Is3D(TextureAHandle texture);

/**
 * @brief 检查是否为纹理数组。
 * @param[in] texture 纹理句柄。
 * @return 如果是纹理数组则返回 true。
 */
LUMA_API bool TextureA_IsArray(TextureAHandle texture);

// =============================================================================
// 图形 - GPU 缓冲区 API
// =============================================================================

/**
 * @brief GPU 缓冲区句柄类型。
 */
typedef void* GpuBufferHandle;

/**
 * @brief 创建一个新的 GPU 缓冲区。
 * @param[in] size 缓冲区大小（字节）。
 * @param[in] usage 用途标志（wgpu::BufferUsage）。
 * @return 缓冲区句柄。
 */
LUMA_API GpuBufferHandle GpuBuffer_Create(uint32_t size, uint32_t usage);

/**
 * @brief 检查缓冲区是否有效。
 * @param[in] buffer 缓冲区句柄。
 * @return 如果缓冲区有效则返回 true。
 */
LUMA_API bool GpuBuffer_IsValid(GpuBufferHandle buffer);

/**
 * @brief 释放缓冲区。
 * @param[in] buffer 缓冲区句柄。
 */
LUMA_API void GpuBuffer_Release(GpuBufferHandle buffer);

/**
 * @brief 获取缓冲区大小（字节）。
 * @param[in] buffer 缓冲区句柄。
 * @return 缓冲区大小。
 */
LUMA_API uint32_t GpuBuffer_GetSize(GpuBufferHandle buffer);

/**
 * @brief 向缓冲区写入数据。
 * @param[in] buffer 缓冲区句柄。
 * @param[in] data 指向源数据的指针。
 * @param[in] size 要写入的数据大小（0 表示使用缓冲区大小）。
 * @param[in] offset 缓冲区中的偏移量。
 * @return 如果成功则返回 true。
 */
LUMA_API bool GpuBuffer_Write(GpuBufferHandle buffer, const void* data, uint32_t size, uint32_t offset);

// =============================================================================
// 相机 API
// =============================================================================

/**
 * @brief 获取相机位置 X 坐标。
 * @return 相机 X 坐标。
 */
LUMA_API float Camera_GetPositionX();

/**
 * @brief 获取相机位置 Y 坐标。
 * @return 相机 Y 坐标。
 */
LUMA_API float Camera_GetPositionY();

/**
 * @brief 设置相机位置。
 * @param[in] x X 坐标。
 * @param[in] y Y 坐标。
 */
LUMA_API void Camera_SetPosition(float x, float y);

/**
 * @brief 获取相机缩放级别。
 * @return 缩放级别。
 */
LUMA_API float Camera_GetZoom();

/**
 * @brief 设置相机缩放级别。
 * @param[in] zoom 缩放级别。
 */
LUMA_API void Camera_SetZoom(float zoom);

/**
 * @brief 获取相机旋转角度（弧度）。
 * @return 旋转角度（弧度）。
 */
LUMA_API float Camera_GetRotation();

/**
 * @brief 设置相机旋转角度（弧度）。
 * @param[in] rotation 旋转角度（弧度）。
 */
LUMA_API void Camera_SetRotation(float rotation);

/**
 * @brief 获取视口宽度。
 * @return 视口宽度。
 */
LUMA_API float Camera_GetViewportWidth();

/**
 * @brief 获取视口高度。
 * @return 视口高度。
 */
LUMA_API float Camera_GetViewportHeight();

/**
 * @brief 获取清除颜色（RGBA）。
 * @param[out] r 红色分量。
 * @param[out] g 绿色分量。
 * @param[out] b 蓝色分量。
 * @param[out] a 透明度分量。
 */
LUMA_API void Camera_GetClearColor(float* r, float* g, float* b, float* a);

/**
 * @brief 设置清除颜色（RGBA）。
 * @param[in] r 红色分量。
 * @param[in] g 绿色分量。
 * @param[in] b 蓝色分量。
 * @param[in] a 透明度分量。
 */
LUMA_API void Camera_SetClearColor(float r, float g, float b, float a);

/**
 * @brief 将屏幕坐标转换为世界坐标。
 * @param[in] screenX 屏幕 X 坐标。
 * @param[in] screenY 屏幕 Y 坐标。
 * @param[out] worldX 输出的世界 X 坐标。
 * @param[out] worldY 输出的世界 Y 坐标。
 */
LUMA_API void Camera_ScreenToWorld(float screenX, float screenY, float* worldX, float* worldY);

/**
 * @brief 将世界坐标转换为屏幕坐标。
 * @param[in] worldX 世界 X 坐标。
 * @param[in] worldY 世界 Y 坐标。
 * @param[out] screenX 输出的屏幕 X 坐标。
 * @param[out] screenY 输出的屏幕 Y 坐标。
 */
LUMA_API void Camera_WorldToScreen(float worldX, float worldY, float* screenX, float* screenY);

// =============================================================================
// 着色器预热 API
// =============================================================================

/**
 * @brief 开始异步预热所有着色器。
 */
LUMA_API void Shader_StartPreWarmingAsync();

/**
 * @brief 同步预热所有着色器（阻塞直到完成）。
 */
LUMA_API void Shader_PreWarming();

/**
 * @brief 请求停止着色器预热。
 */
LUMA_API void Shader_StopPreWarming();

/**
 * @brief 检查着色器预热是否正在运行。
 * @return 如果正在运行则返回 true。
 */
LUMA_API bool Shader_IsPreWarmingRunning();

/**
 * @brief 检查着色器预热是否已完成。
 * @return 如果已完成则返回 true。
 */
LUMA_API bool Shader_IsPreWarmingComplete();

/**
 * @brief 获取着色器预热状态。
 * @param[out] outTotal 输出：总着色器数量。
 * @param[out] outLoaded 输出：已加载数量。
 * @param[out] outIsRunning 输出：是否正在运行。
 * @param[out] outIsComplete 输出：是否已完成。
 */
LUMA_API void Shader_GetPreWarmingState(int* outTotal, int* outLoaded, bool* outIsRunning, bool* outIsComplete);

// ============================================================================
// ImGui C API (供 C# 插件使用)
// ============================================================================

/**
 * @brief 开始一个 ImGui 窗口。
 */
LUMA_API bool ImGui_Begin(const char* name);

/**
 * @brief 开始一个带关闭按钮的 ImGui 窗口。
 */
LUMA_API bool ImGui_BeginWithOpen(const char* name, bool* open);

/**
 * @brief 结束当前 ImGui 窗口。
 */
LUMA_API void ImGui_End();

/**
 * @brief 显示文本。
 */
LUMA_API void ImGui_Text(const char* text);

/**
 * @brief 显示带颜色的文本。
 */
LUMA_API void ImGui_TextColored(float r, float g, float b, float a, const char* text);

/**
 * @brief 显示禁用样式的文本。
 */
LUMA_API void ImGui_TextDisabled(const char* text);

/**
 * @brief 显示自动换行的文本。
 */
LUMA_API void ImGui_TextWrapped(const char* text);

/**
 * @brief 显示标签文本。
 */
LUMA_API void ImGui_LabelText(const char* label, const char* text);

/**
 * @brief 创建按钮。
 */
LUMA_API bool ImGui_Button(const char* label);

/**
 * @brief 创建指定大小的按钮。
 */
LUMA_API bool ImGui_ButtonEx(const char* label, float width, float height);

/**
 * @brief 创建小按钮。
 */
LUMA_API bool ImGui_SmallButton(const char* label);

/**
 * @brief 创建复选框。
 */
LUMA_API bool ImGui_Checkbox(const char* label, bool* value);

/**
 * @brief 创建文本输入框。
 */
LUMA_API bool ImGui_InputText(const char* label, char* buffer, int bufferSize);

/**
 * @brief 创建浮点输入框。
 */
LUMA_API bool ImGui_InputFloat(const char* label, float* value);

/**
 * @brief 创建整数输入框。
 */
LUMA_API bool ImGui_InputInt(const char* label, int* value);

/**
 * @brief 创建浮点滑动条。
 */
LUMA_API bool ImGui_SliderFloat(const char* label, float* value, float min, float max);

/**
 * @brief 创建整数滑动条。
 */
LUMA_API bool ImGui_SliderInt(const char* label, int* value, int min, int max);

/**
 * @brief 添加分隔线。
 */
LUMA_API void ImGui_Separator();

/**
 * @brief 同一行布局。
 */
LUMA_API void ImGui_SameLine();

/**
 * @brief 同一行布局(带偏移)。
 */
LUMA_API void ImGui_SameLineEx(float offsetFromStartX, float spacing);

/**
 * @brief 添加间距。
 */
LUMA_API void ImGui_Spacing();

/**
 * @brief 添加空白区域。
 */
LUMA_API void ImGui_Dummy(float width, float height);

/**
 * @brief 增加缩进。
 */
LUMA_API void ImGui_Indent();

/**
 * @brief 减少缩进。
 */
LUMA_API void ImGui_Unindent();

/**
 * @brief 创建树节点。
 */
LUMA_API bool ImGui_TreeNode(const char* label);

/**
 * @brief 结束树节点。
 */
LUMA_API void ImGui_TreePop();

/**
 * @brief 创建可折叠标题。
 */
LUMA_API bool ImGui_CollapsingHeader(const char* label);

/**
 * @brief 创建可选择项。
 */
LUMA_API bool ImGui_Selectable(const char* label, bool selected);

/**
 * @brief 开始菜单栏。
 */
LUMA_API bool ImGui_BeginMenuBar();

/**
 * @brief 结束菜单栏。
 */
LUMA_API void ImGui_EndMenuBar();

/**
 * @brief 开始菜单。
 */
LUMA_API bool ImGui_BeginMenu(const char* label);

/**
 * @brief 结束菜单。
 */
LUMA_API void ImGui_EndMenu();

/**
 * @brief 创建菜单项。
 */
LUMA_API bool ImGui_MenuItem(const char* label);

/**
 * @brief 创建带快捷键的菜单项。
 */
LUMA_API bool ImGui_MenuItemEx(const char* label, const char* shortcut, bool selected);

/**
 * @brief 打开弹窗。
 */
LUMA_API void ImGui_OpenPopup(const char* strId);

/**
 * @brief 开始弹窗。
 */
LUMA_API bool ImGui_BeginPopup(const char* strId);

/**
 * @brief 开始模态弹窗。
 */
LUMA_API bool ImGui_BeginPopupModal(const char* name);

/**
 * @brief 结束弹窗。
 */
LUMA_API void ImGui_EndPopup();

/**
 * @brief 关闭当前弹窗。
 */
LUMA_API void ImGui_CloseCurrentPopup();

/**
 * @brief 检查当前项是否被悬停。
 */
LUMA_API bool ImGui_IsItemHovered();

/**
 * @brief 检查当前项是否被点击。
 */
LUMA_API bool ImGui_IsItemClicked();

/**
 * @brief 设置工具提示。
 */
LUMA_API void ImGui_SetTooltip(const char* text);

/**
 * @brief 获取窗口宽度。
 */
LUMA_API float ImGui_GetWindowWidth();

/**
 * @brief 获取窗口高度。
 */
LUMA_API float ImGui_GetWindowHeight();

/**
 * @brief 设置下一个窗口大小。
 */
LUMA_API void ImGui_SetNextWindowSize(float width, float height);

/**
 * @brief 设置下一个窗口位置。
 */
LUMA_API void ImGui_SetNextWindowPos(float x, float y);

/**
 * @brief 推入 ID（整数）。
 */
LUMA_API void ImGui_PushID(int id);

/**
 * @brief 推入 ID（字符串）。
 */
LUMA_API void ImGui_PushIDStr(const char* strId);

/**
 * @brief 弹出 ID。
 */
LUMA_API void ImGui_PopID();

/**
 * @brief 颜色编辑器（RGB）。
 */
LUMA_API bool ImGui_ColorEdit3(const char* label, float* r, float* g, float* b);

/**
 * @brief 颜色编辑器（RGBA）。
 */
LUMA_API bool ImGui_ColorEdit4(const char* label, float* r, float* g, float* b, float* a);

/**
 * @brief 浮点拖拽控件。
 */
LUMA_API bool ImGui_DragFloat(const char* label, float* value, float speed, float min, float max);

/**
 * @brief 整数拖拽控件。
 */
LUMA_API bool ImGui_DragInt(const char* label, int* value, float speed, int min, int max);

/**
 * @brief 进度条。
 */
LUMA_API void ImGui_ProgressBar(float fraction, float width, float height, const char* overlay);

/**
 * @brief 开始子窗口。
 */
LUMA_API bool ImGui_BeginChild(const char* strId, float width, float height, bool border);

/**
 * @brief 结束子窗口。
 */
LUMA_API void ImGui_EndChild();

/**
 * @brief 开始标签栏。
 */
LUMA_API bool ImGui_BeginTabBar(const char* strId);

/**
 * @brief 结束标签栏。
 */
LUMA_API void ImGui_EndTabBar();

/**
 * @brief 开始标签项。
 */
LUMA_API bool ImGui_BeginTabItem(const char* label);

/**
 * @brief 结束标签项。
 */
LUMA_API void ImGui_EndTabItem();

/**
 * @brief 开始表格。
 */
LUMA_API bool ImGui_BeginTable(const char* strId, int columns);

/**
 * @brief 结束表格。
 */
LUMA_API void ImGui_EndTable();

/**
 * @brief 表格下一行。
 */
LUMA_API void ImGui_TableNextRow();

/**
 * @brief 表格下一列。
 */
LUMA_API bool ImGui_TableNextColumn();

/**
 * @brief 设置表格列。
 */
LUMA_API void ImGui_TableSetupColumn(const char* label);

/**
 * @brief 表格头部行。
 */
LUMA_API void ImGui_TableHeadersRow();

/**
 * @brief 显示图像。
 */
LUMA_API void ImGui_Image(void* textureId, float width, float height);

/**
 * @brief 图像按钮。
 */
LUMA_API bool ImGui_ImageButton(const char* strId, void* textureId, float width, float height);

/**
 * @brief C# 友好的单行文本输入框。
 */
LUMA_API bool ImGui_InputTextCallback(const char* label, const char* text, char* outBuffer, int outBufferSize, bool* changed);

/**
 * @brief 多行文本输入框。
 */
LUMA_API bool ImGui_InputTextMultiline(const char* label, char* buffer, int bufferSize, float width, float height);

/**
 * @brief C# 友好的多行文本输入框。
 */
LUMA_API bool ImGui_InputTextMultilineCallback(const char* label, const char* text, char* outBuffer, int outBufferSize, float width, float height, bool* changed);

// ============================================================================
// Plugin C API
// ============================================================================

/**
 * @brief 加载插件。
 */
LUMA_API bool Plugin_Load(const char* dllPath, const char* pluginId);

/**
 * @brief 卸载插件。
 */
LUMA_API bool Plugin_Unload(const char* pluginId);

/**
 * @brief 获取插件数量。
 */
LUMA_API int Plugin_GetCount();

/**
 * @brief 获取插件信息。
 */
LUMA_API bool Plugin_GetInfo(int index, char* idBuffer, int idBufferSize,
                             char* nameBuffer, int nameBufferSize,
                             char* versionBuffer, int versionBufferSize,
                             bool* enabled, bool* loaded);
LUMA_API void Luma_GetCurrentSceneName(char* buffer, int bufferSize);
LUMA_API int Luma_GetEntityCount();
LUMA_API int Luma_GetAssetCount();
LUMA_API bool Luma_AssetExists(const char* path);
LUMA_API bool Luma_IsEditorMode();
LUMA_API bool Luma_IsPlaying();
LUMA_API bool Luma_IsProjectLoaded();
LUMA_API void Luma_GetProjectName(char* buffer, int bufferSize);
LUMA_API void Luma_GetProjectPath(char* buffer, int bufferSize);
LUMA_API void Luma_GetAssetsPath(char* buffer, int bufferSize);
LUMA_API void Luma_LogInfo(const char* message);
LUMA_API void Luma_LogWarn(const char* message);
LUMA_API void Luma_LogError(const char* message);
LUMA_API void Luma_LogDebug(const char* message);
LUMA_API int Luma_GetSelectedEntityCount();
LUMA_API Guid_CAPI Luma_GetSelectedEntityGuid(int index);
LUMA_API void Luma_GetSelectedEntityName(int index, char* buffer, int bufferSize);
#ifdef __cplusplus
}

/**
 * @brief 尝试获取实体上的指定类型组件。
 * @tparam TComponent 组件类型。
 * @param[in] runtimeScene 运行时场景指针。
 * @param[in] entity 实体句柄。
 * @return 指向组件的指针，如果实体没有该组件则返回 nullptr。
 */
template <typename TComponent>
static inline TComponent* TryGetComponent(RuntimeScene* runtimeScene, LumaEntityHandle entity)
{
    if (!runtimeScene) return nullptr;
    return runtimeScene->GetRegistry().try_get<TComponent>((entt::entity)entity);
}
#endif

#endif

#ifndef LUMA_CAPI_H
#define LUMA_CAPI_H

#include <cstdint>
#include <cstddef>

#include "Scripting/CoreCLRHost.h"

/**
 * @brief Luma场景的句柄类型。
 */
typedef struct LumaScene_t* LumaSceneHandle;
/**
 * @brief Luma实体的句柄类型。
 */
typedef uint32_t LumaEntityHandle;

/**
 * @brief 表示一个二维整数向量的C API结构体。
 */
struct Vector2i_CAPI
{
    int x; ///< X坐标。
    int y; ///< Y坐标。
};

/**
 * @brief 表示一个二维浮点向量的C API结构体。
 */
struct Vector2f_CAPI
{
    float x; ///< X坐标。
    float y; ///< Y坐标。
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 检查实体是否拥有指定名称的组件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param componentName 组件的名称。
 * @return 如果实体拥有该组件则返回 true，否则返回 false。
 */
LUMA_API bool Entity_HasComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 为实体添加一个指定名称的组件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param componentName 要添加的组件的名称。
 * @return 指向新添加组件数据的指针，如果失败则返回 nullptr。
 */
LUMA_API void* Entity_AddComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 获取实体上指定名称组件的数据指针。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param componentName 要获取的组件的名称。
 * @return 指向组件数据的指针，如果实体没有该组件则返回 nullptr。
 */
LUMA_API void* Entity_GetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 从实体中移除指定名称的组件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param componentName 要移除的组件的名称。
 */
LUMA_API void Entity_RemoveComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName);

/**
 * @brief 设置实体上指定名称组件的数据。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param componentName 要设置的组件的名称。
 * @param componentData 指向组件新数据的指针。
 */
LUMA_API void Entity_SetComponent(LumaSceneHandle scene, LumaEntityHandle entity, const char* componentName,
                                  void* componentData);

/**
 * @brief 定义Luma日志的级别。
 */
typedef enum
{
    LumaLogLevel_Trace, ///< 跟踪级别日志。
    LumaLogLevel_Info, ///< 信息级别日志。
    LumaLogLevel_Warning, ///< 警告级别日志。
    LumaLogLevel_Error, ///< 错误级别日志。
    LumaLogLevel_Critical ///< 严重级别日志。
} LumaLogLevel;

/**
 * @brief 记录一条日志消息。
 * @param level 日志级别。
 * @param message 要记录的消息。
 */
LUMA_API void Logger_Log(LumaLogLevel level, const char* message);

/**
 * @brief 调用一个托管方法。
 * @param handle 托管对象的垃圾回收句柄。
 * @param methodName 要调用的方法名称。
 * @param argsAsYaml 以YAML格式表示的方法参数。
 */
LUMA_API void Scripting_InvokeMethod(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml);
/**
 * @brief 触发一个不带参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 */
LUMA_API void Event_Invoke_Void(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName);

/**
 * @brief 触发一个带字符串参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 * @param arg1 字符串参数。
 */
LUMA_API void Event_Invoke_String(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                  const char* arg1);
/**
 * @brief 触发一个带整数参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 * @param arg1 整数参数。
 */
LUMA_API void Event_Invoke_Int(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName, int arg1);
/**
 * @brief 触发一个带浮点参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 * @param arg1 浮点参数。
 */
LUMA_API void Event_Invoke_Float(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                 float arg1);
/**
 * @brief 触发一个带布尔参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 * @param arg1 布尔参数。
 */
LUMA_API void Event_Invoke_Bool(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                bool arg1);
/**
 * @brief 触发一个带YAML格式参数的事件。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param eventName 事件的名称。
 * @param argsAsYaml 以YAML格式表示的事件参数。
 */
LUMA_API void Event_InvokeWithArgs(LumaSceneHandle scene, LumaEntityHandle entity, const char* eventName,
                                   const char* argsAsYaml);

/**
 * @brief 获取当前场景的句柄。
 * @return 当前场景的句柄。
 */
LUMA_API LumaSceneHandle SceneManager_GetCurrentScene();

/**
 * @brief 同步加载一个场景。
 * @param sceneGuid 场景的全局唯一标识符 (GUID)。
 * @return 如果场景加载成功则返回 true，否则返回 false。
 */
LUMA_API bool SceneManager_LoadScene(const char* sceneGuid);

/**
 * @brief 异步加载一个场景。
 * @param sceneGuid 场景的全局唯一标识符 (GUID)。
 */
LUMA_API void SceneManager_LoadSceneAsync(const char* sceneGuid);

/**
 * @brief 获取当前场景的GUID。
 * @return 当前场景的GUID字符串。
 */
LUMA_API const char* SceneManager_GetCurrentSceneGuid();

/**
 * @brief 在指定场景中通过GUID查找游戏对象。
 * @param scene 场景句柄。
 * @param guid 游戏对象的全局唯一标识符 (GUID)。
 * @return 找到的游戏对象的句柄，如果未找到则返回无效句柄。
 */
LUMA_API LumaEntityHandle Scene_FindGameObjectByGuid(LumaSceneHandle scene, const char* guid);

/**
 * @brief 在指定场景中创建一个新的游戏对象。
 * @param scene 场景句柄。
 * @param name 新游戏对象的名称。
 * @return 新创建的游戏对象的句柄。
 */
LUMA_API LumaEntityHandle Scene_CreateGameObject(LumaSceneHandle scene, const char* name);

/**
 * @brief 获取游戏对象的名称。
 * @param scene 场景句柄。
 * @param entity 游戏实体句柄。
 * @return 游戏对象的名称字符串。
 */
LUMA_API const char* GameObject_GetName(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 设置游戏对象的名称。
 * @param scene 场景句柄。
 * @param entity 游戏实体句柄。
 * @param name 新的名称。
 */
LUMA_API void GameObject_SetName(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);
/**
 * @brief 检查游戏对象是否处于激活状态。
 * @param scene 场景句柄。
 * @param entity 游戏实体句柄。
 * @return 如果游戏对象激活则返回 true，否则返回 false。
 */
LUMA_API bool GameObject_IsActive(LumaSceneHandle scene, LumaEntityHandle entity);
/**
 * @brief 设置游戏对象的激活状态。
 * @param scene 场景句柄。
 * @param entity 游戏实体句柄。
 * @param active 要设置的激活状态。
 */
LUMA_API void GameObject_SetActive(LumaSceneHandle scene, LumaEntityHandle entity, bool active);

/**
 * @brief 获取脚本组件的托管垃圾回收句柄。
 * @param componentPtr 指向脚本组件数据的指针。
 * @return 托管垃圾回收句柄的指针。
 */
LUMA_API intptr_t ScriptComponent_GetGCHandle(void* componentPtr, const char* typeName);
LUMA_API void ScriptComponent_GetAllGCHandles(LumaSceneHandle scene, uint32_t entity, intptr_t* outHandles, int count);
LUMA_API void ScriptComponent_GetAllGCHandlesCount(LumaSceneHandle scene,uint32_t entity, int* outCount);
/**
 * @brief 播放指定实体的动画。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param animationName 动画的名称。
 * @param speed 动画播放速度。
 * @param transitionDuration 动画过渡持续时间。
 */
LUMA_API void AnimationController_Play(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName,
                                       float speed, float transitionDuration);

/**
 * @brief 停止指定实体的所有动画。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 */
LUMA_API void AnimationController_Stop(LumaSceneHandle scene, LumaEntityHandle entity);

/**
 * @brief 检查指定实体的某个动画是否正在播放。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param animationName 动画的名称。
 * @return 如果动画正在播放则返回 true，否则返回 false。
 */
LUMA_API bool AnimationController_IsPlaying(LumaSceneHandle scene, LumaEntityHandle entity, const char* animationName);

/**
 * @brief 设置动画控制器中的浮点参数。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param name 参数的名称。
 * @param value 要设置的浮点值。
 */
LUMA_API void AnimationController_SetFloat(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                           float value);

/**
 * @brief 设置动画控制器中的布尔参数。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param name 参数的名称。
 * @param value 要设置的布尔值。
 */
LUMA_API void AnimationController_SetBool(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                          bool value);

/**
 * @brief 设置动画控制器中的整数参数。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param name 参数的名称。
 * @param value 要设置的整数值。
 */
LUMA_API void AnimationController_SetInt(LumaSceneHandle scene, LumaEntityHandle entity, const char* name,
                                         int value);
/**
 * @brief 设置动画控制器中的触发器。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param name 触发器的名称。
 */
LUMA_API void AnimationController_SetTrigger(LumaSceneHandle scene, LumaEntityHandle entity, const char* name);

/**
 * @brief 设置动画控制器的帧率。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @param frameRate 要设置的帧率。
 */
LUMA_API void AnimationController_SetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity, float frameRate);

/**
 * @brief 获取动画控制器的帧率。
 * @param scene 场景句柄。
 * @param entity 实体句柄。
 * @return 当前的帧率。
 */
LUMA_API float AnimationController_GetFrameRate(LumaSceneHandle scene, LumaEntityHandle entity);
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
 * @brief 检查鼠标左键是否被按下（仅在按下瞬间为true）。
 * @return 如果鼠标左键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonPressed();
/**
 * @brief 检查鼠标左键是否处于按住状态。
 * @return 如果鼠标左键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonDown();
/**
 * @brief 检查鼠标左键是否被释放（仅在释放瞬间为true）。
 * @return 如果鼠标左键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsLeftButtonUp();
/**
 * @brief 获取鼠标左键点击时的位置。
 * @return 鼠标左键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetLeftClickPosition();

/**
 * @brief 检查鼠标右键是否被按下（仅在按下瞬间为true）。
 * @return 如果鼠标右键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonPressed();
/**
 * @brief 检查鼠标右键是否处于按住状态。
 * @return 如果鼠标右键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonDown();
/**
 * @brief 检查鼠标右键是否被释放（仅在释放瞬间为true）。
 * @return 如果鼠标右键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsRightButtonUp();
/**
 * @brief 获取鼠标右键点击时的位置。
 * @return 鼠标右键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetRightClickPosition();

/**
 * @brief 检查鼠标中键是否被按下（仅在按下瞬间为true）。
 * @return 如果鼠标中键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonPressed();
/**
 * @brief 检查鼠标中键是否处于按住状态。
 * @return 如果鼠标中键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonDown();
/**
 * @brief 检查鼠标中键是否被释放（仅在释放瞬间为true）。
 * @return 如果鼠标中键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Cursor_IsMiddleButtonUp();
/**
 * @brief 获取鼠标中键点击时的位置。
 * @return 鼠标中键点击时的二维整数位置。
 */
LUMA_API Vector2i_CAPI Cursor_GetMiddleClickPosition();

/**
 * @brief 检查键盘上的某个键是否被按下（仅在按下瞬间为true）。
 * @param scancode 键的扫描码。
 * @return 如果键被按下则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyPressed(int scancode);
/**
 * @brief 检查键盘上的某个键是否处于按住状态。
 * @param scancode 键的扫描码。
 * @return 如果键被按住则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyDown(int scancode);
/**
 * @brief 检查键盘上的某个键是否被释放（仅在释放瞬间为true）。
 * @param scancode 键的扫描码。
 * @return 如果键被释放则返回 true，否则返回 false。
 */
LUMA_API bool Keyboard_IsKeyUp(int scancode);

/**
 * @brief 作业句柄的C API类型。
 */
typedef struct JobHandle_t* JobHandle_CAPI;

/**
 * @brief 托管作业回调函数的类型定义。
 * @param context 用户提供的上下文数据。
 */
typedef void (*ManagedJobCallback)(void* context);

/**
 * @brief 注册一个用于释放托管垃圾回收句柄的回调函数。
 * @param freeCallback 释放回调函数。
 */
LUMA_API void JobSystem_RegisterGCHandleFreeCallback(ManagedJobCallback freeCallback);

/**
 * @brief 调度一个作业。
 * @param callback 作业执行的回调函数。
 * @param context 传递给回调函数的上下文数据。
 * @return 作业的句柄。
 */
LUMA_API JobHandle_CAPI JobSystem_Schedule(ManagedJobCallback callback, void* context);

/**
 * @brief 完成一个指定的作业。
 * @param handle 要完成的作业句柄。
 */
LUMA_API void JobSystem_Complete(JobHandle_CAPI handle);

/**
 * @brief 完成一组指定的作业。
 * @param handles 作业句柄数组。
 * @param count 数组中作业句柄的数量。
 */
LUMA_API void JobSystem_CompleteAll(JobHandle_CAPI* handles, int count);

/**
 * @brief 获取作业系统中的线程数量。
 * @return 作业系统使用的线程数量。
 */
LUMA_API int JobSystem_GetThreadCount();

LUMA_API void SIMDVectorAdd(const float* a, const float* b, float* result, size_t count);
LUMA_API void SIMDVectorMultiply(const float* a, const float* b, float* result, size_t count);
LUMA_API float SIMDVectorDotProduct(const float* a, const float* b, size_t count);
LUMA_API void SIMDVectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count);
LUMA_API void SIMDVectorSqrt(const float* input, float* result, size_t count);
LUMA_API void SIMDVectorReciprocal(const float* input, float* result, size_t count);
LUMA_API float SIMDVectorMax(const float* input, size_t count);
LUMA_API float SIMDVectorMin(const float* input, size_t count);
LUMA_API void SIMDVectorAbs(const float* input, float* result, size_t count);
LUMA_API void SIMDVectorRotatePoints(const float* points_x, const float* points_y,
                                     const float* sin_vals, const float* cos_vals,
                                     float* result_x, float* result_y, size_t count);
#ifdef __cplusplus
}
#endif

#endif

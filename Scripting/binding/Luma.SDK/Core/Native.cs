using System;
using System.Runtime.InteropServices;

namespace Luma.SDK;

internal enum LumaLogLevel
{
    Trace,
    Info,
    Warning,
    Error,
    Critical
}





internal static class Native
{
    private const string DllName = "LumaEngine";

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool Entity_HasComponent(IntPtr scene, uint entity, string componentName);

    [DllImport(DllName)]
    internal static extern IntPtr Entity_AddComponent(IntPtr scene, uint entity, string componentName);

    [DllImport(DllName)]
    internal static extern IntPtr Entity_GetComponent(IntPtr scene, uint entity, string componentName);

    [DllImport(DllName)]
    internal static extern void Entity_RemoveComponent(IntPtr scene, uint entity, string componentName);

    [DllImport(DllName)]
    internal static extern void Entity_SetComponent(IntPtr scene, uint entity, string componentName,
        IntPtr componentData);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void Logger_Log(LumaLogLevel level, string message);

    [DllImport(DllName)]
    internal static extern void Event_Invoke_Void(IntPtr scene, uint entity, string eventName);

    [DllImport(DllName)]
    internal static extern void Event_Invoke_String(IntPtr scene, uint entity, string eventName, string arg1);

    [DllImport(DllName)]
    internal static extern void Event_Invoke_Int(IntPtr scene, uint entity, string eventName, int arg1);

    [DllImport(DllName)]
    internal static extern void Event_Invoke_Float(IntPtr scene, uint entity, string eventName, float arg1);

    [DllImport(DllName)]
    internal static extern void Event_Invoke_Bool(IntPtr scene, uint entity, string eventName, bool arg1);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void Event_InvokeWithArgs(IntPtr scenePtr, uint entityId, string eventName,
        string argsAsYaml);

    
    [DllImport(DllName, CharSet = CharSet.Ansi)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool SceneManager_LoadScene(string sceneGuid);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void SceneManager_LoadSceneAsync(string sceneGuid);

    [DllImport(DllName)]
    internal static extern IntPtr SceneManager_GetCurrentScene();

    [DllImport(DllName)]
    internal static extern IntPtr SceneManager_GetCurrentSceneGuid();

    
    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern uint Scene_FindGameObjectByGuid(IntPtr scene, string guid);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern uint Scene_CreateGameObject(IntPtr scene, string name);

    
    [DllImport(DllName)]
    internal static extern IntPtr GameObject_GetName(IntPtr scene, uint entity);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void GameObject_SetName(IntPtr scene, uint entity, string name);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool GameObject_IsActive(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void GameObject_SetActive(IntPtr scene, uint entity, bool active);

    [DllImport(DllName)]
    internal static extern IntPtr ScriptComponent_GetGCHandle(IntPtr componentPtr);

    
    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void AnimationController_Play(IntPtr scene, uint entity, string animationName,
        float speed, float transitionDuration);

    [DllImport(DllName)]
    internal static extern void AnimationController_Stop(IntPtr scene, uint entity);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool AnimationController_IsPlaying(IntPtr scene, uint entity, string animationName);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void AnimationController_SetFloat(IntPtr scene, uint entity, string name,
        float value);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void AnimationController_SetBool(IntPtr scene, uint entity, string name,
        bool value);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void AnimationController_SetInt(IntPtr scene, uint entity, string name,
        int value);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void AnimationController_SetTrigger(IntPtr scene, uint entity, string name);

    [DllImport(DllName)]
    internal static extern void AnimationController_SetFrameRate(IntPtr scene, uint entity, float frameRate);

    #region SIMD Bindings

    [DllImport(DllName)]
    internal static extern void SIMDVectorAdd(float[] a, float[] b, float[] result, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorMultiply(float[] a, float[] b, float[] result, int count);

    [DllImport(DllName)]
    internal static extern float SIMDVectorDotProduct(float[] a, float[] b, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorMultiplyAdd(float[] a, float[] b, float[] c, float[] result, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorSqrt(float[] input, float[] result, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorReciprocal(float[] input, float[] result, int count);

    [DllImport(DllName)]
    internal static extern float SIMDVectorMax(float[] input, int count);

    [DllImport(DllName)]
    internal static extern float SIMDVectorMin(float[] input, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorAbs(float[] input, float[] result, int count);

    [DllImport(DllName)]
    internal static extern void SIMDVectorRotatePoints(float[] points_x, float[] points_y,
                                                       float[] sin_vals, float[] cos_vals,
                                                       float[] result_x, float[] result_y, int count);
    #endregion
}
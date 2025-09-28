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

public enum ForceMode
{
    Force,
    Impulse
};

[StructLayout(LayoutKind.Sequential)]
public struct RaycastHit
{
    public readonly uint hitEntityHandle;
    public readonly Vector2 Point;
    public readonly Vector2 Normal;
    public readonly float Fraction;
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
    internal static extern void Entity_SetComponent(IntPtr scene, uint entity,
        [MarshalAs(UnmanagedType.LPStr)] string componentName,
        IntPtr componentData, IntPtr dataSize);

    [DllImport(DllName)]
    internal static extern void Entity_SetComponentProperty(IntPtr scene, uint entityId, string componentName,
        string propertyName, IntPtr valueData);

    [DllImport(DllName)]
    internal static extern void Entity_GetComponentProperty(IntPtr scene, uint entityId, string componentName,
        string propertyName, IntPtr valueData);

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

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern IntPtr ScriptComponent_GetGCHandle(IntPtr componentPtr, string typeName);


    [DllImport(DllName)]
    internal static extern void ScriptComponent_GetAllGCHandlesCount(IntPtr scenePtr, uint entityId, ref int outCount);

    [DllImport(DllName)]
    internal static extern void ScriptComponent_GetAllGCHandles(IntPtr scenePtr, uint entityId, IntPtr outHandles,
        int outCount);

    [DllImport(DllName)]
    public static extern void Physics_ApplyForce(IntPtr scene, uint entity, Vector2 force, ForceMode mode);

    [DllImport(DllName)]
    public static extern bool Physics_RayCast(IntPtr scene, Vector2 start, Vector2 end, bool penetrate,
        [Out] RaycastHit[] outHits, int maxHits, out int outHitCount);

    [DllImport(DllName)]
    public static extern bool Physics_CircleCheck(IntPtr scene, Vector2 center, float radius,
        [In] string[] tags, int tagCount, out RaycastHit outHit);

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

    #region AudioManager

    [DllImport(DllName)]
    public static extern uint AudioManager_Play(PlayDesc desc);


    [DllImport(DllName)]
    public static extern void AudioManager_Stop(uint voiceId);


    [DllImport(DllName)]
    public static extern void AudioManager_StopAll();


    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    public static extern bool AudioManager_IsFinished(uint voiceId);


    [DllImport(DllName)]
    public static extern void AudioManager_SetVolume(uint voiceId, float volume);


    [DllImport(DllName)]
    public static extern void AudioManager_SetLoop(uint voiceId, [MarshalAs(UnmanagedType.I1)] bool loop);


    [DllImport(DllName)]
    public static extern void AudioManager_SetVoicePosition(uint voiceId, float x, float y, float z);

    #endregion

    [DllImport(DllName)]
    internal static extern void TextComponent_SetText(IntPtr scene, uint entityId, string text);

    [DllImport(DllName)]
    internal static extern IntPtr TextComponent_GetText(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void TextComponent_SetName(IntPtr scene, uint entityId, string name);

    [DllImport(DllName)]
    internal static extern IntPtr TextComponent_GetName(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern IntPtr TagComponent_GetName(IntPtr scene, uint entityId);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void TagComponent_SetName(IntPtr scene, uint entityId, string name);

    [DllImport(DllName)]
    internal static extern IntPtr InputTextComponent_GetText(IntPtr scene, uint entityId);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void InputTextComponent_SetText(IntPtr scene, uint entityId, string text);

    [DllImport(DllName)]
    internal static extern IntPtr InputTextComponent_GetPlaceholder(IntPtr scene, uint entityId);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void InputTextComponent_SetPlaceholder(IntPtr scene, uint entityId, string text);

    [DllImport(DllName)]
    internal static extern int PolygonCollider_GetVertexCount(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void PolygonCollider_GetVertices(IntPtr scene, uint entityId, [Out] Vector2[] outVertices);

    [DllImport(DllName)]
    internal static extern void PolygonCollider_SetVertices(IntPtr scene, uint entityId, [In] Vector2[] vertices,
        int count);

    [DllImport(DllName)]
    internal static extern int EdgeCollider_GetVertexCount(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void EdgeCollider_GetVertices(IntPtr scene, uint entityId, [Out] Vector2[] outVertices);

    [DllImport(DllName)]
    internal static extern void EdgeCollider_SetVertices(IntPtr scene, uint entityId, [In] Vector2[] vertices,
        int count);
}






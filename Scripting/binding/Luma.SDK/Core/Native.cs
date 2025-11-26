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

    [DllImport(DllName, EntryPoint = "Platform_HasPermissions")]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool Platform_HasPermissions(
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]
        string[] permissions, int count);

    [DllImport(DllName, EntryPoint = "Platform_RequestPermissions")]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool Platform_RequestPermissions(
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]
        string[] permissions, int count);


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

    [DllImport(DllName)]
    public static extern bool Physics_OverlapCircle(IntPtr scene, Vector2 center, float radius,
        [In] string[]? tags, int tagCount,
        [Out] uint[] outEntities, int maxEntities, out int outEntityCount);

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

    [DllImport(DllName)]
    internal static extern void InputTextComponent_SetText(IntPtr scene, uint entityId, string text);

    [DllImport(DllName)]
    internal static extern IntPtr InputTextComponent_GetPlaceholder(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void InputTextComponent_SetPlaceholder(IntPtr scene, uint entityId, string text);

    [StructLayout(LayoutKind.Sequential)]
    internal struct SerializableEventTargetNative
    {
        public Guid targetEntityGuid;
        public IntPtr targetComponentName;
        public IntPtr targetMethodName;
    }


    [DllImport(DllName)]
    internal static extern int ButtonComponent_GetOnClickTargetCount(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ButtonComponent_GetOnClickTarget(IntPtr scene, uint entityId, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ButtonComponent_ClearOnClickTargets(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void ButtonComponent_AddOnClickTarget(IntPtr scene, uint entityId, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int InputTextComponent_GetOnTextChangedTargetCount(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool InputTextComponent_GetOnTextChangedTarget(IntPtr scene, uint entityId, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void InputTextComponent_ClearOnTextChangedTargets(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void InputTextComponent_AddOnTextChangedTarget(IntPtr scene, uint entityId, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int InputTextComponent_GetOnSubmitTargetCount(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool InputTextComponent_GetOnSubmitTarget(IntPtr scene, uint entityId, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void InputTextComponent_ClearOnSubmitTargets(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    internal static extern void InputTextComponent_AddOnSubmitTarget(IntPtr scene, uint entityId, Guid guid,
        string componentName, string methodName);

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


    [DllImport(DllName)]
    internal static extern int ToggleButtonComponent_GetOnToggleOnTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ToggleButtonComponent_GetOnToggleOnTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ToggleButtonComponent_ClearOnToggleOnTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ToggleButtonComponent_AddOnToggleOnTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int ToggleButtonComponent_GetOnToggleOffTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ToggleButtonComponent_GetOnToggleOffTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ToggleButtonComponent_ClearOnToggleOffTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ToggleButtonComponent_AddOnToggleOffTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int RadioButtonComponent_GetOnSelectedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool RadioButtonComponent_GetOnSelectedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void RadioButtonComponent_ClearOnSelectedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void RadioButtonComponent_AddOnSelectedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int RadioButtonComponent_GetOnDeselectedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool RadioButtonComponent_GetOnDeselectedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void RadioButtonComponent_ClearOnDeselectedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void RadioButtonComponent_AddOnDeselectedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int CheckBoxComponent_GetOnValueChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool CheckBoxComponent_GetOnValueChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void CheckBoxComponent_ClearOnValueChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void CheckBoxComponent_AddOnValueChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int SliderComponent_GetOnValueChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool SliderComponent_GetOnValueChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void SliderComponent_ClearOnValueChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void SliderComponent_AddOnValueChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int SliderComponent_GetOnDragStartedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool SliderComponent_GetOnDragStartedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void SliderComponent_ClearOnDragStartedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void SliderComponent_AddOnDragStartedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int SliderComponent_GetOnDragEndedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool SliderComponent_GetOnDragEndedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void SliderComponent_ClearOnDragEndedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void SliderComponent_AddOnDragEndedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int ComboBoxComponent_GetOnSelectionChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ComboBoxComponent_GetOnSelectionChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ComboBoxComponent_ClearOnSelectionChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ComboBoxComponent_AddOnSelectionChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int ProgressBarComponent_GetOnValueChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ProgressBarComponent_GetOnValueChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ProgressBarComponent_ClearOnValueChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ProgressBarComponent_AddOnValueChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int ProgressBarComponent_GetOnCompletedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ProgressBarComponent_GetOnCompletedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ProgressBarComponent_ClearOnCompletedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ProgressBarComponent_AddOnCompletedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int TabControlComponent_GetOnTabChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool TabControlComponent_GetOnTabChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void TabControlComponent_ClearOnTabChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void TabControlComponent_AddOnTabChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int TabControlComponent_GetOnTabClosedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool TabControlComponent_GetOnTabClosedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void TabControlComponent_ClearOnTabClosedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void TabControlComponent_AddOnTabClosedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);


    [DllImport(DllName)]
    internal static extern int ListBoxComponent_GetOnSelectionChangedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ListBoxComponent_GetOnSelectionChangedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ListBoxComponent_ClearOnSelectionChangedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ListBoxComponent_AddOnSelectionChangedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    internal static extern int ListBoxComponent_GetOnItemActivatedTargetCount(IntPtr scene, uint entity);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool ListBoxComponent_GetOnItemActivatedTarget(IntPtr scene, uint entity, int index,
        out SerializableEventTargetNative outTarget);

    [DllImport(DllName)]
    internal static extern void ListBoxComponent_ClearOnItemActivatedTargets(IntPtr scene, uint entity);

    [DllImport(DllName)]
    internal static extern void ListBoxComponent_AddOnItemActivatedTarget(IntPtr scene, uint entity, Guid guid,
        string componentName, string methodName);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool AssetManager_StartPreload();

    [DllImport(DllName)]
    internal static extern void AssetManager_StopPreload();

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool AssetManager_IsPreloadComplete();

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool AssetManager_IsPreloadRunning();

    [DllImport(DllName)]
    internal static extern void AssetManager_GetPreloadProgress(out int outTotalCount, out int outLoadedCount);

    [DllImport(DllName)]
    internal static extern Guid AssetManager_LoadAsset(string path);

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetExecutableDir();

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetPersistentDataDir();

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetCacheDir();

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetContentDir();

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetAndroidInternalDataDir();

    [DllImport(DllName)]
    internal static extern IntPtr PathUtils_GetAndroidExternalDataDir();

    #region Graphics - WGSLMaterial

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern IntPtr WGSLMaterial_Load(Guid assetGuid);

    [DllImport(DllName)]
    internal static extern IntPtr WGSLMaterial_GetFromSprite(IntPtr scene, uint entityId);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool WGSLMaterial_IsValid(IntPtr material);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetFloat(IntPtr material, string name, float value);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetInt(IntPtr material, string name, int value);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetVec2(IntPtr material, string name, float x, float y);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetVec3(IntPtr material, string name, float x, float y, float z);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetVec4(IntPtr material, string name, float r, float g, float b, float a);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetUniformStruct(IntPtr material, string name, IntPtr data, int size);

    [DllImport(DllName, CharSet = CharSet.Ansi)]
    internal static extern void WGSLMaterial_SetTexture(IntPtr material, string name, IntPtr texture, uint binding, uint group);

    [DllImport(DllName)]
    internal static extern void WGSLMaterial_UpdateUniformBuffer(IntPtr material);

    #endregion

    #region Graphics - TextureA

    [DllImport(DllName)]
    internal static extern IntPtr TextureA_Load(Guid assetGuid);

    [DllImport(DllName)]
    internal static extern IntPtr TextureA_Create(uint width, uint height);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool TextureA_IsValid(IntPtr texture);

    [DllImport(DllName)]
    internal static extern void TextureA_Release(IntPtr texture);

    [DllImport(DllName)]
    internal static extern uint TextureA_GetWidth(IntPtr texture);

    [DllImport(DllName)]
    internal static extern uint TextureA_GetHeight(IntPtr texture);

    [DllImport(DllName)]
    internal static extern uint TextureA_GetDepth(IntPtr texture);

    [DllImport(DllName)]
    internal static extern uint TextureA_GetMipLevelCount(IntPtr texture);

    [DllImport(DllName)]
    internal static extern uint TextureA_GetSampleCount(IntPtr texture);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool TextureA_Is3D(IntPtr texture);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool TextureA_IsArray(IntPtr texture);

    #endregion

    #region Graphics - GpuBuffer

    [DllImport(DllName)]
    internal static extern IntPtr GpuBuffer_Create(uint size, uint usage);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool GpuBuffer_IsValid(IntPtr buffer);

    [DllImport(DllName)]
    internal static extern void GpuBuffer_Release(IntPtr buffer);

    [DllImport(DllName)]
    internal static extern uint GpuBuffer_GetSize(IntPtr buffer);

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool GpuBuffer_Write(IntPtr buffer, IntPtr data, uint size, uint offset);

    #endregion

    #region Camera

    [DllImport(DllName)]
    internal static extern float Camera_GetPositionX();

    [DllImport(DllName)]
    internal static extern float Camera_GetPositionY();

    [DllImport(DllName)]
    internal static extern void Camera_SetPosition(float x, float y);

    [DllImport(DllName)]
    internal static extern float Camera_GetZoom();

    [DllImport(DllName)]
    internal static extern void Camera_SetZoom(float zoom);

    [DllImport(DllName)]
    internal static extern float Camera_GetRotation();

    [DllImport(DllName)]
    internal static extern void Camera_SetRotation(float rotation);

    [DllImport(DllName)]
    internal static extern float Camera_GetViewportWidth();

    [DllImport(DllName)]
    internal static extern float Camera_GetViewportHeight();

    [DllImport(DllName)]
    internal static extern void Camera_GetClearColor(out float r, out float g, out float b, out float a);

    [DllImport(DllName)]
    internal static extern void Camera_SetClearColor(float r, float g, float b, float a);

    [DllImport(DllName)]
    internal static extern void Camera_ScreenToWorld(float screenX, float screenY, out float worldX, out float worldY);

    [DllImport(DllName)]
    internal static extern void Camera_WorldToScreen(float worldX, float worldY, out float screenX, out float screenY);

    #endregion

    #region Shader PreWarming

    [DllImport(DllName)]
    internal static extern void Shader_StartPreWarmingAsync();

    [DllImport(DllName)]
    internal static extern void Shader_PreWarming();

    [DllImport(DllName)]
    internal static extern void Shader_StopPreWarming();

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool Shader_IsPreWarmingRunning();

    [DllImport(DllName)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool Shader_IsPreWarmingComplete();

    [DllImport(DllName)]
    internal static extern void Shader_GetPreWarmingState(out int outTotal, out int outLoaded,
        [MarshalAs(UnmanagedType.I1)] out bool outIsRunning,
        [MarshalAs(UnmanagedType.I1)] out bool outIsComplete);

    #endregion
}
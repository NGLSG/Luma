using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UIProgressBar : ILogicComponent
{
    private const string ComponentName = "ProgressBarComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnValueChanged { get; }
    public SerializedEventBinding OnCompleted { get; }

    public UIProgressBar(Entity entity)
    {
        Entity = entity;
        OnValueChanged = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ProgressBarComponent_GetOnValueChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ProgressBarComponent_GetOnValueChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ProgressBarComponent_ClearOnValueChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ProgressBarComponent_AddOnValueChangedTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty));

        OnCompleted = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ProgressBarComponent_GetOnCompletedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ProgressBarComponent_GetOnCompletedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ProgressBarComponent_ClearOnCompletedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ProgressBarComponent_AddOnCompletedTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


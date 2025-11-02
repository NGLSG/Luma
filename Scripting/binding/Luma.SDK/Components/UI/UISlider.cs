using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UISlider : ILogicComponent
{
    private const string ComponentName = "SliderComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnValueChanged { get; }
    public SerializedEventBinding OnDragStarted { get; }
    public SerializedEventBinding OnDragEnded { get; }

    public UISlider(Entity entity)
    {
        Entity = entity;
        OnValueChanged = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.SliderComponent_GetOnValueChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.SliderComponent_GetOnValueChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.SliderComponent_ClearOnValueChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.SliderComponent_AddOnValueChangedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));

        OnDragStarted = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.SliderComponent_GetOnDragStartedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.SliderComponent_GetOnDragStartedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.SliderComponent_ClearOnDragStartedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.SliderComponent_AddOnDragStartedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));

        OnDragEnded = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.SliderComponent_GetOnDragEndedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.SliderComponent_GetOnDragEndedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.SliderComponent_ClearOnDragEndedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.SliderComponent_AddOnDragEndedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


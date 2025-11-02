using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UIToggleButton : ILogicComponent
{
    private const string ComponentName = "ToggleButtonComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnToggleOn { get; }
    public SerializedEventBinding OnToggleOff { get; }

    public UIToggleButton(Entity entity)
    {
        Entity = entity;
        OnToggleOn = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ToggleButtonComponent_GetOnToggleOnTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ToggleButtonComponent_GetOnToggleOnTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ToggleButtonComponent_ClearOnToggleOnTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ToggleButtonComponent_AddOnToggleOnTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));

        OnToggleOff = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ToggleButtonComponent_GetOnToggleOffTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ToggleButtonComponent_GetOnToggleOffTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ToggleButtonComponent_ClearOnToggleOffTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ToggleButtonComponent_AddOnToggleOffTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


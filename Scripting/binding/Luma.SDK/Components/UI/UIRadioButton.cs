using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UIRadioButton : ILogicComponent
{
    private const string ComponentName = "RadioButtonComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnSelected { get; }
    public SerializedEventBinding OnDeselected { get; }

    public UIRadioButton(Entity entity)
    {
        Entity = entity;
        OnSelected = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.RadioButtonComponent_GetOnSelectedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.RadioButtonComponent_GetOnSelectedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.RadioButtonComponent_ClearOnSelectedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.RadioButtonComponent_AddOnSelectedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));

        OnDeselected = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.RadioButtonComponent_GetOnDeselectedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.RadioButtonComponent_GetOnDeselectedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.RadioButtonComponent_ClearOnDeselectedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.RadioButtonComponent_AddOnDeselectedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


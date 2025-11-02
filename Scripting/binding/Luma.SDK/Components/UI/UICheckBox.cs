using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UICheckBox : ILogicComponent
{
    private const string ComponentName = "CheckBoxComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnValueChanged { get; }

    public UICheckBox(Entity entity)
    {
        Entity = entity;
        OnValueChanged = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.CheckBoxComponent_GetOnValueChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.CheckBoxComponent_GetOnValueChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.CheckBoxComponent_ClearOnValueChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.CheckBoxComponent_AddOnValueChangedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


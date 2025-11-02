using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UIListBox : ILogicComponent
{
    private const string ComponentName = "ListBoxComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnSelectionChanged { get; }
    public SerializedEventBinding OnItemActivated { get; }

    public UIListBox(Entity entity)
    {
        Entity = entity;
        OnSelectionChanged = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ListBoxComponent_GetOnSelectionChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ListBoxComponent_GetOnSelectionChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ListBoxComponent_ClearOnSelectionChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ListBoxComponent_AddOnSelectionChangedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));

        OnItemActivated = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ListBoxComponent_GetOnItemActivatedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ListBoxComponent_GetOnItemActivatedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ListBoxComponent_ClearOnItemActivatedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.ListBoxComponent_AddOnItemActivatedTarget(owner.ScenePtr, owner.Id, guid, componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


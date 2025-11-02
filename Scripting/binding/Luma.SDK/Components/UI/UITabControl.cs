using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UITabControl : ILogicComponent
{
    private const string ComponentName = "TabControlComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnTabChanged { get; }
    public SerializedEventBinding OnTabClosed { get; }

    public UITabControl(Entity entity)
    {
        Entity = entity;
        OnTabChanged = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.TabControlComponent_GetOnTabChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.TabControlComponent_GetOnTabChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.TabControlComponent_ClearOnTabChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.TabControlComponent_AddOnTabChangedTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty));

        OnTabClosed = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.TabControlComponent_GetOnTabClosedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.TabControlComponent_GetOnTabClosedTarget(owner.ScenePtr, owner.Id, index, out var native))
                    return (Guid.Empty, string.Empty, string.Empty);
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.TabControlComponent_ClearOnTabClosedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
                Native.TabControlComponent_AddOnTabClosedTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty));
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


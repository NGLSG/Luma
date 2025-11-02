using System;
using Luma.SDK;

namespace Luma.SDK.Components;


public sealed class UIButton : ILogicComponent
{
    private const string ComponentName = "ButtonComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding OnClick { get; }

    public UIButton(Entity entity)
    {
        Entity = entity;
        OnClick = new SerializedEventBinding(
            Entity,
            getCount: owner => Native.ButtonComponent_GetOnClickTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.ButtonComponent_GetOnClickTarget(owner.ScenePtr, owner.Id, index, out var native))
                {
                    return (Guid.Empty, string.Empty, string.Empty);
                }
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.ButtonComponent_ClearOnClickTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
            {
                Native.ButtonComponent_AddOnClickTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty);
            });
    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }
}


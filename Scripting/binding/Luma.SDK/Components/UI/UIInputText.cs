using System;
using Luma.SDK;

namespace Luma.SDK.Components;

public sealed class UIInputText : ILogicComponent
{
    private const string ComponentName = "InputTextComponent";
    public Entity Entity { get; set; }

    public SerializedEventBinding<string> OnTextChanged { get; }
    public SerializedEventBinding<string> OnSubmit { get; }

    public UIInputText(Entity entity)
    {
        Entity = entity;
        OnTextChanged = new SerializedEventBinding<string>(
            Entity,
            getCount: owner => Native.InputTextComponent_GetOnTextChangedTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.InputTextComponent_GetOnTextChangedTarget(owner.ScenePtr, owner.Id, index, out var native))
                {
                    return (Guid.Empty, string.Empty, string.Empty);
                }
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.InputTextComponent_ClearOnTextChangedTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
            {
                Native.InputTextComponent_AddOnTextChangedTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty);
            });

        OnSubmit = new SerializedEventBinding<string>(
            Entity,
            getCount: owner => Native.InputTextComponent_GetOnSubmitTargetCount(owner.ScenePtr, owner.Id),
            getAt: (owner, index) =>
            {
                if (!Native.InputTextComponent_GetOnSubmitTarget(owner.ScenePtr, owner.Id, index, out var native))
                {
                    return (Guid.Empty, string.Empty, string.Empty);
                }
                string comp = MarshalPtrToString(native.targetComponentName) ?? string.Empty;
                string meth = MarshalPtrToString(native.targetMethodName) ?? string.Empty;
                return (native.targetEntityGuid, comp, meth);
            },
            clear: owner => Native.InputTextComponent_ClearOnSubmitTargets(owner.ScenePtr, owner.Id),
            addByGuid: (owner, guid, componentName, methodName) =>
            {
                Native.InputTextComponent_AddOnSubmitTarget(owner.ScenePtr, owner.Id, guid,
                    componentName ?? string.Empty, methodName ?? string.Empty);
            });

    }

    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr);
    }

    public string Text
    {
        get
        {
            var ptr = Native.InputTextComponent_GetText(Entity.ScenePtr, Entity.Id);
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
        }
        set => Native.InputTextComponent_SetText(Entity.ScenePtr, Entity.Id, value ?? string.Empty);
    }

    public string Placeholder
    {
        get
        {
            var ptr = Native.InputTextComponent_GetPlaceholder(Entity.ScenePtr, Entity.Id);
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
        }
        set => Native.InputTextComponent_SetPlaceholder(Entity.ScenePtr, Entity.Id, value ?? string.Empty);
    }

    

    
}


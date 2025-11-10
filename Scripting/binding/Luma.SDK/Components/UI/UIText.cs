using System;
using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;




public enum TextAlignment
{
    TopLeft = 0,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
}




public sealed class UIText : ILogicComponent
{
    private const string ComponentName = "TextComponent";
    
    public Entity Entity { get; set; }

    public UIText(Entity entity)
    {
        Entity = entity;
    }

    
    
    
    private static string? MarshalPtrToString(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero) return null;
        return Marshal.PtrToStringAnsi(ptr);
    }

    
    
    
    public bool Enable
    {
        get => Entity.GetComponentProperty<bool>(ComponentName, "Enable");
        set => Entity.SetComponentProperty(ComponentName, "Enable", in value);
    }

    
    
    
    public AssetHandle FontHandle
    {
        get => Entity.GetComponentProperty<AssetHandle>(ComponentName, "fontHandle");
        set => Entity.SetComponentProperty(ComponentName, "fontHandle", in value);
    }

    
    
    
    public string Text
    {
        get
        {
            var ptr = Native.TextComponent_GetText(Entity.ScenePtr, Entity.Id);
            return MarshalPtrToString(ptr) ?? string.Empty;
        }
        set => Native.TextComponent_SetText(Entity.ScenePtr, Entity.Id, value ?? string.Empty);
    }

    
    
    
    public float FontSize
    {
        get => Entity.GetComponentProperty<float>(ComponentName, "fontSize");
        set => Entity.SetComponentProperty(ComponentName, "fontSize", in value);
    }

    
    
    
    public Color Color
    {
        get => Entity.GetComponentProperty<Color>(ComponentName, "color");
        set => Entity.SetComponentProperty(ComponentName, "color", in value);
    }

    
    
    
    public TextAlignment Alignment
    {
        get => Entity.GetComponentProperty<TextAlignment>(ComponentName, "alignment");
        set => Entity.SetComponentProperty(ComponentName, "alignment", in value);
    }

    
    
    
    public int ZIndex
    {
        get => Entity.GetComponentProperty<int>(ComponentName, "zIndex");
        set => Entity.SetComponentProperty(ComponentName, "zIndex", in value);
    }

    
    
    
    public string Name
    {
        get
        {
            var ptr = Native.TextComponent_GetName(Entity.ScenePtr, Entity.Id);
            return MarshalPtrToString(ptr) ?? string.Empty;
        }
        set => Native.TextComponent_SetName(Entity.ScenePtr, Entity.Id, value ?? string.Empty);
    }
}
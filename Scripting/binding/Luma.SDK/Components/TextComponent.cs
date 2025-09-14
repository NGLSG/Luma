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
};

[StructLayout(LayoutKind.Sequential)]
public struct TextComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle fontHandle;
    public string text;
    public float fontSize;
    public Color color;
    public TextAlignment alignment;
    public int zIndex;
    public string name;
}



public class LogicTextComponent : LogicComponent<TextComponent>
{
    private const string ComponentName = "TextComponent";

    public LogicTextComponent(Entity entity) : base(entity)
    {
        
        
        
        InitializeCache();
    }

    private void InitializeCache()
    {
        
        _component.Enable = Entity.GetComponentProperty<bool>(ComponentName, "Enable");
        _component.fontHandle = Entity.GetComponentProperty<AssetHandle>(ComponentName, "fontHandle");
        _component.fontSize = FontSize;
        _component.color = Color;
        _component.alignment = Alignment;
        _component.zIndex = ZIndex;
        _component.text = Text;
        _component.name = Name;
    }

    
    
    
    public string Text
    {
        
        get
        {
            IntPtr cstr = Native.TextComponent_GetText(Entity.ScenePtr, Entity.Id);
            return Marshal.PtrToStringAnsi(cstr) ?? "";
        }
        
        
        set
        {
            _component.text = value; 
            Native.TextComponent_SetText(Entity.ScenePtr, Entity.Id, value);
        }
    }
    
    
    
    
    public string Name
    {
        get
        {
            IntPtr cstr = Native.TextComponent_GetName(Entity.ScenePtr, Entity.Id);
            return Marshal.PtrToStringAnsi(cstr) ?? "";
        }
        set
        {
            _component.name = value;
            Native.TextComponent_SetName(Entity.ScenePtr, Entity.Id, value);
        }
    }

    
    
    
    public Color Color
    {
        get => _component.color;
        set
        {
            _component.color = value;
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    
    
    
    public TextAlignment Alignment
    {
        get => _component.alignment;
        set
        {
            _component.alignment = value;
            Entity.SetComponentProperty(ComponentName, "alignment", in value);
        }
    }

    
    
    
    public int ZIndex
    {
        get => _component.zIndex;
        set
        {
            _component.zIndex = value;
            Entity.SetComponentProperty(ComponentName, "zIndex", in value);
        }
    }

    
    
    
    public float FontSize
    {
        get => _component.fontSize;
        set
        {
            _component.fontSize = value;
            Entity.SetComponentProperty(ComponentName, "fontSize", in value);
        }
    }
}
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
[GenerateLogicComponentProperties]
public partial class LogicTextComponent : LogicComponent<TextComponent>
{
    public LogicTextComponent(Entity entity) : base(entity)
    {
    }

    public partial string Text { get; set; }
    public partial Color Color { get; set; }
    public partial TextAlignment Alignment { get; set; }
    public partial int ZIndex { get; set; }
    public partial float FontSize { get;  set; }
}
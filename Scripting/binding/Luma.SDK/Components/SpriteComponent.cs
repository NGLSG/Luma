using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct SpriteComponent : IComponent
{[MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle TextureHandle;
    public AssetHandle MaterialHandle;
    public RectF SourceRect;
    public Color Color;
    public int ZIndex;
}
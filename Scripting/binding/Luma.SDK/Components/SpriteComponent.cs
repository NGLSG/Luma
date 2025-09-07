using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct SpriteComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle textureHandle;
    public AssetHandle materialHandle;
    public RectF sourceRect;
    public Color color;
    public int zIndex;
}

[GenerateLogicComponentProperties]
public partial class Sprite : LogicComponent<SpriteComponent>
{

    public Sprite(Entity entity) : base(entity)
    {
    }

    public partial Color Color { get; set; }
    public partial int ZIndex { get; set; }
    public partial RectF SourceRect { get; set; }
    public partial AssetHandle TextureHandle { get; set; }
    public partial AssetHandle MaterialHandle { get; set; }
}
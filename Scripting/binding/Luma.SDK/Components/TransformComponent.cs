using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct TransformComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public Vector2 position;
    public float rotation;
    public Vector2 scale;
    public Vector2 localPosition;
    public float localRotation;
    public Vector2 localScale;
}

[GenerateLogicComponentProperties]
public partial class Transform : LogicComponent<TransformComponent>
{

    public Transform(Entity entity) : base(entity)
    {
    }

    public partial Vector2 Position { get; set; }
    public partial float Rotation { get; set; }
    public partial Vector2 Scale { get; set; }
    public partial Vector2 LocalPosition { get; set; }
    public partial float LocalRotation { get; set; }
    public partial Vector2 LocalScale { get; set; }
}
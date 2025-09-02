using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct Transform : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public Vector2 Position;
    public float Rotation;
    public Vector2 Scale;
    public Vector2 LocalPosition;
    public float LocalRotation;
    public Vector2 LocalScale;
}
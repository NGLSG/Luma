using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct AnimationControllerComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle animationController;
    public int targetFrame;
}
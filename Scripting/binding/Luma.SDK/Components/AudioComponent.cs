using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public class AudioComponent
{
    [MarshalAs(UnmanagedType.U1)]
    bool Enable;
    AssetHandle AudioHandle;
    [MarshalAs(UnmanagedType.I1)] bool PlayOnStart;
    [MarshalAs(UnmanagedType.I1)] bool Loop = false;
    float Volume = 1.0f;
    bool Spatial = false;
    float MinDistance = 1.0f;
    float MaxDistance = 30.0f;
    float Pitch = 1.0f;
}
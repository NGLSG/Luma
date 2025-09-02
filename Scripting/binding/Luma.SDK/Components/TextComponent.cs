using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

enum TextAlignment
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
struct TextComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    AssetHandle fontHandle;
    string text;
    float fontSize;
    Color color;
    TextAlignment alignment;
    int zIndex;
    string name;
}
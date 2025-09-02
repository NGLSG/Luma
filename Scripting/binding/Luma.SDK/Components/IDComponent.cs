using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct IDComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
    public string Name;

    public Guid Guid;

}
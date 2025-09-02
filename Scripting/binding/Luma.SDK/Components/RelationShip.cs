using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct ParentComponent : IComponent
{
    
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    

    
    
    
    
    public uint Parent;
}

[StructLayout(LayoutKind.Sequential)]
public struct ChildrenComponent : IComponent
{
    
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    

    
    
    
    
    public IntPtr Children;

    
    
    
    public int ChildrenCount;
}
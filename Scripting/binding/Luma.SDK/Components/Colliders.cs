using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Luma.SDK.Components
{
    #region Enums

    
    
    
    
    public enum CapsuleDirection
    {
        Vertical,   
        Horizontal  
    }

    #endregion

    #region Component Structs

    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct BoxColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;

        
        public Vector2 Size;
    }

    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct CircleColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;

        
        public float Radius;
    }

    
    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct PolygonColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;

        
        
        
        
        public IntPtr Vertices;

        
        
        
        public int VertexCount;
    }

    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct EdgeColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;

        
        
        
        
        public IntPtr Vertices;

        
        
        
        public int VertexCount;

        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool Loop;
    }

    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct CapsuleColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;

        
        public Vector2 Size;
        public CapsuleDirection Direction;
    }

    
    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct TilemapColliderComponent : IComponent
    {
        
        [MarshalAs(UnmanagedType.U1)] public bool Enable;

        
        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool IsTrigger;

        
        
        
        [MarshalAs(UnmanagedType.U1)] public bool IsDirty;
    }

    #endregion
}
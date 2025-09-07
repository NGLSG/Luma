using System;
using System.Numerics;
using System.Runtime.InteropServices;
using Luma.SDK.Generation;

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
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;


        public Vector2 Size;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct CircleColliderComponent : IComponent
    {
        [MarshalAs(UnmanagedType.U1)] public bool Enable;


        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;


        public float Radius;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct PolygonColliderComponent : IComponent
    {
        [MarshalAs(UnmanagedType.U1)] public bool Enable;


        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;


        public IntPtr vertices;


        public int vertexCount;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct EdgeColliderComponent : IComponent
    {
        [MarshalAs(UnmanagedType.U1)] public bool Enable;


        public Vector2 offset;
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;


        public IntPtr vertices;


        public int vertexCount;


        [MarshalAs(UnmanagedType.U1)] public bool loop;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct CapsuleColliderComponent : IComponent
    {
        [MarshalAs(UnmanagedType.U1)] public bool Enable;


        public Vector2 offset;
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;


        public Vector2 size;
        public CapsuleDirection direction;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct TilemapColliderComponent : IComponent
    {
        [MarshalAs(UnmanagedType.U1)] public bool Enable;


        public Vector2 Offset;
        [MarshalAs(UnmanagedType.U1)] public bool isTrigger;


        [MarshalAs(UnmanagedType.U1)] public bool isDirty;
    }

    #endregion

    #region Logic Components

    [GenerateLogicComponentProperties]
    public partial class BoxCollider : LogicComponent<BoxColliderComponent>
    {

        public BoxCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
        public partial Vector2 Size { get; set; }
    }

    [GenerateLogicComponentProperties]
    public partial class CircleCollider : LogicComponent<CircleColliderComponent>
    {

        public CircleCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
        public partial float Radius { get; set; }
    }

    [GenerateLogicComponentProperties]
    public partial class PolygonCollider : LogicComponent<PolygonColliderComponent>
    {

        public PolygonCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
        public partial IntPtr Vertices { get; set; }
        public partial int VertexCount { get; set; }
    }

    [GenerateLogicComponentProperties]
    public partial class EdgeCollider : LogicComponent<EdgeColliderComponent>
    {

        public EdgeCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
        public partial IntPtr Vertices { get; set; }
        public partial int VertexCount { get; set; }
        public partial bool Loop { get; set; }
    }

    [GenerateLogicComponentProperties]
    public partial class CapsuleCollider : LogicComponent<CapsuleColliderComponent>
    {

        public CapsuleCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
        public partial Vector2 Size { get; set; }
        public partial CapsuleDirection Direction { get; set; }
    }

    [GenerateLogicComponentProperties]
    public partial class TilemapCollider : LogicComponent<TilemapColliderComponent>
    {

        public TilemapCollider(Entity entity) : base(entity)
        {
        }

        public partial Vector2 Offset { get; set; }
        public partial bool IsTrigger { get; set; }
        public partial bool IsDirty { get; set; }
    }

    #endregion
}
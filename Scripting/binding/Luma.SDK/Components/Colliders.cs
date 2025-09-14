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

    
    
    
    public class BoxCollider : LogicComponent<BoxColliderComponent>
    {
        private const string ComponentName = "BoxColliderComponent";

        public BoxCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.Offset;
            set
            {
                _component.Offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }

        public Vector2 Size
        {
            get => _component.Size;
            set
            {
                _component.Size = value;
                Entity.SetComponentProperty(ComponentName, "size", in value);
            }
        }
    }

    
    
    
    public class CircleCollider : LogicComponent<CircleColliderComponent>
    {
        private const string ComponentName = "CircleColliderComponent";

        public CircleCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.Offset;
            set
            {
                _component.Offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }

        public float Radius
        {
            get => _component.Radius;
            set
            {
                _component.Radius = value;
                Entity.SetComponentProperty(ComponentName, "radius", in value);
            }
        }
    }

    
    
    
    public class CapsuleCollider : LogicComponent<CapsuleColliderComponent>
    {
        private const string ComponentName = "CapsuleColliderComponent";

        public CapsuleCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.offset;
            set
            {
                _component.offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }

        public Vector2 Size
        {
            get => _component.size;
            set
            {
                _component.size = value;
                Entity.SetComponentProperty(ComponentName, "size", in value);
            }
        }

        public CapsuleDirection Direction
        {
            get => _component.direction;
            set
            {
                _component.direction = value;
                Entity.SetComponentProperty(ComponentName, "direction", in value);
            }
        }
    }


    
    
    
    public class PolygonCollider : LogicComponent<PolygonColliderComponent>
    {
        private const string ComponentName = "PolygonColliderComponent";

        public PolygonCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.Offset;
            set
            {
                _component.Offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }

        
        
        
        public Vector2[] Vertices
        {
            get
            {
                int count = Native.PolygonCollider_GetVertexCount(Entity.ScenePtr, Entity.Id);
                if (count == 0)
                {
                    return Array.Empty<Vector2>();
                }

                var vertices = new Vector2[count];
                Native.PolygonCollider_GetVertices(Entity.ScenePtr, Entity.Id, vertices);
                return vertices;
            }
            set
            {
                var vertices = value ?? Array.Empty<Vector2>();
                
                _component.vertexCount = vertices.Length;
                Native.PolygonCollider_SetVertices(Entity.ScenePtr, Entity.Id, vertices, vertices.Length);
            }
        }
    }

    
    
    
    public class EdgeCollider : LogicComponent<EdgeColliderComponent>
    {
        private const string ComponentName = "EdgeColliderComponent";

        public EdgeCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.offset;
            set
            {
                _component.offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }

        public bool Loop
        {
            get => _component.loop;
            set
            {
                _component.loop = value;
                Entity.SetComponentProperty(ComponentName, "loop", in value);
            }
        }

        
        
        
        public Vector2[] Vertices
        {
            get
            {
                int count = Native.EdgeCollider_GetVertexCount(Entity.ScenePtr, Entity.Id);
                if (count == 0)
                {
                    return Array.Empty<Vector2>();
                }

                var vertices = new Vector2[count];
                Native.EdgeCollider_GetVertices(Entity.ScenePtr, Entity.Id, vertices);
                return vertices;
            }
            set
            {
                var vertices = value ?? Array.Empty<Vector2>();
                _component.vertexCount = vertices.Length;
                Native.EdgeCollider_SetVertices(Entity.ScenePtr, Entity.Id, vertices, vertices.Length);
            }
        }
    }

    
    
    
    public class TilemapCollider : LogicComponent<TilemapColliderComponent>
    {
        private const string ComponentName = "TilemapColliderComponent";

        public TilemapCollider(Entity entity) : base(entity)
        {
        }

        public Vector2 Offset
        {
            get => _component.Offset;
            set
            {
                _component.Offset = value;
                Entity.SetComponentProperty(ComponentName, "offset", in value);
            }
        }

        public bool IsTrigger
        {
            get => _component.isTrigger;
            set
            {
                _component.isTrigger = value;
                Entity.SetComponentProperty(ComponentName, "isTrigger", in value);
            }
        }
    }

    #endregion
}
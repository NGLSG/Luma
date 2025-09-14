using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

public enum BodyType
{
    Static,
    Kinematic,
    Dynamic
}

public enum CollisionDetectionType
{
    Discrete,
    Continuous
}

public enum SleepingMode
{
    NeverSleep,
    StartAwake,
    StartAsleep
}

[StructLayout(LayoutKind.Sequential)]
public struct BodyConstraints
{
    [MarshalAs(UnmanagedType.I1)] public bool FreezePositionX;
    [MarshalAs(UnmanagedType.I1)] public bool FreezePositionY;
    [MarshalAs(UnmanagedType.I1)] public bool FreezeRotation;
}

[StructLayout(LayoutKind.Sequential)]
public struct RigidBodyComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public BodyType bodyType;
    public AssetHandle physicsMaterial;
    public Vector2 linearVelocity;
    public float angularVelocity;
    [MarshalAs(UnmanagedType.I1)] public bool simulated;
    public float mass;
    public float linearDamping;
    public float angularDamping;
    public float gravityScale;
    public CollisionDetectionType collisionDetection;
    public SleepingMode sleepingMode;
    public BodyConstraints constraints;
}




public class Rigidbody2D : LogicComponent<RigidBodyComponent>
{
    
    
    
    private const string ComponentName = "RigidBodyComponent";

    
    
    
    
    public Rigidbody2D(Entity entity) : base(entity)
    {
    }

    #region Properties

    public Vector2 LinearVelocity
    {
        
        get => _component.linearVelocity;
        
        
        set
        {
            _component.linearVelocity = value;
            Entity.SetComponentProperty(ComponentName, "linearVelocity", in value);
        }
    }

    public float AngularVelocity
    {
        get => _component.angularVelocity;
        set
        {
            _component.angularVelocity = value;
            Entity.SetComponentProperty(ComponentName, "angularVelocity", in value);
        }
    }

    public bool Simulated
    {
        get => _component.simulated;
        set
        {
            _component.simulated = value;
            Entity.SetComponentProperty(ComponentName, "simulated", in value);
        }
    }

    public float Mass
    {
        get => _component.mass;
        set
        {
            _component.mass = value;
            Entity.SetComponentProperty(ComponentName, "mass", in value);
        }
    }

    public float LinearDamping
    {
        get => _component.linearDamping;
        set
        {
            _component.linearDamping = value;
            Entity.SetComponentProperty(ComponentName, "linearDamping", in value);
        }
    }

    public float AngularDamping
    {
        get => _component.angularDamping;
        set
        {
            _component.angularDamping = value;
            Entity.SetComponentProperty(ComponentName, "angularDamping", in value);
        }
    }

    public float GravityScale
    {
        get => _component.gravityScale;
        set
        {
            _component.gravityScale = value;
            Entity.SetComponentProperty(ComponentName, "gravityScale", in value);
        }
    }

    public AssetHandle PhysicsMaterial
    {
        get => _component.physicsMaterial;
        set
        {
            _component.physicsMaterial = value;
            Entity.SetComponentProperty(ComponentName, "physicsMaterial", in value);
        }
    }

    public BodyType BodyType
    {
        get => _component.bodyType;
        set
        {
            _component.bodyType = value;
            Entity.SetComponentProperty(ComponentName, "bodyType", in value);
        }
    }

    public CollisionDetectionType CollisionDetection
    {
        get => _component.collisionDetection;
        set
        {
            _component.collisionDetection = value;
            Entity.SetComponentProperty(ComponentName, "collisionDetection", in value);
        }
    }

    public SleepingMode SleepingMode
    {
        get => _component.sleepingMode;
        set
        {
            _component.sleepingMode = value;
            Entity.SetComponentProperty(ComponentName, "sleepingMode", in value);
        }
    }

    public BodyConstraints Constraints
    {
        get => _component.constraints;
        set
        {
            _component.constraints = value;
            Entity.SetComponentProperty(ComponentName, "constraints", in value);
        }
    }

    #endregion

    #region Methods

    
    
    
    
    
    public void ApplyForce(Vector2 force, ForceMode mode = ForceMode.Force)
    {
        
        Native.Physics_ApplyForce(Entity.ScenePtr, Entity.Id, force, mode);
    }

    #endregion
}
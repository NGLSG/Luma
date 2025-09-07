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

[GenerateLogicComponentProperties]
public partial class Rigidbody2D : LogicComponent<RigidBodyComponent>
{
    public Rigidbody2D(Entity entity) : base(entity)
    {
    }

    public partial Vector2 LinearVelocity { get; set; }
    public partial float AngularVelocity { get; set; }
    public partial bool Simulated { get; set; }
    public partial float Mass { get; set; }
    public partial float LinearDamping { get; set; }
    public partial float AngularDamping { get; set; }
    public partial float GravityScale { get; set; }
    public partial AssetHandle PhysicsMaterial { get; set; }
    public partial BodyType BodyType { get; set; }
    public partial CollisionDetectionType CollisionDetection { get; set; }
    public partial SleepingMode SleepingMode { get; set; }
    public partial BodyConstraints Constraints { get; set; }


    public void ApplyForce(Vector2 force, ForceMode mode = ForceMode.Force)
    {
        Native.Physics_ApplyForce(Entity.ScenePtr, Entity.Id, force, mode);
    }
}
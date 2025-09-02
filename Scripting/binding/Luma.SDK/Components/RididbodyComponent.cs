using System.Runtime.InteropServices;

namespace Luma.SDK.Components;


public enum BodyType { Static, Kinematic, Dynamic }
public enum CollisionDetectionType { Discrete, Continuous }
public enum SleepingMode { NeverSleep, StartAwake, StartAsleep }


[StructLayout(LayoutKind.Sequential)]
public struct BodyConstraints
{
    [MarshalAs(UnmanagedType.I1)]
    public bool FreezePositionX;
    [MarshalAs(UnmanagedType.I1)]
    public bool FreezePositionY;
    [MarshalAs(UnmanagedType.I1)]
    public bool FreezeRotation;
}

[StructLayout(LayoutKind.Sequential)]
public struct RigidBodyComponent : IComponent
{[MarshalAs(UnmanagedType.U1)] public bool Enable;
    public BodyType BodyType;
    public AssetHandle PhysicsMaterial;
    [MarshalAs(UnmanagedType.I1)]
    public bool Simulated;
    public float Mass;
    public float LinearDamping;
    public float AngularDamping;
    public float GravityScale;
    public CollisionDetectionType CollisionDetection;
    public SleepingMode SleepingMode;
    public BodyConstraints Constraints;
}
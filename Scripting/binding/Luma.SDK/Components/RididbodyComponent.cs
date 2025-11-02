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

        #region Core Methods

        
        
        
        
        
        public void ApplyForce(Vector2 force, ForceMode mode = ForceMode.Force)
        {
            
            Native.Physics_ApplyForce(Entity.ScenePtr, Entity.Id, force, mode);
        }

        #endregion

        #region Behavior Methods

        
        
        
        
        
        
        
        public bool MoveTowards(Vector2 target, float maxSpeed, float stopDistance = 1.0f)
        {
            var tr = this.Entity.GetComponent<Transform>();
            if (tr is null)
            {
                
                return false;
            }

            
            if (this.BodyType == BodyType.Static)
            {
                return false;
            }

            var toTarget = Sub(target, tr.Position);
            float dist = Length(toTarget);
            if (dist <= stopDistance)
            {
                this.LinearVelocity = default;
                return true;
            }

            var dir = SafeNormalize(toTarget);
            this.LinearVelocity = Scale(dir, maxSpeed);
            return false;
        }

        
        
        
        
        
        
        
        
        public bool Arrive(Vector2 target, float maxSpeed, float arriveRadius = 64f, float stopDistance = 1.0f)
        {
            var tr = this.Entity.GetComponent<Transform>();
            if (tr is null)
            {
                return false;
            }
            if (this.BodyType == BodyType.Static)
            {
                return false;
            }

            var toTarget = Sub(target, tr.Position);
            float dist = Length(toTarget);
            if (dist <= stopDistance)
            {
                this.LinearVelocity = default;
                return true;
            }

            float speed = maxSpeed;
            if (dist < arriveRadius && arriveRadius > 1e-5f)
            {
                
                speed *= dist / arriveRadius;
            }
            this.LinearVelocity = Scale(SafeNormalize(toTarget), speed);
            return false;
        }

        
        
        
        
        
        
        
        public void Seek(Vector2 target, float desiredSpeed, float steeringForce)
        {
            var tr = this.Entity.GetComponent<Transform>();
            if (tr is null)
            {
                return;
            }
            if (this.BodyType == BodyType.Static)
            {
                return;
            }

            var desiredVel = Scale(SafeNormalize(Sub(target, tr.Position)), desiredSpeed);

            
            var currentVel = this.Entity.GetComponentProperty<Vector2>(ComponentName, "linearVelocity");

            var steering = Sub(desiredVel, currentVel);
            var force = Scale(SafeNormalize(steering), steeringForce);
            this.ApplyForce(force, ForceMode.Force);
        }

        
        
        
        
        
        public void ApplyImpulseTowards(Vector2 target, float impulse)
        {
            var tr = this.Entity.GetComponent<Transform>();
            if (tr is null)
            {
                return;
            }

            var dir = SafeNormalize(Sub(target, tr.Position));
            this.ApplyForce(Scale(dir, impulse), ForceMode.Impulse);
        }

        
        
        
        public void Stop()
        {
            this.LinearVelocity = default;
            this.AngularVelocity = 0f;
        }

        
        
        
        
        
        
        
        public void Teleport(Vector2 position, float? rotationRadians = null)
        {
            
            var tr = this.Entity.GetComponent<Transform>() ?? this.Entity.AddComponent<Transform>();
            tr.Position = position;
            if (rotationRadians.HasValue)
            {
                tr.Rotation = rotationRadians.Value;
            }
        }

        
        
        
        
        
        
        public void SetConstraints(bool freezeX = false, bool freezeY = false, bool freezeRotation = false)
        {
            
            var c = this.Constraints;

            
            c.FreezePositionX = freezeX;
            c.FreezePositionY = freezeY;
            c.FreezeRotation = freezeRotation;

            
            this.Constraints = c;
        }

        #endregion

        #region Private Vector Helpers

        

        private static float Length(in Vector2 v) => MathF.Sqrt(v.X * v.X + v.Y * v.Y);

        private static Vector2 SafeNormalize(in Vector2 v)
        {
            float len = Length(v);
            
            if (len <= 1e-5f) return default;
            float inv = 1.0f / len;
            return new Vector2 { X = v.X * inv, Y = v.Y * inv };
        }

        private static Vector2 Scale(in Vector2 v, float s) => new() { X = v.X * s, Y = v.Y * s };

        private static Vector2 Sub(in Vector2 a, in Vector2 b) => new() { X = a.X - b.X, Y = a.Y - b.Y };

        #endregion
    }
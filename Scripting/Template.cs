using Luma.SDK;
using Luma.SDK.Components;
using System.Numerics;

namespace GameScripts
{
    
    
    
    
    public class Template : Script
    {
        
        
        
        

        
        
        
        [Export] public float MoveSpeed = 300.0f;

        
        
        
        [Export] public float JumpForce = 500.0f;

        
        
        
        [Export] public AssetHandle BulletPrefab;
        
        
        
        
        [Export] public BodyType PlayerBodyType = BodyType.Dynamic;

        
        
        
        
        private Transform _transform;
        private RigidBodyComponent _rigidBody;
        private AnimationController _animController;
        private BoxColliderComponent _boxCollider; 
        
        private bool _isGrounded = false;
        private float _shootCooldown = 0.0f;

        
        
        
        private class SimpleCalculationJob : IJob
        {
            public void Execute()
            {
                
                long result = 0;
                for(int i = 0; i < 100000; i++)
                {
                    result += i;
                }
                Debug.Log($"[JobSystem] SimpleCalculationJob completed with result: {result}");
            }
        }

        
        
        

        
        
        
        public override void OnCreate()
        {
            
            
            _transform = Self.GetComponent<Transform>();
            _rigidBody = Self.GetComponent<RigidBodyComponent>();
            
            
            if (Self.HasComponent<BoxColliderComponent>())
            {
                _boxCollider = Self.GetComponent<BoxColliderComponent>();
            }
            else
            {
                Debug.LogWarning("Player is missing BoxColliderComponent. Adding one automatically.");
                _boxCollider = (BoxColliderComponent)Self.AddComponent<BoxColliderComponent>();
            }
            
            
            _animController = AnimationSystem.GetController(Self);
            
            Debug.Log($"Player '{Self.Name}' created at position: {_transform.Position}");

            
            
            JobSystem.Schedule(new SimpleCalculationJob());
        }

        
        
        
        public override void OnUpdate(float deltaTime)
        {
            
            
            
            _rigidBody = Self.GetComponent<RigidBodyComponent>();
            
            
            Vector2 velocity = _rigidBody.LinearVelocity;
            float horizontalInput = 0;

            if (Input.IsKeyPressed(Scancode.A) || Input.IsKeyPressed(Scancode.Left))
            {
                horizontalInput = -1;
            }
            else if (Input.IsKeyPressed(Scancode.D) || Input.IsKeyPressed(Scancode.Right))
            {
                horizontalInput = 1;
            }

            velocity.X = horizontalInput * MoveSpeed * deltaTime;
            
            if (_isGrounded && Input.IsKeyJustPressed(Scancode.Space))
            {
                velocity.Y = -JumpForce; 
                _animController?.SetTrigger("Jump");
            }
            
            
            
            _rigidBody.LinearVelocity = velocity;
            Self.SetComponent(_rigidBody);

            
            _animController?.SetBool("IsRunning", horizontalInput != 0);
            _animController?.SetBool("IsGrounded", _isGrounded);

            
            _shootCooldown -= deltaTime;
            if (Input.IsKeyPressed(Scancode.F) && _shootCooldown <= 0)
            {
                if (BulletPrefab.IsValid())
                {
                    
                    Scene.Instantiate(BulletPrefab, Self);
                    _shootCooldown = 0.5f; 
                }
            }
        }

        
        
        
        
        
        
        
        public override void OnCollisionEnter(Entity other)
        {
            Debug.Log($"{Self.Name} collided with {other.Name}");
            
            if (other.Name == "Ground")
            {
                _isGrounded = true;
            }
        }

        
        
        
        public override void OnCollisionExit(Entity other)
        {
            if (other.Name == "Ground")
            {
                _isGrounded = false;
            }
        }
        
        
        
        
        public override void OnTriggerEnter(Entity other)
        {
            
            if (other.Name.StartsWith("Coin"))
            {
                Debug.Log("Coin collected!");
                Scene.Destroy(other);
            }
        }
        
        
        
        

        public override void OnEnable()
        {
            Debug.Log($"{Self.Name} script was enabled.");
        }

        public override void OnDisable()
        {
            Debug.Log($"{Self.Name} script was disabled.");
        }
        
        
        
        
        public override void OnDestroy()
        {
            Debug.Log($"Player '{Self.Name}' is being destroyed.");
        }
    }
}
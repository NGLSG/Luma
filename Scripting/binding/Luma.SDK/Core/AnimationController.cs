using System;

namespace Luma.SDK
{
    
    
    
    
    public sealed class AnimationController
    {
        private readonly Entity _entity;

        
        
        
        
        
        
        internal AnimationController(Entity entity)
        {
            if (!entity.IsValid())
            {
                throw new ArgumentException("Cannot create AnimationController for an invalid entity.", nameof(entity));
            }

            _entity = entity;
        }

        
        
        
        
        
        
        public void Play(string animationName, float speed = 1.0f, float transitionDuration = 0.1f)
        {
            Native.AnimationController_Play(_entity.ScenePtr, _entity.Id, animationName, speed, transitionDuration);
        }

        
        
        
        public void Stop()
        {
            Native.AnimationController_Stop(_entity.ScenePtr, _entity.Id);
        }

        
        
        
        
        
        public bool IsPlaying(string animationName)
        {
            return Native.AnimationController_IsPlaying(_entity.ScenePtr, _entity.Id, animationName);
        }

        
        
        
        
        
        
        public void SetFloat(string name, float value)
        {
            Native.AnimationController_SetFloat(_entity.ScenePtr, _entity.Id, name, value);
        }

        
        
        
        
        
        public void SetBool(string name, bool value)
        {
            Native.AnimationController_SetBool(_entity.ScenePtr, _entity.Id, name, value);
        }

        
        
        
        
        
        public void SetInt(string name, int value)
        {
            Native.AnimationController_SetInt(_entity.ScenePtr, _entity.Id, name, value);
        }

        
        
        
        
        public void SetTrigger(string name)
        {
            
            
            Native.AnimationController_SetBool(_entity.ScenePtr, _entity.Id, name, true);
        }

        
        
        
        
        public void SetFrameRate(float frameRate)
        {
            Native.AnimationController_SetFrameRate(_entity.ScenePtr, _entity.Id, frameRate);
        }
    }
}
using System;
using Luma.SDK.Components;

namespace Luma.SDK
{
    
    
    
    
    public static class AnimationSystem
    {
        
        
        

        
        public static AnimationController GetController(Entity entity)
        {
            
            if (!entity.IsValid())
            {
                throw new ArgumentException("Cannot get an AnimationController for an invalid Entity.", nameof(entity));
            }

            
            if (!entity.HasComponent<AnimationControllerComponent>())
            {
                throw new InvalidOperationException(
                    $"Entity '{entity.Id}' does not have an 'AnimatorControllerComponent', which is required to get an AnimationController.");
            }

            
            return new AnimationController(entity);
        }
    }
}
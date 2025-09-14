using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct AnimationControllerComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle animationControllerHandle;
    public int targetFrame;
}
public class AnimationController : LogicComponent<AnimationControllerComponent>
{
    
    
    
    private const string ComponentName = "AnimationControllerComponent";

    
    
    
    
    public AnimationController(Entity entity) : base(entity)
    {
    }

    
    
    
    public AssetHandle AnimationControllerHandle
    {
        
        get => _component.animationControllerHandle;

        
        set
        {
            _component.animationControllerHandle = value;
            
            Entity.SetComponentProperty(ComponentName, "animationController", in value);
        }
    }

    
    
    
    public int TargetFrame
    {
        get => _component.targetFrame;
        set
        {
            _component.targetFrame = value;
            Entity.SetComponentProperty(ComponentName, "targetFrame", in value);
        }
    }

    #region Animation Control Methods

    

    public void Play(string animationName, float speed = 1.0f, float transitionDuration = 0.1f)
    {
        Native.AnimationController_Play(Entity.ScenePtr, Entity.Id, animationName, speed, transitionDuration);
    }
    
    public void Stop()
    {
        Native.AnimationController_Stop(Entity.ScenePtr, Entity.Id);
    }
    
    public bool IsPlaying(string animationName)
    {
        return Native.AnimationController_IsPlaying(Entity.ScenePtr, Entity.Id, animationName);
    }
    
    public void SetFloat(string name, float value)
    {
        Native.AnimationController_SetFloat(Entity.ScenePtr, Entity.Id, name, value);
    }
    
    public void SetBool(string name, bool value)
    {
        Native.AnimationController_SetBool(Entity.ScenePtr, Entity.Id, name, value);
    }
    
    public void SetInt(string name, int value)
    {
        Native.AnimationController_SetInt(Entity.ScenePtr, Entity.Id, name, value);
    }
    
    public void SetTrigger(string name)
    {
        Native.AnimationController_SetTrigger(Entity.ScenePtr, Entity.Id, name);
    }
    
    public void SetFrameRate(float frameRate)
    {
        Native.AnimationController_SetFrameRate(Entity.ScenePtr, Entity.Id, frameRate);
    }

    #endregion
}
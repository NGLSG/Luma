using System.Runtime.InteropServices;
using Luma.SDK.Generation;
namespace Luma.SDK.Components;
public enum ParticlePlayState
{
    Stopped = 0,
    Playing = 1,
    Paused = 2
}
[StructLayout(LayoutKind.Sequential)]
public struct ParticleSystemComponentData : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
}
public class ParticleSystem : LogicComponent<ParticleSystemComponentData>
{
    private const string ComponentName = "ParticleSystemComponent";
    public ParticleSystem(Entity entity) : base(entity)
    {
    }
    #region Playback Control
    public void Play()
    {
        Native.ParticleSystem_Play(Entity.ScenePtr, Entity.Id);
    }
    public void Pause()
    {
        Native.ParticleSystem_Pause(Entity.ScenePtr, Entity.Id);
    }
    public void Stop(bool clearParticles = true)
    {
        Native.ParticleSystem_Stop(Entity.ScenePtr, Entity.Id, clearParticles);
    }
    public void Restart()
    {
        Native.ParticleSystem_Restart(Entity.ScenePtr, Entity.Id);
    }
    public void Burst(uint count)
    {
        Native.ParticleSystem_Burst(Entity.ScenePtr, Entity.Id, count);
    }
    #endregion
    #region Properties
    public uint ParticleCount => Native.ParticleSystem_GetParticleCount(Entity.ScenePtr, Entity.Id);
    public ParticlePlayState PlayState => (ParticlePlayState)Native.ParticleSystem_GetPlayState(Entity.ScenePtr, Entity.Id);
    public bool IsPlaying => Native.ParticleSystem_IsPlaying(Entity.ScenePtr, Entity.Id);
    public bool IsComplete => Native.ParticleSystem_IsComplete(Entity.ScenePtr, Entity.Id);
    public float Duration
    {
        get => Native.ParticleSystem_GetDuration(Entity.ScenePtr, Entity.Id);
        set => Native.ParticleSystem_SetDuration(Entity.ScenePtr, Entity.Id, value);
    }
    public bool Loop
    {
        get => Native.ParticleSystem_GetLoop(Entity.ScenePtr, Entity.Id);
        set => Native.ParticleSystem_SetLoop(Entity.ScenePtr, Entity.Id, value);
    }
    public float SimulationSpeed
    {
        get => Native.ParticleSystem_GetSimulationSpeed(Entity.ScenePtr, Entity.Id);
        set => Native.ParticleSystem_SetSimulationSpeed(Entity.ScenePtr, Entity.Id, value);
    }
    public float SystemTime => Native.ParticleSystem_GetSystemTime(Entity.ScenePtr, Entity.Id);
    public float EmissionRate
    {
        get => Native.ParticleSystem_GetEmissionRate(Entity.ScenePtr, Entity.Id);
        set => Native.ParticleSystem_SetEmissionRate(Entity.ScenePtr, Entity.Id, value);
    }
    public bool PlayOnAwake
    {
        get => Native.ParticleSystem_GetPlayOnAwake(Entity.ScenePtr, Entity.Id);
        set => Native.ParticleSystem_SetPlayOnAwake(Entity.ScenePtr, Entity.Id, value);
    }
    #endregion
}

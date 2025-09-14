using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct AudioComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle audioHandle;
    [MarshalAs(UnmanagedType.I1)] public bool playOnStart;
    [MarshalAs(UnmanagedType.I1)] public bool loop;
    public float volume;
    public bool spatial;
    public float minDistance;
    public float maxDistance;
    public float pitch;
}



public class Audio : LogicComponent<AudioComponent>
{
    
    
    
    private const string ComponentName = "AudioComponent";
    
    private AudioSource? _playingSource;

    
    
    
    public Audio(Entity entity) : base(entity) { }

    #region Component Properties

    public AssetHandle AudioHandle
    {
        get => _component.audioHandle;
        set
        {
            _component.audioHandle = value;
            Entity.SetComponentProperty(ComponentName, "audioHandle", in value);
        }
    }

    public bool PlayOnStart
    {
        get => _component.playOnStart;
        set
        {
            _component.playOnStart = value;
            Entity.SetComponentProperty(ComponentName, "playOnStart", in value);
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

    public float Volume
    {
        get => _component.volume;
        set
        {
            _component.volume = value;
            Entity.SetComponentProperty(ComponentName, "volume", in value);
        }
    }

    public bool Spatial
    {
        get => _component.spatial;
        set
        {
            _component.spatial = value;
            Entity.SetComponentProperty(ComponentName, "spatial", in value);
        }
    }

    public float MinDistance
    {
        get => _component.minDistance;
        set
        {
            _component.minDistance = value;
            Entity.SetComponentProperty(ComponentName, "minDistance", in value);
        }
    }

    public float MaxDistance
    {
        get => _component.maxDistance;
        set
        {
            _component.maxDistance = value;
            Entity.SetComponentProperty(ComponentName, "maxDistance", in value);
        }
    }

    public float Pitch
    {
        get => _component.pitch;
        set
        {
            _component.pitch = value;
            Entity.SetComponentProperty(ComponentName, "pitch", in value);
        }
    }

    #endregion

    #region Playback Control

    
    
    
    
    public AudioSource? Play()
    {
        Stop();

        var desc = new PlayDesc
        {
            AudioHandle = AudioHandle,
            Loop = Loop,
            Volume = Volume,
            Spatial = Spatial,
            MinDistance = MinDistance,
            MaxDistance = MaxDistance
            
        };

        if (Spatial)
        {
            
            var transform = Entity.GetComponent<Transform>();
            if (transform != null)
            {
                var position = transform.Position; 
                desc.SourceX = position.X;
                desc.SourceY = position.Y;
                desc.SourceZ = 0;
            }
        }

        _playingSource = AudioManager.Play(desc);
        return _playingSource;
    }

    
    
    
    public void Stop()
    {
        if (_playingSource != null && !_playingSource.IsFinished)
        {
            _playingSource.Stop();
        }
        _playingSource = null;
    }

    
    
    
    public bool IsPlaying => _playingSource != null && !_playingSource.IsFinished;
    
    #endregion
}
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

[GenerateLogicComponentProperties]
    public partial class Audio : LogicComponent<AudioComponent>
    {
        private AudioSource? _playingSource;
        
        public Audio(Entity entity) : base(entity) { }

        
        public partial AssetHandle AudioHandle { get; set; }
        public partial bool PlayOnStart { get; set; }
        public partial bool Loop { get; set; }
        public partial float Volume { get; set; }
        public partial bool Spatial { get; set; }
        public partial float MinDistance { get; set; }
        public partial float MaxDistance { get; set; }
        public partial float Pitch { get; set; } 

        
        
        
        
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
                    desc.SourceX = transform.Position.X;
                    desc.SourceY = transform.Position.Y;
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
    }
using System.Numerics;
using System.Runtime.InteropServices;
using Luma.SDK.Components;

namespace Luma.SDK
{
    
    
    
    [StructLayout(LayoutKind.Sequential)]
    public struct PlayDesc
    {
        public AssetHandle AudioHandle;
        [MarshalAs(UnmanagedType.I1)] public bool Loop;
        public float Volume;
        [MarshalAs(UnmanagedType.I1)] public bool Spatial;
        public float SourceX, SourceY, SourceZ;
        public float MinDistance;
        public float MaxDistance;
    }

    
    
    
    public static class AudioManager
    {
        
        
        
        
        
        public static AudioSource? Play(PlayDesc desc)
        {
            uint voiceId = Native.AudioManager_Play(desc);
            return voiceId != 0 ? new AudioSource(voiceId) : null;
        }

        
        
        
        
        
        
        
        public static AudioSource? Play(AssetHandle handle, float volume = 1.0f, bool loop = false)
        {
            var desc = new PlayDesc
            {
                AudioHandle = handle,
                Volume = volume,
                Loop = loop,
                Spatial = false
            };
            return Play(desc);
        }

        
        
        
        
        
        
        
        
        public static AudioSource? PlayAtPoint(AssetHandle handle, Vector3 position, float volume = 1.0f, bool loop = false)
        {
            var desc = new PlayDesc
            {
                AudioHandle = handle,
                Volume = volume,
                Loop = loop,
                Spatial = true,
                SourceX = position.X,
                SourceY = position.Y,
                SourceZ = position.Z,
                MinDistance = 1.0f,  
                MaxDistance = 30.0f
            };
            return Play(desc);
        }

        
        
        
        public static void StopAll() => Native.AudioManager_StopAll();
    }
}
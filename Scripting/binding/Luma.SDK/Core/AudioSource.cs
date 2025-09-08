using System.Numerics;

namespace Luma.SDK
{
    
    
    
    
    public class AudioSource
    {
        private readonly uint _voiceId;

        internal AudioSource(uint voiceId)
        {
            _voiceId = voiceId;
        }
        
        
        
        
        public bool IsFinished => Native.AudioManager_IsFinished(_voiceId);

        
        
        
        public void Stop() => Native.AudioManager_Stop(_voiceId);
        
        
        
        
        
        public void SetVolume(float volume) => Native.AudioManager_SetVolume(_voiceId, volume);
        
        
        
        
        
        public void SetLoop(bool loop) => Native.AudioManager_SetLoop(_voiceId, loop);
        
        
        
        
        
        public void SetPosition(Vector3 position) => Native.AudioManager_SetVoicePosition(_voiceId, position.X, position.Y, position.Z);
    }
}
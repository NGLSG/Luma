#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H
#include <SDL3/SDL.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "../Utils/LazySingleton.h"
#include "../Utils/Guid.h"
#include "../Resources/RuntimeAsset/RuntimeAudio.h"
class AudioManager : public LazySingleton<AudioManager>
{
public:
    friend class LazySingleton<AudioManager>;
    struct PlayDesc
    {
        sk_sp<RuntimeAudio> audio; 
        bool loop = false;         
        float volume = 1.0f;       
        bool spatial = false;      
        float sourceX = 0.0f;      
        float sourceY = 0.0f;      
        float sourceZ = 0.0f;      
        float minDistance = 1.0f;  
        float maxDistance = 30.0f; 
    };
public:
    bool Initialize(int sampleRate = 48000, int channels = 2);
    void Shutdown();
    uint32_t Play(const PlayDesc& desc);
    void Stop(uint32_t voiceId);
    void StopAll();
    bool IsFinished(uint32_t voiceId) const;
    void SetVolume(uint32_t voiceId, float volume);
    void SetLoop(uint32_t voiceId, bool loop);
    int GetSampleRate() const { return m_sampleRate; }
    int GetChannels() const { return m_channels; }
    void SetVoicePosition(uint32_t voiceId, float x, float y, float z);
    void SetVoiceSpatial(uint32_t voiceId, bool spatial, float minD, float maxD);
    void Mix(float* out, int frames);
private:
    AudioManager() = default;
    struct Voice
    {
        uint32_t id = 0;             
        sk_sp<RuntimeAudio> audio;   
        size_t cursorFrames = 0;     
        bool loop = false;           
        float volume = 1.0f;         
        bool spatial = false;        
        float x = 0.0f, y = 0.0f, z = 0.0f; 
        float minDistance = 1.0f;    
        float maxDistance = 30.0f;   
        bool finished = false;       
    };
    static void SDLAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    void GetListener(float& lx, float& ly, float& lz, float& rx, float& ry, float& rz) const;
private:
    SDL_AudioDeviceID deviceId = 0; 
    int m_sampleRate = 48000;       
    int m_channels = 2;             
    mutable std::mutex mutex;      
    std::unordered_map<uint32_t, Voice> voices; 
    uint32_t nextVoiceId = 1;       
    float masterVolume = 1.0f;      
    SDL_AudioStream* m_audioStream = nullptr; 
};
#endif

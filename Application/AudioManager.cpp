#include "AudioManager.h"
#include "../Utils/Logger.h"
#include <algorithm>
#include <vector>
#include <cmath>
#include "../Renderer/Camera.h"
bool AudioManager::Initialize(int sampleRate, int channels)
{
    if (m_audioStream)
    {
        return true;
    }
    SDL_AudioSpec spec{};
    spec.freq = sampleRate;
    spec.format = SDL_AUDIO_F32;
    spec.channels = static_cast<Uint8>(channels);
    m_audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        &AudioManager::SDLAudioCallback,
        this
    );
    if (m_audioStream == nullptr)
    {
        LogError("AudioManager: Failed to open audio device stream: {}", SDL_GetError());
        return false;
    }
    m_sampleRate = spec.freq;
    m_channels = spec.channels;
    SDL_ResumeAudioStreamDevice(m_audioStream);
    LogInfo("AudioManager: Initialized. {} Hz, channels {}", m_sampleRate, m_channels);
    return true;
}
void AudioManager::Shutdown()
{
    if (m_audioStream)
    {
        SDL_DestroyAudioStream(m_audioStream);
        m_audioStream = nullptr;
    }
    std::scoped_lock lk(mutex);
    voices.clear();
    nextVoiceId = 1;
}
uint32_t AudioManager::Play(const PlayDesc& desc)
{
    if (!m_audioStream || !desc.audio)
    {
        return 0;
    }
    std::scoped_lock lk(mutex);
    Voice v;
    v.id = nextVoiceId++;
    v.audio = desc.audio;
    v.cursorFrames = 0;
    v.loop = desc.loop;
    v.volume = std::clamp(desc.volume, 0.0f, 1.0f);
    v.spatial = desc.spatial;
    v.x = desc.sourceX;
    v.y = desc.sourceY;
    v.z = desc.sourceZ;
    v.minDistance = std::max(0.001f, desc.minDistance);
    v.maxDistance = std::max(v.minDistance, desc.maxDistance);
    v.finished = false;
    voices[v.id] = std::move(v);
    return v.id;
}
void AudioManager::Stop(uint32_t voiceId)
{
    std::scoped_lock lk(mutex);
    voices.erase(voiceId);
}
void AudioManager::StopAll()
{
    std::scoped_lock lk(mutex);
    voices.clear();
}
bool AudioManager::IsFinished(uint32_t voiceId) const
{
    std::scoped_lock lk(mutex);
    auto it = voices.find(voiceId);
    if (it == voices.end()) return true;
    return it->second.finished;
}
void AudioManager::SetVolume(uint32_t voiceId, float volume)
{
    std::scoped_lock lk(mutex);
    auto it = voices.find(voiceId);
    if (it != voices.end()) it->second.volume = std::clamp(volume, 0.0f, 1.0f);
}
void AudioManager::SetLoop(uint32_t voiceId, bool loop)
{
    std::scoped_lock lk(mutex);
    auto it = voices.find(voiceId);
    if (it != voices.end()) it->second.loop = loop;
}
void AudioManager::SetVoicePosition(uint32_t voiceId, float x, float y, float z)
{
    std::scoped_lock lk(mutex);
    auto it = voices.find(voiceId);
    if (it != voices.end())
    {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
    }
}
void AudioManager::SetVoiceSpatial(uint32_t voiceId, bool spatial, float minD, float maxD)
{
    std::scoped_lock lk(mutex);
    auto it = voices.find(voiceId);
    if (it != voices.end())
    {
        it->second.spatial = spatial;
        it->second.minDistance = std::max(0.001f, minD);
        it->second.maxDistance = std::max(it->second.minDistance, maxD);
    }
}
void AudioManager::SDLAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    auto* self = static_cast<AudioManager*>(userdata);
    const int frames_needed = total_amount / (sizeof(float) * self->m_channels);
    if (frames_needed <= 0)
    {
        return;
    }
    std::vector<float> mix_buffer(static_cast<size_t>(frames_needed) * self->m_channels);
    self->Mix(mix_buffer.data(), frames_needed);
    SDL_PutAudioStreamData(stream, mix_buffer.data(), total_amount);
}
void AudioManager::GetListener(float& lx, float& ly, float& lz, float& rx, float& ry, float& rz) const
{
    const auto props = CameraManager::GetInstance().GetActiveCamera().GetProperties();
    lx = props.position.x();
    ly = props.position.y();
    lz = 0.0f;
    const float theta = props.rotation;
    rx = std::cos(theta);
    ry = std::sin(theta);
    rz = 0.0f;
}
void AudioManager::Mix(float* out, int frames)
{
    const int ch = m_channels;
    std::fill_n(out, static_cast<size_t>(frames) * ch, 0.0f);
    float lx = 0, ly = 0, lz = 0, rx = 1, ry = 0, rz = 0;
    GetListener(lx, ly, lz, rx, ry, rz);
    std::scoped_lock lk(mutex);
    std::vector<uint32_t> toRemove;
    toRemove.reserve(8);
    for (auto& kv : voices)
    {
        Voice& v = kv.second;
        if (!v.audio)
        {
            toRemove.push_back(v.id);
            continue;
        }
        const auto& pcm = v.audio->GetPCMData();
        const int srcCh = v.audio->GetChannels();
        const size_t totalFrames = v.audio->GetFrameCount();
        if (srcCh <= 0 || totalFrames == 0)
        {
            toRemove.push_back(v.id);
            continue;
        }
        float gainL = v.volume * masterVolume;
        float gainR = v.volume * masterVolume;
        if (v.spatial)
        {
            float dx = v.x - lx, dy = v.y - ly, dz = v.z - lz;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            dist = std::max(v.minDistance, std::min(dist, v.maxDistance));
            float att = 1.0f - (dist - v.minDistance) / std::max(0.001f, (v.maxDistance - v.minDistance));
            att = std::clamp(att, 0.0f, 1.0f);
            float dotR = (dx * rx + dy * ry + dz * rz);
            float pan = std::clamp(dist > 0.0f ? (dotR / dist) : 0.0f, -1.0f, 1.0f);
            float panL = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
            float panR = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
            gainL *= att * panL;
            gainR *= att * panR;
        }
        for (int f = 0; f < frames; ++f)
        {
            if (v.cursorFrames >= totalFrames)
            {
                if (v.loop)
                {
                    v.cursorFrames = 0;
                }
                else
                {
                    v.finished = true;
                    toRemove.push_back(v.id);
                    break;
                }
            }
            if (v.cursorFrames >= totalFrames) break;
            const size_t base = v.cursorFrames * static_cast<size_t>(srcCh);
            float sL = 0.0f, sR = 0.0f;
            if (srcCh == 1)
            {
                float s = pcm[base];
                sL = sR = s;
            }
            else
            {
                sL = pcm[base + 0];
                sR = pcm[base + 1];
            }
            size_t outBase = static_cast<size_t>(f) * ch;
            out[outBase + 0] += sL * gainL;
            if (ch > 1) out[outBase + 1] += sR * gainR;
            v.cursorFrames++;
        }
    }
    for (uint32_t id : toRemove)
    {
        voices.erase(id);
    }
}

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <SDL3/SDL.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "../Utils/LazySingleton.h"
#include "../Utils/Guid.h"
#include "../Resources/RuntimeAsset/RuntimeAudio.h"

/**
 * @brief 音频管理器，负责音频的播放、停止、音量控制和空间音频处理。
 *
 * 这是一个单例类，通过 LazySingleton 模式确保全局只有一个实例。
 */
class AudioManager : public LazySingleton<AudioManager>
{
public:
    friend class LazySingleton<AudioManager>;

    /**
     * @brief 音频播放描述结构体。
     *
     * 包含播放音频所需的所有参数，如音频资源、循环状态、音量和空间音频设置。
     */
    struct PlayDesc
    {
        sk_sp<RuntimeAudio> audio; ///< 要播放的运行时音频资源。
        bool loop = false;         ///< 是否循环播放。
        float volume = 1.0f;       ///< 播放音量，范围 [0.0f, 1.0f]。
        bool spatial = false;      ///< 是否启用空间音频。

        float sourceX = 0.0f;      ///< 音源的X坐标（启用空间音频时）。
        float sourceY = 0.0f;      ///< 音源的Y坐标（启用空间音频时）。
        float sourceZ = 0.0f;      ///< 音源的Z坐标（启用空间音频时）。
        float minDistance = 1.0f;  ///< 空间音频的最小距离，在此距离内音量最大。
        float maxDistance = 30.0f; ///< 空间音频的最大距离，在此距离外音量为零。
    };

public:
    /**
     * @brief 初始化音频管理器。
     *
     * 设置音频设备的采样率和通道数。
     * @param sampleRate 音频采样率，默认为 48000 Hz。
     * @param channels 音频通道数，默认为 2 (立体声)。
     * @return 如果初始化成功则返回 true，否则返回 false。
     */
    bool Initialize(int sampleRate = 48000, int channels = 2);

    /**
     * @brief 关闭音频管理器。
     *
     * 释放所有音频资源和设备。
     */
    void Shutdown();

    /**
     * @brief 播放一个音频。
     *
     * 根据提供的播放描述播放音频，并返回一个唯一的语音ID。
     * @param desc 包含播放参数的 PlayDesc 结构体。
     * @return 播放音频的唯一语音ID，如果播放失败则返回 0。
     */
    uint32_t Play(const PlayDesc& desc);

    /**
     * @brief 停止指定ID的音频播放。
     * @param voiceId 要停止的音频的语音ID。
     */
    void Stop(uint32_t voiceId);

    /**
     * @brief 停止所有正在播放的音频。
     */
    void StopAll();

    /**
     * @brief 检查指定ID的音频是否已播放完毕。
     * @param voiceId 要检查的音频的语音ID。
     * @return 如果音频已播放完毕或ID无效则返回 true，否则返回 false。
     */
    bool IsFinished(uint32_t voiceId) const;

    /**
     * @brief 设置指定ID音频的播放音量。
     * @param voiceId 要设置音量的音频的语音ID。
     * @param volume 新的音量值，范围 [0.0f, 1.0f]。
     */
    void SetVolume(uint32_t voiceId, float volume);

    /**
     * @brief 设置指定ID音频的循环播放状态。
     * @param voiceId 要设置循环状态的音频的语音ID。
     * @param loop 是否循环播放。
     */
    void SetLoop(uint32_t voiceId, bool loop);

    /**
     * @brief 获取当前音频设备的采样率。
     * @return 音频设备的采样率。
     */
    int GetSampleRate() const { return m_sampleRate; }

    /**
     * @brief 获取当前音频设备的通道数。
     * @return 音频设备的通道数。
     */
    int GetChannels() const { return m_channels; }

    /**
     * @brief 设置指定ID音频的音源位置（用于空间音频）。
     * @param voiceId 要设置位置的音频的语音ID。
     * @param x 音源的X坐标。
     * @param y 音源的Y坐标。
     * @param z 音源的Z坐标。
     */
    void SetVoicePosition(uint32_t voiceId, float x, float y, float z);

    /**
     * @brief 设置指定ID音频的空间音频参数。
     * @param voiceId 要设置空间音频参数的音频的语音ID。
     * @param spatial 是否启用空间音频。
     * @param minD 空间音频的最小距离。
     * @param maxD 空间音频的最大距离。
     */
    void SetVoiceSpatial(uint32_t voiceId, bool spatial, float minD, float maxD);

    /**
     * @brief 混合音频数据到输出缓冲区。
     *
     * 此方法通常由音频回调函数调用，用于将所有活动语音的音频数据混合到提供的输出缓冲区中。
     * @param out 指向浮点型输出缓冲区的指针。
     * @param frames 要混合的音频帧数。
     */
    void Mix(float* out, int frames);

private:
    AudioManager() = default;

    /**
     * @brief 内部使用的语音结构体。
     *
     * 存储每个正在播放的音频的详细状态和参数。
     */
    struct Voice
    {
        uint32_t id = 0;             ///< 语音的唯一ID。
        sk_sp<RuntimeAudio> audio;   ///< 关联的运行时音频资源。
        size_t cursorFrames = 0;     ///< 当前播放到的帧位置。
        bool loop = false;           ///< 是否循环播放。
        float volume = 1.0f;         ///< 播放音量。
        bool spatial = false;        ///< 是否启用空间音频。
        float x = 0.0f, y = 0.0f, z = 0.0f; ///< 音源的X, Y, Z坐标。
        float minDistance = 1.0f;    ///< 空间音频的最小距离。
        float maxDistance = 30.0f;   ///< 空间音频的最大距离。
        bool finished = false;       ///< 语音是否已播放完毕。
    };

    /**
     * @brief SDL音频回调函数。
     *
     * 当SDL音频设备需要更多音频数据时，会调用此函数。
     * @param userdata 用户自定义数据，通常是 AudioManager 实例的指针。
     * @param stream SDL音频流对象。
     * @param additional_amount 额外请求的音频数据量。
     * @param total_amount 总共请求的音频数据量。
     */
    static void SDLAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

    /**
     * @brief 获取听者的位置和方向。
     *
     * 此方法用于空间音频计算，获取当前听者的位置和朝向。
     * @param lx 听者的X坐标。
     * @param ly 听者的Y坐标。
     * @param lz 听者的Z坐标。
     * @param rx 听者右向量的X分量。
     * @param ry 听者右向量的Y分量。
     * @param rz 听者右向量的Z分量。
     */
    void GetListener(float& lx, float& ly, float& lz, float& rx, float& ry, float& rz) const;

private:
    SDL_AudioDeviceID deviceId = 0; ///< SDL音频设备ID。
    int m_sampleRate = 48000;       ///< 音频采样率。
    int m_channels = 2;             ///< 音频通道数。

    mutable std::mutex mutex;      ///< 用于保护共享数据（如 voices 映射）的互斥锁。
    std::unordered_map<uint32_t, Voice> voices; ///< 存储所有活动语音的映射。
    uint32_t nextVoiceId = 1;       ///< 下一个可用的语音ID。
    float masterVolume = 1.0f;      ///< 主音量。
    SDL_AudioStream* m_audioStream = nullptr; ///< SDL音频流。
};

#endif
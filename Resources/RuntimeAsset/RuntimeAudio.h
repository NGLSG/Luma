#ifndef RUNTIMEAUDIO_H
#define RUNTIMEAUDIO_H

#include "IRuntimeAsset.h"
#include <vector>

/**
 * @brief 表示运行时音频数据的类。
 *
 * 该类继承自 IRuntimeAsset，用于存储和管理PCM格式的音频数据。
 */
class RuntimeAudio : public IRuntimeAsset
{
public:
    /**
     * @brief 默认构造函数。
     */
    RuntimeAudio() = default;

    /**
     * @brief 设置PCM音频数据、采样率和声道数。
     * @param data PCM数据向量，使用右值引用进行高效移动。
     * @param sampleRate 音频的采样率（每秒样本数）。
     * @param channels 音频的声道数。
     */
    void SetPCMData(std::vector<float>&& data, int sampleRate, int channels)
    {
        pcmData = std::move(data);
        this->sampleRate = sampleRate;
        this->channels = channels;
    }

    /**
     * @brief 获取PCM音频数据。
     * @return PCM数据向量的常量引用。
     */
    const std::vector<float>& GetPCMData() const { return pcmData; }

    /**
     * @brief 获取音频的采样率。
     * @return 音频的采样率。
     */
    int GetSampleRate() const { return sampleRate; }

    /**
     * @brief 获取音频的声道数。
     * @return 音频的声道数。
     */
    int GetChannels() const { return channels; }

    /**
     * @brief 获取音频的帧数。
     * @return 音频的帧数。如果声道数为0，则返回0。
     */
    size_t GetFrameCount() const
    {
        return channels > 0 ? (pcmData.size() / static_cast<size_t>(channels)) : 0;
    }

    /**
     * @brief 获取音频的总时长（秒）。
     * @return 音频的总时长（秒）。如果采样率为0，则返回0.0f。
     */
    float GetDurationSeconds() const
    {
        size_t frames = GetFrameCount();
        return sampleRate > 0 ? (static_cast<float>(frames) / static_cast<float>(sampleRate)) : 0.0f;
    }

private:
    std::vector<float> pcmData; ///< 存储PCM音频数据的向量。
    int sampleRate = 0;         ///< 音频的采样率。
    int channels = 0;           ///< 音频的声道数。
};

#endif
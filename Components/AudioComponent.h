#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H

#include "IComponent.h"
#include "AssetHandle.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 音频组件，用于管理实体上的音频播放属性。
     */
    struct AudioComponent : public IComponent
    {
        AssetHandle audioHandle = AssetHandle(AssetType::Audio); ///< 音频资源的句柄。
        bool playOnStart = false; ///< 是否在组件启用时自动播放。
        bool loop = false; ///< 音频是否循环播放。
        float volume = 1.0f; ///< 音频的音量，范围通常为0.0到1.0。
        bool spatial = false; ///< 是否启用空间音频效果。
        float minDistance = 1.0f; ///< 空间音频的最小距离，在此距离内音量保持最大。
        float maxDistance = 30.0f; ///< 空间音频的最大距离，在此距离外音量逐渐衰减至无声。
        float pitch = 1.0f; ///< 音频的音高，1.0为正常音高。

        uint32_t voiceId = 0; ///< 内部使用的语音ID，用于管理播放实例。
        bool requestedPlay = false; ///< 内部标志，指示是否已请求播放。

        /**
         * @brief 默认构造函数。
         */
        AudioComponent() = default;
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于将 ECS::AudioComponent 序列化和反序列化为 YAML 节点。
     * @tparam ECS::AudioComponent 要转换的类型。
     */
    template <>
    struct convert<ECS::AudioComponent>
    {
        /**
         * @brief 将 ECS::AudioComponent 编码为 YAML 节点。
         * @param c 要编码的音频组件。
         * @return 表示音频组件的 YAML 节点。
         */
        static Node encode(const ECS::AudioComponent& c)
        {
            Node n;
            n["Enable"] = true;
            n["audioHandle"] = c.audioHandle;
            n["playOnStart"] = c.playOnStart;
            n["loop"] = c.loop;
            n["volume"] = c.volume;
            n["spatial"] = c.spatial;
            n["minDistance"] = c.minDistance;
            n["maxDistance"] = c.maxDistance;
            n["pitch"] = c.pitch;
            return n;
        }

        /**
         * @brief 从 YAML 节点解码 ECS::AudioComponent。
         * @param n 包含音频组件数据的 YAML 节点。
         * @param c 要填充的音频组件。
         * @return 如果解码成功则为 true，否则为 false。
         */
        static bool decode(const Node& n, ECS::AudioComponent& c)
        {
            if (!n.IsMap()) return false;
            if (!n["audioHandle"])
            {
                LogError("AudioComponent: Missing required field 'audioHandle'.");
                return false;
            }
            c.Enable = n["Enable"].as<bool>(true);
            c.audioHandle = n["audioHandle"].as<AssetHandle>();
            c.playOnStart = n["playOnStart"].as<bool>(false);
            c.loop = n["loop"].as<bool>(false);
            c.volume = n["volume"].as<float>(1.0f);
            c.spatial = n["spatial"].as<bool>(false);
            c.minDistance = n["minDistance"].as<float>(1.0f);
            c.maxDistance = n["maxDistance"].as<float>(30.0f);
            c.pitch = n["pitch"].as<float>(1.0f);
            c.voiceId = 0;
            c.requestedPlay = false;
            return true;
        }
    };
}

/**
 * @brief 注册 ECS::AudioComponent 及其可序列化属性到组件注册表。
 */
REGISTRY
{
    Registry_<ECS::AudioComponent>("AudioComponent")
        .property("audioHandle", &ECS::AudioComponent::audioHandle)
        .property("playOnStart", &ECS::AudioComponent::playOnStart)
        .property("loop", &ECS::AudioComponent::loop)
        .property("volume", &ECS::AudioComponent::volume)
        .property("spatial", &ECS::AudioComponent::spatial)
        .property("minDistance", &ECS::AudioComponent::minDistance)
        .property("maxDistance", &ECS::AudioComponent::maxDistance)
        .property("pitch", &ECS::AudioComponent::pitch);
}

#endif
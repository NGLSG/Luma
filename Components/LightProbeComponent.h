#ifndef LIGHT_PROBE_COMPONENT_H
#define LIGHT_PROBE_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"
#include "../Utils/LayerMask.h"

namespace ECS
{
    /**
     * @brief 光照探针网格配置结构体
     * 
     * 定义光照探针网格的生成参数，用于自动生成探针网格。
     * 
     * Requirements: 3.1, 3.6
     */
    struct LightProbeGridConfig
    {
        Vector2f gridOrigin = Vector2f(0.0f, 0.0f);  ///< 网格原点（世界坐标）
        Vector2f gridSize = Vector2f(100.0f, 100.0f);///< 网格尺寸（世界单位）
        Vector2i probeCount = Vector2i(10, 10);      ///< 探针数量（X, Y）
        float updateFrequency = 0.1f;                ///< 实时更新频率（秒）
        bool autoGenerate = true;                    ///< 是否自动生成探针

        /**
         * @brief 默认构造函数
         */
        LightProbeGridConfig() = default;

        /**
         * @brief 带参数的构造函数
         * @param origin 网格原点
         * @param size 网格尺寸
         * @param count 探针数量
         * @param frequency 更新频率
         * @param autoGen 是否自动生成
         */
        LightProbeGridConfig(const Vector2f& origin, const Vector2f& size,
                            const Vector2i& count, float frequency, bool autoGen)
            : gridOrigin(origin)
            , gridSize(size)
            , probeCount(count)
            , updateFrequency(frequency)
            , autoGenerate(autoGen)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightProbeGridConfig& other) const
        {
            return gridOrigin == other.gridOrigin &&
                   gridSize == other.gridSize &&
                   probeCount == other.probeCount &&
                   updateFrequency == other.updateFrequency &&
                   autoGenerate == other.autoGenerate;
        }

        bool operator!=(const LightProbeGridConfig& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 光照探针组件
     * 
     * 光照探针在场景中采样点存储光照信息，用于间接光照计算。
     * 支持烘焙模式（预计算静态光照）和实时模式（动态更新）。
     * 
     * Requirements: 3.1, 3.2, 3.6
     */
    struct LightProbeComponent : public IComponent
    {
        Color sampledColor = Color(0.0f, 0.0f, 0.0f, 1.0f);  ///< 采样的间接光颜色
        float sampledIntensity = 0.0f;                        ///< 采样的强度
        float influenceRadius = 5.0f;                         ///< 影响半径（世界单位）
        bool isBaked = false;                                 ///< 是否已烘焙
        LayerMask layerMask;                                  ///< 影响的光照层掩码

        /**
         * @brief 默认构造函数
         */
        LightProbeComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param color 采样颜色
         * @param intensity 采样强度
         * @param radius 影响半径
         * @param baked 是否已烘焙
         */
        LightProbeComponent(const Color& color, float intensity, 
                           float radius, bool baked)
            : sampledColor(color)
            , sampledIntensity(intensity)
            , influenceRadius(radius)
            , isBaked(baked)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightProbeComponent& other) const
        {
            return sampledColor == other.sampledColor &&
                   sampledIntensity == other.sampledIntensity &&
                   influenceRadius == other.influenceRadius &&
                   isBaked == other.isBaked &&
                   layerMask.value == other.layerMask.value &&
                   Enable == other.Enable;
        }

        bool operator!=(const LightProbeComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 LightProbeData 结构
         * @param position 探针世界坐标位置
         * @return LightProbeData 结构
         */
        LightProbeData ToLightProbeData(const glm::vec2& position) const
        {
            LightProbeData data;
            data.position = position;
            data.influenceRadius = influenceRadius;
            data.sampledColor = glm::vec3(sampledColor.r, sampledColor.g, sampledColor.b);
            data.sampledIntensity = sampledIntensity;
            return data;
        }
    };
}

namespace YAML
{
    /**
     * @brief LightProbeGridConfig 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::LightProbeGridConfig>
    {
        /**
         * @brief 将 LightProbeGridConfig 编码为 YAML 节点
         * @param config 光照探针网格配置
         * @return YAML 节点
         */
        static Node encode(const ECS::LightProbeGridConfig& config)
        {
            Node node;
            node["gridOrigin"] = config.gridOrigin;
            node["gridSize"] = config.gridSize;
            node["probeCount"] = config.probeCount;
            node["updateFrequency"] = config.updateFrequency;
            node["autoGenerate"] = config.autoGenerate;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 LightProbeGridConfig
         * @param node YAML 节点
         * @param config 光照探针网格配置
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::LightProbeGridConfig& config)
        {
            if (!node.IsMap())
                return false;

            if (node["gridOrigin"])
                config.gridOrigin = node["gridOrigin"].as<ECS::Vector2f>();

            if (node["gridSize"])
                config.gridSize = node["gridSize"].as<ECS::Vector2f>();

            if (node["probeCount"])
                config.probeCount = node["probeCount"].as<ECS::Vector2i>();

            if (node["updateFrequency"])
                config.updateFrequency = node["updateFrequency"].as<float>();

            if (node["autoGenerate"])
                config.autoGenerate = node["autoGenerate"].as<bool>();

            return true;
        }
    };

    /**
     * @brief LightProbeComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::LightProbeComponent>
    {
        /**
         * @brief 将 LightProbeComponent 编码为 YAML 节点
         * @param probe 光照探针组件
         * @return YAML 节点
         */
        static Node encode(const ECS::LightProbeComponent& probe)
        {
            Node node;
            node["Enable"] = probe.Enable;
            node["sampledColor"] = probe.sampledColor;
            node["sampledIntensity"] = probe.sampledIntensity;
            node["influenceRadius"] = probe.influenceRadius;
            node["isBaked"] = probe.isBaked;
            node["layerMask"] = probe.layerMask.value;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 LightProbeComponent
         * @param node YAML 节点
         * @param probe 光照探针组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::LightProbeComponent& probe)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                probe.Enable = node["Enable"].as<bool>();

            // 采样颜色
            if (node["sampledColor"])
                probe.sampledColor = node["sampledColor"].as<ECS::Color>();

            // 采样强度
            if (node["sampledIntensity"])
                probe.sampledIntensity = node["sampledIntensity"].as<float>();

            // 影响半径
            if (node["influenceRadius"])
                probe.influenceRadius = node["influenceRadius"].as<float>();

            // 是否已烘焙
            if (node["isBaked"])
                probe.isBaked = node["isBaked"].as<bool>();

            // 光照层掩码
            if (node["layerMask"])
                probe.layerMask.value = node["layerMask"].as<uint32_t>();

            return true;
        }
    };
}

/**
 * @brief 注册 LightProbeComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::LightProbeComponent>("LightProbeComponent")
        .property("sampledColor", &ECS::LightProbeComponent::sampledColor)
        .property("sampledIntensity", &ECS::LightProbeComponent::sampledIntensity)
        .property("influenceRadius", &ECS::LightProbeComponent::influenceRadius)
        .property("isBaked", &ECS::LightProbeComponent::isBaked)
        .property("layerMask", &ECS::LightProbeComponent::layerMask);
}

#endif // LIGHT_PROBE_COMPONENT_H

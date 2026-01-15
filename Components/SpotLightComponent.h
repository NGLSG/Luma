#ifndef SPOT_LIGHT_COMPONENT_H
#define SPOT_LIGHT_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"
#include "../Utils/LayerMask.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ECS
{
    /**
     * @brief 聚光灯组件
     * 
     * 聚光灯从一个点向特定方向发射锥形光线。
     * 可用于创建手电筒、探照灯等定向照明效果。
     */
    struct SpotLightComponent : public IComponent
    {
        Color color = Colors::White;                              ///< 光源颜色
        float intensity = 1.0f;                                   ///< 光照强度 [0, ∞)
        float radius = 10.0f;                                     ///< 影响半径（世界单位）
        float innerAngle = 30.0f;                                 ///< 内角（度），完全照亮区域
        float outerAngle = 45.0f;                                 ///< 外角（度），光照衰减边界
        AttenuationType attenuation = AttenuationType::Quadratic; ///< 衰减类型
        LayerMask layerMask;                                      ///< 影响的光照层掩码
        int priority = 0;                                         ///< 优先级（用于剔除时排序）
        bool castShadows = false;                                 ///< 是否投射阴影

        /**
         * @brief 默认构造函数
         */
        SpotLightComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param lightColor 光源颜色
         * @param lightIntensity 光照强度
         * @param lightRadius 影响半径
         * @param inner 内角（度）
         * @param outer 外角（度）
         */
        SpotLightComponent(const Color& lightColor, float lightIntensity, float lightRadius,
                           float inner, float outer)
            : color(lightColor)
            , intensity(lightIntensity)
            , radius(lightRadius)
            , innerAngle(inner)
            , outerAngle(outer)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const SpotLightComponent& other) const
        {
            return color == other.color &&
                   intensity == other.intensity &&
                   radius == other.radius &&
                   innerAngle == other.innerAngle &&
                   outerAngle == other.outerAngle &&
                   attenuation == other.attenuation &&
                   layerMask.value == other.layerMask.value &&
                   priority == other.priority &&
                   castShadows == other.castShadows &&
                   Enable == other.Enable;
        }

        bool operator!=(const SpotLightComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 LightData 结构
         * @param position 光源世界坐标位置
         * @param direction 光源方向（归一化）
         * @return LightData 结构
         */
        LightData ToLightData(const glm::vec2& position, const glm::vec2& direction) const
        {
            LightData data;
            data.position = position;
            data.direction = direction;
            data.color = glm::vec4(color.r, color.g, color.b, color.a);
            data.intensity = intensity;
            data.radius = radius;
            // 将角度从度转换为弧度
            data.innerAngle = innerAngle * static_cast<float>(M_PI) / 180.0f;
            data.outerAngle = outerAngle * static_cast<float>(M_PI) / 180.0f;
            data.lightType = static_cast<uint32_t>(LightType::Spot);
            data.layerMask = layerMask.value;
            data.attenuation = static_cast<float>(attenuation);
            data.castShadows = castShadows ? 1u : 0u;
            return data;
        }

        /**
         * @brief 获取内角的弧度值
         * @return 内角（弧度）
         */
        float GetInnerAngleRadians() const
        {
            return innerAngle * static_cast<float>(M_PI) / 180.0f;
        }

        /**
         * @brief 获取外角的弧度值
         * @return 外角（弧度）
         */
        float GetOuterAngleRadians() const
        {
            return outerAngle * static_cast<float>(M_PI) / 180.0f;
        }
    };
}

namespace YAML
{
    /**
     * @brief SpotLightComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::SpotLightComponent>
    {
        /**
         * @brief 将 SpotLightComponent 编码为 YAML 节点
         * @param light 聚光灯组件
         * @return YAML 节点
         */
        static Node encode(const ECS::SpotLightComponent& light)
        {
            Node node;
            node["Enable"] = light.Enable;
            node["color"] = light.color;
            node["intensity"] = light.intensity;
            node["radius"] = light.radius;
            node["innerAngle"] = light.innerAngle;
            node["outerAngle"] = light.outerAngle;
            node["attenuation"] = light.attenuation;
            node["layerMask"] = light.layerMask.value;
            node["priority"] = light.priority;
            node["castShadows"] = light.castShadows;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 SpotLightComponent
         * @param node YAML 节点
         * @param light 聚光灯组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::SpotLightComponent& light)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                light.Enable = node["Enable"].as<bool>();

            // 可选字段，使用默认值
            if (node["color"])
                light.color = node["color"].as<ECS::Color>();

            if (node["intensity"])
                light.intensity = node["intensity"].as<float>();

            if (node["radius"])
                light.radius = node["radius"].as<float>();

            if (node["innerAngle"])
                light.innerAngle = node["innerAngle"].as<float>();

            if (node["outerAngle"])
                light.outerAngle = node["outerAngle"].as<float>();

            if (node["attenuation"])
                light.attenuation = node["attenuation"].as<ECS::AttenuationType>();

            if (node["layerMask"])
                light.layerMask.value = node["layerMask"].as<uint32_t>();

            if (node["priority"])
                light.priority = node["priority"].as<int>();

            if (node["castShadows"])
                light.castShadows = node["castShadows"].as<bool>();

            return true;
        }
    };
}
/**
 * @brief 注册 SpotLightComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::SpotLightComponent>("SpotLightComponent")
        .property("color", &ECS::SpotLightComponent::color)
        .property("intensity", &ECS::SpotLightComponent::intensity)
        .property("radius", &ECS::SpotLightComponent::radius)
        .property("innerAngle", &ECS::SpotLightComponent::innerAngle)
        .property("outerAngle", &ECS::SpotLightComponent::outerAngle)
        .property("attenuation", &ECS::SpotLightComponent::attenuation)
        .property("layerMask", &ECS::SpotLightComponent::layerMask)
        .property("priority", &ECS::SpotLightComponent::priority)
        .property("castShadows", &ECS::SpotLightComponent::castShadows);
}

#endif // SPOT_LIGHT_COMPONENT_H

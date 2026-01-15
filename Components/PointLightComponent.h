#ifndef POINT_LIGHT_COMPONENT_H
#define POINT_LIGHT_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"
#include "../Utils/LayerMask.h"

namespace ECS
{
    /**
     * @brief 点光源组件
     * 
     * 点光源从一个点向所有方向发射光线，光照强度随距离衰减。
     * 可用于创建火把、灯泡等局部照明效果。
     */
    struct PointLightComponent : public IComponent
    {
        Color color = Colors::White;                              ///< 光源颜色
        float intensity = 1.0f;                                   ///< 光照强度 [0, ∞)
        float radius = 5.0f;                                      ///< 影响半径（世界单位）
        AttenuationType attenuation = AttenuationType::Quadratic; ///< 衰减类型
        LayerMask layerMask;                                      ///< 影响的光照层掩码
        int priority = 0;                                         ///< 优先级（用于剔除时排序）
        bool castShadows = false;                                 ///< 是否投射阴影

        /**
         * @brief 默认构造函数
         */
        PointLightComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param lightColor 光源颜色
         * @param lightIntensity 光照强度
         * @param lightRadius 影响半径
         */
        PointLightComponent(const Color& lightColor, float lightIntensity, float lightRadius)
            : color(lightColor)
            , intensity(lightIntensity)
            , radius(lightRadius)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const PointLightComponent& other) const
        {
            return color == other.color &&
                   intensity == other.intensity &&
                   radius == other.radius &&
                   attenuation == other.attenuation &&
                   layerMask.value == other.layerMask.value &&
                   priority == other.priority &&
                   castShadows == other.castShadows &&
                   Enable == other.Enable;
        }

        bool operator!=(const PointLightComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 LightData 结构
         * @param position 光源世界坐标位置
         * @return LightData 结构
         */
        LightData ToLightData(const glm::vec2& position) const
        {
            LightData data;
            data.position = position;
            data.direction = glm::vec2(0.0f, 0.0f);
            data.color = glm::vec4(color.r, color.g, color.b, color.a);
            data.intensity = intensity;
            data.radius = radius;
            data.innerAngle = 0.0f;
            data.outerAngle = 0.0f;
            data.lightType = static_cast<uint32_t>(LightType::Point);
            data.layerMask = layerMask.value;
            data.attenuation = static_cast<float>(attenuation);
            data.castShadows = castShadows ? 1u : 0u;
            return data;
        }
    };
}

namespace YAML
{
#ifndef YAML_CONVERT_ATTENUATION_TYPE_DEFINED
#define YAML_CONVERT_ATTENUATION_TYPE_DEFINED
    /**
     * @brief AttenuationType 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AttenuationType>
    {
        static Node encode(const ECS::AttenuationType& att)
        {
            switch (att)
            {
                case ECS::AttenuationType::Linear: return Node("Linear");
                case ECS::AttenuationType::Quadratic: return Node("Quadratic");
                case ECS::AttenuationType::InverseSquare: return Node("InverseSquare");
                default: return Node("Quadratic");
            }
        }

        static bool decode(const Node& node, ECS::AttenuationType& att)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Linear")
                att = ECS::AttenuationType::Linear;
            else if (value == "Quadratic")
                att = ECS::AttenuationType::Quadratic;
            else if (value == "InverseSquare")
                att = ECS::AttenuationType::InverseSquare;
            else
                att = ECS::AttenuationType::Quadratic; // 默认值

            return true;
        }
    };
#endif // YAML_CONVERT_ATTENUATION_TYPE_DEFINED

    /**
     * @brief PointLightComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::PointLightComponent>
    {
        /**
         * @brief 将 PointLightComponent 编码为 YAML 节点
         * @param light 点光源组件
         * @return YAML 节点
         */
        static Node encode(const ECS::PointLightComponent& light)
        {
            Node node;
            node["Enable"] = light.Enable;
            node["color"] = light.color;
            node["intensity"] = light.intensity;
            node["radius"] = light.radius;
            node["attenuation"] = light.attenuation;
            node["layerMask"] = light.layerMask.value;
            node["priority"] = light.priority;
            node["castShadows"] = light.castShadows;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 PointLightComponent
         * @param node YAML 节点
         * @param light 点光源组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::PointLightComponent& light)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                light.Enable = node["Enable"].as<bool>();

            // 必需字段
            if (node["color"])
                light.color = node["color"].as<ECS::Color>();

            if (node["intensity"])
                light.intensity = node["intensity"].as<float>();

            if (node["radius"])
                light.radius = node["radius"].as<float>();

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
 * @brief 注册 PointLightComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::PointLightComponent>("PointLightComponent")
        .property("color", &ECS::PointLightComponent::color)
        .property("intensity", &ECS::PointLightComponent::intensity)
        .property("radius", &ECS::PointLightComponent::radius)
        .property("attenuation", &ECS::PointLightComponent::attenuation)
        .property("layerMask", &ECS::PointLightComponent::layerMask)
        .property("priority", &ECS::PointLightComponent::priority)
        .property("castShadows", &ECS::PointLightComponent::castShadows);
}

#endif // POINT_LIGHT_COMPONENT_H

#ifndef DIRECTIONAL_LIGHT_COMPONENT_H
#define DIRECTIONAL_LIGHT_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"
#include "../Utils/LayerMask.h"
#include "glm/geometric.hpp"

namespace ECS
{
    /**
     * @brief 方向光组件
     * 
     * 方向光模拟无限远处的光源（如太阳），光线平行。
     * 可用于模拟太阳光或月光等全局照明。
     */
    struct DirectionalLightComponent : public IComponent
    {
        Color color = Colors::White;                  ///< 光源颜色
        float intensity = 1.0f;                       ///< 光照强度 [0, ∞)
        Vector2f direction = Vector2f(0.0f, -1.0f);   ///< 光照方向（归一化）
        LayerMask layerMask;                          ///< 影响的光照层掩码
        bool castShadows = true;                      ///< 是否投射阴影

        /**
         * @brief 默认构造函数
         */
        DirectionalLightComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param lightColor 光源颜色
         * @param lightIntensity 光照强度
         * @param lightDirection 光照方向（会被归一化）
         */
        DirectionalLightComponent(const Color& lightColor, float lightIntensity, 
                                   const Vector2f& lightDirection)
            : color(lightColor)
            , intensity(lightIntensity)
            , direction(lightDirection.Normalize())
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const DirectionalLightComponent& other) const
        {
            return color == other.color &&
                   intensity == other.intensity &&
                   direction == other.direction &&
                   layerMask.value == other.layerMask.value &&
                   castShadows == other.castShadows &&
                   Enable == other.Enable;
        }

        bool operator!=(const DirectionalLightComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 LightData 结构
         * @return LightData 结构
         */
        LightData ToLightData() const
        {
            LightData data;
            data.position = glm::vec2(0.0f, 0.0f); // 方向光没有位置
            data.direction = glm::vec2(direction.x, direction.y);
            data.color = glm::vec4(color.r, color.g, color.b, color.a);
            data.intensity = intensity;
            data.radius = 0.0f; // 方向光没有半径限制
            data.innerAngle = 0.0f;
            data.outerAngle = 0.0f;
            data.lightType = static_cast<uint32_t>(LightType::Directional);
            data.layerMask = layerMask.value;
            data.attenuation = 0.0f; // 方向光没有衰减
            data.castShadows = 0u; // 方向光通常不使用这种阴影算法
            return data;
        }

        /**
         * @brief 设置光照方向（会自动归一化）
         * @param newDirection 新的光照方向
         */
        void SetDirection(const Vector2f& newDirection)
        {
            direction = newDirection.Normalize();
        }

        /**
         * @brief 获取归一化的光照方向
         * @return 归一化的光照方向
         */
        Vector2f GetNormalizedDirection() const
        {
            return direction.Normalize();
        }
    };
}

namespace YAML
{
    /**
     * @brief DirectionalLightComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::DirectionalLightComponent>
    {
        /**
         * @brief 将 DirectionalLightComponent 编码为 YAML 节点
         * @param light 方向光组件
         * @return YAML 节点
         */
        static Node encode(const ECS::DirectionalLightComponent& light)
        {
            Node node;
            node["Enable"] = light.Enable;
            node["color"] = light.color;
            node["intensity"] = light.intensity;
            node["direction"] = light.direction;
            node["layerMask"] = light.layerMask.value;
            node["castShadows"] = light.castShadows;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 DirectionalLightComponent
         * @param node YAML 节点
         * @param light 方向光组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::DirectionalLightComponent& light)
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

            if (node["direction"])
                light.direction = node["direction"].as<ECS::Vector2f>();

            if (node["layerMask"])
                light.layerMask.value = node["layerMask"].as<uint32_t>();

            if (node["castShadows"])
                light.castShadows = node["castShadows"].as<bool>();

            return true;
        }
    };
}

/**
 * @brief 注册 DirectionalLightComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::DirectionalLightComponent>("DirectionalLightComponent")
        .property("color", &ECS::DirectionalLightComponent::color)
        .property("intensity", &ECS::DirectionalLightComponent::intensity)
        .property("direction", &ECS::DirectionalLightComponent::direction)
        .property("layerMask", &ECS::DirectionalLightComponent::layerMask)
        .property("castShadows", &ECS::DirectionalLightComponent::castShadows);
}

#endif // DIRECTIONAL_LIGHT_COMPONENT_H

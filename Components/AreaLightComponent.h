#ifndef AREA_LIGHT_COMPONENT_H
#define AREA_LIGHT_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"
#include "../Utils/LayerMask.h"

namespace ECS
{
    /**
     * @brief 面光源组件
     * 
     * 面光源从矩形或圆形区域发射光线，产生更柔和的照明效果。
     * 可用于创建窗户、屏幕等产生柔和照明的光源效果。
     * 
     * Requirements: 1.1, 1.2, 1.3, 1.6
     */
    struct AreaLightComponent : public IComponent
    {
        Color color = Colors::White;                              ///< 光源颜色
        float intensity = 1.0f;                                   ///< 光照强度 [0, ∞)
        AreaLightShape shape = AreaLightShape::Rectangle;         ///< 面光源形状
        float width = 2.0f;                                       ///< 矩形宽度或圆形直径
        float height = 1.0f;                                      ///< 矩形高度（圆形忽略）
        float radius = 10.0f;                                     ///< 影响半径（世界单位）
        AttenuationType attenuation = AttenuationType::Quadratic; ///< 衰减类型
        LayerMask layerMask;                                      ///< 影响的光照层掩码
        int priority = 0;                                         ///< 优先级（用于剔除时排序）
        bool castShadows = true;                                  ///< 是否投射阴影
        float shadowSoftness = 2.0f;                              ///< 阴影柔和度倍数

        /**
         * @brief 默认构造函数
         */
        AreaLightComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param lightColor 光源颜色
         * @param lightIntensity 光照强度
         * @param lightShape 面光源形状
         * @param lightWidth 宽度
         * @param lightHeight 高度
         * @param lightRadius 影响半径
         */
        AreaLightComponent(const Color& lightColor, float lightIntensity, 
                          AreaLightShape lightShape, float lightWidth, 
                          float lightHeight, float lightRadius)
            : color(lightColor)
            , intensity(lightIntensity)
            , shape(lightShape)
            , width(lightWidth)
            , height(lightHeight)
            , radius(lightRadius)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const AreaLightComponent& other) const
        {
            return color == other.color &&
                   intensity == other.intensity &&
                   shape == other.shape &&
                   width == other.width &&
                   height == other.height &&
                   radius == other.radius &&
                   attenuation == other.attenuation &&
                   layerMask.value == other.layerMask.value &&
                   priority == other.priority &&
                   castShadows == other.castShadows &&
                   shadowSoftness == other.shadowSoftness &&
                   Enable == other.Enable;
        }

        bool operator!=(const AreaLightComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 AreaLightData 结构
         * @param position 光源世界坐标位置
         * @return AreaLightData 结构
         */
        AreaLightData ToAreaLightData(const glm::vec2& position) const
        {
            AreaLightData data;
            data.position = position;
            data.size = glm::vec2(width, height);
            data.color = glm::vec4(color.r, color.g, color.b, color.a);
            data.intensity = intensity;
            data.radius = radius;
            data.shape = static_cast<uint32_t>(shape);
            data.layerMask = layerMask.value;
            data.attenuation = static_cast<float>(attenuation);
            data.shadowSoftness = shadowSoftness;
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
     * @brief AreaLightShape 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AreaLightShape>
    {
        static Node encode(const ECS::AreaLightShape& shape)
        {
            switch (shape)
            {
                case ECS::AreaLightShape::Rectangle: return Node("Rectangle");
                case ECS::AreaLightShape::Circle: return Node("Circle");
                default: return Node("Rectangle");
            }
        }

        static bool decode(const Node& node, ECS::AreaLightShape& shape)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Rectangle")
                shape = ECS::AreaLightShape::Rectangle;
            else if (value == "Circle")
                shape = ECS::AreaLightShape::Circle;
            else
                shape = ECS::AreaLightShape::Rectangle; // 默认值

            return true;
        }
    };

    /**
     * @brief AreaLightComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AreaLightComponent>
    {
        /**
         * @brief 将 AreaLightComponent 编码为 YAML 节点
         * @param light 面光源组件
         * @return YAML 节点
         */
        static Node encode(const ECS::AreaLightComponent& light)
        {
            Node node;
            node["Enable"] = light.Enable;
            node["color"] = light.color;
            node["intensity"] = light.intensity;
            node["shape"] = light.shape;
            node["width"] = light.width;
            node["height"] = light.height;
            node["radius"] = light.radius;
            node["attenuation"] = light.attenuation;
            node["layerMask"] = light.layerMask.value;
            node["priority"] = light.priority;
            node["castShadows"] = light.castShadows;
            node["shadowSoftness"] = light.shadowSoftness;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 AreaLightComponent
         * @param node YAML 节点
         * @param light 面光源组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::AreaLightComponent& light)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                light.Enable = node["Enable"].as<bool>();

            // 颜色
            if (node["color"])
                light.color = node["color"].as<ECS::Color>();

            // 强度
            if (node["intensity"])
                light.intensity = node["intensity"].as<float>();

            // 形状
            if (node["shape"])
                light.shape = node["shape"].as<ECS::AreaLightShape>();

            // 尺寸
            if (node["width"])
                light.width = node["width"].as<float>();

            if (node["height"])
                light.height = node["height"].as<float>();

            // 影响半径
            if (node["radius"])
                light.radius = node["radius"].as<float>();

            // 衰减类型
            if (node["attenuation"])
                light.attenuation = node["attenuation"].as<ECS::AttenuationType>();

            // 光照层掩码
            if (node["layerMask"])
                light.layerMask.value = node["layerMask"].as<uint32_t>();

            // 优先级
            if (node["priority"])
                light.priority = node["priority"].as<int>();

            // 阴影设置
            if (node["castShadows"])
                light.castShadows = node["castShadows"].as<bool>();

            if (node["shadowSoftness"])
                light.shadowSoftness = node["shadowSoftness"].as<float>();

            return true;
        }
    };
}

/**
 * @brief 注册 AreaLightComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::AreaLightComponent>("AreaLightComponent")
        .property("color", &ECS::AreaLightComponent::color)
        .property("intensity", &ECS::AreaLightComponent::intensity)
        .property("shape", &ECS::AreaLightComponent::shape)
        .property("width", &ECS::AreaLightComponent::width)
        .property("height", &ECS::AreaLightComponent::height)
        .property("radius", &ECS::AreaLightComponent::radius)
        .property("attenuation", &ECS::AreaLightComponent::attenuation)
        .property("layerMask", &ECS::AreaLightComponent::layerMask)
        .property("priority", &ECS::AreaLightComponent::priority)
        .property("castShadows", &ECS::AreaLightComponent::castShadows)
        .property("shadowSoftness", &ECS::AreaLightComponent::shadowSoftness);
}

#endif // AREA_LIGHT_COMPONENT_H

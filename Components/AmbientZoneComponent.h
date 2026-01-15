#ifndef AMBIENT_ZONE_COMPONENT_H
#define AMBIENT_ZONE_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 环境光区域组件
     * 
     * 环境光区域定义了局部环境光颜色和强度的区域，可用于为室内、洞穴等
     * 区域设置独特的氛围照明。支持矩形和圆形区域，以及颜色渐变效果。
     * 
     * Requirements: 2.1, 2.2, 2.3, 2.6
     */
    struct AmbientZoneComponent : public IComponent
    {
        AmbientZoneShape shape = AmbientZoneShape::Rectangle;           ///< 区域形状
        float width = 10.0f;                                            ///< 区域宽度（世界单位）
        float height = 10.0f;                                           ///< 区域高度（世界单位）
        Color primaryColor = Color(0.2f, 0.2f, 0.3f, 1.0f);            ///< 主颜色
        Color secondaryColor = Color(0.1f, 0.1f, 0.15f, 1.0f);         ///< 次颜色（用于渐变）
        AmbientGradientMode gradientMode = AmbientGradientMode::None;   ///< 渐变模式
        float intensity = 1.0f;                                         ///< 环境光强度 [0, ∞)
        float edgeSoftness = 1.0f;                                      ///< 边缘过渡柔和度 [0, ∞)
        int priority = 0;                                               ///< 重叠时的优先级
        float blendWeight = 1.0f;                                       ///< 混合权重 [0, 1]

        /**
         * @brief 默认构造函数
         */
        AmbientZoneComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param zoneShape 区域形状
         * @param zoneWidth 区域宽度
         * @param zoneHeight 区域高度
         * @param primary 主颜色
         * @param secondary 次颜色
         * @param gradient 渐变模式
         * @param zoneIntensity 环境光强度
         */
        AmbientZoneComponent(AmbientZoneShape zoneShape, float zoneWidth, float zoneHeight,
                            const Color& primary, const Color& secondary,
                            AmbientGradientMode gradient, float zoneIntensity)
            : shape(zoneShape)
            , width(zoneWidth)
            , height(zoneHeight)
            , primaryColor(primary)
            , secondaryColor(secondary)
            , gradientMode(gradient)
            , intensity(zoneIntensity)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const AmbientZoneComponent& other) const
        {
            return shape == other.shape &&
                   width == other.width &&
                   height == other.height &&
                   primaryColor == other.primaryColor &&
                   secondaryColor == other.secondaryColor &&
                   gradientMode == other.gradientMode &&
                   intensity == other.intensity &&
                   edgeSoftness == other.edgeSoftness &&
                   priority == other.priority &&
                   blendWeight == other.blendWeight &&
                   Enable == other.Enable;
        }

        bool operator!=(const AmbientZoneComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 AmbientZoneData 结构
         * @param position 区域世界坐标位置
         * @return AmbientZoneData 结构
         */
        AmbientZoneData ToAmbientZoneData(const glm::vec2& position) const
        {
            AmbientZoneData data;
            data.position = position;
            data.size = glm::vec2(width, height);
            data.primaryColor = glm::vec4(primaryColor.r, primaryColor.g, primaryColor.b, primaryColor.a);
            data.secondaryColor = glm::vec4(secondaryColor.r, secondaryColor.g, secondaryColor.b, secondaryColor.a);
            data.intensity = intensity;
            data.edgeSoftness = edgeSoftness;
            data.gradientMode = static_cast<uint32_t>(gradientMode);
            data.shape = static_cast<uint32_t>(shape);
            data.priority = priority;
            data.blendWeight = blendWeight;
            return data;
        }
    };
}

namespace YAML
{
    /**
     * @brief AmbientZoneShape 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AmbientZoneShape>
    {
        static Node encode(const ECS::AmbientZoneShape& shape)
        {
            switch (shape)
            {
                case ECS::AmbientZoneShape::Rectangle: return Node("Rectangle");
                case ECS::AmbientZoneShape::Circle: return Node("Circle");
                default: return Node("Rectangle");
            }
        }

        static bool decode(const Node& node, ECS::AmbientZoneShape& shape)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Rectangle")
                shape = ECS::AmbientZoneShape::Rectangle;
            else if (value == "Circle")
                shape = ECS::AmbientZoneShape::Circle;
            else
                shape = ECS::AmbientZoneShape::Rectangle; // 默认值

            return true;
        }
    };

    /**
     * @brief AmbientGradientMode 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AmbientGradientMode>
    {
        static Node encode(const ECS::AmbientGradientMode& mode)
        {
            switch (mode)
            {
                case ECS::AmbientGradientMode::None: return Node("None");
                case ECS::AmbientGradientMode::Vertical: return Node("Vertical");
                case ECS::AmbientGradientMode::Horizontal: return Node("Horizontal");
                default: return Node("None");
            }
        }

        static bool decode(const Node& node, ECS::AmbientGradientMode& mode)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "None")
                mode = ECS::AmbientGradientMode::None;
            else if (value == "Vertical")
                mode = ECS::AmbientGradientMode::Vertical;
            else if (value == "Horizontal")
                mode = ECS::AmbientGradientMode::Horizontal;
            else
                mode = ECS::AmbientGradientMode::None; // 默认值

            return true;
        }
    };

    /**
     * @brief AmbientZoneComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::AmbientZoneComponent>
    {
        /**
         * @brief 将 AmbientZoneComponent 编码为 YAML 节点
         * @param zone 环境光区域组件
         * @return YAML 节点
         */
        static Node encode(const ECS::AmbientZoneComponent& zone)
        {
            Node node;
            node["Enable"] = zone.Enable;
            node["shape"] = zone.shape;
            node["width"] = zone.width;
            node["height"] = zone.height;
            node["primaryColor"] = zone.primaryColor;
            node["secondaryColor"] = zone.secondaryColor;
            node["gradientMode"] = zone.gradientMode;
            node["intensity"] = zone.intensity;
            node["edgeSoftness"] = zone.edgeSoftness;
            node["priority"] = zone.priority;
            node["blendWeight"] = zone.blendWeight;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 AmbientZoneComponent
         * @param node YAML 节点
         * @param zone 环境光区域组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::AmbientZoneComponent& zone)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                zone.Enable = node["Enable"].as<bool>();

            // 形状
            if (node["shape"])
                zone.shape = node["shape"].as<ECS::AmbientZoneShape>();

            // 尺寸
            if (node["width"])
                zone.width = node["width"].as<float>();

            if (node["height"])
                zone.height = node["height"].as<float>();

            // 颜色
            if (node["primaryColor"])
                zone.primaryColor = node["primaryColor"].as<ECS::Color>();

            if (node["secondaryColor"])
                zone.secondaryColor = node["secondaryColor"].as<ECS::Color>();

            // 渐变模式
            if (node["gradientMode"])
                zone.gradientMode = node["gradientMode"].as<ECS::AmbientGradientMode>();

            // 强度
            if (node["intensity"])
                zone.intensity = node["intensity"].as<float>();

            // 边缘柔和度
            if (node["edgeSoftness"])
                zone.edgeSoftness = node["edgeSoftness"].as<float>();

            // 优先级
            if (node["priority"])
                zone.priority = node["priority"].as<int>();

            // 混合权重
            if (node["blendWeight"])
                zone.blendWeight = node["blendWeight"].as<float>();

            return true;
        }
    };
}
/**
 * @brief 注册 AmbientZoneComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::AmbientZoneComponent>("AmbientZoneComponent")
        .property("shape", &ECS::AmbientZoneComponent::shape)
        .property("width", &ECS::AmbientZoneComponent::width)
        .property("height", &ECS::AmbientZoneComponent::height)
        .property("primaryColor", &ECS::AmbientZoneComponent::primaryColor)
        .property("secondaryColor", &ECS::AmbientZoneComponent::secondaryColor)
        .property("gradientMode", &ECS::AmbientZoneComponent::gradientMode)
        .property("intensity", &ECS::AmbientZoneComponent::intensity)
        .property("edgeSoftness", &ECS::AmbientZoneComponent::edgeSoftness)
        .property("priority", &ECS::AmbientZoneComponent::priority)
        .property("blendWeight", &ECS::AmbientZoneComponent::blendWeight);
}

#endif // AMBIENT_ZONE_COMPONENT_H

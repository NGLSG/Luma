#ifndef LIGHTING_SETTINGS_COMPONENT_H
#define LIGHTING_SETTINGS_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 光照设置组件
     * 
     * 定义全局光照系统的配置参数，包括环境光、阴影设置等。
     * 通常附加到场景的根实体或专用的光照管理实体上。
     */
    struct LightingSettingsComponent : public IComponent
    {
        Color ambientColor = Color(0.1f, 0.1f, 0.15f, 1.0f);  ///< 环境光颜色
        float ambientIntensity = 0.2f;                         ///< 环境光强度 [0, 1]
        int maxLightsPerPixel = 8;                             ///< 每像素最大光源数
        bool enableShadows = true;                             ///< 是否启用阴影
        float shadowSoftness = 1.0f;                           ///< 软阴影程度 [0, 1]
        bool enableNormalMapping = true;                       ///< 是否启用法线贴图

        // 阴影贴图配置
        uint32_t shadowMapResolution = 1024;                   ///< 阴影贴图分辨率
        uint32_t maxShadowCasters = 64;                        ///< 最大阴影投射器数量
        float shadowBias = 0.005f;                             ///< 阴影偏移
        float shadowNormalBias = 0.02f;                        ///< 法线方向偏移

        // 间接光照配置
        bool enableIndirectLighting = true;                    ///< 是否启用间接光照
        float indirectIntensity = 0.3f;                        ///< 间接光照强度 [0, 1]
        float bounceDecay = 0.5f;                              ///< 反弹衰减系数 [0, 1]
        float indirectRadius = 200.0f;                         ///< 间接光照影响半径

        /**
         * @brief 默认构造函数
         */
        LightingSettingsComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param ambient 环境光颜色
         * @param ambientInt 环境光强度
         */
        LightingSettingsComponent(const Color& ambient, float ambientInt)
            : ambientColor(ambient)
            , ambientIntensity(ambientInt)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightingSettingsComponent& other) const
        {
            return ambientColor == other.ambientColor &&
                   ambientIntensity == other.ambientIntensity &&
                   maxLightsPerPixel == other.maxLightsPerPixel &&
                   enableShadows == other.enableShadows &&
                   shadowSoftness == other.shadowSoftness &&
                   enableNormalMapping == other.enableNormalMapping &&
                   shadowMapResolution == other.shadowMapResolution &&
                   maxShadowCasters == other.maxShadowCasters &&
                   shadowBias == other.shadowBias &&
                   shadowNormalBias == other.shadowNormalBias &&
                   enableIndirectLighting == other.enableIndirectLighting &&
                   indirectIntensity == other.indirectIntensity &&
                   bounceDecay == other.bounceDecay &&
                   indirectRadius == other.indirectRadius &&
                   Enable == other.Enable;
        }

        bool operator!=(const LightingSettingsComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 LightingSettingsData 结构
         * @return LightingSettingsData 结构
         */
        LightingSettingsData ToSettingsData() const
        {
            LightingSettingsData data;
            data.ambientColor = ambientColor;
            data.ambientIntensity = ambientIntensity;
            data.maxLightsPerPixel = maxLightsPerPixel;
            data.enableShadows = enableShadows;
            data.shadowSoftness = shadowSoftness;
            data.enableNormalMapping = enableNormalMapping;
            data.shadowConfig.resolution = shadowMapResolution;
            data.shadowConfig.maxShadowCasters = maxShadowCasters;
            data.shadowConfig.bias = shadowBias;
            data.shadowConfig.normalBias = shadowNormalBias;
            data.enableIndirectLighting = enableIndirectLighting;
            data.indirectIntensity = indirectIntensity;
            data.bounceDecay = bounceDecay;
            data.indirectRadius = indirectRadius;
            return data;
        }

        /**
         * @brief 从 LightingSettingsData 结构设置属性
         * @param data LightingSettingsData 结构
         */
        void FromSettingsData(const LightingSettingsData& data)
        {
            ambientColor = data.ambientColor;
            ambientIntensity = data.ambientIntensity;
            maxLightsPerPixel = data.maxLightsPerPixel;
            enableShadows = data.enableShadows;
            shadowSoftness = data.shadowSoftness;
            enableNormalMapping = data.enableNormalMapping;
            shadowMapResolution = data.shadowConfig.resolution;
            maxShadowCasters = data.shadowConfig.maxShadowCasters;
            shadowBias = data.shadowConfig.bias;
            shadowNormalBias = data.shadowConfig.normalBias;
            enableIndirectLighting = data.enableIndirectLighting;
            indirectIntensity = data.indirectIntensity;
            bounceDecay = data.bounceDecay;
            indirectRadius = data.indirectRadius;
        }

        /**
         * @brief 转换为 GPU 传输用的 LightingGlobalData 结构
         * @return LightingGlobalData 结构
         */
        LightingGlobalData ToGlobalData() const
        {
            return LightingGlobalData(ToSettingsData());
        }
    };
}

namespace YAML
{
    /**
     * @brief LightingSettingsComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::LightingSettingsComponent>
    {
        /**
         * @brief 将 LightingSettingsComponent 编码为 YAML 节点
         * @param settings 光照设置组件
         * @return YAML 节点
         */
        static Node encode(const ECS::LightingSettingsComponent& settings)
        {
            Node node;
            node["Enable"] = settings.Enable;
            node["ambientColor"] = settings.ambientColor;
            node["ambientIntensity"] = settings.ambientIntensity;
            node["maxLightsPerPixel"] = settings.maxLightsPerPixel;
            node["enableShadows"] = settings.enableShadows;
            node["shadowSoftness"] = settings.shadowSoftness;
            node["enableNormalMapping"] = settings.enableNormalMapping;
            
            // 阴影贴图配置
            Node shadowConfig;
            shadowConfig["resolution"] = settings.shadowMapResolution;
            shadowConfig["maxShadowCasters"] = settings.maxShadowCasters;
            shadowConfig["bias"] = settings.shadowBias;
            shadowConfig["normalBias"] = settings.shadowNormalBias;
            node["shadowConfig"] = shadowConfig;
            
            // 间接光照配置
            node["enableIndirectLighting"] = settings.enableIndirectLighting;
            node["indirectIntensity"] = settings.indirectIntensity;
            node["bounceDecay"] = settings.bounceDecay;
            node["indirectRadius"] = settings.indirectRadius;
            
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 LightingSettingsComponent
         * @param node YAML 节点
         * @param settings 光照设置组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::LightingSettingsComponent& settings)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                settings.Enable = node["Enable"].as<bool>();

            // 可选字段，使用默认值
            if (node["ambientColor"])
                settings.ambientColor = node["ambientColor"].as<ECS::Color>();

            if (node["ambientIntensity"])
                settings.ambientIntensity = node["ambientIntensity"].as<float>();

            if (node["maxLightsPerPixel"])
                settings.maxLightsPerPixel = node["maxLightsPerPixel"].as<int>();

            if (node["enableShadows"])
                settings.enableShadows = node["enableShadows"].as<bool>();

            if (node["shadowSoftness"])
                settings.shadowSoftness = node["shadowSoftness"].as<float>();

            if (node["enableNormalMapping"])
                settings.enableNormalMapping = node["enableNormalMapping"].as<bool>();

            // 阴影贴图配置
            if (node["shadowConfig"])
            {
                const Node& shadowConfig = node["shadowConfig"];
                if (shadowConfig["resolution"])
                    settings.shadowMapResolution = shadowConfig["resolution"].as<uint32_t>();
                if (shadowConfig["maxShadowCasters"])
                    settings.maxShadowCasters = shadowConfig["maxShadowCasters"].as<uint32_t>();
                if (shadowConfig["bias"])
                    settings.shadowBias = shadowConfig["bias"].as<float>();
                if (shadowConfig["normalBias"])
                    settings.shadowNormalBias = shadowConfig["normalBias"].as<float>();
            }

            // 间接光照配置
            if (node["enableIndirectLighting"])
                settings.enableIndirectLighting = node["enableIndirectLighting"].as<bool>();
            if (node["indirectIntensity"])
                settings.indirectIntensity = node["indirectIntensity"].as<float>();
            if (node["bounceDecay"])
                settings.bounceDecay = node["bounceDecay"].as<float>();
            if (node["indirectRadius"])
                settings.indirectRadius = node["indirectRadius"].as<float>();

            return true;
        }
    };
}

/**
 * @brief 注册 LightingSettingsComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::LightingSettingsComponent>("LightingSettingsComponent")
        .property("ambientColor", &ECS::LightingSettingsComponent::ambientColor)
        .property("ambientIntensity", &ECS::LightingSettingsComponent::ambientIntensity)
        .property("maxLightsPerPixel", &ECS::LightingSettingsComponent::maxLightsPerPixel)
        .property("enableShadows", &ECS::LightingSettingsComponent::enableShadows)
        .property("shadowSoftness", &ECS::LightingSettingsComponent::shadowSoftness)
        .property("enableNormalMapping", &ECS::LightingSettingsComponent::enableNormalMapping)
        .property("shadowMapResolution", &ECS::LightingSettingsComponent::shadowMapResolution)
        .property("maxShadowCasters", &ECS::LightingSettingsComponent::maxShadowCasters)
        .property("shadowBias", &ECS::LightingSettingsComponent::shadowBias)
        .property("shadowNormalBias", &ECS::LightingSettingsComponent::shadowNormalBias)
        .property("enableIndirectLighting", &ECS::LightingSettingsComponent::enableIndirectLighting)
        .property("indirectIntensity", &ECS::LightingSettingsComponent::indirectIntensity)
        .property("bounceDecay", &ECS::LightingSettingsComponent::bounceDecay)
        .property("indirectRadius", &ECS::LightingSettingsComponent::indirectRadius);
}

#endif // LIGHTING_SETTINGS_COMPONENT_H

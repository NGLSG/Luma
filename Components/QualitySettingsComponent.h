#ifndef QUALITY_SETTINGS_COMPONENT_H
#define QUALITY_SETTINGS_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 质量设置组件
     * 
     * 质量设置组件定义了渲染质量的配置参数，包括质量等级预设、光照设置、
     * 阴影设置、后处理设置和自动质量调整参数。这些设置可以在运行时动态调整，
     * 以适应不同性能的设备。
     * 
     * Requirements: 9.1, 9.2, 9.4, 9.6
     */
    struct QualitySettingsComponent : public IComponent
    {
        // ==================== 质量等级 ====================
        QualityLevel level = QualityLevel::High;              ///< 当前质量等级

        // ==================== 光照设置 ====================
        int maxLightsPerFrame = 64;                           ///< 每帧最大光源数量 [1, 256]
        int maxLightsPerPixel = 8;                            ///< 每像素最大光源数量 [1, 32]
        bool enableAreaLights = true;                         ///< 是否启用面光源
        bool enableIndirectLighting = true;                   ///< 是否启用间接光照

        // ==================== 阴影设置 ====================
        ShadowMethod shadowMethod = ShadowMethod::Basic;      ///< 阴影计算方法
        int shadowMapResolution = 1024;                       ///< 阴影贴图分辨率 [256, 4096]
        bool enableShadowCache = true;                        ///< 是否启用阴影缓存

        // ==================== 后处理设置 ====================
        bool enableBloom = true;                              ///< 是否启用 Bloom 效果
        bool enableLightShafts = false;                       ///< 是否启用光束效果
        bool enableFog = true;                                ///< 是否启用雾效
        bool enableColorGrading = true;                       ///< 是否启用颜色分级
        float renderScale = 1.0f;                             ///< 渲染分辨率缩放 [0.25, 2.0]

        // ==================== 自动质量调整 ====================
        bool enableAutoQuality = false;                       ///< 是否启用自动质量调整
        float targetFrameRate = 60.0f;                        ///< 目标帧率 [30, 144]
        float qualityAdjustThreshold = 5.0f;                  ///< 帧率偏差阈值 [1, 30]

        /**
         * @brief 默认构造函数
         */
        QualitySettingsComponent() = default;

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const QualitySettingsComponent& other) const
        {
            return Enable == other.Enable &&
                   // 质量等级
                   level == other.level &&
                   // 光照设置
                   maxLightsPerFrame == other.maxLightsPerFrame &&
                   maxLightsPerPixel == other.maxLightsPerPixel &&
                   enableAreaLights == other.enableAreaLights &&
                   enableIndirectLighting == other.enableIndirectLighting &&
                   // 阴影设置
                   shadowMethod == other.shadowMethod &&
                   shadowMapResolution == other.shadowMapResolution &&
                   enableShadowCache == other.enableShadowCache &&
                   // 后处理设置
                   enableBloom == other.enableBloom &&
                   enableLightShafts == other.enableLightShafts &&
                   enableFog == other.enableFog &&
                   enableColorGrading == other.enableColorGrading &&
                   renderScale == other.renderScale &&
                   // 自动质量调整
                   enableAutoQuality == other.enableAutoQuality &&
                   targetFrameRate == other.targetFrameRate &&
                   qualityAdjustThreshold == other.qualityAdjustThreshold;
        }

        bool operator!=(const QualitySettingsComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 获取指定质量等级的预设配置
         * @param presetLevel 质量等级
         * @return 预设的质量设置
         */
        static QualitySettingsComponent GetPreset(QualityLevel presetLevel)
        {
            QualitySettingsComponent settings;
            settings.level = presetLevel;

            switch (presetLevel)
            {
                case QualityLevel::Low:
                    settings.maxLightsPerFrame = 16;
                    settings.maxLightsPerPixel = 4;
                    settings.enableAreaLights = false;
                    settings.enableIndirectLighting = false;
                    settings.shadowMethod = ShadowMethod::Basic;
                    settings.shadowMapResolution = 512;
                    settings.enableShadowCache = true;
                    settings.enableBloom = false;
                    settings.enableLightShafts = false;
                    settings.enableFog = false;
                    settings.enableColorGrading = false;
                    settings.renderScale = 0.75f;
                    break;

                case QualityLevel::Medium:
                    settings.maxLightsPerFrame = 32;
                    settings.maxLightsPerPixel = 6;
                    settings.enableAreaLights = true;
                    settings.enableIndirectLighting = false;
                    settings.shadowMethod = ShadowMethod::Basic;
                    settings.shadowMapResolution = 1024;
                    settings.enableShadowCache = true;
                    settings.enableBloom = true;
                    settings.enableLightShafts = false;
                    settings.enableFog = true;
                    settings.enableColorGrading = false;
                    settings.renderScale = 1.0f;
                    break;

                case QualityLevel::High:
                    settings.maxLightsPerFrame = 64;
                    settings.maxLightsPerPixel = 8;
                    settings.enableAreaLights = true;
                    settings.enableIndirectLighting = true;
                    settings.shadowMethod = ShadowMethod::Basic;
                    settings.shadowMapResolution = 1024;
                    settings.enableShadowCache = true;
                    settings.enableBloom = true;
                    settings.enableLightShafts = false;
                    settings.enableFog = true;
                    settings.enableColorGrading = true;
                    settings.renderScale = 1.0f;
                    break;

                case QualityLevel::Ultra:
                    settings.maxLightsPerFrame = 128;
                    settings.maxLightsPerPixel = 16;
                    settings.enableAreaLights = true;
                    settings.enableIndirectLighting = true;
                    settings.shadowMethod = ShadowMethod::SDF;
                    settings.shadowMapResolution = 2048;
                    settings.enableShadowCache = true;
                    settings.enableBloom = true;
                    settings.enableLightShafts = true;
                    settings.enableFog = true;
                    settings.enableColorGrading = true;
                    settings.renderScale = 1.0f;
                    break;

                case QualityLevel::Custom:
                default:
                    // 使用默认值
                    break;
            }

            return settings;
        }

        /**
         * @brief 应用质量等级预设
         * @param presetLevel 要应用的质量等级
         */
        void ApplyPreset(QualityLevel presetLevel)
        {
            *this = GetPreset(presetLevel);
        }

        /**
         * @brief 钳制参数到有效范围
         */
        void ClampValues()
        {
            maxLightsPerFrame = std::clamp(maxLightsPerFrame, 1, 256);
            maxLightsPerPixel = std::clamp(maxLightsPerPixel, 1, 32);
            shadowMapResolution = std::clamp(shadowMapResolution, 256, 4096);
            renderScale = std::clamp(renderScale, 0.25f, 2.0f);
            targetFrameRate = std::clamp(targetFrameRate, 30.0f, 144.0f);
            qualityAdjustThreshold = std::clamp(qualityAdjustThreshold, 1.0f, 30.0f);
        }
    };
}


namespace YAML
{
    /**
     * @brief QualityLevel 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::QualityLevel>
    {
        static Node encode(const ECS::QualityLevel& level)
        {
            switch (level)
            {
                case ECS::QualityLevel::Low: return Node("Low");
                case ECS::QualityLevel::Medium: return Node("Medium");
                case ECS::QualityLevel::High: return Node("High");
                case ECS::QualityLevel::Ultra: return Node("Ultra");
                case ECS::QualityLevel::Custom: return Node("Custom");
                default: return Node("High");
            }
        }

        static bool decode(const Node& node, ECS::QualityLevel& level)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Low")
                level = ECS::QualityLevel::Low;
            else if (value == "Medium")
                level = ECS::QualityLevel::Medium;
            else if (value == "High")
                level = ECS::QualityLevel::High;
            else if (value == "Ultra")
                level = ECS::QualityLevel::Ultra;
            else if (value == "Custom")
                level = ECS::QualityLevel::Custom;
            else
                level = ECS::QualityLevel::High; // 默认值

            return true;
        }
    };

    /**
     * @brief ShadowMethod 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::ShadowMethod>
    {
        static Node encode(const ECS::ShadowMethod& method)
        {
            switch (method)
            {
                case ECS::ShadowMethod::Basic: return Node("Basic");
                case ECS::ShadowMethod::SDF: return Node("SDF");
                case ECS::ShadowMethod::ScreenSpace: return Node("ScreenSpace");
                default: return Node("Basic");
            }
        }

        static bool decode(const Node& node, ECS::ShadowMethod& method)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Basic")
                method = ECS::ShadowMethod::Basic;
            else if (value == "SDF")
                method = ECS::ShadowMethod::SDF;
            else if (value == "ScreenSpace")
                method = ECS::ShadowMethod::ScreenSpace;
            else
                method = ECS::ShadowMethod::Basic; // 默认值

            return true;
        }
    };

    /**
     * @brief QualitySettingsComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::QualitySettingsComponent>
    {
        /**
         * @brief 将 QualitySettingsComponent 编码为 YAML 节点
         * @param settings 质量设置组件
         * @return YAML 节点
         */
        static Node encode(const ECS::QualitySettingsComponent& settings)
        {
            Node node;
            node["Enable"] = settings.Enable;
            
            // 质量等级
            node["level"] = settings.level;
            
            // 光照设置
            node["maxLightsPerFrame"] = settings.maxLightsPerFrame;
            node["maxLightsPerPixel"] = settings.maxLightsPerPixel;
            node["enableAreaLights"] = settings.enableAreaLights;
            node["enableIndirectLighting"] = settings.enableIndirectLighting;
            
            // 阴影设置
            node["shadowMethod"] = settings.shadowMethod;
            node["shadowMapResolution"] = settings.shadowMapResolution;
            node["enableShadowCache"] = settings.enableShadowCache;
            
            // 后处理设置
            node["enableBloom"] = settings.enableBloom;
            node["enableLightShafts"] = settings.enableLightShafts;
            node["enableFog"] = settings.enableFog;
            node["enableColorGrading"] = settings.enableColorGrading;
            node["renderScale"] = settings.renderScale;
            
            // 自动质量调整
            node["enableAutoQuality"] = settings.enableAutoQuality;
            node["targetFrameRate"] = settings.targetFrameRate;
            node["qualityAdjustThreshold"] = settings.qualityAdjustThreshold;
            
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 QualitySettingsComponent
         * @param node YAML 节点
         * @param settings 质量设置组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::QualitySettingsComponent& settings)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                settings.Enable = node["Enable"].as<bool>();

            // 质量等级
            if (node["level"])
                settings.level = node["level"].as<ECS::QualityLevel>();

            // 光照设置
            if (node["maxLightsPerFrame"])
                settings.maxLightsPerFrame = node["maxLightsPerFrame"].as<int>();
            if (node["maxLightsPerPixel"])
                settings.maxLightsPerPixel = node["maxLightsPerPixel"].as<int>();
            if (node["enableAreaLights"])
                settings.enableAreaLights = node["enableAreaLights"].as<bool>();
            if (node["enableIndirectLighting"])
                settings.enableIndirectLighting = node["enableIndirectLighting"].as<bool>();

            // 阴影设置
            if (node["shadowMethod"])
                settings.shadowMethod = node["shadowMethod"].as<ECS::ShadowMethod>();
            if (node["shadowMapResolution"])
                settings.shadowMapResolution = node["shadowMapResolution"].as<int>();
            if (node["enableShadowCache"])
                settings.enableShadowCache = node["enableShadowCache"].as<bool>();

            // 后处理设置
            if (node["enableBloom"])
                settings.enableBloom = node["enableBloom"].as<bool>();
            if (node["enableLightShafts"])
                settings.enableLightShafts = node["enableLightShafts"].as<bool>();
            if (node["enableFog"])
                settings.enableFog = node["enableFog"].as<bool>();
            if (node["enableColorGrading"])
                settings.enableColorGrading = node["enableColorGrading"].as<bool>();
            if (node["renderScale"])
                settings.renderScale = node["renderScale"].as<float>();

            // 自动质量调整
            if (node["enableAutoQuality"])
                settings.enableAutoQuality = node["enableAutoQuality"].as<bool>();
            if (node["targetFrameRate"])
                settings.targetFrameRate = node["targetFrameRate"].as<float>();
            if (node["qualityAdjustThreshold"])
                settings.qualityAdjustThreshold = node["qualityAdjustThreshold"].as<float>();

            return true;
        }
    };
}

/**
 * @brief 注册 QualitySettingsComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::QualitySettingsComponent>("QualitySettingsComponent")
        // 质量等级
        .property("level", &ECS::QualitySettingsComponent::level)
        // 光照设置
        .property("maxLightsPerFrame", &ECS::QualitySettingsComponent::maxLightsPerFrame)
        .property("maxLightsPerPixel", &ECS::QualitySettingsComponent::maxLightsPerPixel)
        .property("enableAreaLights", &ECS::QualitySettingsComponent::enableAreaLights)
        .property("enableIndirectLighting", &ECS::QualitySettingsComponent::enableIndirectLighting)
        // 阴影设置
        .property("shadowMethod", &ECS::QualitySettingsComponent::shadowMethod)
        .property("shadowMapResolution", &ECS::QualitySettingsComponent::shadowMapResolution)
        .property("enableShadowCache", &ECS::QualitySettingsComponent::enableShadowCache)
        // 后处理设置
        .property("enableBloom", &ECS::QualitySettingsComponent::enableBloom)
        .property("enableLightShafts", &ECS::QualitySettingsComponent::enableLightShafts)
        .property("enableFog", &ECS::QualitySettingsComponent::enableFog)
        .property("enableColorGrading", &ECS::QualitySettingsComponent::enableColorGrading)
        .property("renderScale", &ECS::QualitySettingsComponent::renderScale)
        // 自动质量调整
        .property("enableAutoQuality", &ECS::QualitySettingsComponent::enableAutoQuality)
        .property("targetFrameRate", &ECS::QualitySettingsComponent::targetFrameRate)
        .property("qualityAdjustThreshold", &ECS::QualitySettingsComponent::qualityAdjustThreshold);
}

#endif // QUALITY_SETTINGS_COMPONENT_H

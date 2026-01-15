#ifndef POST_PROCESS_SETTINGS_COMPONENT_H
#define POST_PROCESS_SETTINGS_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "LightingTypes.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 后处理设置组件
     * 
     * 后处理设置组件定义了场景的后处理效果配置，包括 Bloom、光束、雾效、
     * 色调映射和颜色分级等效果。这些效果可以独立启用/禁用，并支持实时调整。
     * 
     * Requirements: 5.2, 5.4, 6.2, 10.1, 10.3, 10.4, 11.2
     */
    struct PostProcessSettingsComponent : public IComponent
    {
        // ==================== Bloom 设置 ====================
        bool enableBloom = true;                              ///< 是否启用 Bloom 效果
        float bloomThreshold = 1.0f;                          ///< Bloom 亮度阈值 [0, ∞)
        float bloomIntensity = 0.5f;                          ///< Bloom 强度 [0, ∞)
        float bloomRadius = 4.0f;                             ///< Bloom 半径（像素）[0, ∞)
        int bloomIterations = 5;                              ///< Bloom 模糊迭代次数 [1, 16]
        Color bloomTint = Colors::White;                      ///< Bloom 颜色偏移

        // ==================== 光束设置 ====================
        bool enableLightShafts = false;                       ///< 是否启用光束效果
        float lightShaftDensity = 0.5f;                       ///< 光束密度 [0, 1]
        float lightShaftDecay = 0.95f;                        ///< 光束衰减 [0, 1]
        float lightShaftWeight = 0.5f;                        ///< 光束权重 [0, 1]
        float lightShaftExposure = 0.3f;                      ///< 光束曝光 [0, ∞)

        // ==================== 雾效设置 ====================
        bool enableFog = false;                               ///< 是否启用雾效
        FogMode fogMode = FogMode::Linear;                    ///< 雾效模式
        Color fogColor = Color(0.5f, 0.5f, 0.6f, 1.0f);      ///< 雾效颜色
        float fogDensity = 0.01f;                             ///< 雾效密度 [0, ∞)
        float fogStart = 10.0f;                               ///< 雾效起始距离（世界单位）
        float fogEnd = 100.0f;                                ///< 雾效结束距离（世界单位）
        bool enableHeightFog = false;                         ///< 是否启用高度雾
        float heightFogBase = 0.0f;                           ///< 高度雾基准高度
        float heightFogDensity = 0.1f;                        ///< 高度雾密度 [0, ∞)

        // ==================== 色调映射设置 ====================
        ToneMappingMode toneMappingMode = ToneMappingMode::ACES; ///< 色调映射算法
        float exposure = 1.0f;                                ///< 曝光值 [0, ∞)
        float contrast = 1.0f;                                ///< 对比度 [0, ∞)
        float saturation = 1.0f;                              ///< 饱和度 [0, ∞)
        float gamma = 2.2f;                                   ///< Gamma 值 [0.1, 10]

        // ==================== LUT 颜色分级设置 ====================
        bool enableColorGrading = false;                      ///< 是否启用颜色分级
        std::string lutTexturePath;                           ///< LUT 纹理路径
        float lutIntensity = 1.0f;                            ///< LUT 强度 [0, 1]

        /**
         * @brief 默认构造函数
         */
        PostProcessSettingsComponent() = default;

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const PostProcessSettingsComponent& other) const
        {
            return Enable == other.Enable &&
                   // Bloom
                   enableBloom == other.enableBloom &&
                   bloomThreshold == other.bloomThreshold &&
                   bloomIntensity == other.bloomIntensity &&
                   bloomRadius == other.bloomRadius &&
                   bloomIterations == other.bloomIterations &&
                   bloomTint == other.bloomTint &&
                   // Light Shafts
                   enableLightShafts == other.enableLightShafts &&
                   lightShaftDensity == other.lightShaftDensity &&
                   lightShaftDecay == other.lightShaftDecay &&
                   lightShaftWeight == other.lightShaftWeight &&
                   lightShaftExposure == other.lightShaftExposure &&
                   // Fog
                   enableFog == other.enableFog &&
                   fogMode == other.fogMode &&
                   fogColor == other.fogColor &&
                   fogDensity == other.fogDensity &&
                   fogStart == other.fogStart &&
                   fogEnd == other.fogEnd &&
                   enableHeightFog == other.enableHeightFog &&
                   heightFogBase == other.heightFogBase &&
                   heightFogDensity == other.heightFogDensity &&
                   // Tone Mapping
                   toneMappingMode == other.toneMappingMode &&
                   exposure == other.exposure &&
                   contrast == other.contrast &&
                   saturation == other.saturation &&
                   gamma == other.gamma &&
                   // LUT
                   enableColorGrading == other.enableColorGrading &&
                   lutTexturePath == other.lutTexturePath &&
                   lutIntensity == other.lutIntensity;
        }

        bool operator!=(const PostProcessSettingsComponent& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief 转换为 GPU 传输用的 PostProcessGlobalData 结构
         * @return PostProcessGlobalData 结构
         */
        PostProcessGlobalData ToPostProcessGlobalData() const
        {
            PostProcessGlobalData data;
            
            // Bloom
            data.bloomThreshold = bloomThreshold;
            data.bloomIntensity = bloomIntensity;
            data.bloomRadius = bloomRadius;
            data.bloomIterations = static_cast<uint32_t>(bloomIterations);
            data.bloomTint = glm::vec4(bloomTint.r, bloomTint.g, bloomTint.b, bloomTint.a);
            
            // Light Shafts
            data.lightShaftDensity = lightShaftDensity;
            data.lightShaftDecay = lightShaftDecay;
            data.lightShaftWeight = lightShaftWeight;
            data.lightShaftExposure = lightShaftExposure;
            
            // Fog
            data.fogColor = glm::vec4(fogColor.r, fogColor.g, fogColor.b, fogColor.a);
            data.fogDensity = fogDensity;
            data.fogStart = fogStart;
            data.fogEnd = fogEnd;
            data.fogMode = static_cast<uint32_t>(fogMode);
            data.heightFogBase = heightFogBase;
            data.heightFogDensity = heightFogDensity;
            
            // Tone Mapping
            data.exposure = exposure;
            data.contrast = contrast;
            data.saturation = saturation;
            data.gamma = gamma;
            data.toneMappingMode = static_cast<uint32_t>(toneMappingMode);
            
            // Flags
            data.enableBloom = enableBloom ? 1u : 0u;
            data.enableLightShafts = enableLightShafts ? 1u : 0u;
            data.enableFog = enableFog ? 1u : 0u;
            
            return data;
        }
    };
}


namespace YAML
{
    /**
     * @brief PostProcessSettingsComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::PostProcessSettingsComponent>
    {
        /**
         * @brief 将 PostProcessSettingsComponent 编码为 YAML 节点
         * @param settings 后处理设置组件
         * @return YAML 节点
         */
        static Node encode(const ECS::PostProcessSettingsComponent& settings)
        {
            Node node;
            node["Enable"] = settings.Enable;
            
            // Bloom 设置
            node["enableBloom"] = settings.enableBloom;
            node["bloomThreshold"] = settings.bloomThreshold;
            node["bloomIntensity"] = settings.bloomIntensity;
            node["bloomRadius"] = settings.bloomRadius;
            node["bloomIterations"] = settings.bloomIterations;
            node["bloomTint"] = settings.bloomTint;
            
            // 光束设置
            node["enableLightShafts"] = settings.enableLightShafts;
            node["lightShaftDensity"] = settings.lightShaftDensity;
            node["lightShaftDecay"] = settings.lightShaftDecay;
            node["lightShaftWeight"] = settings.lightShaftWeight;
            node["lightShaftExposure"] = settings.lightShaftExposure;
            
            // 雾效设置
            node["enableFog"] = settings.enableFog;
            node["fogMode"] = settings.fogMode;
            node["fogColor"] = settings.fogColor;
            node["fogDensity"] = settings.fogDensity;
            node["fogStart"] = settings.fogStart;
            node["fogEnd"] = settings.fogEnd;
            node["enableHeightFog"] = settings.enableHeightFog;
            node["heightFogBase"] = settings.heightFogBase;
            node["heightFogDensity"] = settings.heightFogDensity;
            
            // 色调映射设置
            node["toneMappingMode"] = settings.toneMappingMode;
            node["exposure"] = settings.exposure;
            node["contrast"] = settings.contrast;
            node["saturation"] = settings.saturation;
            node["gamma"] = settings.gamma;
            
            // LUT 设置
            node["enableColorGrading"] = settings.enableColorGrading;
            node["lutTexturePath"] = settings.lutTexturePath;
            node["lutIntensity"] = settings.lutIntensity;
            
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 PostProcessSettingsComponent
         * @param node YAML 节点
         * @param settings 后处理设置组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::PostProcessSettingsComponent& settings)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                settings.Enable = node["Enable"].as<bool>();

            // Bloom 设置
            if (node["enableBloom"])
                settings.enableBloom = node["enableBloom"].as<bool>();
            if (node["bloomThreshold"])
                settings.bloomThreshold = node["bloomThreshold"].as<float>();
            if (node["bloomIntensity"])
                settings.bloomIntensity = node["bloomIntensity"].as<float>();
            if (node["bloomRadius"])
                settings.bloomRadius = node["bloomRadius"].as<float>();
            if (node["bloomIterations"])
                settings.bloomIterations = node["bloomIterations"].as<int>();
            if (node["bloomTint"])
                settings.bloomTint = node["bloomTint"].as<ECS::Color>();

            // 光束设置
            if (node["enableLightShafts"])
                settings.enableLightShafts = node["enableLightShafts"].as<bool>();
            if (node["lightShaftDensity"])
                settings.lightShaftDensity = node["lightShaftDensity"].as<float>();
            if (node["lightShaftDecay"])
                settings.lightShaftDecay = node["lightShaftDecay"].as<float>();
            if (node["lightShaftWeight"])
                settings.lightShaftWeight = node["lightShaftWeight"].as<float>();
            if (node["lightShaftExposure"])
                settings.lightShaftExposure = node["lightShaftExposure"].as<float>();

            // 雾效设置
            if (node["enableFog"])
                settings.enableFog = node["enableFog"].as<bool>();
            if (node["fogMode"])
                settings.fogMode = node["fogMode"].as<ECS::FogMode>();
            if (node["fogColor"])
                settings.fogColor = node["fogColor"].as<ECS::Color>();
            if (node["fogDensity"])
                settings.fogDensity = node["fogDensity"].as<float>();
            if (node["fogStart"])
                settings.fogStart = node["fogStart"].as<float>();
            if (node["fogEnd"])
                settings.fogEnd = node["fogEnd"].as<float>();
            if (node["enableHeightFog"])
                settings.enableHeightFog = node["enableHeightFog"].as<bool>();
            if (node["heightFogBase"])
                settings.heightFogBase = node["heightFogBase"].as<float>();
            if (node["heightFogDensity"])
                settings.heightFogDensity = node["heightFogDensity"].as<float>();

            // 色调映射设置
            if (node["toneMappingMode"])
                settings.toneMappingMode = node["toneMappingMode"].as<ECS::ToneMappingMode>();
            if (node["exposure"])
                settings.exposure = node["exposure"].as<float>();
            if (node["contrast"])
                settings.contrast = node["contrast"].as<float>();
            if (node["saturation"])
                settings.saturation = node["saturation"].as<float>();
            if (node["gamma"])
                settings.gamma = node["gamma"].as<float>();

            // LUT 设置
            if (node["enableColorGrading"])
                settings.enableColorGrading = node["enableColorGrading"].as<bool>();
            if (node["lutTexturePath"])
                settings.lutTexturePath = node["lutTexturePath"].as<std::string>();
            if (node["lutIntensity"])
                settings.lutIntensity = node["lutIntensity"].as<float>();

            return true;
        }
    };

    /**
     * @brief ToneMappingMode 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::ToneMappingMode>
    {
        static Node encode(const ECS::ToneMappingMode& mode)
        {
            switch (mode)
            {
                case ECS::ToneMappingMode::None: return Node("None");
                case ECS::ToneMappingMode::Reinhard: return Node("Reinhard");
                case ECS::ToneMappingMode::ACES: return Node("ACES");
                case ECS::ToneMappingMode::Filmic: return Node("Filmic");
                default: return Node("ACES");
            }
        }

        static bool decode(const Node& node, ECS::ToneMappingMode& mode)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "None")
                mode = ECS::ToneMappingMode::None;
            else if (value == "Reinhard")
                mode = ECS::ToneMappingMode::Reinhard;
            else if (value == "ACES")
                mode = ECS::ToneMappingMode::ACES;
            else if (value == "Filmic")
                mode = ECS::ToneMappingMode::Filmic;
            else
                mode = ECS::ToneMappingMode::ACES; // 默认值

            return true;
        }
    };

    /**
     * @brief FogMode 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::FogMode>
    {
        static Node encode(const ECS::FogMode& mode)
        {
            switch (mode)
            {
                case ECS::FogMode::Linear: return Node("Linear");
                case ECS::FogMode::Exponential: return Node("Exponential");
                case ECS::FogMode::ExponentialSquared: return Node("ExponentialSquared");
                default: return Node("Linear");
            }
        }

        static bool decode(const Node& node, ECS::FogMode& mode)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Linear")
                mode = ECS::FogMode::Linear;
            else if (value == "Exponential")
                mode = ECS::FogMode::Exponential;
            else if (value == "ExponentialSquared")
                mode = ECS::FogMode::ExponentialSquared;
            else
                mode = ECS::FogMode::Linear; // 默认值

            return true;
        }
    };
}

/**
 * @brief 注册 PostProcessSettingsComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::PostProcessSettingsComponent>("PostProcessSettingsComponent")
        // Bloom 设置
        .property("enableBloom", &ECS::PostProcessSettingsComponent::enableBloom)
        .property("bloomThreshold", &ECS::PostProcessSettingsComponent::bloomThreshold)
        .property("bloomIntensity", &ECS::PostProcessSettingsComponent::bloomIntensity)
        .property("bloomRadius", &ECS::PostProcessSettingsComponent::bloomRadius)
        .property("bloomIterations", &ECS::PostProcessSettingsComponent::bloomIterations)
        .property("bloomTint", &ECS::PostProcessSettingsComponent::bloomTint)
        // 光束设置
        .property("enableLightShafts", &ECS::PostProcessSettingsComponent::enableLightShafts)
        .property("lightShaftDensity", &ECS::PostProcessSettingsComponent::lightShaftDensity)
        .property("lightShaftDecay", &ECS::PostProcessSettingsComponent::lightShaftDecay)
        .property("lightShaftWeight", &ECS::PostProcessSettingsComponent::lightShaftWeight)
        .property("lightShaftExposure", &ECS::PostProcessSettingsComponent::lightShaftExposure)
        // 雾效设置
        .property("enableFog", &ECS::PostProcessSettingsComponent::enableFog)
        .property("fogMode", &ECS::PostProcessSettingsComponent::fogMode)
        .property("fogColor", &ECS::PostProcessSettingsComponent::fogColor)
        .property("fogDensity", &ECS::PostProcessSettingsComponent::fogDensity)
        .property("fogStart", &ECS::PostProcessSettingsComponent::fogStart)
        .property("fogEnd", &ECS::PostProcessSettingsComponent::fogEnd)
        .property("enableHeightFog", &ECS::PostProcessSettingsComponent::enableHeightFog)
        .property("heightFogBase", &ECS::PostProcessSettingsComponent::heightFogBase)
        .property("heightFogDensity", &ECS::PostProcessSettingsComponent::heightFogDensity)
        // 色调映射设置
        .property("toneMappingMode", &ECS::PostProcessSettingsComponent::toneMappingMode)
        .property("exposure", &ECS::PostProcessSettingsComponent::exposure)
        .property("contrast", &ECS::PostProcessSettingsComponent::contrast)
        .property("saturation", &ECS::PostProcessSettingsComponent::saturation)
        .property("gamma", &ECS::PostProcessSettingsComponent::gamma)
        // LUT 设置
        .property("enableColorGrading", &ECS::PostProcessSettingsComponent::enableColorGrading)
        .property("lutTexturePath", &ECS::PostProcessSettingsComponent::lutTexturePath)
        .property("lutIntensity", &ECS::PostProcessSettingsComponent::lutIntensity);
}

#endif // POST_PROCESS_SETTINGS_COMPONENT_H

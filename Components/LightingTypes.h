#ifndef LIGHTING_TYPES_H
#define LIGHTING_TYPES_H

#include "Core.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <cstdint>

namespace ECS
{
    /**
     * @brief 光源类型枚举
     * 
     * 定义了 2D 光照系统支持的光源类型。
     */
    enum class LightType : uint8_t
    {
        Point, ///< 点光源，从一个点向所有方向发射光线
        Spot, ///< 聚光灯，从一个点向特定方向发射锥形光线
        Directional ///< 方向光，模拟无限远处的光源（如太阳），光线平行
    };

    /**
     * @brief 光照衰减类型枚举
     *
     * 定义了光照强度随距离衰减的计算方式。
     */
    enum class AttenuationType : uint8_t
    {
        Linear, ///< 线性衰减: 1 - (distance / radius)
        Quadratic, ///< 二次衰减: 1 - (distance / radius)^2
        InverseSquare ///< 平方反比衰减: 1 / (1 + distance^2)
    };

    /**
     * @brief 面光源形状类型枚举
     *
     * 定义了面光源支持的形状类型。
     * Requirements: 1.2
     */
    enum class AreaLightShape : uint8_t
    {
        Rectangle, ///< 矩形面光源
        Circle     ///< 圆形面光源
    };

    /**
     * @brief 环境光区域形状枚举
     *
     * 定义了环境光区域支持的形状类型。
     * Requirements: 2.2
     */
    enum class AmbientZoneShape : uint8_t
    {
        Rectangle, ///< 矩形区域
        Circle     ///< 圆形区域
    };

    /**
     * @brief 环境光渐变模式枚举
     *
     * 定义了环境光区域的颜色渐变方式。
     * Requirements: 2.2
     */
    enum class AmbientGradientMode : uint8_t
    {
        None,       ///< 纯色，无渐变
        Vertical,   ///< 垂直渐变（顶部到底部）
        Horizontal  ///< 水平渐变（左到右）
    };

    /**
     * @brief 色调映射算法枚举
     *
     * 定义了 HDR 到 LDR 转换支持的色调映射算法。
     * Requirements: 10.2
     */
    enum class ToneMappingMode : uint8_t
    {
        None,     ///< 无色调映射
        Reinhard, ///< Reinhard 色调映射
        ACES,     ///< ACES 电影色调映射
        Filmic    ///< Filmic 色调映射
    };

    /**
     * @brief 雾效模式枚举
     *
     * 定义了雾效的衰减计算方式。
     * Requirements: 11.3
     */
    enum class FogMode : uint8_t
    {
        Linear,            ///< 线性衰减
        Exponential,       ///< 指数衰减
        ExponentialSquared ///< 指数平方衰减
    };

    /**
     * @brief 质量等级枚举
     *
     * 定义了渲染质量的预设等级。
     * Requirements: 9.1
     */
    enum class QualityLevel : uint8_t
    {
        Low,    ///< 低质量，适合低端设备
        Medium, ///< 中等质量
        High,   ///< 高质量
        Ultra,  ///< 超高质量
        Custom  ///< 自定义质量设置
    };

    /**
     * @brief 阴影方法枚举
     *
     * 定义了阴影计算的不同方法。
     * Requirements: 7.1, 7.2
     */
    enum class ShadowMethod : uint8_t
    {
        Basic,       ///< 基础 2D 阴影
        SDF,         ///< SDF 软阴影（高质量）
        ScreenSpace  ///< 屏幕空间阴影（快速近似）
    };

    /**
     * @brief 光源数据结构（GPU 传输用）
     *
     * 该结构体用于将光源数据传输到 GPU，需要对齐到 16 字节边界以满足 WebGPU 的 uniform buffer 要求。
     * 总大小: 64 字节（4 个 16 字节块）
     */
    struct alignas(16) LightData
    {
        glm::vec2 position; ///< 世界坐标位置（8 字节）
        glm::vec2 direction; ///< 方向（聚光灯/方向光）（8 字节）
        // -- 16 字节边界 --
        glm::vec4 color; ///< RGBA 颜色（16 字节）
        // -- 16 字节边界 --
        float intensity; ///< 强度（4 字节）
        float radius; ///< 影响半径（4 字节）
        float innerAngle; ///< 内角（聚光灯，弧度）（4 字节）
        float outerAngle; ///< 外角（聚光灯，弧度）（4 字节）
        // -- 16 字节边界 --
        uint32_t lightType; ///< 光源类型（4 字节）
        uint32_t layerMask; ///< 光照层掩码（4 字节）
        float attenuation; ///< 衰减系数（4 字节）
        uint32_t castShadows; ///< 是否投射阴影（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        LightData()
            : position(0.0f, 0.0f)
              , direction(0.0f, -1.0f)
              , color(1.0f, 1.0f, 1.0f, 1.0f)
              , intensity(1.0f)
              , radius(5.0f)
              , innerAngle(0.0f)
              , outerAngle(0.0f)
              , lightType(static_cast<uint32_t>(LightType::Point))
              , layerMask(0xFFFFFFFF)
              , attenuation(1.0f)
              , castShadows(1)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightData& other) const
        {
            return position == other.position &&
                direction == other.direction &&
                color == other.color &&
                intensity == other.intensity &&
                radius == other.radius &&
                innerAngle == other.innerAngle &&
                outerAngle == other.outerAngle &&
                lightType == other.lightType &&
                layerMask == other.layerMask &&
                attenuation == other.attenuation &&
                castShadows == other.castShadows;
        }

        bool operator!=(const LightData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 LightData 大小和对齐正确
    static_assert(sizeof(LightData) == 64, "LightData must be 64 bytes for GPU alignment");
    static_assert(alignof(LightData) == 16, "LightData must be aligned to 16 bytes");

    /**
     * @brief 阴影贴图配置结构体
     *
     * 定义阴影贴图的渲染参数。
     */
    struct ShadowMapConfig
    {
        uint32_t resolution = 1024; ///< 阴影贴图分辨率（像素）
        uint32_t maxShadowCasters = 64; ///< 最大阴影投射器数量
        float bias = 0.005f; ///< 阴影偏移，用于减少阴影失真
        float normalBias = 0.02f; ///< 法线方向偏移

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const ShadowMapConfig& other) const
        {
            return resolution == other.resolution &&
                maxShadowCasters == other.maxShadowCasters &&
                bias == other.bias &&
                normalBias == other.normalBias;
        }

        bool operator!=(const ShadowMapConfig& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 光照设置数据结构体
     *
     * 定义全局光照系统的配置参数。
     */
    struct LightingSettingsData
    {
        Color ambientColor = Color(0.1f, 0.1f, 0.15f, 1.0f); ///< 环境光颜色
        float ambientIntensity = 0.2f; ///< 环境光强度 [0, 1]
        int maxLightsPerPixel = 8; ///< 每像素最大光源数
        bool enableShadows = true; ///< 是否启用阴影
        float shadowSoftness = 1.0f; ///< 软阴影程度 [0, 1]
        bool enableNormalMapping = true; ///< 是否启用法线贴图
        ShadowMapConfig shadowConfig; ///< 阴影贴图配置
        
        // 间接光照配置
        bool enableIndirectLighting = true; ///< 是否启用间接光照
        float indirectIntensity = 0.3f; ///< 间接光照强度 [0, 1]
        float bounceDecay = 0.5f; ///< 反弹衰减系数 [0, 1]
        float indirectRadius = 200.0f; ///< 间接光照影响半径

        /**
         * @brief 默认构造函数
         */
        LightingSettingsData() = default;

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightingSettingsData& other) const
        {
            return ambientColor == other.ambientColor &&
                ambientIntensity == other.ambientIntensity &&
                maxLightsPerPixel == other.maxLightsPerPixel &&
                enableShadows == other.enableShadows &&
                shadowSoftness == other.shadowSoftness &&
                enableNormalMapping == other.enableNormalMapping &&
                shadowConfig == other.shadowConfig &&
                enableIndirectLighting == other.enableIndirectLighting &&
                indirectIntensity == other.indirectIntensity &&
                bounceDecay == other.bounceDecay &&
                indirectRadius == other.indirectRadius;
        }

        bool operator!=(const LightingSettingsData& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 光照全局数据结构（GPU 传输用）
     *
     * 该结构体用于将全局光照设置传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 32 字节（2 个 16 字节块）
     */
    struct alignas(16) LightingGlobalData
    {
        glm::vec4 ambientColor; ///< 环境光颜色（16 字节）
        // -- 16 字节边界 --
        float ambientIntensity; ///< 环境光强度（4 字节）
        uint32_t lightCount; ///< 当前光源数量（4 字节）
        uint32_t maxLightsPerPixel; ///< 每像素最大光源数（4 字节）
        uint32_t enableShadows; ///< 是否启用阴影（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        LightingGlobalData()
            : ambientColor(0.1f, 0.1f, 0.15f, 1.0f)
              , ambientIntensity(0.2f)
              , lightCount(0)
              , maxLightsPerPixel(8)
              , enableShadows(1)
        {
        }

        /**
         * @brief 从 LightingSettingsData 构造
         */
        explicit LightingGlobalData(const LightingSettingsData& settings)
            : ambientColor(settings.ambientColor.r, settings.ambientColor.g,
                           settings.ambientColor.b, settings.ambientColor.a)
              , ambientIntensity(settings.ambientIntensity)
              , lightCount(0)
              , maxLightsPerPixel(static_cast<uint32_t>(settings.maxLightsPerPixel))
              , enableShadows(settings.enableShadows ? 1u : 0u)
        {
        }
    };

    // 静态断言确保 LightingGlobalData 大小和对齐正确
    static_assert(sizeof(LightingGlobalData) == 32, "LightingGlobalData must be 32 bytes for GPU alignment");
    static_assert(alignof(LightingGlobalData) == 16, "LightingGlobalData must be aligned to 16 bytes");

    /**
     * @brief 间接光照反射体数据结构（GPU 传输用）
     *
     * 该结构体用于将反射体数据传输到 GPU，用于计算间接光照。
     * 总大小: 48 字节（3 个 16 字节块）
     */
    struct alignas(16) IndirectLightData
    {
        glm::vec2 position;     ///< 世界坐标位置（8 字节）
        glm::vec2 size;         ///< 反射体尺寸（8 字节）
        // -- 16 字节边界 --
        glm::vec4 color;        ///< 反射颜色（16 字节）
        // -- 16 字节边界 --
        float intensity;        ///< 反射强度（4 字节）
        float radius;           ///< 影响半径（4 字节）
        uint32_t layerMask;     ///< 光照层掩码（4 字节）
        float padding;          ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        IndirectLightData()
            : position(0.0f, 0.0f)
            , size(1.0f, 1.0f)
            , color(1.0f, 1.0f, 1.0f, 1.0f)
            , intensity(0.5f)
            , radius(100.0f)
            , layerMask(0xFFFFFFFF)
            , padding(0.0f)
        {
        }
    };

    // 静态断言确保 IndirectLightData 大小和对齐正确
    static_assert(sizeof(IndirectLightData) == 48, "IndirectLightData must be 48 bytes for GPU alignment");
    static_assert(alignof(IndirectLightData) == 16, "IndirectLightData must be aligned to 16 bytes");

    /**
     * @brief 间接光照全局数据结构（GPU 传输用）
     *
     * 总大小: 16 字节
     */
    struct alignas(16) IndirectLightingGlobalData
    {
        uint32_t reflectorCount;        ///< 反射体数量（4 字节）
        float indirectIntensity;        ///< 间接光照强度（4 字节）
        float bounceDecay;              ///< 反弹衰减系数（4 字节）
        uint32_t enableIndirect;        ///< 是否启用间接光照（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        IndirectLightingGlobalData()
            : reflectorCount(0)
            , indirectIntensity(0.3f)
            , bounceDecay(0.5f)
            , enableIndirect(1)
        {
        }
    };

    // 静态断言确保 IndirectLightingGlobalData 大小和对齐正确
    static_assert(sizeof(IndirectLightingGlobalData) == 16, "IndirectLightingGlobalData must be 16 bytes for GPU alignment");
    static_assert(alignof(IndirectLightingGlobalData) == 16, "IndirectLightingGlobalData must be aligned to 16 bytes");

    /**
     * @brief 面光源数据结构（GPU 传输用）
     *
     * 该结构体用于将面光源数据传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 64 字节（4 个 16 字节块）
     * Requirements: 1.1
     */
    struct alignas(16) AreaLightData
    {
        glm::vec2 position;      ///< 世界坐标位置（8 字节）
        glm::vec2 size;          ///< 宽度和高度（8 字节）
        // -- 16 字节边界 --
        glm::vec4 color;         ///< RGBA 颜色（16 字节）
        // -- 16 字节边界 --
        float intensity;         ///< 光照强度（4 字节）
        float radius;            ///< 影响半径（4 字节）
        uint32_t shape;          ///< 形状类型: 0=Rectangle, 1=Circle（4 字节）
        uint32_t layerMask;      ///< 光照层掩码（4 字节）
        // -- 16 字节边界 --
        float attenuation;       ///< 衰减系数（4 字节）
        float shadowSoftness;    ///< 阴影柔和度倍数（4 字节）
        float padding1;          ///< 对齐填充（4 字节）
        float padding2;          ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        AreaLightData()
            : position(0.0f, 0.0f)
            , size(2.0f, 1.0f)
            , color(1.0f, 1.0f, 1.0f, 1.0f)
            , intensity(1.0f)
            , radius(10.0f)
            , shape(static_cast<uint32_t>(AreaLightShape::Rectangle))
            , layerMask(0xFFFFFFFF)
            , attenuation(1.0f)
            , shadowSoftness(2.0f)
            , padding1(0.0f)
            , padding2(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const AreaLightData& other) const
        {
            return position == other.position &&
                size == other.size &&
                color == other.color &&
                intensity == other.intensity &&
                radius == other.radius &&
                shape == other.shape &&
                layerMask == other.layerMask &&
                attenuation == other.attenuation &&
                shadowSoftness == other.shadowSoftness;
        }

        bool operator!=(const AreaLightData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 AreaLightData 大小和对齐正确
    static_assert(sizeof(AreaLightData) == 64, "AreaLightData must be 64 bytes for GPU alignment");
    static_assert(alignof(AreaLightData) == 16, "AreaLightData must be aligned to 16 bytes");

    /**
     * @brief 环境光区域数据结构（GPU 传输用）
     *
     * 该结构体用于将环境光区域数据传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 80 字节（5 个 16 字节块）
     * Requirements: 2.1
     */
    struct alignas(16) AmbientZoneData
    {
        glm::vec2 position;        ///< 世界坐标位置（8 字节）
        glm::vec2 size;            ///< 宽度和高度（8 字节）
        // -- 16 字节边界 --
        glm::vec4 primaryColor;    ///< 主颜色（16 字节）
        // -- 16 字节边界 --
        glm::vec4 secondaryColor;  ///< 次颜色（用于渐变）（16 字节）
        // -- 16 字节边界 --
        float intensity;           ///< 环境光强度（4 字节）
        float edgeSoftness;        ///< 边缘过渡柔和度（4 字节）
        uint32_t gradientMode;     ///< 渐变模式: 0=None, 1=Vertical, 2=Horizontal（4 字节）
        uint32_t shape;            ///< 形状类型: 0=Rectangle, 1=Circle（4 字节）
        // -- 16 字节边界 --
        int32_t priority;          ///< 重叠时的优先级（4 字节）
        float blendWeight;         ///< 混合权重（4 字节）
        float padding1;            ///< 对齐填充（4 字节）
        float padding2;            ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        AmbientZoneData()
            : position(0.0f, 0.0f)
            , size(10.0f, 10.0f)
            , primaryColor(0.2f, 0.2f, 0.3f, 1.0f)
            , secondaryColor(0.1f, 0.1f, 0.15f, 1.0f)
            , intensity(1.0f)
            , edgeSoftness(1.0f)
            , gradientMode(static_cast<uint32_t>(AmbientGradientMode::None))
            , shape(static_cast<uint32_t>(AmbientZoneShape::Rectangle))
            , priority(0)
            , blendWeight(1.0f)
            , padding1(0.0f)
            , padding2(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const AmbientZoneData& other) const
        {
            return position == other.position &&
                size == other.size &&
                primaryColor == other.primaryColor &&
                secondaryColor == other.secondaryColor &&
                intensity == other.intensity &&
                edgeSoftness == other.edgeSoftness &&
                gradientMode == other.gradientMode &&
                shape == other.shape &&
                priority == other.priority &&
                blendWeight == other.blendWeight;
        }

        bool operator!=(const AmbientZoneData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 AmbientZoneData 大小和对齐正确
    static_assert(sizeof(AmbientZoneData) == 80, "AmbientZoneData must be 80 bytes for GPU alignment");
    static_assert(alignof(AmbientZoneData) == 16, "AmbientZoneData must be aligned to 16 bytes");

    /**
     * @brief 光照探针数据结构（GPU 传输用）
     *
     * 该结构体用于将光照探针数据传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 32 字节（2 个 16 字节块）
     * Requirements: 3.1
     */
    struct alignas(16) LightProbeData
    {
        glm::vec2 position;        ///< 世界坐标位置（8 字节）
        float influenceRadius;     ///< 影响半径（4 字节）
        float padding1;            ///< 对齐填充（4 字节）
        // -- 16 字节边界 --
        glm::vec3 sampledColor;    ///< 采样的间接光颜色（12 字节）
        float sampledIntensity;    ///< 采样的强度（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        LightProbeData()
            : position(0.0f, 0.0f)
            , influenceRadius(5.0f)
            , padding1(0.0f)
            , sampledColor(0.0f, 0.0f, 0.0f)
            , sampledIntensity(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightProbeData& other) const
        {
            return position == other.position &&
                influenceRadius == other.influenceRadius &&
                sampledColor == other.sampledColor &&
                sampledIntensity == other.sampledIntensity;
        }

        bool operator!=(const LightProbeData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 LightProbeData 大小和对齐正确
    static_assert(sizeof(LightProbeData) == 32, "LightProbeData must be 32 bytes for GPU alignment");
    static_assert(alignof(LightProbeData) == 16, "LightProbeData must be aligned to 16 bytes");

    /**
     * @brief 后处理全局数据结构（GPU 传输用）
     *
     * 该结构体用于将后处理设置传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 128 字节（8 个 16 字节块）
     * Requirements: 5.1
     */
    struct alignas(16) PostProcessGlobalData
    {
        // Bloom 设置（16 字节）
        float bloomThreshold;      ///< Bloom 阈值（4 字节）
        float bloomIntensity;      ///< Bloom 强度（4 字节）
        float bloomRadius;         ///< Bloom 半径（4 字节）
        uint32_t bloomIterations;  ///< Bloom 迭代次数（4 字节）
        // -- 16 字节边界 --
        glm::vec4 bloomTint;       ///< Bloom 颜色偏移（16 字节）
        // -- 16 字节边界 --

        // 光束设置（16 字节）
        float lightShaftDensity;   ///< 光束密度（4 字节）
        float lightShaftDecay;     ///< 光束衰减（4 字节）
        float lightShaftWeight;    ///< 光束权重（4 字节）
        float lightShaftExposure;  ///< 光束曝光（4 字节）
        // -- 16 字节边界 --

        // 雾效设置（32 字节）
        glm::vec4 fogColor;        ///< 雾效颜色（16 字节）
        // -- 16 字节边界 --
        float fogDensity;          ///< 雾效密度（4 字节）
        float fogStart;            ///< 雾效起始距离（4 字节）
        float fogEnd;              ///< 雾效结束距离（4 字节）
        uint32_t fogMode;          ///< 雾效模式: 0=Linear, 1=Exp, 2=ExpSq（4 字节）
        // -- 16 字节边界 --
        float heightFogBase;       ///< 高度雾基准高度（4 字节）
        float heightFogDensity;    ///< 高度雾密度（4 字节）
        float padding1;            ///< 对齐填充（4 字节）
        float padding2;            ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        // 色调映射设置（16 字节）
        float exposure;            ///< 曝光（4 字节）
        float contrast;            ///< 对比度（4 字节）
        float saturation;          ///< 饱和度（4 字节）
        float gamma;               ///< Gamma 值（4 字节）
        // -- 16 字节边界 --

        // 标志位（16 字节）
        uint32_t toneMappingMode;  ///< 色调映射模式（4 字节）
        uint32_t enableBloom;      ///< 是否启用 Bloom（4 字节）
        uint32_t enableLightShafts;///< 是否启用光束（4 字节）
        uint32_t enableFog;        ///< 是否启用雾效（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        PostProcessGlobalData()
            : bloomThreshold(1.0f)
            , bloomIntensity(0.5f)
            , bloomRadius(4.0f)
            , bloomIterations(5)
            , bloomTint(1.0f, 1.0f, 1.0f, 1.0f)
            , lightShaftDensity(0.5f)
            , lightShaftDecay(0.95f)
            , lightShaftWeight(0.5f)
            , lightShaftExposure(0.3f)
            , fogColor(0.5f, 0.5f, 0.6f, 1.0f)
            , fogDensity(0.01f)
            , fogStart(10.0f)
            , fogEnd(100.0f)
            , fogMode(static_cast<uint32_t>(FogMode::Linear))
            , heightFogBase(0.0f)
            , heightFogDensity(0.1f)
            , padding1(0.0f)
            , padding2(0.0f)
            , exposure(1.0f)
            , contrast(1.0f)
            , saturation(1.0f)
            , gamma(2.2f)
            , toneMappingMode(static_cast<uint32_t>(ToneMappingMode::ACES))
            , enableBloom(1)
            , enableLightShafts(0)
            , enableFog(0)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const PostProcessGlobalData& other) const
        {
            return bloomThreshold == other.bloomThreshold &&
                bloomIntensity == other.bloomIntensity &&
                bloomRadius == other.bloomRadius &&
                bloomIterations == other.bloomIterations &&
                bloomTint == other.bloomTint &&
                lightShaftDensity == other.lightShaftDensity &&
                lightShaftDecay == other.lightShaftDecay &&
                lightShaftWeight == other.lightShaftWeight &&
                lightShaftExposure == other.lightShaftExposure &&
                fogColor == other.fogColor &&
                fogDensity == other.fogDensity &&
                fogStart == other.fogStart &&
                fogEnd == other.fogEnd &&
                fogMode == other.fogMode &&
                heightFogBase == other.heightFogBase &&
                heightFogDensity == other.heightFogDensity &&
                exposure == other.exposure &&
                contrast == other.contrast &&
                saturation == other.saturation &&
                gamma == other.gamma &&
                toneMappingMode == other.toneMappingMode &&
                enableBloom == other.enableBloom &&
                enableLightShafts == other.enableLightShafts &&
                enableFog == other.enableFog;
        }

        bool operator!=(const PostProcessGlobalData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 PostProcessGlobalData 大小和对齐正确
    static_assert(sizeof(PostProcessGlobalData) == 128, "PostProcessGlobalData must be 128 bytes for GPU alignment");
    static_assert(alignof(PostProcessGlobalData) == 16, "PostProcessGlobalData must be aligned to 16 bytes");

    /**
     * @brief 光束效果参数结构（GPU 传输用）
     *
     * 该结构体用于将光束效果参数传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 64 字节（4 个 16 字节块）
     * Requirements: 6.1, 6.2
     */
    struct alignas(16) LightShaftParams
    {
        // 光源屏幕空间位置（UV 坐标 [0,1]）
        glm::vec2 lightScreenPos;  ///< 光源屏幕位置（8 字节）
        // 光源世界位置
        glm::vec2 lightWorldPos;   ///< 光源世界位置（8 字节）
        // -- 16 字节边界 --

        // 光束颜色
        glm::vec4 lightColor;      ///< 光束颜色（16 字节）
        // -- 16 字节边界 --

        // 光束参数
        float density;             ///< 光束密度 [0, 1]（4 字节）
        float decay;               ///< 光束衰减 [0, 1]（4 字节）
        float weight;              ///< 光束权重 [0, 1]（4 字节）
        float exposure;            ///< 光束曝光 [0, ∞)（4 字节）
        // -- 16 字节边界 --

        // 额外参数
        uint32_t numSamples;       ///< 采样次数（4 字节）
        float lightRadius;         ///< 光源影响半径（4 字节）
        float lightIntensity;      ///< 光源强度（4 字节）
        uint32_t enableOcclusion;  ///< 是否启用遮挡 (0 = false, 1 = true)（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        LightShaftParams()
            : lightScreenPos(0.5f, 0.5f)
            , lightWorldPos(0.0f, 0.0f)
            , lightColor(1.0f, 1.0f, 1.0f, 1.0f)
            , density(0.5f)
            , decay(0.95f)
            , weight(0.5f)
            , exposure(0.3f)
            , numSamples(64)
            , lightRadius(1.0f)
            , lightIntensity(1.0f)
            , enableOcclusion(0)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const LightShaftParams& other) const
        {
            return lightScreenPos == other.lightScreenPos &&
                lightWorldPos == other.lightWorldPos &&
                lightColor == other.lightColor &&
                density == other.density &&
                decay == other.decay &&
                weight == other.weight &&
                exposure == other.exposure &&
                numSamples == other.numSamples &&
                lightRadius == other.lightRadius &&
                lightIntensity == other.lightIntensity &&
                enableOcclusion == other.enableOcclusion;
        }

        bool operator!=(const LightShaftParams& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 LightShaftParams 大小和对齐正确
    static_assert(sizeof(LightShaftParams) == 64, "LightShaftParams must be 64 bytes for GPU alignment");
    static_assert(alignof(LightShaftParams) == 16, "LightShaftParams must be aligned to 16 bytes");

    /**
     * @brief 雾效参数结构（GPU 传输用）
     *
     * 该结构体用于将雾效参数传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 64 字节（4 个 16 字节块）
     * Requirements: 11.1, 11.3, 11.5
     */
    struct alignas(16) FogParams
    {
        // 雾效颜色（16 字节）
        glm::vec4 fogColor;        ///< 雾效颜色
        // -- 16 字节边界 --

        // 雾效参数（16 字节）
        float fogDensity;          ///< 雾效密度 [0, ∞)
        float fogStart;            ///< 雾效起始距离（世界单位）
        float fogEnd;              ///< 雾效结束距离（世界单位）
        uint32_t fogMode;          ///< 雾效模式: 0=Linear, 1=Exp, 2=ExpSq
        // -- 16 字节边界 --

        // 高度雾参数（16 字节）
        float heightFogBase;       ///< 高度雾基准高度
        float heightFogDensity;    ///< 高度雾密度 [0, ∞)
        uint32_t enableHeightFog;  ///< 是否启用高度雾 (0 = false, 1 = true)
        uint32_t enableFog;        ///< 是否启用雾效 (0 = false, 1 = true)
        // -- 16 字节边界 --

        // 相机参数（16 字节）
        glm::vec2 cameraPosition;  ///< 相机世界位置
        float cameraZoom;          ///< 相机缩放
        float padding;             ///< 对齐填充
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        FogParams()
            : fogColor(0.5f, 0.5f, 0.6f, 1.0f)
            , fogDensity(0.01f)
            , fogStart(10.0f)
            , fogEnd(100.0f)
            , fogMode(static_cast<uint32_t>(FogMode::Linear))
            , heightFogBase(0.0f)
            , heightFogDensity(0.1f)
            , enableHeightFog(0)
            , enableFog(1)
            , cameraPosition(0.0f, 0.0f)
            , cameraZoom(1.0f)
            , padding(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const FogParams& other) const
        {
            return fogColor == other.fogColor &&
                fogDensity == other.fogDensity &&
                fogStart == other.fogStart &&
                fogEnd == other.fogEnd &&
                fogMode == other.fogMode &&
                heightFogBase == other.heightFogBase &&
                heightFogDensity == other.heightFogDensity &&
                enableHeightFog == other.enableHeightFog &&
                enableFog == other.enableFog &&
                cameraPosition == other.cameraPosition &&
                cameraZoom == other.cameraZoom;
        }

        bool operator!=(const FogParams& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 FogParams 大小和对齐正确
    static_assert(sizeof(FogParams) == 64, "FogParams must be 64 bytes for GPU alignment");
    static_assert(alignof(FogParams) == 16, "FogParams must be aligned to 16 bytes");

    /**
     * @brief 光源穿透雾效数据结构（GPU 传输用）
     *
     * 该结构体用于将光源穿透雾效数据传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 48 字节（3 个 16 字节块）
     * Requirements: 11.4
     */
    struct alignas(16) FogLightData
    {
        glm::vec2 position;           ///< 光源世界位置（8 字节）
        float radius;                 ///< 光源影响半径（4 字节）
        float intensity;              ///< 光源强度（4 字节）
        // -- 16 字节边界 --
        glm::vec4 color;              ///< 光源颜色（16 字节）
        // -- 16 字节边界 --
        float penetrationStrength;    ///< 穿透强度 [0, 1]（4 字节）
        float falloff;                ///< 衰减系数（4 字节）
        float padding1;               ///< 对齐填充（4 字节）
        float padding2;               ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        FogLightData()
            : position(0.0f, 0.0f)
            , radius(10.0f)
            , intensity(1.0f)
            , color(1.0f, 1.0f, 1.0f, 1.0f)
            , penetrationStrength(0.5f)
            , falloff(2.0f)
            , padding1(0.0f)
            , padding2(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const FogLightData& other) const
        {
            return position == other.position &&
                radius == other.radius &&
                intensity == other.intensity &&
                color == other.color &&
                penetrationStrength == other.penetrationStrength &&
                falloff == other.falloff;
        }

        bool operator!=(const FogLightData& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 FogLightData 大小和对齐正确
    static_assert(sizeof(FogLightData) == 48, "FogLightData must be 48 bytes for GPU alignment");
    static_assert(alignof(FogLightData) == 16, "FogLightData must be aligned to 16 bytes");

    /**
     * @brief 光源穿透参数结构（GPU 传输用）
     *
     * 该结构体用于将光源穿透参数传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 16 字节（1 个 16 字节块）
     * Requirements: 11.4
     */
    struct alignas(16) FogLightParams
    {
        uint32_t lightCount;          ///< 光源数量（4 字节）
        uint32_t enableLightPenetration; ///< 是否启用光源穿透 (0 = false, 1 = true)（4 字节）
        float maxPenetration;         ///< 最大穿透量 [0, 1]（4 字节）
        float padding;                ///< 对齐填充（4 字节）
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        FogLightParams()
            : lightCount(0)
            , enableLightPenetration(0)
            , maxPenetration(0.8f)
            , padding(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const FogLightParams& other) const
        {
            return lightCount == other.lightCount &&
                enableLightPenetration == other.enableLightPenetration &&
                maxPenetration == other.maxPenetration;
        }

        bool operator!=(const FogLightParams& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 FogLightParams 大小和对齐正确
    static_assert(sizeof(FogLightParams) == 16, "FogLightParams must be 16 bytes for GPU alignment");
    static_assert(alignof(FogLightParams) == 16, "FogLightParams must be aligned to 16 bytes");

    /**
     * @brief 色调映射参数结构（GPU 传输用）
     *
     * 该结构体用于将色调映射和颜色分级参数传输到 GPU，需要对齐到 16 字节边界。
     * 总大小: 64 字节（4 个 16 字节块）
     * Requirements: 10.1, 10.2, 10.3, 10.4
     */
    struct alignas(16) ToneMappingParams
    {
        // 色调映射参数（16 字节）
        float exposure;            ///< 曝光调整 [0, ∞)
        float contrast;            ///< 对比度调整 [0, ∞)，1.0 = 无变化
        float saturation;          ///< 饱和度调整 [0, ∞)，1.0 = 无变化
        float gamma;               ///< Gamma 校正值，通常 2.2
        // -- 16 字节边界 --

        // 色调映射模式和标志（16 字节）
        uint32_t toneMappingMode;  ///< 色调映射模式
        uint32_t enableToneMapping;///< 是否启用色调映射 (0 = false, 1 = true)
        uint32_t enableColorGrading;///< 是否启用颜色分级 (0 = false, 1 = true)
        uint32_t enableLUT;        ///< 是否启用 LUT (0 = false, 1 = true)
        // -- 16 字节边界 --

        // LUT 参数（16 字节）
        float lutIntensity;        ///< LUT 强度 [0, 1]
        float lutSize;             ///< LUT 尺寸（通常 32）
        float whitePoint;          ///< 白点值（用于 Reinhard 扩展）
        float padding1;            ///< 对齐填充
        // -- 16 字节边界 --

        // 额外颜色调整（16 字节）
        glm::vec3 colorBalance;    ///< RGB 颜色平衡
        float padding2;            ///< 对齐填充
        // -- 16 字节边界 --

        /**
         * @brief 默认构造函数
         */
        ToneMappingParams()
            : exposure(1.0f)
            , contrast(1.0f)
            , saturation(1.0f)
            , gamma(2.2f)
            , toneMappingMode(static_cast<uint32_t>(ToneMappingMode::ACES))
            , enableToneMapping(1)
            , enableColorGrading(0)
            , enableLUT(0)
            , lutIntensity(1.0f)
            , lutSize(32.0f)
            , whitePoint(4.0f)
            , padding1(0.0f)
            , colorBalance(1.0f, 1.0f, 1.0f)
            , padding2(0.0f)
        {
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const ToneMappingParams& other) const
        {
            return exposure == other.exposure &&
                contrast == other.contrast &&
                saturation == other.saturation &&
                gamma == other.gamma &&
                toneMappingMode == other.toneMappingMode &&
                enableToneMapping == other.enableToneMapping &&
                enableColorGrading == other.enableColorGrading &&
                enableLUT == other.enableLUT &&
                lutIntensity == other.lutIntensity &&
                lutSize == other.lutSize &&
                whitePoint == other.whitePoint &&
                colorBalance == other.colorBalance;
        }

        bool operator!=(const ToneMappingParams& other) const
        {
            return !(*this == other);
        }
    };

    // 静态断言确保 ToneMappingParams 大小和对齐正确
    static_assert(sizeof(ToneMappingParams) == 64, "ToneMappingParams must be 64 bytes for GPU alignment");
    static_assert(alignof(ToneMappingParams) == 16, "ToneMappingParams must be aligned to 16 bytes");
}

#endif // LIGHTING_TYPES_H

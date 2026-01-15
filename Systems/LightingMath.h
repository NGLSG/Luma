#ifndef LIGHTING_MATH_H
#define LIGHTING_MATH_H

/**
 * @file LightingMath.h
 * @brief 光照计算数学函数
 * 
 * 提供光照系统所需的数学计算函数，包括：
 * - 光照衰减函数（Linear、Quadratic、InverseSquare）
 * - 聚光灯角度衰减函数
 * - 光照层过滤函数
 * 
 * Feature: 2d-lighting-system
 */

#include "../Components/LightingTypes.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lighting
{
    /**
     * @brief 最小半径值，防止除零错误
     */
    constexpr float MIN_RADIUS = 0.001f;

    /**
     * @brief 计算线性衰减
     * 
     * 公式: attenuation = max(0, 1 - distance / radius)
     * 
     * @param distance 到光源的距离
     * @param radius 光源影响半径
     * @return 衰减值 [0, 1]，0 表示完全衰减，1 表示无衰减
     */
    inline float CalculateLinearAttenuation(float distance, float radius)
    {
        // 确保半径有效
        float safeRadius = std::max(radius, MIN_RADIUS);
        
        // 超出半径范围，完全衰减
        if (distance >= safeRadius)
        {
            return 0.0f;
        }
        
        // 线性衰减: 1 - (distance / radius)
        return std::max(0.0f, 1.0f - (distance / safeRadius));
    }

    /**
     * @brief 计算二次衰减
     * 
     * 公式: attenuation = max(0, 1 - (distance / radius)^2)
     * 
     * @param distance 到光源的距离
     * @param radius 光源影响半径
     * @return 衰减值 [0, 1]，0 表示完全衰减，1 表示无衰减
     */
    inline float CalculateQuadraticAttenuation(float distance, float radius)
    {
        // 确保半径有效
        float safeRadius = std::max(radius, MIN_RADIUS);
        
        // 超出半径范围，完全衰减
        if (distance >= safeRadius)
        {
            return 0.0f;
        }
        
        // 二次衰减: 1 - (distance / radius)^2
        float ratio = distance / safeRadius;
        return std::max(0.0f, 1.0f - (ratio * ratio));
    }

    /**
     * @brief 计算平方反比衰减
     * 
     * 公式: attenuation = 1 / (1 + (distance / radius)^2)
     * 当 distance >= radius 时返回 0
     * 
     * @param distance 到光源的距离
     * @param radius 光源影响半径
     * @return 衰减值 [0, 1]，0 表示完全衰减，1 表示无衰减
     */
    inline float CalculateInverseSquareAttenuation(float distance, float radius)
    {
        // 确保半径有效
        float safeRadius = std::max(radius, MIN_RADIUS);
        
        // 超出半径范围，完全衰减
        if (distance >= safeRadius)
        {
            return 0.0f;
        }
        
        // 平方反比衰减: 1 / (1 + (distance / radius)^2)
        // 使用归一化距离以确保在 radius 处衰减到接近 0
        float normalizedDist = distance / safeRadius;
        float factor = normalizedDist * normalizedDist;
        
        // 使用平滑衰减，确保在边界处平滑过渡到 0
        float baseAttenuation = 1.0f / (1.0f + factor * 4.0f);
        
        // 应用边界衰减，确保在 radius 处为 0
        float edgeFalloff = 1.0f - normalizedDist;
        return baseAttenuation * edgeFalloff;
    }

    /**
     * @brief 根据衰减类型计算衰减值
     * 
     * @param distance 到光源的距离
     * @param radius 光源影响半径
     * @param type 衰减类型
     * @return 衰减值 [0, 1]
     */
    inline float CalculateAttenuation(float distance, float radius, ECS::AttenuationType type)
    {
        switch (type)
        {
            case ECS::AttenuationType::Linear:
                return CalculateLinearAttenuation(distance, radius);
            case ECS::AttenuationType::Quadratic:
                return CalculateQuadraticAttenuation(distance, radius);
            case ECS::AttenuationType::InverseSquare:
                return CalculateInverseSquareAttenuation(distance, radius);
            default:
                return CalculateQuadraticAttenuation(distance, radius);
        }
    }

    /**
     * @brief 计算聚光灯角度衰减
     * 
     * 根据目标点与光源方向的夹角计算衰减：
     * - 夹角 < innerAngle: 强度为 100%
     * - 夹角 > outerAngle: 强度为 0%
     * - innerAngle <= 夹角 <= outerAngle: 平滑插值
     * 
     * @param cosAngle 目标点方向与光源方向的夹角余弦值
     * @param innerAngleCos 内角的余弦值
     * @param outerAngleCos 外角的余弦值
     * @return 角度衰减值 [0, 1]
     */
    inline float CalculateSpotAngleAttenuation(float cosAngle, float innerAngleCos, float outerAngleCos)
    {
        // 如果在内角范围内，完全照亮
        if (cosAngle >= innerAngleCos)
        {
            return 1.0f;
        }
        
        // 如果在外角范围外，完全不照亮
        if (cosAngle <= outerAngleCos)
        {
            return 0.0f;
        }
        
        // 在内外角之间，平滑插值
        // 使用 smoothstep 风格的插值以获得更平滑的过渡
        float range = innerAngleCos - outerAngleCos;
        if (range <= 0.0f)
        {
            return 0.0f;
        }
        
        float t = (cosAngle - outerAngleCos) / range;
        // Smoothstep: 3t^2 - 2t^3
        return t * t * (3.0f - 2.0f * t);
    }

    /**
     * @brief 计算聚光灯角度衰减（使用角度值）
     * 
     * @param angle 目标点与光源方向的夹角（弧度）
     * @param innerAngle 内角（弧度）
     * @param outerAngle 外角（弧度）
     * @return 角度衰减值 [0, 1]
     */
    inline float CalculateSpotAngleAttenuationFromAngles(float angle, float innerAngle, float outerAngle)
    {
        // 确保角度有效
        float safeInner = std::max(0.0f, innerAngle);
        float safeOuter = std::max(safeInner, outerAngle);
        
        // 转换为余弦值（注意：角度越大，余弦值越小）
        float cosAngle = std::cos(angle);
        float innerAngleCos = std::cos(safeInner);
        float outerAngleCos = std::cos(safeOuter);
        
        return CalculateSpotAngleAttenuation(cosAngle, innerAngleCos, outerAngleCos);
    }

    /**
     * @brief 计算两个向量之间的夹角余弦值
     * 
     * @param dir1 第一个方向向量（应归一化）
     * @param dir2 第二个方向向量（应归一化）
     * @return 夹角余弦值 [-1, 1]
     */
    inline float CalculateCosAngle(const glm::vec2& dir1, const glm::vec2& dir2)
    {
        return glm::dot(dir1, dir2);
    }

    /**
     * @brief 检查光源是否影响指定的光照层
     * 
     * 使用位运算检查光源的 layerMask 是否与精灵的 lightLayer 有交集。
     * 
     * @param lightLayerMask 光源的光照层掩码
     * @param spriteLayer 精灵的光照层
     * @return true 如果光源影响该精灵，false 否则
     */
    inline bool LightAffectsLayer(uint32_t lightLayerMask, uint32_t spriteLayer)
    {
        return (lightLayerMask & spriteLayer) != 0;
    }

    /**
     * @brief 检查光源是否影响指定的光照层（使用层索引）
     * 
     * @param lightLayerMask 光源的光照层掩码
     * @param layerIndex 光照层索引 [0, 31]
     * @return true 如果光源影响该层，false 否则
     */
    inline bool LightAffectsLayerIndex(uint32_t lightLayerMask, int layerIndex)
    {
        if (layerIndex < 0 || layerIndex > 31)
        {
            return false;
        }
        uint32_t layerBit = 1u << layerIndex;
        return (lightLayerMask & layerBit) != 0;
    }

    /**
     * @brief 将角度从度转换为弧度
     * 
     * @param degrees 角度（度）
     * @return 角度（弧度）
     */
    inline float DegreesToRadians(float degrees)
    {
        return degrees * static_cast<float>(M_PI) / 180.0f;
    }

    /**
     * @brief 将角度从弧度转换为度
     * 
     * @param radians 角度（弧度）
     * @return 角度（度）
     */
    inline float RadiansToDegrees(float radians)
    {
        return radians * 180.0f / static_cast<float>(M_PI);
    }

    // ============ 法线贴图支持 ============

    /**
     * @brief 从法线贴图采样值转换为世界空间法线
     * 
     * 法线贴图存储的是切线空间法线，范围 [0, 1]，需要转换到 [-1, 1]
     * 
     * @param normalMapValue 法线贴图采样值 (RGB in [0, 1])
     * @return 世界空间法线向量 (XYZ in [-1, 1])
     */
    inline glm::vec3 UnpackNormal(const glm::vec3& normalMapValue)
    {
        return normalMapValue * 2.0f - 1.0f;
    }

    /**
     * @brief 检查法线是否为默认法线（纯蓝色 (0.5, 0.5, 1.0)）
     * 
     * 默认法线指向 Z 轴正方向，解包后为 (0, 0, 1)
     * 
     * @param normalMapValue 法线贴图采样值 (RGB in [0, 1])
     * @param epsilon 比较容差
     * @return true 如果是默认法线，false 否则
     */
    inline bool IsDefaultNormal(const glm::vec3& normalMapValue, float epsilon = 0.01f)
    {
        // 默认法线贴图值为 (0.5, 0.5, 1.0)，解包后为 (0, 0, 1)
        const glm::vec3 defaultNormalMapValue(0.5f, 0.5f, 1.0f);
        return std::abs(normalMapValue.x - defaultNormalMapValue.x) < epsilon &&
               std::abs(normalMapValue.y - defaultNormalMapValue.y) < epsilon &&
               std::abs(normalMapValue.z - defaultNormalMapValue.z) < epsilon;
    }

    /**
     * @brief 计算带法线贴图的点光源光照贡献
     * 
     * @param lightPosition 光源位置（世界空间）
     * @param worldPos 目标点位置（世界空间）
     * @param normal 表面法线（世界空间，已归一化）
     * @param lightColor 光源颜色
     * @param intensity 光源强度
     * @param radius 光源半径
     * @param attType 衰减类型
     * @param lightLayerMask 光源光照层掩码
     * @param spriteLayer 精灵光照层
     * @return 光照颜色贡献 (RGB)
     */
    inline glm::vec3 CalculatePointLightWithNormal(
        const glm::vec2& lightPosition,
        const glm::vec2& worldPos,
        const glm::vec3& normal,
        const glm::vec4& lightColor,
        float intensity,
        float radius,
        ECS::AttenuationType attType,
        uint32_t lightLayerMask,
        uint32_t spriteLayer)
    {
        // 检查光照层
        if (!LightAffectsLayer(lightLayerMask, spriteLayer))
        {
            return glm::vec3(0.0f);
        }
        
        // 计算到光源的方向和距离
        glm::vec2 toLight = lightPosition - worldPos;
        float distance = glm::length(toLight);
        
        // 计算衰减
        float attenuation = CalculateAttenuation(distance, radius, attType);
        
        if (attenuation <= 0.0f)
        {
            return glm::vec3(0.0f);
        }
        
        // 计算光照方向（3D，假设光源在 z=1 的高度）
        glm::vec3 lightDir3D = glm::normalize(glm::vec3(toLight, 1.0f));
        
        // 计算漫反射因子（Lambert）
        float NdotL = std::max(glm::dot(normal, lightDir3D), 0.0f);
        
        return glm::vec3(lightColor.r, lightColor.g, lightColor.b) * intensity * attenuation * NdotL;
    }

    /**
     * @brief 计算不带法线贴图的点光源光照贡献
     * 
     * @param lightPosition 光源位置（世界空间）
     * @param worldPos 目标点位置（世界空间）
     * @param lightColor 光源颜色
     * @param intensity 光源强度
     * @param radius 光源半径
     * @param attType 衰减类型
     * @param lightLayerMask 光源光照层掩码
     * @param spriteLayer 精灵光照层
     * @return 光照颜色贡献 (RGB)
     */
    inline glm::vec3 CalculatePointLightWithoutNormal(
        const glm::vec2& lightPosition,
        const glm::vec2& worldPos,
        const glm::vec4& lightColor,
        float intensity,
        float radius,
        ECS::AttenuationType attType,
        uint32_t lightLayerMask,
        uint32_t spriteLayer)
    {
        // 检查光照层
        if (!LightAffectsLayer(lightLayerMask, spriteLayer))
        {
            return glm::vec3(0.0f);
        }
        
        // 计算到光源的方向和距离
        glm::vec2 toLight = lightPosition - worldPos;
        float distance = glm::length(toLight);
        
        // 计算衰减
        float attenuation = CalculateAttenuation(distance, radius, attType);
        
        if (attenuation <= 0.0f)
        {
            return glm::vec3(0.0f);
        }
        
        // 不使用法线，直接返回光照贡献
        return glm::vec3(lightColor.r, lightColor.g, lightColor.b) * intensity * attenuation;
    }
}

#endif // LIGHTING_MATH_H

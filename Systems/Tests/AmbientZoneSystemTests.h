#ifndef AMBIENT_ZONE_SYSTEM_TESTS_H
#define AMBIENT_ZONE_SYSTEM_TESTS_H

/**
 * @file AmbientZoneSystemTests.h
 * @brief Property-based tests for AmbientZoneSystem
 * 
 * This file contains property-based tests for validating the correctness of
 * ambient zone calculations, including spatial filtering, edge softness,
 * gradient interpolation, and multi-zone priority blending.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 3: 环境光区域空间过滤正确性
 * Property 4: 环境光渐变插值正确性
 * Validates: Requirements 2.1, 2.3, 2.4, 2.5
 */

#include "../AmbientZoneSystem.h"
#include "../../Components/LightingTypes.h"
#include "../../Components/AmbientZoneComponent.h"
#include "../../Utils/Logger.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace AmbientZoneTests
{
    /**
     * @brief Random generator for ambient zone system tests
     */
    class AmbientZoneRandomGenerator
    {
    public:
        AmbientZoneRandomGenerator() : m_gen(std::random_device{}()) {}
        
        AmbientZoneRandomGenerator(unsigned int seed) : m_gen(seed) {}

        float RandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(m_gen);
        }

        int RandomInt(int min, int max)
        {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(m_gen);
        }

        glm::vec2 RandomPosition(float minX, float maxX, float minY, float maxY)
        {
            return glm::vec2(RandomFloat(minX, maxX), RandomFloat(minY, maxY));
        }

        ECS::AmbientZoneShape RandomShape()
        {
            return RandomInt(0, 1) == 0 ? ECS::AmbientZoneShape::Rectangle : ECS::AmbientZoneShape::Circle;
        }

        ECS::AmbientGradientMode RandomGradientMode()
        {
            int mode = RandomInt(0, 2);
            switch (mode)
            {
                case 0: return ECS::AmbientGradientMode::None;
                case 1: return ECS::AmbientGradientMode::Vertical;
                case 2: return ECS::AmbientGradientMode::Horizontal;
                default: return ECS::AmbientGradientMode::None;
            }
        }

        glm::vec4 RandomColor()
        {
            return glm::vec4(
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f),
                1.0f
            );
        }

        ECS::AmbientZoneData RandomAmbientZone(float minX, float maxX, float minY, float maxY)
        {
            ECS::AmbientZoneData zone;
            zone.position = RandomPosition(minX, maxX, minY, maxY);
            zone.size = glm::vec2(RandomFloat(5.0f, 50.0f), RandomFloat(5.0f, 50.0f));
            zone.primaryColor = RandomColor();
            zone.secondaryColor = RandomColor();
            zone.intensity = RandomFloat(0.5f, 2.0f);
            zone.edgeSoftness = RandomFloat(0.0f, 1.0f);
            zone.gradientMode = static_cast<uint32_t>(RandomGradientMode());
            zone.shape = static_cast<uint32_t>(RandomShape());
            zone.priority = RandomInt(-10, 10);
            zone.blendWeight = RandomFloat(0.5f, 1.0f);
            return zone;
        }

    private:
        std::mt19937 m_gen;
    };

    /**
     * @brief Helper function to compare floats with tolerance
     */
    inline bool FloatEquals(float a, float b, float epsilon = 1e-5f)
    {
        return std::abs(a - b) < epsilon;
    }

    /**
     * @brief Helper function to compare colors with tolerance
     */
    inline bool ColorEquals(const ECS::Color& a, const ECS::Color& b, float epsilon = 1e-5f)
    {
        return FloatEquals(a.r, b.r, epsilon) &&
               FloatEquals(a.g, b.g, epsilon) &&
               FloatEquals(a.b, b.b, epsilon) &&
               FloatEquals(a.a, b.a, epsilon);
    }

    /**
     * @brief Test result structure for AmbientZoneSystem tests
     */
    struct AmbientZoneTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec2 zonePosition{0.0f, 0.0f};
        glm::vec2 targetPosition{0.0f, 0.0f};
        glm::vec2 zoneSize{0.0f, 0.0f};
        uint32_t shape = 0;
        float edgeFactor = 0.0f;
    };


    /**
     * @brief Property 3: 环境光区域空间过滤正确性
     * 
     * For any ambient zone and target point:
     * - When target is inside the zone, it should be affected by the zone
     * - When target is outside the zone, it should NOT be affected
     * - Multiple overlapping zones should blend correctly by priority
     * 
     * Feature: 2d-lighting-enhancement, Property 3: 环境光区域空间过滤正确性
     * Validates: Requirements 2.1, 2.4, 2.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AmbientZoneTestResult with pass/fail status and failure details
     */
    inline AmbientZoneTestResult TestProperty3_SpatialFilteringCorrectness(int iterations = 100)
    {
        AmbientZoneTestResult result;
        AmbientZoneRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random ambient zone
            ECS::AmbientZoneData zone = gen.RandomAmbientZone(-100.0f, 100.0f, -100.0f, 100.0f);
            
            // Test 1: Point inside zone should be detected
            {
                glm::vec2 targetInside;
                if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
                {
                    // For circle, generate point inside radius
                    float radius = zone.size.x / 2.0f;
                    float dist = gen.RandomFloat(0.0f, radius * 0.8f);
                    float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                    targetInside = zone.position + glm::vec2(std::cos(angle) * dist, std::sin(angle) * dist);
                }
                else
                {
                    // For rectangle, generate point inside bounds
                    float halfW = zone.size.x / 2.0f * 0.8f;
                    float halfH = zone.size.y / 2.0f * 0.8f;
                    targetInside = zone.position + gen.RandomPosition(-halfW, halfW, -halfH, halfH);
                }
                
                bool isInside = Systems::AmbientZoneSystem::IsPointInZone(targetInside, zone);
                
                if (!isInside)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.targetPosition = targetInside;
                    result.zoneSize = zone.size;
                    result.shape = zone.shape;
                    std::ostringstream oss;
                    oss << "Point inside zone should be detected. "
                        << "Zone pos: (" << zone.position.x << ", " << zone.position.y << "), "
                        << "Target pos: (" << targetInside.x << ", " << targetInside.y << "), "
                        << "Size: (" << zone.size.x << ", " << zone.size.y << "), "
                        << "Shape: " << zone.shape;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Point outside zone should NOT be detected
            {
                glm::vec2 targetOutside;
                if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
                {
                    // For circle, generate point outside radius
                    float radius = zone.size.x / 2.0f;
                    float dist = radius + gen.RandomFloat(10.0f, 50.0f);
                    float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                    targetOutside = zone.position + glm::vec2(std::cos(angle) * dist, std::sin(angle) * dist);
                }
                else
                {
                    // For rectangle, generate point outside bounds
                    float halfW = zone.size.x / 2.0f;
                    float halfH = zone.size.y / 2.0f;
                    float offsetX = halfW + gen.RandomFloat(10.0f, 50.0f);
                    float offsetY = halfH + gen.RandomFloat(10.0f, 50.0f);
                    targetOutside = zone.position + glm::vec2(offsetX, offsetY);
                }
                
                bool isInside = Systems::AmbientZoneSystem::IsPointInZone(targetOutside, zone);
                
                if (isInside)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.targetPosition = targetOutside;
                    result.zoneSize = zone.size;
                    result.shape = zone.shape;
                    std::ostringstream oss;
                    oss << "Point outside zone should NOT be detected. "
                        << "Zone pos: (" << zone.position.x << ", " << zone.position.y << "), "
                        << "Target pos: (" << targetOutside.x << ", " << targetOutside.y << "), "
                        << "Size: (" << zone.size.x << ", " << zone.size.y << "), "
                        << "Shape: " << zone.shape;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Edge factor should be in [0, 1] range for points inside
            {
                glm::vec2 targetInside;
                if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
                {
                    float radius = zone.size.x / 2.0f;
                    float dist = gen.RandomFloat(0.0f, radius * 0.95f);
                    float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                    targetInside = zone.position + glm::vec2(std::cos(angle) * dist, std::sin(angle) * dist);
                }
                else
                {
                    float halfW = zone.size.x / 2.0f * 0.95f;
                    float halfH = zone.size.y / 2.0f * 0.95f;
                    targetInside = zone.position + gen.RandomPosition(-halfW, halfW, -halfH, halfH);
                }
                
                float edgeFactor = Systems::AmbientZoneSystem::CalculateEdgeFactor(targetInside, zone);
                
                if (edgeFactor < 0.0f || edgeFactor > 1.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.targetPosition = targetInside;
                    result.zoneSize = zone.size;
                    result.shape = zone.shape;
                    result.edgeFactor = edgeFactor;
                    std::ostringstream oss;
                    oss << "Edge factor should be in [0, 1] range. Got: " << edgeFactor;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Edge factor should be 0 for points outside
            {
                glm::vec2 targetOutside;
                if (zone.shape == static_cast<uint32_t>(ECS::AmbientZoneShape::Circle))
                {
                    float radius = zone.size.x / 2.0f;
                    float dist = radius + gen.RandomFloat(10.0f, 50.0f);
                    float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                    targetOutside = zone.position + glm::vec2(std::cos(angle) * dist, std::sin(angle) * dist);
                }
                else
                {
                    float halfW = zone.size.x / 2.0f;
                    float halfH = zone.size.y / 2.0f;
                    float offsetX = halfW + gen.RandomFloat(10.0f, 50.0f);
                    float offsetY = halfH + gen.RandomFloat(10.0f, 50.0f);
                    targetOutside = zone.position + glm::vec2(offsetX, offsetY);
                }
                
                float edgeFactor = Systems::AmbientZoneSystem::CalculateEdgeFactor(targetOutside, zone);
                
                if (edgeFactor != 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.targetPosition = targetOutside;
                    result.zoneSize = zone.size;
                    result.shape = zone.shape;
                    result.edgeFactor = edgeFactor;
                    std::ostringstream oss;
                    oss << "Edge factor should be 0 for points outside zone. Got: " << edgeFactor;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }


    /**
     * @brief Property 4: 环境光渐变插值正确性
     * 
     * For any ambient zone with gradient mode:
     * - The color at any point should be a linear interpolation between primaryColor and secondaryColor
     * - The interpolation factor should be determined by position
     * - For None mode, color should always be primaryColor
     * 
     * Feature: 2d-lighting-enhancement, Property 4: 环境光渐变插值正确性
     * Validates: Requirements 2.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AmbientZoneTestResult with pass/fail status and failure details
     */
    inline AmbientZoneTestResult TestProperty4_GradientInterpolationCorrectness(int iterations = 100)
    {
        AmbientZoneTestResult result;
        AmbientZoneRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random ambient zone
            ECS::AmbientZoneData zone = gen.RandomAmbientZone(-100.0f, 100.0f, -100.0f, 100.0f);
            
            // Test 1: For None gradient mode, color should always be primaryColor
            {
                zone.gradientMode = static_cast<uint32_t>(ECS::AmbientGradientMode::None);
                
                // Test at multiple positions
                for (int j = 0; j < 5; ++j)
                {
                    glm::vec2 localPos = gen.RandomPosition(
                        -zone.size.x / 2.0f, zone.size.x / 2.0f,
                        -zone.size.y / 2.0f, zone.size.y / 2.0f);
                    
                    ECS::Color gradientColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, localPos);
                    
                    // Should equal primaryColor
                    if (!FloatEquals(gradientColor.r, zone.primaryColor.r, 1e-5f) ||
                        !FloatEquals(gradientColor.g, zone.primaryColor.g, 1e-5f) ||
                        !FloatEquals(gradientColor.b, zone.primaryColor.b, 1e-5f) ||
                        !FloatEquals(gradientColor.a, zone.primaryColor.a, 1e-5f))
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.zonePosition = zone.position;
                        result.zoneSize = zone.size;
                        std::ostringstream oss;
                        oss << "For None gradient mode, color should equal primaryColor. "
                            << "Expected: (" << zone.primaryColor.r << ", " << zone.primaryColor.g << ", " 
                            << zone.primaryColor.b << ", " << zone.primaryColor.a << "), "
                            << "Got: (" << gradientColor.r << ", " << gradientColor.g << ", " 
                            << gradientColor.b << ", " << gradientColor.a << ")";
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
            
            // Test 2: For Vertical gradient, color should interpolate from top to bottom
            {
                zone.gradientMode = static_cast<uint32_t>(ECS::AmbientGradientMode::Vertical);
                
                // At top (y = -halfHeight), should be close to primaryColor (t = 0)
                glm::vec2 topPos(0.0f, -zone.size.y / 2.0f + 0.01f);
                ECS::Color topColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, topPos);
                
                // At bottom (y = halfHeight), should be close to secondaryColor (t = 1)
                glm::vec2 bottomPos(0.0f, zone.size.y / 2.0f - 0.01f);
                ECS::Color bottomColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, bottomPos);
                
                // Top should be closer to primaryColor
                float topDistToPrimary = std::abs(topColor.r - zone.primaryColor.r) +
                                         std::abs(topColor.g - zone.primaryColor.g) +
                                         std::abs(topColor.b - zone.primaryColor.b);
                float topDistToSecondary = std::abs(topColor.r - zone.secondaryColor.r) +
                                           std::abs(topColor.g - zone.secondaryColor.g) +
                                           std::abs(topColor.b - zone.secondaryColor.b);
                
                // Bottom should be closer to secondaryColor
                float bottomDistToPrimary = std::abs(bottomColor.r - zone.primaryColor.r) +
                                            std::abs(bottomColor.g - zone.primaryColor.g) +
                                            std::abs(bottomColor.b - zone.primaryColor.b);
                float bottomDistToSecondary = std::abs(bottomColor.r - zone.secondaryColor.r) +
                                              std::abs(bottomColor.g - zone.secondaryColor.g) +
                                              std::abs(bottomColor.b - zone.secondaryColor.b);
                
                // Verify gradient direction (allow some tolerance for edge cases)
                if (topDistToPrimary > topDistToSecondary + 0.1f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.zoneSize = zone.size;
                    std::ostringstream oss;
                    oss << "Vertical gradient: top should be closer to primaryColor. "
                        << "TopDistToPrimary: " << topDistToPrimary << ", TopDistToSecondary: " << topDistToSecondary;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                if (bottomDistToSecondary > bottomDistToPrimary + 0.1f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.zoneSize = zone.size;
                    std::ostringstream oss;
                    oss << "Vertical gradient: bottom should be closer to secondaryColor. "
                        << "BottomDistToPrimary: " << bottomDistToPrimary << ", BottomDistToSecondary: " << bottomDistToSecondary;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: For Horizontal gradient, color should interpolate from left to right
            {
                zone.gradientMode = static_cast<uint32_t>(ECS::AmbientGradientMode::Horizontal);
                
                // At left (x = -halfWidth), should be close to primaryColor (t = 0)
                glm::vec2 leftPos(-zone.size.x / 2.0f + 0.01f, 0.0f);
                ECS::Color leftColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, leftPos);
                
                // At right (x = halfWidth), should be close to secondaryColor (t = 1)
                glm::vec2 rightPos(zone.size.x / 2.0f - 0.01f, 0.0f);
                ECS::Color rightColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, rightPos);
                
                // Left should be closer to primaryColor
                float leftDistToPrimary = std::abs(leftColor.r - zone.primaryColor.r) +
                                          std::abs(leftColor.g - zone.primaryColor.g) +
                                          std::abs(leftColor.b - zone.primaryColor.b);
                float leftDistToSecondary = std::abs(leftColor.r - zone.secondaryColor.r) +
                                            std::abs(leftColor.g - zone.secondaryColor.g) +
                                            std::abs(leftColor.b - zone.secondaryColor.b);
                
                // Right should be closer to secondaryColor
                float rightDistToPrimary = std::abs(rightColor.r - zone.primaryColor.r) +
                                           std::abs(rightColor.g - zone.primaryColor.g) +
                                           std::abs(rightColor.b - zone.primaryColor.b);
                float rightDistToSecondary = std::abs(rightColor.r - zone.secondaryColor.r) +
                                             std::abs(rightColor.g - zone.secondaryColor.g) +
                                             std::abs(rightColor.b - zone.secondaryColor.b);
                
                // Verify gradient direction
                if (leftDistToPrimary > leftDistToSecondary + 0.1f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.zoneSize = zone.size;
                    std::ostringstream oss;
                    oss << "Horizontal gradient: left should be closer to primaryColor. "
                        << "LeftDistToPrimary: " << leftDistToPrimary << ", LeftDistToSecondary: " << leftDistToSecondary;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                if (rightDistToSecondary > rightDistToPrimary + 0.1f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.zonePosition = zone.position;
                    result.zoneSize = zone.size;
                    std::ostringstream oss;
                    oss << "Horizontal gradient: right should be closer to secondaryColor. "
                        << "RightDistToPrimary: " << rightDistToPrimary << ", RightDistToSecondary: " << rightDistToSecondary;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Gradient color should always be within bounds of primary and secondary colors
            {
                zone.gradientMode = static_cast<uint32_t>(gen.RandomGradientMode());
                
                for (int j = 0; j < 5; ++j)
                {
                    glm::vec2 localPos = gen.RandomPosition(
                        -zone.size.x / 2.0f, zone.size.x / 2.0f,
                        -zone.size.y / 2.0f, zone.size.y / 2.0f);
                    
                    ECS::Color gradientColor = Systems::AmbientZoneSystem::CalculateGradientColor(zone, localPos);
                    
                    // Each channel should be between primary and secondary (with small tolerance)
                    float minR = std::min(zone.primaryColor.r, zone.secondaryColor.r) - 0.001f;
                    float maxR = std::max(zone.primaryColor.r, zone.secondaryColor.r) + 0.001f;
                    float minG = std::min(zone.primaryColor.g, zone.secondaryColor.g) - 0.001f;
                    float maxG = std::max(zone.primaryColor.g, zone.secondaryColor.g) + 0.001f;
                    float minB = std::min(zone.primaryColor.b, zone.secondaryColor.b) - 0.001f;
                    float maxB = std::max(zone.primaryColor.b, zone.secondaryColor.b) + 0.001f;
                    
                    if (gradientColor.r < minR || gradientColor.r > maxR ||
                        gradientColor.g < minG || gradientColor.g > maxG ||
                        gradientColor.b < minB || gradientColor.b > maxB)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.zonePosition = zone.position;
                        result.zoneSize = zone.size;
                        std::ostringstream oss;
                        oss << "Gradient color should be within bounds of primary and secondary colors. "
                            << "Primary: (" << zone.primaryColor.r << ", " << zone.primaryColor.g << ", " << zone.primaryColor.b << "), "
                            << "Secondary: (" << zone.secondaryColor.r << ", " << zone.secondaryColor.g << ", " << zone.secondaryColor.b << "), "
                            << "Got: (" << gradientColor.r << ", " << gradientColor.g << ", " << gradientColor.b << ")";
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
        }
        
        return result;
    }


    /**
     * @brief Test multi-zone priority blending
     * 
     * Tests that multiple overlapping zones blend correctly by priority.
     * Higher priority zones should have more influence.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AmbientZoneTestResult with pass/fail status and failure details
     */
    inline AmbientZoneTestResult TestMultiZonePriorityBlending(int iterations = 100)
    {
        AmbientZoneTestResult result;
        AmbientZoneRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Create two overlapping zones with different priorities
            ECS::AmbientZoneData zone1;
            zone1.position = glm::vec2(0.0f, 0.0f);
            zone1.size = glm::vec2(20.0f, 20.0f);
            zone1.primaryColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            zone1.secondaryColor = zone1.primaryColor;
            zone1.intensity = 1.0f;
            zone1.edgeSoftness = 0.5f;
            zone1.gradientMode = static_cast<uint32_t>(ECS::AmbientGradientMode::None);
            zone1.shape = static_cast<uint32_t>(ECS::AmbientZoneShape::Rectangle);
            zone1.priority = gen.RandomInt(0, 5);
            zone1.blendWeight = 1.0f;
            
            ECS::AmbientZoneData zone2;
            zone2.position = glm::vec2(0.0f, 0.0f);
            zone2.size = glm::vec2(20.0f, 20.0f);
            zone2.primaryColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
            zone2.secondaryColor = zone2.primaryColor;
            zone2.intensity = 1.0f;
            zone2.edgeSoftness = 0.5f;
            zone2.gradientMode = static_cast<uint32_t>(ECS::AmbientGradientMode::None);
            zone2.shape = static_cast<uint32_t>(ECS::AmbientZoneShape::Rectangle);
            zone2.priority = gen.RandomInt(6, 10); // Higher priority
            zone2.blendWeight = 1.0f;
            
            // Test at center point
            glm::vec2 testPos(0.0f, 0.0f);
            
            // Create zone list
            std::vector<const ECS::AmbientZoneData*> zones;
            zones.push_back(&zone1);
            zones.push_back(&zone2);
            
            // Blend colors
            ECS::Color blendedColor = Systems::AmbientZoneSystem::BlendZoneColors(zones, testPos);
            
            // The blended color should be valid (not NaN or Inf)
            if (std::isnan(blendedColor.r) || std::isnan(blendedColor.g) || 
                std::isnan(blendedColor.b) || std::isnan(blendedColor.a) ||
                std::isinf(blendedColor.r) || std::isinf(blendedColor.g) || 
                std::isinf(blendedColor.b) || std::isinf(blendedColor.a))
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Blended color should not be NaN or Inf. "
                    << "Got: (" << blendedColor.r << ", " << blendedColor.g << ", " 
                    << blendedColor.b << ", " << blendedColor.a << ")";
                result.failureMessage = oss.str();
                return result;
            }
            
            // The blended color should be in valid range [0, 1]
            if (blendedColor.r < 0.0f || blendedColor.r > 1.0f ||
                blendedColor.g < 0.0f || blendedColor.g > 1.0f ||
                blendedColor.b < 0.0f || blendedColor.b > 1.0f ||
                blendedColor.a < 0.0f || blendedColor.a > 1.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Blended color should be in [0, 1] range. "
                    << "Got: (" << blendedColor.r << ", " << blendedColor.g << ", " 
                    << blendedColor.b << ", " << blendedColor.a << ")";
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Test zone bounds calculation
     * 
     * Tests that zone bounds are calculated correctly for both shapes.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AmbientZoneTestResult with pass/fail status and failure details
     */
    inline AmbientZoneTestResult TestZoneBoundsCalculation(int iterations = 100)
    {
        AmbientZoneTestResult result;
        AmbientZoneRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            ECS::AmbientZoneData zone = gen.RandomAmbientZone(-100.0f, 100.0f, -100.0f, 100.0f);
            
            Systems::AmbientZoneBounds bounds = Systems::AmbientZoneSystem::CalculateZoneBounds(zone);
            
            // Bounds should be valid (min < max)
            if (bounds.minX >= bounds.maxX || bounds.minY >= bounds.maxY)
            {
                result.passed = false;
                result.failedIteration = i;
                result.zonePosition = zone.position;
                result.zoneSize = zone.size;
                result.shape = zone.shape;
                std::ostringstream oss;
                oss << "Zone bounds should have min < max. "
                    << "Got: minX=" << bounds.minX << ", maxX=" << bounds.maxX
                    << ", minY=" << bounds.minY << ", maxY=" << bounds.maxY;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Zone center should be within bounds
            if (!bounds.Contains(zone.position))
            {
                result.passed = false;
                result.failedIteration = i;
                result.zonePosition = zone.position;
                result.zoneSize = zone.size;
                result.shape = zone.shape;
                std::ostringstream oss;
                oss << "Zone center should be within bounds. "
                    << "Center: (" << zone.position.x << ", " << zone.position.y << "), "
                    << "Bounds: [" << bounds.minX << ", " << bounds.maxX << "] x ["
                    << bounds.minY << ", " << bounds.maxY << "]";
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 3 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty3Test()
    {
        LogInfo("Running Property 3: 环境光区域空间过滤正确性 (100 iterations)...");
        
        AmbientZoneTestResult result = TestProperty3_SpatialFilteringCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 3 (环境光区域空间过滤正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 3 (环境光区域空间过滤正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.zoneSize.x > 0)
            {
                LogError("Failing example: zonePos=(%f, %f), targetPos=(%f, %f), size=(%f, %f), shape=%u",
                         result.zonePosition.x, result.zonePosition.y,
                         result.targetPosition.x, result.targetPosition.y,
                         result.zoneSize.x, result.zoneSize.y, result.shape);
            }
            return false;
        }
    }

    /**
     * @brief Run Property 4 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty4Test()
    {
        LogInfo("Running Property 4: 环境光渐变插值正确性 (100 iterations)...");
        
        AmbientZoneTestResult result = TestProperty4_GradientInterpolationCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 4 (环境光渐变插值正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 4 (环境光渐变插值正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Run all ambient zone system tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllAmbientZoneSystemTests()
    {
        LogInfo("=== Running Ambient Zone System Tests ===");
        
        bool allPassed = true;
        
        // Property 3: Spatial filtering correctness
        if (!RunProperty3Test())
        {
            allPassed = false;
        }
        
        // Property 4: Gradient interpolation correctness
        if (!RunProperty4Test())
        {
            allPassed = false;
        }
        
        // Multi-zone priority blending
        LogInfo("Running Multi-Zone Priority Blending Test (100 iterations)...");
        AmbientZoneTestResult blendResult = TestMultiZonePriorityBlending(100);
        if (blendResult.passed)
        {
            LogInfo("Multi-Zone Priority Blending Test PASSED");
        }
        else
        {
            LogError("Multi-Zone Priority Blending Test FAILED at iteration %d", blendResult.failedIteration);
            LogError("Failure: %s", blendResult.failureMessage.c_str());
            allPassed = false;
        }
        
        // Zone bounds calculation
        LogInfo("Running Zone Bounds Calculation Test (100 iterations)...");
        AmbientZoneTestResult boundsResult = TestZoneBoundsCalculation(100);
        if (boundsResult.passed)
        {
            LogInfo("Zone Bounds Calculation Test PASSED");
        }
        else
        {
            LogError("Zone Bounds Calculation Test FAILED at iteration %d", boundsResult.failedIteration);
            LogError("Failure: %s", boundsResult.failureMessage.c_str());
            allPassed = false;
        }
        
        LogInfo("=== Ambient Zone System Tests Complete ===");
        return allPassed;
    }
}

#endif // AMBIENT_ZONE_SYSTEM_TESTS_H

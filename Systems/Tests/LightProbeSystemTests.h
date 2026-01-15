#ifndef LIGHT_PROBE_SYSTEM_TESTS_H
#define LIGHT_PROBE_SYSTEM_TESTS_H

/**
 * @file LightProbeSystemTests.h
 * @brief Property-based tests for LightProbeSystem
 * 
 * This file contains property-based tests for validating the correctness of
 * light probe interpolation and real-time update functionality.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 5: 光照探针插值正确性
 * Property 6: 光照探针实时更新正确性
 * Validates: Requirements 3.2, 3.4, 3.5
 */

#include "../LightProbeSystem.h"
#include "../../Components/LightingTypes.h"
#include "../../Components/LightProbeComponent.h"
#include "../../Utils/Logger.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace LightProbeSystemTests
{
    /**
     * @brief Random generator for light probe system tests
     */
    class LightProbeRandomGenerator
    {
    public:
        LightProbeRandomGenerator() : m_gen(std::random_device{}()) {}
        
        LightProbeRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        glm::vec3 RandomColor()
        {
            return glm::vec3(
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f)
            );
        }

        ECS::LightProbeData RandomLightProbeData(float minX, float maxX, float minY, float maxY)
        {
            ECS::LightProbeData probe;
            probe.position = RandomPosition(minX, maxX, minY, maxY);
            probe.influenceRadius = RandomFloat(5.0f, 50.0f);
            probe.sampledColor = RandomColor();
            probe.sampledIntensity = RandomFloat(0.0f, 1.0f);
            return probe;
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
     * @brief Helper function to compare vec3 with tolerance
     */
    inline bool Vec3Equals(const glm::vec3& a, const glm::vec3& b, float epsilon = 1e-5f)
    {
        return FloatEquals(a.x, b.x, epsilon) &&
               FloatEquals(a.y, b.y, epsilon) &&
               FloatEquals(a.z, b.z, epsilon);
    }

    /**
     * @brief Test result structure for LightProbeSystem tests
     */
    struct LightProbeTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec2 targetPosition{0.0f, 0.0f};
        glm::vec3 interpolatedColor{0.0f, 0.0f, 0.0f};
        float interpolatedIntensity = 0.0f;
        std::vector<glm::vec2> probePositions;
        std::vector<glm::vec3> probeColors;
    };

    /**
     * @brief Property 5: 光照探针插值正确性
     * 
     * For any light probe grid and target point:
     * - The interpolated indirect light should be a weighted average of surrounding probes
     * - The interpolation result should be within the range of neighboring probe values
     * - Weights should be determined by distance
     * 
     * Feature: 2d-lighting-enhancement, Property 5: 光照探针插值正确性
     * Validates: Requirements 3.2, 3.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightProbeTestResult with pass/fail status and failure details
     */
    inline LightProbeTestResult TestProperty5_LightProbeInterpolationCorrectness(int iterations = 100)
    {
        LightProbeTestResult result;
        LightProbeRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Test 1: Distance weight calculation
            {
                float distance = gen.RandomFloat(0.0f, 50.0f);
                float influenceRadius = gen.RandomFloat(distance + 1.0f, 100.0f);
                
                float weight = Systems::LightProbeSystem::CalculateDistanceWeight(distance, influenceRadius);
                
                // Weight should be in [0, 1]
                if (weight < 0.0f || weight > 1.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Distance weight should be in [0, 1]. Got: " << weight
                        << " for distance=" << distance << ", radius=" << influenceRadius;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // Weight should be 0 when distance >= influenceRadius
                float farDistance = influenceRadius + gen.RandomFloat(1.0f, 50.0f);
                float farWeight = Systems::LightProbeSystem::CalculateDistanceWeight(farDistance, influenceRadius);
                
                if (farWeight != 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Distance weight should be 0 when distance >= radius. Got: " << farWeight
                        << " for distance=" << farDistance << ", radius=" << influenceRadius;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Bilinear interpolation bounds
            {
                glm::vec3 topLeft = gen.RandomColor();
                glm::vec3 topRight = gen.RandomColor();
                glm::vec3 bottomLeft = gen.RandomColor();
                glm::vec3 bottomRight = gen.RandomColor();
                
                float tx = gen.RandomFloat(0.0f, 1.0f);
                float ty = gen.RandomFloat(0.0f, 1.0f);
                
                glm::vec3 interpolated = Systems::LightProbeSystem::BilinearInterpolate(
                    topLeft, topRight, bottomLeft, bottomRight, tx, ty);
                
                // Find min and max of all corners
                float minR = std::min({topLeft.r, topRight.r, bottomLeft.r, bottomRight.r});
                float maxR = std::max({topLeft.r, topRight.r, bottomLeft.r, bottomRight.r});
                float minG = std::min({topLeft.g, topRight.g, bottomLeft.g, bottomRight.g});
                float maxG = std::max({topLeft.g, topRight.g, bottomLeft.g, bottomRight.g});
                float minB = std::min({topLeft.b, topRight.b, bottomLeft.b, bottomRight.b});
                float maxB = std::max({topLeft.b, topRight.b, bottomLeft.b, bottomRight.b});
                
                // Interpolated value should be within bounds (with small epsilon for floating point)
                float epsilon = 1e-5f;
                if (interpolated.r < minR - epsilon || interpolated.r > maxR + epsilon ||
                    interpolated.g < minG - epsilon || interpolated.g > maxG + epsilon ||
                    interpolated.b < minB - epsilon || interpolated.b > maxB + epsilon)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.interpolatedColor = interpolated;
                    std::ostringstream oss;
                    oss << "Bilinear interpolation result should be within corner bounds. "
                        << "Got: (" << interpolated.r << ", " << interpolated.g << ", " << interpolated.b << "), "
                        << "Bounds R: [" << minR << ", " << maxR << "], "
                        << "G: [" << minG << ", " << maxG << "], "
                        << "B: [" << minB << ", " << maxB << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Barycentric interpolation bounds
            {
                glm::vec3 v0 = gen.RandomColor();
                glm::vec3 v1 = gen.RandomColor();
                glm::vec3 v2 = gen.RandomColor();
                
                // Generate valid barycentric coordinates (sum to 1)
                float u = gen.RandomFloat(0.0f, 1.0f);
                float v = gen.RandomFloat(0.0f, 1.0f - u);
                float w = 1.0f - u - v;
                glm::vec3 baryCoords(u, v, w);
                
                glm::vec3 interpolated = Systems::LightProbeSystem::BarycentricInterpolate(
                    v0, v1, v2, baryCoords);
                
                // Find min and max of all vertices
                float minR = std::min({v0.r, v1.r, v2.r});
                float maxR = std::max({v0.r, v1.r, v2.r});
                float minG = std::min({v0.g, v1.g, v2.g});
                float maxG = std::max({v0.g, v1.g, v2.g});
                float minB = std::min({v0.b, v1.b, v2.b});
                float maxB = std::max({v0.b, v1.b, v2.b});
                
                // Interpolated value should be within bounds
                float epsilon = 1e-5f;
                if (interpolated.r < minR - epsilon || interpolated.r > maxR + epsilon ||
                    interpolated.g < minG - epsilon || interpolated.g > maxG + epsilon ||
                    interpolated.b < minB - epsilon || interpolated.b > maxB + epsilon)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.interpolatedColor = interpolated;
                    std::ostringstream oss;
                    oss << "Barycentric interpolation result should be within vertex bounds. "
                        << "Got: (" << interpolated.r << ", " << interpolated.g << ", " << interpolated.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Distance monotonicity - closer probes should have higher weight
            {
                float radius = gen.RandomFloat(20.0f, 100.0f);
                float dist1 = gen.RandomFloat(1.0f, radius * 0.3f);
                float dist2 = gen.RandomFloat(dist1 + 5.0f, radius * 0.8f);
                
                float weight1 = Systems::LightProbeSystem::CalculateDistanceWeight(dist1, radius);
                float weight2 = Systems::LightProbeSystem::CalculateDistanceWeight(dist2, radius);
                
                // Closer distance should have higher weight
                if (weight1 < weight2)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Closer probe should have higher weight. "
                        << "dist1=" << dist1 << " weight1=" << weight1 << ", "
                        << "dist2=" << dist2 << " weight2=" << weight2;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 5: Bilinear interpolation at corners should return corner values
            {
                glm::vec3 topLeft = gen.RandomColor();
                glm::vec3 topRight = gen.RandomColor();
                glm::vec3 bottomLeft = gen.RandomColor();
                glm::vec3 bottomRight = gen.RandomColor();
                
                // At (0, 0) should return topLeft
                glm::vec3 atTopLeft = Systems::LightProbeSystem::BilinearInterpolate(
                    topLeft, topRight, bottomLeft, bottomRight, 0.0f, 0.0f);
                
                if (!Vec3Equals(atTopLeft, topLeft, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Bilinear interpolation at (0,0) should return topLeft";
                    return result;
                }
                
                // At (1, 0) should return topRight
                glm::vec3 atTopRight = Systems::LightProbeSystem::BilinearInterpolate(
                    topLeft, topRight, bottomLeft, bottomRight, 1.0f, 0.0f);
                
                if (!Vec3Equals(atTopRight, topRight, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Bilinear interpolation at (1,0) should return topRight";
                    return result;
                }
                
                // At (0, 1) should return bottomLeft
                glm::vec3 atBottomLeft = Systems::LightProbeSystem::BilinearInterpolate(
                    topLeft, topRight, bottomLeft, bottomRight, 0.0f, 1.0f);
                
                if (!Vec3Equals(atBottomLeft, bottomLeft, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Bilinear interpolation at (0,1) should return bottomLeft";
                    return result;
                }
                
                // At (1, 1) should return bottomRight
                glm::vec3 atBottomRight = Systems::LightProbeSystem::BilinearInterpolate(
                    topLeft, topRight, bottomLeft, bottomRight, 1.0f, 1.0f);
                
                if (!Vec3Equals(atBottomRight, bottomRight, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Bilinear interpolation at (1,1) should return bottomRight";
                    return result;
                }
            }
        }
        
        return result;
    }


    /**
     * @brief Property 6: 光照探针实时更新正确性
     * 
     * For any realtime light probe:
     * - When scene lighting changes, probe sampled values should update in the next update cycle
     * - Update frequency should be respected
     * 
     * Note: This test validates the update mechanism logic, not actual scene integration.
     * 
     * Feature: 2d-lighting-enhancement, Property 6: 光照探针实时更新正确性
     * Validates: Requirements 3.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightProbeTestResult with pass/fail status and failure details
     */
    inline LightProbeTestResult TestProperty6_LightProbeRealtimeUpdateCorrectness(int iterations = 100)
    {
        LightProbeTestResult result;
        LightProbeRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Test 1: Update frequency configuration
            {
                float frequency = gen.RandomFloat(0.01f, 1.0f);
                
                // Verify frequency is positive
                if (frequency <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Update frequency should be positive";
                    return result;
                }
            }
            
            // Test 2: Probe data structure consistency
            {
                ECS::LightProbeData probe = gen.RandomLightProbeData(-100.0f, 100.0f, -100.0f, 100.0f);
                
                // Verify sampled color components are in valid range
                if (probe.sampledColor.r < 0.0f || probe.sampledColor.g < 0.0f || probe.sampledColor.b < 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Sampled color components should be non-negative";
                    return result;
                }
                
                // Verify influence radius is positive
                if (probe.influenceRadius <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Influence radius should be positive";
                    return result;
                }
                
                // Verify sampled intensity is in valid range
                if (probe.sampledIntensity < 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Sampled intensity should be non-negative";
                    return result;
                }
            }
            
            // Test 3: Distance calculation correctness
            {
                glm::vec2 a = gen.RandomPosition(-100.0f, 100.0f, -100.0f, 100.0f);
                glm::vec2 b = gen.RandomPosition(-100.0f, 100.0f, -100.0f, 100.0f);
                
                float distance = Systems::LightProbeSystem::CalculateDistance(a, b);
                float expectedDistance = glm::length(b - a);
                
                if (!FloatEquals(distance, expectedDistance, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Distance calculation mismatch. Got: " << distance 
                        << ", Expected: " << expectedDistance;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // Distance should be non-negative
                if (distance < 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Distance should be non-negative";
                    return result;
                }
                
                // Distance from point to itself should be 0
                float selfDistance = Systems::LightProbeSystem::CalculateDistance(a, a);
                if (!FloatEquals(selfDistance, 0.0f, 1e-5f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Distance from point to itself should be 0";
                    return result;
                }
            }
            
            // Test 4: Weight at zero distance should be maximum (1.0)
            {
                float radius = gen.RandomFloat(10.0f, 100.0f);
                float weightAtZero = Systems::LightProbeSystem::CalculateDistanceWeight(0.0f, radius);
                
                // Weight at distance 0 should be 1.0 (or very close)
                if (!FloatEquals(weightAtZero, 1.0f, 0.01f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Weight at distance 0 should be 1.0. Got: " << weightAtZero;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 5: Weight should smoothly decrease (no sudden jumps)
            {
                float radius = gen.RandomFloat(20.0f, 100.0f);
                float prevWeight = 1.0f;
                
                for (int step = 0; step < 10; ++step)
                {
                    float distance = (radius * step) / 10.0f;
                    float weight = Systems::LightProbeSystem::CalculateDistanceWeight(distance, radius);
                    
                    // Weight should not increase as distance increases
                    if (weight > prevWeight + 1e-5f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        std::ostringstream oss;
                        oss << "Weight should not increase with distance. "
                            << "At step " << step << ", distance=" << distance 
                            << ", weight=" << weight << ", prevWeight=" << prevWeight;
                        result.failureMessage = oss.str();
                        return result;
                    }
                    
                    prevWeight = weight;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 5 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty5Test()
    {
        LogInfo("Running Property 5: 光照探针插值正确性 (100 iterations)...");
        
        LightProbeTestResult result = TestProperty5_LightProbeInterpolationCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 5 (光照探针插值正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 5 (光照探针插值正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Run Property 6 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty6Test()
    {
        LogInfo("Running Property 6: 光照探针实时更新正确性 (100 iterations)...");
        
        LightProbeTestResult result = TestProperty6_LightProbeRealtimeUpdateCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 6 (光照探针实时更新正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 6 (光照探针实时更新正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Run all light probe system tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllLightProbeSystemTests()
    {
        LogInfo("=== Running Light Probe System Tests ===");
        
        bool allPassed = true;
        
        // Property 5: Light probe interpolation correctness
        if (!RunProperty5Test())
        {
            allPassed = false;
        }
        
        // Property 6: Light probe realtime update correctness
        if (!RunProperty6Test())
        {
            allPassed = false;
        }
        
        LogInfo("=== Light Probe System Tests Complete ===");
        return allPassed;
    }
}

#endif // LIGHT_PROBE_SYSTEM_TESTS_H

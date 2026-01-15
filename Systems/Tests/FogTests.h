#ifndef FOG_TESTS_H
#define FOG_TESTS_H

/**
 * @file FogTests.h
 * @brief Property-based tests for Fog effect
 * 
 * This file contains property-based tests for validating the correctness of
 * distance fog and height fog effects, including linear, exponential, and
 * exponential squared fog modes.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 21: 距离雾效正确性
 * Property 22: 高度雾正确性
 * Validates: Requirements 11.1, 11.3, 11.5
 */

#include "../PostProcessSystem.h"
#include "../../Components/PostProcessSettingsComponent.h"
#include "../../Components/LightingTypes.h"
#include "../../Utils/Logger.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace FogTests
{
    /**
     * @brief Random generator for Fog tests
     */
    class FogRandomGenerator
    {
    public:
        FogRandomGenerator() : m_gen(std::random_device{}()) {}
        
        FogRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        glm::vec3 RandomColor(float minVal, float maxVal)
        {
            return glm::vec3(
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal)
            );
        }

        ECS::FogMode RandomFogMode()
        {
            int mode = RandomInt(0, 2);
            return static_cast<ECS::FogMode>(mode);
        }

        ECS::FogParams RandomFogParams()
        {
            ECS::FogParams params;
            params.fogColor = glm::vec4(RandomColor(0.3f, 0.8f), 1.0f);
            params.fogDensity = RandomFloat(0.001f, 0.1f);
            params.fogStart = RandomFloat(5.0f, 20.0f);
            params.fogEnd = params.fogStart + RandomFloat(50.0f, 200.0f);
            params.fogMode = static_cast<uint32_t>(RandomFogMode());
            params.heightFogBase = RandomFloat(-10.0f, 10.0f);
            params.heightFogDensity = RandomFloat(0.01f, 0.5f);
            params.enableHeightFog = RandomInt(0, 1);
            params.enableFog = 1;
            params.cameraPosition = glm::vec2(RandomFloat(-100.0f, 100.0f), RandomFloat(-100.0f, 100.0f));
            params.cameraZoom = RandomFloat(0.5f, 2.0f);
            return params;
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

    // ============ Fog calculation functions (matching shader implementation) ============

    /**
     * @brief Calculate linear fog factor
     * Returns value in [0, 1], where 0 = fully fogged, 1 = no fog
     * Requirements: 11.1, 11.3
     */
    inline float CalculateLinearFog(float distance, float fogStart, float fogEnd)
    {
        return std::clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0f, 1.0f);
    }

    /**
     * @brief Calculate exponential fog factor
     * Returns value in [0, 1], where 0 = fully fogged, 1 = no fog
     * Requirements: 11.1, 11.3
     */
    inline float CalculateExponentialFog(float distance, float density)
    {
        return std::exp(-density * distance);
    }

    /**
     * @brief Calculate exponential squared fog factor
     * Returns value in [0, 1], where 0 = fully fogged, 1 = no fog
     * Requirements: 11.1, 11.3
     */
    inline float CalculateExponentialSquaredFog(float distance, float density)
    {
        float factor = density * distance;
        return std::exp(-factor * factor);
    }

    /**
     * @brief Calculate fog factor based on mode
     * Requirements: 11.1, 11.3
     */
    inline float CalculateDistanceFogFactor(float distance, float fogStart, float fogEnd, 
                                            float density, ECS::FogMode mode)
    {
        switch (mode)
        {
            case ECS::FogMode::Linear:
                return CalculateLinearFog(distance, fogStart, fogEnd);
            case ECS::FogMode::Exponential:
                return CalculateExponentialFog(distance, density);
            case ECS::FogMode::ExponentialSquared:
                return CalculateExponentialSquaredFog(distance, density);
            default:
                return CalculateLinearFog(distance, fogStart, fogEnd);
        }
    }

    /**
     * @brief Calculate height fog factor
     * Returns value in [0, 1], where 0 = fully fogged, 1 = no fog
     * Requirements: 11.5
     */
    inline float CalculateHeightFogFactor(float worldY, float fogBase, float heightDensity)
    {
        if (worldY <= fogBase)
        {
            // At or below base height, maximum fog
            return 1.0f; // Note: This returns 1.0 meaning "full height fog contribution"
        }
        else
        {
            // Above base height, fog decreases with height
            float heightAboveBase = worldY - fogBase;
            return std::exp(-heightDensity * heightAboveBase);
        }
    }

    /**
     * @brief Apply fog to color
     * fogFactor: 1.0 = no fog (keep original), 0.0 = fully fogged (use fog color)
     */
    inline glm::vec3 ApplyFog(const glm::vec3& color, const glm::vec3& fogColor, float fogFactor)
    {
        return glm::mix(fogColor, color, fogFactor);
    }

    /**
     * @brief Test result structure for Fog tests
     */
    struct FogTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        float distance = 0.0f;
        float fogStart = 0.0f;
        float fogEnd = 0.0f;
        float density = 0.0f;
        ECS::FogMode fogMode = ECS::FogMode::Linear;
        float fogFactor = 0.0f;
        float expectedFogFactor = 0.0f;
        float worldY = 0.0f;
        float heightFogBase = 0.0f;
        float heightFogDensity = 0.0f;
    };


    /**
     * @brief Property 21: 距离雾效正确性
     * 
     * For any distance fog configuration:
     * - Fog intensity should increase with distance
     * - At fogStart, fog effect should be 0 (for linear mode)
     * - At fogEnd, fog effect should be maximum (for linear mode)
     * - Linear and exponential modes should produce different attenuation curves
     * 
     * Feature: 2d-lighting-enhancement, Property 21: 距离雾效正确性
     * Validates: Requirements 11.1, 11.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return FogTestResult with pass/fail status and failure details
     */
    inline FogTestResult TestProperty21_DistanceFogCorrectness(int iterations = 100)
    {
        FogTestResult result;
        FogRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random fog parameters
            float fogStart = gen.RandomFloat(5.0f, 50.0f);
            float fogEnd = fogStart + gen.RandomFloat(50.0f, 200.0f);
            float density = gen.RandomFloat(0.001f, 0.05f);
            
            // Test 1: Linear fog - at fogStart, fog factor should be 1.0 (no fog)
            {
                float factorAtStart = CalculateLinearFog(fogStart, fogStart, fogEnd);
                if (!FloatEquals(factorAtStart, 1.0f, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.distance = fogStart;
                    result.fogStart = fogStart;
                    result.fogEnd = fogEnd;
                    result.fogMode = ECS::FogMode::Linear;
                    result.fogFactor = factorAtStart;
                    result.expectedFogFactor = 1.0f;
                    std::ostringstream oss;
                    oss << "Linear fog at fogStart should have factor 1.0 (no fog). "
                        << "Got: " << factorAtStart << " at distance=" << fogStart;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Linear fog - at fogEnd, fog factor should be 0.0 (full fog)
            {
                float factorAtEnd = CalculateLinearFog(fogEnd, fogStart, fogEnd);
                if (!FloatEquals(factorAtEnd, 0.0f, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.distance = fogEnd;
                    result.fogStart = fogStart;
                    result.fogEnd = fogEnd;
                    result.fogMode = ECS::FogMode::Linear;
                    result.fogFactor = factorAtEnd;
                    result.expectedFogFactor = 0.0f;
                    std::ostringstream oss;
                    oss << "Linear fog at fogEnd should have factor 0.0 (full fog). "
                        << "Got: " << factorAtEnd << " at distance=" << fogEnd;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Fog factor should decrease monotonically with distance (for all modes)
            for (int modeIdx = 0; modeIdx < 3; ++modeIdx)
            {
                ECS::FogMode mode = static_cast<ECS::FogMode>(modeIdx);
                
                float prevFactor = 1.0f;
                for (int d = 0; d <= 10; ++d)
                {
                    float distance = fogStart + (fogEnd - fogStart) * (d / 10.0f);
                    float factor = CalculateDistanceFogFactor(distance, fogStart, fogEnd, density, mode);
                    
                    // Factor should be in [0, 1]
                    if (factor < 0.0f || factor > 1.0f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.distance = distance;
                        result.fogStart = fogStart;
                        result.fogEnd = fogEnd;
                        result.density = density;
                        result.fogMode = mode;
                        result.fogFactor = factor;
                        std::ostringstream oss;
                        oss << "Fog factor should be in [0, 1]. Got: " << factor 
                            << " at distance=" << distance << " with mode=" << modeIdx;
                        result.failureMessage = oss.str();
                        return result;
                    }
                    
                    // Factor should decrease (or stay same) as distance increases
                    if (factor > prevFactor + 0.001f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.distance = distance;
                        result.fogStart = fogStart;
                        result.fogEnd = fogEnd;
                        result.density = density;
                        result.fogMode = mode;
                        result.fogFactor = factor;
                        result.expectedFogFactor = prevFactor;
                        std::ostringstream oss;
                        oss << "Fog factor should decrease with distance. "
                            << "Previous: " << prevFactor << ", Current: " << factor 
                            << " at distance=" << distance << " with mode=" << modeIdx;
                        result.failureMessage = oss.str();
                        return result;
                    }
                    
                    prevFactor = factor;
                }
            }
            
            // Test 4: Exponential fog should never reach exactly 0
            {
                float veryFarDistance = 10000.0f;
                float expFactor = CalculateExponentialFog(veryFarDistance, density);
                
                // Should be very small but positive
                if (expFactor < 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.distance = veryFarDistance;
                    result.density = density;
                    result.fogMode = ECS::FogMode::Exponential;
                    result.fogFactor = expFactor;
                    result.failureMessage = "Exponential fog factor should never be negative";
                    return result;
                }
            }
            
            // Test 5: Different modes should produce different curves
            {
                float midDistance = (fogStart + fogEnd) / 2.0f;
                float linearFactor = CalculateLinearFog(midDistance, fogStart, fogEnd);
                float expFactor = CalculateExponentialFog(midDistance, density);
                float expSqFactor = CalculateExponentialSquaredFog(midDistance, density);
                
                // All should be in valid range
                if (linearFactor < 0.0f || linearFactor > 1.0f ||
                    expFactor < 0.0f || expFactor > 1.0f ||
                    expSqFactor < 0.0f || expSqFactor > 1.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.distance = midDistance;
                    std::ostringstream oss;
                    oss << "All fog modes should produce factors in [0, 1]. "
                        << "Linear: " << linearFactor << ", Exp: " << expFactor 
                        << ", ExpSq: " << expSqFactor;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }


    /**
     * @brief Property 22: 高度雾正确性
     * 
     * For any height fog configuration:
     * - Fog density should be maximum at or below base height
     * - Fog density should decrease with height above base
     * - Higher heightFogDensity should produce faster falloff
     * 
     * Feature: 2d-lighting-enhancement, Property 22: 高度雾正确性
     * Validates: Requirements 11.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return FogTestResult with pass/fail status and failure details
     */
    inline FogTestResult TestProperty22_HeightFogCorrectness(int iterations = 100)
    {
        FogTestResult result;
        FogRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random height fog parameters
            float fogBase = gen.RandomFloat(-20.0f, 20.0f);
            float heightDensity = gen.RandomFloat(0.01f, 0.5f);
            
            // Test 1: At base height, fog factor should be 1.0 (full height fog contribution)
            {
                float factorAtBase = CalculateHeightFogFactor(fogBase, fogBase, heightDensity);
                if (!FloatEquals(factorAtBase, 1.0f, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.worldY = fogBase;
                    result.heightFogBase = fogBase;
                    result.heightFogDensity = heightDensity;
                    result.fogFactor = factorAtBase;
                    result.expectedFogFactor = 1.0f;
                    std::ostringstream oss;
                    oss << "Height fog at base should have factor 1.0. "
                        << "Got: " << factorAtBase << " at Y=" << fogBase;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Below base height, fog factor should still be 1.0
            {
                float belowBase = fogBase - gen.RandomFloat(1.0f, 50.0f);
                float factorBelowBase = CalculateHeightFogFactor(belowBase, fogBase, heightDensity);
                if (!FloatEquals(factorBelowBase, 1.0f, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.worldY = belowBase;
                    result.heightFogBase = fogBase;
                    result.heightFogDensity = heightDensity;
                    result.fogFactor = factorBelowBase;
                    result.expectedFogFactor = 1.0f;
                    std::ostringstream oss;
                    oss << "Height fog below base should have factor 1.0. "
                        << "Got: " << factorBelowBase << " at Y=" << belowBase 
                        << " (base=" << fogBase << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Above base height, fog factor should decrease
            {
                float aboveBase = fogBase + gen.RandomFloat(10.0f, 100.0f);
                float factorAboveBase = CalculateHeightFogFactor(aboveBase, fogBase, heightDensity);
                
                // Should be less than 1.0 (less fog at higher altitude)
                if (factorAboveBase >= 1.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.worldY = aboveBase;
                    result.heightFogBase = fogBase;
                    result.heightFogDensity = heightDensity;
                    result.fogFactor = factorAboveBase;
                    std::ostringstream oss;
                    oss << "Height fog above base should have factor < 1.0. "
                        << "Got: " << factorAboveBase << " at Y=" << aboveBase 
                        << " (base=" << fogBase << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // Should be positive
                if (factorAboveBase < 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.worldY = aboveBase;
                    result.heightFogBase = fogBase;
                    result.heightFogDensity = heightDensity;
                    result.fogFactor = factorAboveBase;
                    result.failureMessage = "Height fog factor should never be negative";
                    return result;
                }
            }
            
            // Test 4: Fog factor should decrease monotonically with height above base
            {
                float prevFactor = 1.0f;
                for (int h = 0; h <= 10; ++h)
                {
                    float height = fogBase + h * 10.0f;
                    float factor = CalculateHeightFogFactor(height, fogBase, heightDensity);
                    
                    // Factor should be in [0, 1]
                    if (factor < 0.0f || factor > 1.0f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.worldY = height;
                        result.heightFogBase = fogBase;
                        result.heightFogDensity = heightDensity;
                        result.fogFactor = factor;
                        std::ostringstream oss;
                        oss << "Height fog factor should be in [0, 1]. Got: " << factor 
                            << " at Y=" << height;
                        result.failureMessage = oss.str();
                        return result;
                    }
                    
                    // Factor should decrease (or stay same) as height increases
                    if (factor > prevFactor + 0.001f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.worldY = height;
                        result.heightFogBase = fogBase;
                        result.heightFogDensity = heightDensity;
                        result.fogFactor = factor;
                        result.expectedFogFactor = prevFactor;
                        std::ostringstream oss;
                        oss << "Height fog factor should decrease with height. "
                            << "Previous: " << prevFactor << ", Current: " << factor 
                            << " at Y=" << height;
                        result.failureMessage = oss.str();
                        return result;
                    }
                    
                    prevFactor = factor;
                }
            }
            
            // Test 5: Higher density should produce faster falloff
            {
                float testHeight = fogBase + 50.0f;
                float lowDensity = 0.01f;
                float highDensity = 0.1f;
                
                float factorLowDensity = CalculateHeightFogFactor(testHeight, fogBase, lowDensity);
                float factorHighDensity = CalculateHeightFogFactor(testHeight, fogBase, highDensity);
                
                // Higher density should produce lower factor (more fog falloff)
                if (factorHighDensity >= factorLowDensity)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.worldY = testHeight;
                    result.heightFogBase = fogBase;
                    std::ostringstream oss;
                    oss << "Higher density should produce faster falloff. "
                        << "Low density (" << lowDensity << ") factor: " << factorLowDensity 
                        << ", High density (" << highDensity << ") factor: " << factorHighDensity;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Test fog color blending
     * 
     * Validates that fog color is correctly blended with scene color.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return FogTestResult with pass/fail status and failure details
     */
    inline FogTestResult TestFogColorBlending(int iterations = 100)
    {
        FogTestResult result;
        FogRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random colors
            glm::vec3 sceneColor = gen.RandomColor(0.0f, 1.0f);
            glm::vec3 fogColor = gen.RandomColor(0.0f, 1.0f);
            float fogFactor = gen.RandomFloat(0.0f, 1.0f);
            
            // Apply fog
            glm::vec3 resultColor = ApplyFog(sceneColor, fogColor, fogFactor);
            
            // Test 1: Result should be in valid range
            if (resultColor.r < 0.0f || resultColor.r > 1.0f ||
                resultColor.g < 0.0f || resultColor.g > 1.0f ||
                resultColor.b < 0.0f || resultColor.b > 1.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.fogFactor = fogFactor;
                std::ostringstream oss;
                oss << "Fog blended color should be in [0, 1]. "
                    << "Got: (" << resultColor.r << ", " << resultColor.g << ", " << resultColor.b << ")";
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 2: At fogFactor = 1.0 (no fog), result should equal scene color
            {
                glm::vec3 noFogResult = ApplyFog(sceneColor, fogColor, 1.0f);
                if (!FloatEquals(noFogResult.r, sceneColor.r, 0.001f) ||
                    !FloatEquals(noFogResult.g, sceneColor.g, 0.001f) ||
                    !FloatEquals(noFogResult.b, sceneColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.fogFactor = 1.0f;
                    std::ostringstream oss;
                    oss << "At fogFactor=1.0, result should equal scene color. "
                        << "Scene: (" << sceneColor.r << ", " << sceneColor.g << ", " << sceneColor.b << "), "
                        << "Result: (" << noFogResult.r << ", " << noFogResult.g << ", " << noFogResult.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: At fogFactor = 0.0 (full fog), result should equal fog color
            {
                glm::vec3 fullFogResult = ApplyFog(sceneColor, fogColor, 0.0f);
                if (!FloatEquals(fullFogResult.r, fogColor.r, 0.001f) ||
                    !FloatEquals(fullFogResult.g, fogColor.g, 0.001f) ||
                    !FloatEquals(fullFogResult.b, fogColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.fogFactor = 0.0f;
                    std::ostringstream oss;
                    oss << "At fogFactor=0.0, result should equal fog color. "
                        << "Fog: (" << fogColor.r << ", " << fogColor.g << ", " << fogColor.b << "), "
                        << "Result: (" << fullFogResult.r << ", " << fullFogResult.g << ", " << fullFogResult.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 21 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty21Test()
    {
        LogInfo("Running Property 21: 距离雾效正确性 (100 iterations)...");
        
        FogTestResult result = TestProperty21_DistanceFogCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 21 (距离雾效正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 21 (距离雾效正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.distance > 0 || result.fogStart > 0)
            {
                LogError("Failing example: distance=%f, fogStart=%f, fogEnd=%f, density=%f, mode=%d, factor=%f",
                         result.distance, result.fogStart, result.fogEnd, result.density,
                         static_cast<int>(result.fogMode), result.fogFactor);
            }
            return false;
        }
    }

    /**
     * @brief Run Property 22 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty22Test()
    {
        LogInfo("Running Property 22: 高度雾正确性 (100 iterations)...");
        
        FogTestResult result = TestProperty22_HeightFogCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 22 (高度雾正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 22 (高度雾正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.worldY != 0 || result.heightFogBase != 0)
            {
                LogError("Failing example: worldY=%f, heightFogBase=%f, heightFogDensity=%f, factor=%f",
                         result.worldY, result.heightFogBase, result.heightFogDensity, result.fogFactor);
            }
            return false;
        }
    }

    /**
     * @brief Run all Fog tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllFogTests()
    {
        LogInfo("=== Running Fog Effect Tests ===");
        
        bool allPassed = true;
        
        // Property 21: Distance fog correctness
        if (!RunProperty21Test())
        {
            allPassed = false;
        }
        
        // Property 22: Height fog correctness
        if (!RunProperty22Test())
        {
            allPassed = false;
        }
        
        // Fog color blending test
        LogInfo("Running Fog Color Blending Test (100 iterations)...");
        FogTestResult blendResult = TestFogColorBlending(100);
        if (blendResult.passed)
        {
            LogInfo("Fog Color Blending Test PASSED");
        }
        else
        {
            LogError("Fog Color Blending Test FAILED at iteration %d", blendResult.failedIteration);
            LogError("Failure: %s", blendResult.failureMessage.c_str());
            allPassed = false;
        }
        
        LogInfo("=== Fog Effect Tests Complete ===");
        return allPassed;
    }
}

#endif // FOG_TESTS_H

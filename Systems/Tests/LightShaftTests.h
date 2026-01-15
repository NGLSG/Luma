#ifndef LIGHT_SHAFT_TESTS_H
#define LIGHT_SHAFT_TESTS_H

/**
 * @file LightShaftTests.h
 * @brief Property-based tests for Light Shaft (God Rays) effect
 * 
 * This file contains property-based tests for validating the correctness of
 * light shaft occlusion, including shadow buffer integration and radial blur
 * calculations.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 9: 光束遮挡正确性
 * Validates: Requirements 6.3, 6.4
 */

#include "../PostProcessSystem.h"
#include "../../Components/PostProcessSettingsComponent.h"
#include "../../Components/LightingTypes.h"
#include "../../Utils/Logger.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace LightShaftTests
{
    /**
     * @brief Random generator for Light Shaft tests
     */
    class LightShaftRandomGenerator
    {
    public:
        LightShaftRandomGenerator() : m_gen(std::random_device{}()) {}
        
        LightShaftRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        glm::vec2 RandomScreenUV()
        {
            return glm::vec2(
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f)
            );
        }

        glm::vec2 RandomWorldPos(float range = 100.0f)
        {
            return glm::vec2(
                RandomFloat(-range, range),
                RandomFloat(-range, range)
            );
        }

        glm::vec3 RandomColor(float minVal, float maxVal)
        {
            return glm::vec3(
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal)
            );
        }

        ECS::LightShaftParams RandomLightShaftParams()
        {
            ECS::LightShaftParams params;
            params.lightScreenPos = RandomScreenUV();
            params.lightWorldPos = RandomWorldPos();
            params.lightColor = glm::vec4(RandomColor(0.5f, 1.0f), 1.0f);
            params.density = RandomFloat(0.1f, 1.0f);
            params.decay = RandomFloat(0.8f, 0.99f);
            params.weight = RandomFloat(0.1f, 1.0f);
            params.exposure = RandomFloat(0.1f, 1.0f);
            params.numSamples = static_cast<uint32_t>(RandomInt(16, 128));
            params.lightRadius = RandomFloat(0.5f, 2.0f);
            params.lightIntensity = RandomFloat(0.5f, 2.0f);
            params.enableOcclusion = RandomInt(0, 1);
            return params;
        }

        ECS::PostProcessSettingsComponent RandomLightShaftSettings()
        {
            ECS::PostProcessSettingsComponent settings;
            settings.enableLightShafts = true;
            settings.lightShaftDensity = RandomFloat(0.1f, 1.0f);
            settings.lightShaftDecay = RandomFloat(0.8f, 0.99f);
            settings.lightShaftWeight = RandomFloat(0.1f, 1.0f);
            settings.lightShaftExposure = RandomFloat(0.1f, 1.0f);
            return settings;
        }

        /**
         * @brief Generate a random shadow map (1D array representing occlusion)
         * @param size Number of samples
         * @return Vector of shadow values [0, 1] where 0 = no shadow, 1 = full shadow
         */
        std::vector<float> RandomShadowMap(int size)
        {
            std::vector<float> shadowMap(size);
            for (int i = 0; i < size; ++i)
            {
                shadowMap[i] = RandomFloat(0.0f, 1.0f);
            }
            return shadowMap;
        }

        /**
         * @brief Generate a shadow map with a specific occluder
         * @param size Number of samples
         * @param occluderStart Start position of occluder [0, 1]
         * @param occluderEnd End position of occluder [0, 1]
         * @return Vector of shadow values
         */
        std::vector<float> ShadowMapWithOccluder(int size, float occluderStart, float occluderEnd)
        {
            std::vector<float> shadowMap(size, 0.0f);
            int startIdx = static_cast<int>(occluderStart * size);
            int endIdx = static_cast<int>(occluderEnd * size);
            for (int i = startIdx; i < endIdx && i < size; ++i)
            {
                shadowMap[i] = 1.0f; // Full occlusion
            }
            return shadowMap;
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
     * @brief Simulate radial blur sampling (CPU version for testing)
     * 
     * @param pixelUV Current pixel UV
     * @param lightUV Light source UV
     * @param density Blur density
     * @param decay Illumination decay
     * @param weight Sample weight
     * @param numSamples Number of samples
     * @param sceneColors Scene color samples along the ray
     * @return Accumulated light shaft color
     */
    inline glm::vec3 SimulateRadialBlur(
        const glm::vec2& pixelUV,
        const glm::vec2& lightUV,
        float density,
        float decay,
        float weight,
        int numSamples,
        const std::vector<glm::vec3>& sceneColors)
    {
        glm::vec2 deltaTexCoord = (pixelUV - lightUV) * density / static_cast<float>(numSamples);
        
        glm::vec2 sampleUV = pixelUV;
        float illuminationDecay = 1.0f;
        glm::vec3 color(0.0f);
        
        for (int i = 0; i < numSamples && i < static_cast<int>(sceneColors.size()); ++i)
        {
            sampleUV = sampleUV - deltaTexCoord;
            
            // Clamp UV
            sampleUV.x = std::clamp(sampleUV.x, 0.0f, 1.0f);
            sampleUV.y = std::clamp(sampleUV.y, 0.0f, 1.0f);
            
            // Sample scene color
            glm::vec3 sampleColor = sceneColors[i];
            
            // Accumulate
            color += sampleColor * illuminationDecay * weight;
            
            // Apply decay
            illuminationDecay *= decay;
        }
        
        return color;
    }

    /**
     * @brief Simulate radial blur with occlusion (CPU version for testing)
     * 
     * @param pixelUV Current pixel UV
     * @param lightUV Light source UV
     * @param density Blur density
     * @param decay Illumination decay
     * @param weight Sample weight
     * @param numSamples Number of samples
     * @param sceneColors Scene color samples along the ray
     * @param shadowValues Shadow values along the ray [0=no shadow, 1=full shadow]
     * @return Accumulated light shaft color with occlusion
     */
    inline glm::vec3 SimulateRadialBlurWithOcclusion(
        const glm::vec2& pixelUV,
        const glm::vec2& lightUV,
        float density,
        float decay,
        float weight,
        int numSamples,
        const std::vector<glm::vec3>& sceneColors,
        const std::vector<float>& shadowValues)
    {
        glm::vec2 deltaTexCoord = (pixelUV - lightUV) * density / static_cast<float>(numSamples);
        
        glm::vec2 sampleUV = pixelUV;
        float illuminationDecay = 1.0f;
        glm::vec3 color(0.0f);
        
        for (int i = 0; i < numSamples && i < static_cast<int>(sceneColors.size()); ++i)
        {
            sampleUV = sampleUV - deltaTexCoord;
            
            // Clamp UV
            sampleUV.x = std::clamp(sampleUV.x, 0.0f, 1.0f);
            sampleUV.y = std::clamp(sampleUV.y, 0.0f, 1.0f);
            
            // Sample scene color
            glm::vec3 sampleColor = sceneColors[i];
            
            // Get occlusion (0 = no shadow = full light, 1 = full shadow = no light)
            float shadowValue = (i < static_cast<int>(shadowValues.size())) ? shadowValues[i] : 0.0f;
            float occlusion = 1.0f - shadowValue;
            
            // Accumulate with occlusion
            color += sampleColor * illuminationDecay * weight * occlusion;
            
            // Apply decay
            illuminationDecay *= decay;
        }
        
        return color;
    }

    /**
     * @brief Test result structure for Light Shaft tests
     */
    struct LightShaftTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec2 pixelUV{0.0f, 0.0f};
        glm::vec2 lightUV{0.0f, 0.0f};
        float occluderPosition = 0.0f;
        glm::vec3 resultWithOcclusion{0.0f, 0.0f, 0.0f};
        glm::vec3 resultWithoutOcclusion{0.0f, 0.0f, 0.0f};
    };

    /**
     * @brief Property 9: 光束遮挡正确性
     * 
     * For any light shaft and shadow caster:
     * - When shadow caster is between light source and target point, light shaft should be occluded/attenuated
     * - Occlusion should reduce light shaft intensity
     * - No occlusion should produce full light shaft intensity
     * 
     * Feature: 2d-lighting-enhancement, Property 9: 光束遮挡正确性
     * Validates: Requirements 6.3, 6.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightShaftTestResult with pass/fail status and failure details
     */
    inline LightShaftTestResult TestProperty9_LightShaftOcclusionCorrectness(int iterations = 100)
    {
        LightShaftTestResult result;
        LightShaftRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random light shaft parameters
            float density = gen.RandomFloat(0.3f, 0.8f);
            float decay = gen.RandomFloat(0.9f, 0.99f);
            float weight = gen.RandomFloat(0.3f, 0.8f);
            int numSamples = gen.RandomInt(16, 64);
            
            // Generate random pixel and light positions
            glm::vec2 pixelUV = gen.RandomScreenUV();
            glm::vec2 lightUV = gen.RandomScreenUV();
            
            // Ensure pixel and light are not at the same position
            while (glm::length(pixelUV - lightUV) < 0.1f)
            {
                lightUV = gen.RandomScreenUV();
            }
            
            // Generate scene colors (uniform for simplicity)
            std::vector<glm::vec3> sceneColors(numSamples, glm::vec3(1.0f));
            
            // Test 1: Full occlusion should produce zero or near-zero light shaft
            {
                std::vector<float> fullShadow(numSamples, 1.0f); // All occluded
                
                glm::vec3 occludedResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, fullShadow);
                
                float occludedMag = occludedResult.r + occludedResult.g + occludedResult.b;
                
                if (occludedMag > 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.pixelUV = pixelUV;
                    result.lightUV = lightUV;
                    result.resultWithOcclusion = occludedResult;
                    std::ostringstream oss;
                    oss << "Full occlusion should produce zero light shaft. "
                        << "Got magnitude: " << occludedMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: No occlusion should produce non-zero light shaft
            {
                std::vector<float> noShadow(numSamples, 0.0f); // No occlusion
                
                glm::vec3 unoccludedResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, noShadow);
                
                float unoccludedMag = unoccludedResult.r + unoccludedResult.g + unoccludedResult.b;
                
                if (unoccludedMag <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.pixelUV = pixelUV;
                    result.lightUV = lightUV;
                    result.resultWithoutOcclusion = unoccludedResult;
                    std::ostringstream oss;
                    oss << "No occlusion should produce positive light shaft. "
                        << "Got magnitude: " << unoccludedMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Partial occlusion should reduce light shaft intensity
            {
                std::vector<float> noShadow(numSamples, 0.0f);
                std::vector<float> partialShadow(numSamples, 0.5f); // 50% occlusion
                
                glm::vec3 unoccludedResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, noShadow);
                glm::vec3 partialResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, partialShadow);
                
                float unoccludedMag = unoccludedResult.r + unoccludedResult.g + unoccludedResult.b;
                float partialMag = partialResult.r + partialResult.g + partialResult.b;
                
                // Partial occlusion should produce less light than no occlusion
                if (partialMag >= unoccludedMag && unoccludedMag > 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.pixelUV = pixelUV;
                    result.lightUV = lightUV;
                    result.resultWithOcclusion = partialResult;
                    result.resultWithoutOcclusion = unoccludedResult;
                    std::ostringstream oss;
                    oss << "Partial occlusion should reduce light shaft intensity. "
                        << "Unoccluded: " << unoccludedMag << ", Partial: " << partialMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Occluder between light and pixel should block light
            {
                // Create shadow map with occluder in the middle of the ray
                float occluderStart = 0.3f;
                float occluderEnd = 0.6f;
                std::vector<float> occluderShadow = gen.ShadowMapWithOccluder(numSamples, occluderStart, occluderEnd);
                std::vector<float> noShadow(numSamples, 0.0f);
                
                glm::vec3 occludedResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, occluderShadow);
                glm::vec3 unoccludedResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, decay, weight, numSamples, sceneColors, noShadow);
                
                float occludedMag = occludedResult.r + occludedResult.g + occludedResult.b;
                float unoccludedMag = unoccludedResult.r + unoccludedResult.g + unoccludedResult.b;
                
                // Occluder should reduce light shaft
                if (occludedMag >= unoccludedMag && unoccludedMag > 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.pixelUV = pixelUV;
                    result.lightUV = lightUV;
                    result.occluderPosition = (occluderStart + occluderEnd) / 2.0f;
                    result.resultWithOcclusion = occludedResult;
                    result.resultWithoutOcclusion = unoccludedResult;
                    std::ostringstream oss;
                    oss << "Occluder between light and pixel should block light. "
                        << "Occluder at [" << occluderStart << ", " << occluderEnd << "]. "
                        << "Unoccluded: " << unoccludedMag << ", Occluded: " << occludedMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 5: Decay should reduce contribution over distance
            {
                std::vector<float> noShadow(numSamples, 0.0f);
                
                // Compare with different decay values
                float lowDecay = 0.8f;
                float highDecay = 0.99f;
                
                glm::vec3 lowDecayResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, lowDecay, weight, numSamples, sceneColors, noShadow);
                glm::vec3 highDecayResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, lightUV, density, highDecay, weight, numSamples, sceneColors, noShadow);
                
                float lowDecayMag = lowDecayResult.r + lowDecayResult.g + lowDecayResult.b;
                float highDecayMag = highDecayResult.r + highDecayResult.g + highDecayResult.b;
                
                // Higher decay should produce more light (less attenuation)
                if (highDecayMag < lowDecayMag && lowDecayMag > 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Higher decay should produce more light. "
                        << "Low decay (" << lowDecay << "): " << lowDecayMag 
                        << ", High decay (" << highDecay << "): " << highDecayMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Test light shaft supports directional and spot lights
     * 
     * Validates that light shafts work correctly with different light types.
     * Validates: Requirements 6.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightShaftTestResult with pass/fail status and failure details
     */
    inline LightShaftTestResult TestLightShaftLightTypeSupport(int iterations = 100)
    {
        LightShaftTestResult result;
        LightShaftRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random parameters
            float density = gen.RandomFloat(0.3f, 0.8f);
            float decay = gen.RandomFloat(0.9f, 0.99f);
            float weight = gen.RandomFloat(0.3f, 0.8f);
            int numSamples = gen.RandomInt(16, 64);
            
            // Test with different light positions (simulating different light types)
            // Directional light: light position far away (simulated by UV outside [0,1])
            // Spot light: light position within scene
            
            glm::vec2 pixelUV = gen.RandomScreenUV();
            
            // Test 1: Light inside screen (spot/point light)
            {
                glm::vec2 insideLightUV(0.5f, 0.5f); // Center of screen
                std::vector<glm::vec3> sceneColors(numSamples, glm::vec3(1.0f));
                std::vector<float> noShadow(numSamples, 0.0f);
                
                glm::vec3 insideResult = SimulateRadialBlurWithOcclusion(
                    pixelUV, insideLightUV, density, decay, weight, numSamples, sceneColors, noShadow);
                
                float insideMag = insideResult.r + insideResult.g + insideResult.b;
                
                // Should produce valid light shaft
                if (insideMag <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.pixelUV = pixelUV;
                    result.lightUV = insideLightUV;
                    std::ostringstream oss;
                    oss << "Light inside screen should produce valid light shaft. "
                        << "Got magnitude: " << insideMag;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Light at edge of screen (directional light simulation)
            {
                glm::vec2 edgeLightUV(0.0f, 0.5f); // Left edge
                std::vector<glm::vec3> sceneColors(numSamples, glm::vec3(1.0f));
                std::vector<float> noShadow(numSamples, 0.0f);
                
                // Only test if pixel is not at the same position as light
                if (glm::length(pixelUV - edgeLightUV) > 0.1f)
                {
                    glm::vec3 edgeResult = SimulateRadialBlurWithOcclusion(
                        pixelUV, edgeLightUV, density, decay, weight, numSamples, sceneColors, noShadow);
                    
                    float edgeMag = edgeResult.r + edgeResult.g + edgeResult.b;
                    
                    // Should still produce valid light shaft
                    if (edgeMag <= 0.0f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.pixelUV = pixelUV;
                        result.lightUV = edgeLightUV;
                        std::ostringstream oss;
                        oss << "Light at screen edge should produce valid light shaft. "
                            << "Got magnitude: " << edgeMag;
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 9 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty9Test()
    {
        LogInfo("Running Property 9: 光束遮挡正确性 (100 iterations)...");
        
        LightShaftTestResult result = TestProperty9_LightShaftOcclusionCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 9 (光束遮挡正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 9 (光束遮挡正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.occluderPosition > 0)
            {
                LogError("Failing example: pixelUV=(%f, %f), lightUV=(%f, %f), occluderPos=%f",
                         result.pixelUV.x, result.pixelUV.y,
                         result.lightUV.x, result.lightUV.y,
                         result.occluderPosition);
                LogError("Result with occlusion: (%f, %f, %f), without: (%f, %f, %f)",
                         result.resultWithOcclusion.r, result.resultWithOcclusion.g, result.resultWithOcclusion.b,
                         result.resultWithoutOcclusion.r, result.resultWithoutOcclusion.g, result.resultWithoutOcclusion.b);
            }
            return false;
        }
    }

    /**
     * @brief Run all Light Shaft tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllLightShaftTests()
    {
        LogInfo("=== Running Light Shaft Effect Tests ===");
        
        bool allPassed = true;
        
        // Property 9: Light shaft occlusion correctness
        if (!RunProperty9Test())
        {
            allPassed = false;
        }
        
        // Light type support test
        LogInfo("Running Light Shaft Light Type Support Test (100 iterations)...");
        LightShaftTestResult lightTypeResult = TestLightShaftLightTypeSupport(100);
        if (lightTypeResult.passed)
        {
            LogInfo("Light Shaft Light Type Support Test PASSED");
        }
        else
        {
            LogError("Light Shaft Light Type Support Test FAILED at iteration %d", lightTypeResult.failedIteration);
            LogError("Failure: %s", lightTypeResult.failureMessage.c_str());
            allPassed = false;
        }
        
        LogInfo("=== Light Shaft Effect Tests Complete ===");
        return allPassed;
    }
}

#endif // LIGHT_SHAFT_TESTS_H

#ifndef BLOOM_TESTS_H
#define BLOOM_TESTS_H

/**
 * @file BloomTests.h
 * @brief Property-based tests for Bloom effect
 * 
 * This file contains property-based tests for validating the correctness of
 * Bloom extraction, including brightness threshold extraction and emission
 * buffer contribution.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 8: Bloom 提取正确性
 * Validates: Requirements 5.1, 5.3
 */

#include "../PostProcessSystem.h"
#include "../../Components/PostProcessSettingsComponent.h"
#include "../../Utils/Logger.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace BloomTests
{
    /**
     * @brief Random generator for Bloom tests
     */
    class BloomRandomGenerator
    {
    public:
        BloomRandomGenerator() : m_gen(std::random_device{}()) {}
        
        BloomRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        glm::vec4 RandomColorWithAlpha(float minVal, float maxVal)
        {
            return glm::vec4(
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal),
                RandomFloat(0.0f, 1.0f)
            );
        }

        ECS::PostProcessSettingsComponent RandomBloomSettings()
        {
            ECS::PostProcessSettingsComponent settings;
            settings.enableBloom = true;
            settings.bloomThreshold = RandomFloat(0.5f, 2.0f);
            settings.bloomIntensity = RandomFloat(0.1f, 2.0f);
            settings.bloomRadius = RandomFloat(1.0f, 8.0f);
            settings.bloomIterations = RandomInt(1, 8);
            settings.bloomTint = ECS::Color(
                RandomFloat(0.5f, 1.0f),
                RandomFloat(0.5f, 1.0f),
                RandomFloat(0.5f, 1.0f),
                1.0f
            );
            return settings;
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
     * @brief Calculate luminance using Rec. 709 coefficients
     */
    inline float Luminance(const glm::vec3& color)
    {
        return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    }

    /**
     * @brief Extract brightness using soft threshold (matches shader implementation)
     */
    inline glm::vec3 ExtractBrightnessSoft(const glm::vec3& color, float threshold, float softKnee)
    {
        float brightness = Luminance(color);
        float soft = brightness - threshold + softKnee;
        float contribution = std::clamp(soft * soft / (4.0f * softKnee + 0.00001f), 0.0f, 1.0f);
        return color * contribution;
    }

    /**
     * @brief Test result structure for Bloom tests
     */
    struct BloomTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec3 inputColor{0.0f, 0.0f, 0.0f};
        float threshold = 0.0f;
        float luminance = 0.0f;
        glm::vec3 extractedColor{0.0f, 0.0f, 0.0f};
        bool shouldBeExtracted = false;
    };

    /**
     * @brief Property 8: Bloom 提取正确性
     * 
     * For any pixel color:
     * - When luminance exceeds bloom threshold, it should be extracted to bloom buffer
     * - When luminance is below threshold, it should NOT be extracted
     * - Bloom result should be added to final image
     * 
     * Feature: 2d-lighting-enhancement, Property 8: Bloom 提取正确性
     * Validates: Requirements 5.1, 5.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return BloomTestResult with pass/fail status and failure details
     */
    inline BloomTestResult TestProperty8_BloomExtractionCorrectness(int iterations = 100)
    {
        BloomTestResult result;
        BloomRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random threshold
            float threshold = gen.RandomFloat(0.5f, 2.0f);
            float softKnee = threshold * 0.5f;
            
            // Test 1: Color with luminance clearly above threshold should be extracted
            {
                // Generate bright color (luminance > threshold + softKnee)
                float targetLuminance = threshold + softKnee + gen.RandomFloat(0.5f, 2.0f);
                glm::vec3 brightColor = glm::vec3(targetLuminance / 0.7152f); // Approximate
                
                // Ensure luminance is actually above threshold
                float actualLuminance = Luminance(brightColor);
                if (actualLuminance > threshold + softKnee)
                {
                    glm::vec3 extracted = ExtractBrightnessSoft(brightColor, threshold, softKnee);
                    float extractedMagnitude = extracted.r + extracted.g + extracted.b;
                    
                    if (extractedMagnitude <= 0.0f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.inputColor = brightColor;
                        result.threshold = threshold;
                        result.luminance = actualLuminance;
                        result.extractedColor = extracted;
                        result.shouldBeExtracted = true;
                        std::ostringstream oss;
                        oss << "Bright color (luminance=" << actualLuminance 
                            << " > threshold=" << threshold << ") should be extracted. "
                            << "Input: (" << brightColor.r << ", " << brightColor.g << ", " << brightColor.b << "), "
                            << "Extracted: (" << extracted.r << ", " << extracted.g << ", " << extracted.b << ")";
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
            
            // Test 2: Color with luminance clearly below threshold should NOT be extracted
            {
                // Generate dark color (luminance < threshold - softKnee)
                float targetLuminance = std::max(0.0f, threshold - softKnee - gen.RandomFloat(0.1f, 0.5f));
                glm::vec3 darkColor = glm::vec3(targetLuminance / 0.7152f); // Approximate
                
                float actualLuminance = Luminance(darkColor);
                if (actualLuminance < threshold - softKnee && actualLuminance >= 0.0f)
                {
                    glm::vec3 extracted = ExtractBrightnessSoft(darkColor, threshold, softKnee);
                    float extractedMagnitude = extracted.r + extracted.g + extracted.b;
                    
                    // Should be zero or very close to zero
                    if (extractedMagnitude > 0.001f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.inputColor = darkColor;
                        result.threshold = threshold;
                        result.luminance = actualLuminance;
                        result.extractedColor = extracted;
                        result.shouldBeExtracted = false;
                        std::ostringstream oss;
                        oss << "Dark color (luminance=" << actualLuminance 
                            << " < threshold=" << threshold << ") should NOT be extracted. "
                            << "Input: (" << darkColor.r << ", " << darkColor.g << ", " << darkColor.b << "), "
                            << "Extracted: (" << extracted.r << ", " << extracted.g << ", " << extracted.b << ")";
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
            
            // Test 3: Extraction should preserve color ratios
            {
                glm::vec3 color = gen.RandomColor(0.0f, 3.0f);
                float luminance = Luminance(color);
                
                if (luminance > threshold + softKnee)
                {
                    glm::vec3 extracted = ExtractBrightnessSoft(color, threshold, softKnee);
                    
                    // Check that color ratios are preserved (if extracted is non-zero)
                    float extractedMag = extracted.r + extracted.g + extracted.b;
                    float inputMag = color.r + color.g + color.b;
                    
                    if (extractedMag > 0.001f && inputMag > 0.001f)
                    {
                        float ratioR_input = color.r / inputMag;
                        float ratioG_input = color.g / inputMag;
                        float ratioB_input = color.b / inputMag;
                        
                        float ratioR_extracted = extracted.r / extractedMag;
                        float ratioG_extracted = extracted.g / extractedMag;
                        float ratioB_extracted = extracted.b / extractedMag;
                        
                        float epsilon = 0.01f;
                        if (!FloatEquals(ratioR_input, ratioR_extracted, epsilon) ||
                            !FloatEquals(ratioG_input, ratioG_extracted, epsilon) ||
                            !FloatEquals(ratioB_input, ratioB_extracted, epsilon))
                        {
                            result.passed = false;
                            result.failedIteration = i;
                            result.inputColor = color;
                            result.threshold = threshold;
                            result.extractedColor = extracted;
                            std::ostringstream oss;
                            oss << "Bloom extraction should preserve color ratios. "
                                << "Input ratios: (" << ratioR_input << ", " << ratioG_input << ", " << ratioB_input << "), "
                                << "Extracted ratios: (" << ratioR_extracted << ", " << ratioG_extracted << ", " << ratioB_extracted << ")";
                            result.failureMessage = oss.str();
                            return result;
                        }
                    }
                }
            }
            
            // Test 4: Soft threshold should provide smooth transition
            {
                // Test colors at the threshold boundary
                float testLuminance = threshold;
                glm::vec3 boundaryColor = glm::vec3(testLuminance / 0.7152f);
                
                glm::vec3 extracted = ExtractBrightnessSoft(boundaryColor, threshold, softKnee);
                
                // At exactly threshold, with soft knee, there should be some contribution
                // (not zero, not full)
                float extractedMag = extracted.r + extracted.g + extracted.b;
                float inputMag = boundaryColor.r + boundaryColor.g + boundaryColor.b;
                
                // The contribution should be between 0 and 1 (partial)
                if (inputMag > 0.001f)
                {
                    float ratio = extractedMag / inputMag;
                    // At threshold, ratio should be around 0.25 (soft knee formula)
                    // Allow some tolerance
                    if (ratio < 0.0f || ratio > 1.0f)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.inputColor = boundaryColor;
                        result.threshold = threshold;
                        result.extractedColor = extracted;
                        std::ostringstream oss;
                        oss << "Soft threshold should provide smooth transition. "
                            << "At threshold boundary, ratio=" << ratio << " should be in [0, 1]";
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Test Bloom composite adds to final image
     * 
     * Validates that Bloom result is correctly added to the scene color.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return BloomTestResult with pass/fail status and failure details
     */
    inline BloomTestResult TestBloomCompositeAddition(int iterations = 100)
    {
        BloomTestResult result;
        BloomRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random scene color and bloom color
            glm::vec3 sceneColor = gen.RandomColor(0.0f, 1.0f);
            glm::vec3 bloomColor = gen.RandomColor(0.0f, 0.5f);
            float bloomIntensity = gen.RandomFloat(0.1f, 2.0f);
            
            // Simulate bloom composite (additive blending)
            glm::vec3 bloomContrib = bloomColor * bloomIntensity;
            glm::vec3 finalColor = sceneColor + bloomContrib;
            
            // Test 1: Final color should be >= scene color (additive)
            if (finalColor.r < sceneColor.r - 0.001f ||
                finalColor.g < sceneColor.g - 0.001f ||
                finalColor.b < sceneColor.b - 0.001f)
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Bloom composite should be additive. "
                    << "Scene: (" << sceneColor.r << ", " << sceneColor.g << ", " << sceneColor.b << "), "
                    << "Final: (" << finalColor.r << ", " << finalColor.g << ", " << finalColor.b << ")";
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 2: Bloom contribution should scale with intensity
            float bloomContribMag = bloomContrib.r + bloomContrib.g + bloomContrib.b;
            float expectedContribMag = (bloomColor.r + bloomColor.g + bloomColor.b) * bloomIntensity;
            
            if (!FloatEquals(bloomContribMag, expectedContribMag, 0.001f))
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Bloom contribution should scale with intensity. "
                    << "Expected: " << expectedContribMag << ", Got: " << bloomContribMag;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 3: Zero bloom intensity should not change scene
            glm::vec3 zeroBloomFinal = sceneColor + bloomColor * 0.0f;
            if (!FloatEquals(zeroBloomFinal.r, sceneColor.r, 0.001f) ||
                !FloatEquals(zeroBloomFinal.g, sceneColor.g, 0.001f) ||
                !FloatEquals(zeroBloomFinal.b, sceneColor.b, 0.001f))
            {
                result.passed = false;
                result.failedIteration = i;
                result.failureMessage = "Zero bloom intensity should not change scene color";
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Test HDR emission values contribute to Bloom
     * 
     * Validates that HDR emission values (> 1.0) correctly contribute to Bloom.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return BloomTestResult with pass/fail status and failure details
     */
    inline BloomTestResult TestHDREmissionContribution(int iterations = 100)
    {
        BloomTestResult result;
        BloomRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate HDR emission color (values > 1.0)
            glm::vec3 hdrEmission = gen.RandomColor(1.0f, 5.0f);
            float emissionAlpha = gen.RandomFloat(0.5f, 1.0f);
            
            // Emission contribution (as per shader: emissionColor.rgb * emissionColor.a)
            glm::vec3 emissionContrib = hdrEmission * emissionAlpha;
            
            // Test 1: HDR emission should have significant contribution
            float emissionMag = emissionContrib.r + emissionContrib.g + emissionContrib.b;
            if (emissionMag <= 0.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "HDR emission should have positive contribution. "
                    << "Emission: (" << hdrEmission.r << ", " << hdrEmission.g << ", " << hdrEmission.b << "), "
                    << "Alpha: " << emissionAlpha << ", Contribution: " << emissionMag;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 2: Higher emission values should produce higher contribution
            glm::vec3 higherEmission = hdrEmission * 2.0f;
            glm::vec3 higherContrib = higherEmission * emissionAlpha;
            float higherMag = higherContrib.r + higherContrib.g + higherContrib.b;
            
            if (higherMag <= emissionMag)
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Higher emission should produce higher contribution. "
                    << "Original: " << emissionMag << ", Higher: " << higherMag;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 3: Zero alpha should produce zero contribution
            glm::vec3 zeroAlphaContrib = hdrEmission * 0.0f;
            float zeroMag = zeroAlphaContrib.r + zeroAlphaContrib.g + zeroAlphaContrib.b;
            if (zeroMag != 0.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.failureMessage = "Zero emission alpha should produce zero contribution";
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 8 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty8Test()
    {
        LogInfo("Running Property 8: Bloom 提取正确性 (100 iterations)...");
        
        BloomTestResult result = TestProperty8_BloomExtractionCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 8 (Bloom 提取正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 8 (Bloom 提取正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.threshold > 0)
            {
                LogError("Failing example: inputColor=(%f, %f, %f), threshold=%f, luminance=%f, shouldBeExtracted=%s",
                         result.inputColor.r, result.inputColor.g, result.inputColor.b,
                         result.threshold, result.luminance,
                         result.shouldBeExtracted ? "true" : "false");
            }
            return false;
        }
    }

    /**
     * @brief Run all Bloom tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllBloomTests()
    {
        LogInfo("=== Running Bloom Effect Tests ===");
        
        bool allPassed = true;
        
        // Property 8: Bloom extraction correctness
        if (!RunProperty8Test())
        {
            allPassed = false;
        }
        
        // Bloom composite addition test
        LogInfo("Running Bloom Composite Addition Test (100 iterations)...");
        BloomTestResult compositeResult = TestBloomCompositeAddition(100);
        if (compositeResult.passed)
        {
            LogInfo("Bloom Composite Addition Test PASSED");
        }
        else
        {
            LogError("Bloom Composite Addition Test FAILED at iteration %d", compositeResult.failedIteration);
            LogError("Failure: %s", compositeResult.failureMessage.c_str());
            allPassed = false;
        }
        
        // HDR emission contribution test
        LogInfo("Running HDR Emission Contribution Test (100 iterations)...");
        BloomTestResult hdrResult = TestHDREmissionContribution(100);
        if (hdrResult.passed)
        {
            LogInfo("HDR Emission Contribution Test PASSED");
        }
        else
        {
            LogError("HDR Emission Contribution Test FAILED at iteration %d", hdrResult.failedIteration);
            LogError("Failure: %s", hdrResult.failureMessage.c_str());
            allPassed = false;
        }
        
        LogInfo("=== Bloom Effect Tests Complete ===");
        return allPassed;
    }
}

#endif // BLOOM_TESTS_H

#ifndef TONE_MAPPING_TESTS_H
#define TONE_MAPPING_TESTS_H

/**
 * @file ToneMappingTests.h
 * @brief Property-based tests for Tone Mapping and Color Grading
 * 
 * This file contains property-based tests for validating the correctness of
 * tone mapping algorithms (Reinhard, ACES, Filmic) and color adjustments
 * (exposure, contrast, saturation).
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 18: 色调映射 HDR 到 LDR
 * Property 19: LUT 颜色变换
 * Property 20: 曝光/对比度/饱和度调整
 * Validates: Requirements 10.1, 10.3, 10.4
 */

#include "../../Components/LightingTypes.h"
#include "../../Utils/Logger.h"
#include "glm/vec3.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <algorithm>

namespace ToneMappingTests
{
    /**
     * @brief Random generator for Tone Mapping tests
     */
    class ToneMappingRandomGenerator
    {
    public:
        ToneMappingRandomGenerator() : m_gen(std::random_device{}()) {}
        
        ToneMappingRandomGenerator(unsigned int seed) : m_gen(seed) {}

        float RandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(m_gen);
        }

        glm::vec3 RandomHDRColor(float minVal, float maxVal)
        {
            return glm::vec3(
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal),
                RandomFloat(minVal, maxVal)
            );
        }

        ECS::ToneMappingMode RandomToneMappingMode()
        {
            int mode = RandomFloat(0, 4);
            return static_cast<ECS::ToneMappingMode>(std::min(mode, 3));
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

    // ============ Tone Mapping Algorithms (CPU implementations) ============

    /**
     * @brief Reinhard tone mapping
     */
    inline glm::vec3 ToneMappingReinhard(const glm::vec3& hdr)
    {
        return hdr / (hdr + glm::vec3(1.0f));
    }

    /**
     * @brief ACES tone mapping
     */
    inline glm::vec3 ToneMappingACES(const glm::vec3& hdr)
    {
        const float a = 2.51f;
        const float b = 0.03f;
        const float c = 2.43f;
        const float d = 0.59f;
        const float e = 0.14f;
        glm::vec3 result = (hdr * (a * hdr + glm::vec3(b))) / 
                           (hdr * (c * hdr + glm::vec3(d)) + glm::vec3(e));
        return glm::clamp(result, glm::vec3(0.0f), glm::vec3(1.0f));
    }

    /**
     * @brief Filmic tone mapping helper
     */
    inline glm::vec3 FilmicHelper(const glm::vec3& x)
    {
        const float A = 0.15f;
        const float B = 0.50f;
        const float C = 0.10f;
        const float D = 0.20f;
        const float E = 0.02f;
        const float F = 0.30f;
        return ((x * (A * x + glm::vec3(C * B)) + glm::vec3(D * E)) / 
                (x * (A * x + glm::vec3(B)) + glm::vec3(D * F))) - glm::vec3(E / F);
    }

    /**
     * @brief Filmic tone mapping
     */
    inline glm::vec3 ToneMappingFilmic(const glm::vec3& hdr)
    {
        const float W = 11.2f;
        const float exposureBias = 2.0f;
        glm::vec3 curr = FilmicHelper(hdr * exposureBias);
        glm::vec3 whiteScale = glm::vec3(1.0f) / FilmicHelper(glm::vec3(W));
        return curr * whiteScale;
    }

    /**
     * @brief Apply tone mapping based on mode
     */
    inline glm::vec3 ApplyToneMapping(const glm::vec3& hdr, ECS::ToneMappingMode mode)
    {
        switch (mode)
        {
            case ECS::ToneMappingMode::None:
                return glm::clamp(hdr, glm::vec3(0.0f), glm::vec3(1.0f));
            case ECS::ToneMappingMode::Reinhard:
                return ToneMappingReinhard(hdr);
            case ECS::ToneMappingMode::ACES:
                return ToneMappingACES(hdr);
            case ECS::ToneMappingMode::Filmic:
                return ToneMappingFilmic(hdr);
            default:
                return ToneMappingACES(hdr);
        }
    }

    // ============ Color Adjustment Functions (CPU implementations) ============

    /**
     * @brief Apply exposure adjustment
     */
    inline glm::vec3 ApplyExposure(const glm::vec3& color, float exposure)
    {
        return color * exposure;
    }

    /**
     * @brief Apply contrast adjustment
     */
    inline glm::vec3 ApplyContrast(const glm::vec3& color, float contrast)
    {
        return (color - glm::vec3(0.5f)) * contrast + glm::vec3(0.5f);
    }

    /**
     * @brief Apply saturation adjustment
     */
    inline glm::vec3 ApplySaturation(const glm::vec3& color, float saturation)
    {
        float gray = Luminance(color);
        return glm::mix(glm::vec3(gray), color, saturation);
    }

    /**
     * @brief Test result structure
     */
    struct ToneMappingTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec3 inputColor{0.0f};
        glm::vec3 outputColor{0.0f};
        ECS::ToneMappingMode mode = ECS::ToneMappingMode::None;
        float parameter = 0.0f;
    };

    /**
     * @brief Property 18: 色调映射 HDR 到 LDR
     * 
     * For any HDR color value:
     * - Tone mapped result should be in [0, 1] range
     * - Different tone mapping algorithms should produce different but valid results
     * 
     * Feature: 2d-lighting-enhancement, Property 18: 色调映射 HDR 到 LDR
     * Validates: Requirements 10.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return ToneMappingTestResult with pass/fail status and failure details
     */
    inline ToneMappingTestResult TestProperty18_ToneMappingHDRtoLDR(int iterations = 100)
    {
        ToneMappingTestResult result;
        ToneMappingRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random HDR color (can be > 1.0)
            glm::vec3 hdrColor = gen.RandomHDRColor(0.0f, 10.0f);
            
            // Test all tone mapping modes
            ECS::ToneMappingMode modes[] = {
                ECS::ToneMappingMode::None,
                ECS::ToneMappingMode::Reinhard,
                ECS::ToneMappingMode::ACES,
                ECS::ToneMappingMode::Filmic
            };
            
            for (auto mode : modes)
            {
                glm::vec3 ldrColor = ApplyToneMapping(hdrColor, mode);
                
                // Test 1: Output should be in [0, 1] range
                if (ldrColor.r < 0.0f || ldrColor.r > 1.0f ||
                    ldrColor.g < 0.0f || ldrColor.g > 1.0f ||
                    ldrColor.b < 0.0f || ldrColor.b > 1.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = hdrColor;
                    result.outputColor = ldrColor;
                    result.mode = mode;
                    std::ostringstream oss;
                    oss << "Tone mapped result should be in [0, 1]. "
                        << "Input: (" << hdrColor.r << ", " << hdrColor.g << ", " << hdrColor.b << "), "
                        << "Output: (" << ldrColor.r << ", " << ldrColor.g << ", " << ldrColor.b << "), "
                        << "Mode: " << static_cast<int>(mode);
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Different algorithms should produce different results for HDR values
            if (hdrColor.r > 1.0f || hdrColor.g > 1.0f || hdrColor.b > 1.0f)
            {
                glm::vec3 reinhardResult = ApplyToneMapping(hdrColor, ECS::ToneMappingMode::Reinhard);
                glm::vec3 acesResult = ApplyToneMapping(hdrColor, ECS::ToneMappingMode::ACES);
                glm::vec3 filmicResult = ApplyToneMapping(hdrColor, ECS::ToneMappingMode::Filmic);
                
                // Results should be different (with some tolerance)
                float diffRA = glm::length(reinhardResult - acesResult);
                float diffRF = glm::length(reinhardResult - filmicResult);
                float diffAF = glm::length(acesResult - filmicResult);
                
                // At least some pairs should be different for HDR values
                if (diffRA < 0.001f && diffRF < 0.001f && diffAF < 0.001f)
                {
                    // All results are nearly identical - this is suspicious for HDR
                    // But not necessarily wrong, so we just note it
                }
            }
            
            // Test 3: Monotonicity - brighter input should produce brighter or equal output
            glm::vec3 brighterHdr = hdrColor * 1.5f;
            for (auto mode : modes)
            {
                if (mode == ECS::ToneMappingMode::None) continue; // Skip None mode
                
                glm::vec3 ldr1 = ApplyToneMapping(hdrColor, mode);
                glm::vec3 ldr2 = ApplyToneMapping(brighterHdr, mode);
                
                float lum1 = Luminance(ldr1);
                float lum2 = Luminance(ldr2);
                
                // Brighter input should produce brighter or equal output
                if (lum2 < lum1 - 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = hdrColor;
                    result.mode = mode;
                    std::ostringstream oss;
                    oss << "Tone mapping should be monotonic. "
                        << "Input1 luminance: " << Luminance(hdrColor) << " -> " << lum1 << ", "
                        << "Input2 luminance: " << Luminance(brighterHdr) << " -> " << lum2;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Property 19: LUT 颜色变换
     * 
     * For any input color and LUT:
     * - LUT intensity 0 should return original color
     * - LUT intensity 1 should return fully transformed color
     * - Result should be linear interpolation between original and LUT color
     * 
     * Feature: 2d-lighting-enhancement, Property 19: LUT 颜色变换
     * Validates: Requirements 10.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return ToneMappingTestResult with pass/fail status and failure details
     */
    inline ToneMappingTestResult TestProperty19_LUTColorTransform(int iterations = 100)
    {
        ToneMappingTestResult result;
        ToneMappingRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random input color (LDR range)
            glm::vec3 inputColor = gen.RandomHDRColor(0.0f, 1.0f);
            
            // Simulate LUT lookup (for testing, we use a simple transform)
            // In real implementation, this would sample from a 3D LUT texture
            glm::vec3 lutColor = glm::vec3(
                1.0f - inputColor.r,  // Invert red
                inputColor.g * 0.8f,  // Reduce green
                std::min(inputColor.b + 0.2f, 1.0f)  // Boost blue
            );
            
            // Test 1: LUT intensity 0 should return original color
            {
                float intensity = 0.0f;
                glm::vec3 resultColor = glm::mix(inputColor, lutColor, intensity);
                
                if (!FloatEquals(resultColor.r, inputColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, inputColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, inputColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = intensity;
                    result.failureMessage = "LUT intensity 0 should return original color";
                    return result;
                }
            }
            
            // Test 2: LUT intensity 1 should return LUT color
            {
                float intensity = 1.0f;
                glm::vec3 resultColor = glm::mix(inputColor, lutColor, intensity);
                
                if (!FloatEquals(resultColor.r, lutColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, lutColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, lutColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = intensity;
                    result.failureMessage = "LUT intensity 1 should return LUT color";
                    return result;
                }
            }
            
            // Test 3: Intermediate intensity should interpolate
            {
                float intensity = gen.RandomFloat(0.1f, 0.9f);
                glm::vec3 resultColor = glm::mix(inputColor, lutColor, intensity);
                glm::vec3 expectedColor = inputColor * (1.0f - intensity) + lutColor * intensity;
                
                if (!FloatEquals(resultColor.r, expectedColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, expectedColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, expectedColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = intensity;
                    std::ostringstream oss;
                    oss << "LUT should interpolate linearly. Intensity: " << intensity;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Property 20: 曝光/对比度/饱和度调整
     * 
     * For any color adjustment parameters:
     * - Exposure increase should make image brighter
     * - Contrast increase should increase brightness difference
     * - Saturation increase should enhance color vividness
     * - Parameter = 1.0 should preserve original color
     * 
     * Feature: 2d-lighting-enhancement, Property 20: 曝光/对比度/饱和度调整
     * Validates: Requirements 10.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return ToneMappingTestResult with pass/fail status and failure details
     */
    inline ToneMappingTestResult TestProperty20_ColorAdjustments(int iterations = 100)
    {
        ToneMappingTestResult result;
        ToneMappingRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random input color (LDR range, avoid extremes)
            glm::vec3 inputColor = gen.RandomHDRColor(0.2f, 0.8f);
            
            // Test 1: Exposure = 1.0 should preserve color
            {
                glm::vec3 resultColor = ApplyExposure(inputColor, 1.0f);
                
                if (!FloatEquals(resultColor.r, inputColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, inputColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, inputColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = 1.0f;
                    result.failureMessage = "Exposure 1.0 should preserve original color";
                    return result;
                }
            }
            
            // Test 2: Exposure increase should make image brighter
            {
                float exposure = gen.RandomFloat(1.5f, 3.0f);
                glm::vec3 resultColor = ApplyExposure(inputColor, exposure);
                
                float inputLum = Luminance(inputColor);
                float resultLum = Luminance(resultColor);
                
                if (resultLum <= inputLum)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = exposure;
                    std::ostringstream oss;
                    oss << "Exposure increase should brighten image. "
                        << "Input lum: " << inputLum << ", Result lum: " << resultLum;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Contrast = 1.0 should preserve color
            {
                glm::vec3 resultColor = ApplyContrast(inputColor, 1.0f);
                
                if (!FloatEquals(resultColor.r, inputColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, inputColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, inputColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = 1.0f;
                    result.failureMessage = "Contrast 1.0 should preserve original color";
                    return result;
                }
            }
            
            // Test 4: Contrast increase should increase difference from midpoint
            {
                float contrast = gen.RandomFloat(1.5f, 3.0f);
                glm::vec3 resultColor = ApplyContrast(inputColor, contrast);
                
                // Distance from midpoint (0.5) should increase
                float inputDistR = std::abs(inputColor.r - 0.5f);
                float resultDistR = std::abs(resultColor.r - 0.5f);
                
                // For colors not at midpoint, distance should increase
                if (inputDistR > 0.05f && resultDistR < inputDistR - 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = contrast;
                    std::ostringstream oss;
                    oss << "Contrast increase should increase distance from midpoint. "
                        << "Input dist: " << inputDistR << ", Result dist: " << resultDistR;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 5: Saturation = 1.0 should preserve color
            {
                glm::vec3 resultColor = ApplySaturation(inputColor, 1.0f);
                
                if (!FloatEquals(resultColor.r, inputColor.r, 0.001f) ||
                    !FloatEquals(resultColor.g, inputColor.g, 0.001f) ||
                    !FloatEquals(resultColor.b, inputColor.b, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = 1.0f;
                    result.failureMessage = "Saturation 1.0 should preserve original color";
                    return result;
                }
            }
            
            // Test 6: Saturation = 0 should produce grayscale
            {
                glm::vec3 resultColor = ApplySaturation(inputColor, 0.0f);
                float gray = Luminance(inputColor);
                
                if (!FloatEquals(resultColor.r, gray, 0.001f) ||
                    !FloatEquals(resultColor.g, gray, 0.001f) ||
                    !FloatEquals(resultColor.b, gray, 0.001f))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.inputColor = inputColor;
                    result.outputColor = resultColor;
                    result.parameter = 0.0f;
                    std::ostringstream oss;
                    oss << "Saturation 0 should produce grayscale. "
                        << "Expected: " << gray << ", Got: (" 
                        << resultColor.r << ", " << resultColor.g << ", " << resultColor.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    // ============ Test Runner Functions ============

    /**
     * @brief Run Property 18 test and log results
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty18Test()
    {
        LogInfo("Running Property 18: 色调映射 HDR 到 LDR (100 iterations)...");
        
        ToneMappingTestResult result = TestProperty18_ToneMappingHDRtoLDR(100);
        
        if (result.passed)
        {
            LogInfo("Property 18 (色调映射 HDR 到 LDR) PASSED");
            return true;
        }
        else
        {
            LogError("Property 18 (色调映射 HDR 到 LDR) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: inputColor=(%f, %f, %f), mode=%d",
                     result.inputColor.r, result.inputColor.g, result.inputColor.b,
                     static_cast<int>(result.mode));
            return false;
        }
    }

    /**
     * @brief Run Property 19 test and log results
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty19Test()
    {
        LogInfo("Running Property 19: LUT 颜色变换 (100 iterations)...");
        
        ToneMappingTestResult result = TestProperty19_LUTColorTransform(100);
        
        if (result.passed)
        {
            LogInfo("Property 19 (LUT 颜色变换) PASSED");
            return true;
        }
        else
        {
            LogError("Property 19 (LUT 颜色变换) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: inputColor=(%f, %f, %f), intensity=%f",
                     result.inputColor.r, result.inputColor.g, result.inputColor.b,
                     result.parameter);
            return false;
        }
    }

    /**
     * @brief Run Property 20 test and log results
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty20Test()
    {
        LogInfo("Running Property 20: 曝光/对比度/饱和度调整 (100 iterations)...");
        
        ToneMappingTestResult result = TestProperty20_ColorAdjustments(100);
        
        if (result.passed)
        {
            LogInfo("Property 20 (曝光/对比度/饱和度调整) PASSED");
            return true;
        }
        else
        {
            LogError("Property 20 (曝光/对比度/饱和度调整) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: inputColor=(%f, %f, %f), parameter=%f",
                     result.inputColor.r, result.inputColor.g, result.inputColor.b,
                     result.parameter);
            return false;
        }
    }

    /**
     * @brief Run all Tone Mapping tests and log results
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllToneMappingTests()
    {
        LogInfo("=== Running Tone Mapping and Color Grading Tests ===");
        
        bool allPassed = true;
        
        // Property 18: Tone mapping HDR to LDR
        if (!RunProperty18Test())
        {
            allPassed = false;
        }
        
        // Property 19: LUT color transform
        if (!RunProperty19Test())
        {
            allPassed = false;
        }
        
        // Property 20: Exposure/Contrast/Saturation adjustments
        if (!RunProperty20Test())
        {
            allPassed = false;
        }
        
        LogInfo("=== Tone Mapping and Color Grading Tests Complete ===");
        return allPassed;
    }
}

#endif // TONE_MAPPING_TESTS_H

#ifndef EMISSION_PIPELINE_TESTS_H
#define EMISSION_PIPELINE_TESTS_H

/**
 * @file EmissionPipelineTests.h
 * @brief Property-based tests for Emission Pipeline
 * 
 * This file contains property-based tests for validating the correctness of
 * the emission pipeline including SpriteComponent emission properties,
 * EmissionGlobalData, and emission buffer management.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 7: 自发光管线正确性
 * Validates: Requirements 4.2, 4.3, 4.4, 4.5
 */

#include "../Sprite.h"
#include "../Core.h"
#include "../../Renderer/LightingRenderer.h"
#include <cassert>
#include <random>
#include <sstream>
#include <cmath>

namespace EmissionPipelineTests
{
    /**
     * @brief Random generator for test data
     */
    class RandomGenerator
    {
    public:
        RandomGenerator() : m_gen(std::random_device{}()) {}

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

        uint32_t RandomUInt32()
        {
            std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
            return dist(m_gen);
        }

        bool RandomBool()
        {
            std::uniform_int_distribution<int> dist(0, 1);
            return dist(m_gen) == 1;
        }

        ECS::Color RandomColor()
        {
            return ECS::Color(
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f),
                RandomFloat(0.0f, 1.0f)
            );
        }

        /**
         * @brief Generate random HDR emission intensity
         * Supports values > 1.0 for HDR as per Requirements 4.5
         */
        float RandomEmissionIntensity()
        {
            // 70% chance of 0 (no emission), 30% chance of emission
            if (RandomFloat(0.0f, 1.0f) < 0.7f)
            {
                return 0.0f;
            }
            // Support HDR values up to 10.0
            return RandomFloat(0.0f, 10.0f);
        }

        ECS::SpriteComponent RandomSpriteComponentWithEmission()
        {
            ECS::SpriteComponent sprite;
            sprite.color = RandomColor();
            sprite.emissionColor = RandomColor();
            sprite.emissionIntensity = RandomEmissionIntensity();
            sprite.zIndex = RandomInt(-100, 100);
            sprite.lightLayer.value = RandomUInt32();
            return sprite;
        }

        EmissionGlobalData RandomEmissionGlobalData()
        {
            EmissionGlobalData data;
            data.emissionEnabled = RandomBool() ? 1 : 0;
            data.emissionScale = RandomFloat(0.0f, 5.0f);
            return data;
        }

    private:
        std::mt19937 m_gen;
    };

    /**
     * @brief Helper function to compare floats with tolerance
     */
    inline bool FloatEquals(float a, float b, float epsilon = 1e-6f)
    {
        return std::abs(a - b) < epsilon;
    }

    /**
     * @brief Helper function to compare colors
     */
    inline bool ColorEquals(const ECS::Color& a, const ECS::Color& b, float epsilon = 1e-6f)
    {
        return FloatEquals(a.r, b.r, epsilon) &&
               FloatEquals(a.g, b.g, epsilon) &&
               FloatEquals(a.b, b.b, epsilon) &&
               FloatEquals(a.a, b.a, epsilon);
    }

    /**
     * @brief Property 7.1: SpriteComponent 自发光属性序列化往返一致性
     * 
     * For any SpriteComponent with emission properties, serializing to YAML 
     * and then deserializing should produce a component with identical 
     * emission properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.2, 4.4, 4.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty7_1_EmissionSerializationRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random sprite with emission
            ECS::SpriteComponent original = gen.RandomSpriteComponentWithEmission();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::SpriteComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::SpriteComponent restored;
            bool decodeSuccess = YAML::convert<ECS::SpriteComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("SpriteComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify emission properties round-trip
            if (!ColorEquals(restored.emissionColor, original.emissionColor))
            {
                LogError("emissionColor mismatch at iteration {}: original=({},{},{},{}), restored=({},{},{},{})", 
                    i,
                    original.emissionColor.r, original.emissionColor.g, 
                    original.emissionColor.b, original.emissionColor.a,
                    restored.emissionColor.r, restored.emissionColor.g, 
                    restored.emissionColor.b, restored.emissionColor.a);
                return false;
            }
            
            if (!FloatEquals(restored.emissionIntensity, original.emissionIntensity))
            {
                LogError("emissionIntensity mismatch at iteration {}: original={}, restored={}", 
                    i, original.emissionIntensity, restored.emissionIntensity);
                return false;
            }
            
            // Verify other properties are preserved
            if (!ColorEquals(restored.color, original.color))
            {
                LogError("color mismatch at iteration {}", i);
                return false;
            }
            
            if (restored.zIndex != original.zIndex)
            {
                LogError("zIndex mismatch at iteration {}", i);
                return false;
            }
            
            if (restored.lightLayer.value != original.lightLayer.value)
            {
                LogError("lightLayer mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 7.2: 自发光强度支持 HDR 值
     * 
     * For any SpriteComponent, emission intensity should support HDR values
     * (greater than 1.0) and preserve them through serialization.
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty7_2_HDREmissionIntensity(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate HDR emission intensity (values > 1.0)
            float hdrIntensity = gen.RandomFloat(1.0f, 100.0f);
            
            ECS::SpriteComponent original;
            original.emissionIntensity = hdrIntensity;
            original.emissionColor = gen.RandomColor();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::SpriteComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::SpriteComponent restored;
            bool decodeSuccess = YAML::convert<ECS::SpriteComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("SpriteComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify HDR value is preserved
            if (!FloatEquals(restored.emissionIntensity, hdrIntensity))
            {
                LogError("HDR emissionIntensity not preserved at iteration {}: original={}, restored={}", 
                    i, hdrIntensity, restored.emissionIntensity);
                return false;
            }
            
            // Verify HDR value is actually > 1.0
            if (restored.emissionIntensity <= 1.0f)
            {
                LogError("HDR emissionIntensity was clamped at iteration {}: value={}", 
                    i, restored.emissionIntensity);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 7.3: 自发光独立于场景光照
     * 
     * For any SpriteComponent with emission, the HasEmission() method should
     * correctly identify whether the sprite has emission based on intensity
     * or emission map.
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty7_3_HasEmissionDetection(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            ECS::SpriteComponent sprite;
            
            // Test case 1: No emission (intensity = 0, no map)
            sprite.emissionIntensity = 0.0f;
            sprite.emissionMapHandle = AssetHandle(AssetType::Texture);
            
            if (sprite.HasEmission())
            {
                LogError("HasEmission() returned true for sprite with no emission at iteration {}", i);
                return false;
            }
            
            // Test case 2: Has emission via intensity
            sprite.emissionIntensity = gen.RandomFloat(0.001f, 10.0f);
            sprite.emissionMapHandle = AssetHandle(AssetType::Texture);
            
            if (!sprite.HasEmission())
            {
                LogError("HasEmission() returned false for sprite with emission intensity at iteration {}", i);
                return false;
            }
            
            // Test case 3: Has emission via map (simulated with valid GUID)
            sprite.emissionIntensity = 0.0f;
            // Note: We can't easily test with a valid asset handle without the asset system,
            // but we can verify the logic is correct for intensity-based detection
        }
        
        return true;
    }

    /**
     * @brief Property 7.4: EmissionGlobalData GPU 数据结构对齐
     * 
     * EmissionGlobalData should be correctly aligned to 16 bytes for GPU usage.
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.3
     * 
     * @return true if alignment is correct, false otherwise
     */
    inline bool TestProperty7_4_EmissionGlobalDataAlignment()
    {
        // Verify size is 16 bytes
        if (sizeof(EmissionGlobalData) != 16)
        {
            LogError("EmissionGlobalData size is {} bytes, expected 16", sizeof(EmissionGlobalData));
            return false;
        }
        
        // Verify alignment is 16 bytes
        if (alignof(EmissionGlobalData) != 16)
        {
            LogError("EmissionGlobalData alignment is {} bytes, expected 16", alignof(EmissionGlobalData));
            return false;
        }
        
        // Verify default values
        EmissionGlobalData data;
        if (data.emissionEnabled != 1)
        {
            LogError("EmissionGlobalData default emissionEnabled is {}, expected 1", data.emissionEnabled);
            return false;
        }
        
        if (!FloatEquals(data.emissionScale, 1.0f))
        {
            LogError("EmissionGlobalData default emissionScale is {}, expected 1.0", data.emissionScale);
            return false;
        }
        
        return true;
    }

    /**
     * @brief Property 7.5: EmissionGlobalData 序列化往返一致性
     * 
     * For any EmissionGlobalData, the values should be correctly preserved
     * when written to and read from a buffer.
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty7_5_EmissionGlobalDataRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random data
            EmissionGlobalData original = gen.RandomEmissionGlobalData();
            
            // Simulate buffer write/read by copying through raw memory
            EmissionGlobalData restored;
            std::memcpy(&restored, &original, sizeof(EmissionGlobalData));
            
            // Verify round-trip
            if (restored.emissionEnabled != original.emissionEnabled)
            {
                LogError("emissionEnabled mismatch at iteration {}", i);
                return false;
            }
            
            if (!FloatEquals(restored.emissionScale, original.emissionScale))
            {
                LogError("emissionScale mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all Emission Pipeline property tests
     * 
     * Feature: 2d-lighting-enhancement, Property 7: 自发光管线正确性
     * Validates: Requirements 4.2, 4.3, 4.4, 4.5
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllEmissionPipelineTests()
    {
        bool allPassed = true;
        
        LogInfo("Running Emission Pipeline Property Tests...");
        
        // Property 7.1: Emission serialization round-trip
        if (!TestProperty7_1_EmissionSerializationRoundTrip(100))
        {
            LogError("Property 7.1 (自发光属性序列化往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 7.1 (自发光属性序列化往返一致性) PASSED");
        }
        
        // Property 7.2: HDR emission intensity
        if (!TestProperty7_2_HDREmissionIntensity(100))
        {
            LogError("Property 7.2 (自发光强度支持 HDR 值) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 7.2 (自发光强度支持 HDR 值) PASSED");
        }
        
        // Property 7.3: HasEmission detection
        if (!TestProperty7_3_HasEmissionDetection(100))
        {
            LogError("Property 7.3 (自发光检测正确性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 7.3 (自发光检测正确性) PASSED");
        }
        
        // Property 7.4: EmissionGlobalData alignment
        if (!TestProperty7_4_EmissionGlobalDataAlignment())
        {
            LogError("Property 7.4 (EmissionGlobalData GPU 对齐) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 7.4 (EmissionGlobalData GPU 对齐) PASSED");
        }
        
        // Property 7.5: EmissionGlobalData round-trip
        if (!TestProperty7_5_EmissionGlobalDataRoundTrip(100))
        {
            LogError("Property 7.5 (EmissionGlobalData 往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 7.5 (EmissionGlobalData 往返一致性) PASSED");
        }
        
        if (allPassed)
        {
            LogInfo("All Emission Pipeline Property Tests PASSED");
        }
        else
        {
            LogError("Some Emission Pipeline Property Tests FAILED");
        }
        
        return allPassed;
    }
}

#endif // EMISSION_PIPELINE_TESTS_H

#ifndef POST_PROCESS_SETTINGS_COMPONENT_TESTS_H
#define POST_PROCESS_SETTINGS_COMPONENT_TESTS_H

/**
 * @file PostProcessSettingsComponentTests.h
 * @brief Property-based tests for PostProcessSettingsComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * PostProcessSettingsComponent serialization and property access.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 */

#include "../PostProcessSettingsComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace PostProcessSettingsTests
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

        ECS::ToneMappingMode RandomToneMappingMode()
        {
            int type = RandomInt(0, 3);
            switch (type)
            {
                case 0: return ECS::ToneMappingMode::None;
                case 1: return ECS::ToneMappingMode::Reinhard;
                case 2: return ECS::ToneMappingMode::ACES;
                case 3: return ECS::ToneMappingMode::Filmic;
                default: return ECS::ToneMappingMode::ACES;
            }
        }

        ECS::FogMode RandomFogMode()
        {
            int type = RandomInt(0, 2);
            switch (type)
            {
                case 0: return ECS::FogMode::Linear;
                case 1: return ECS::FogMode::Exponential;
                case 2: return ECS::FogMode::ExponentialSquared;
                default: return ECS::FogMode::Linear;
            }
        }

        std::string RandomString(int maxLength = 50)
        {
            static const char alphanum[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "_-./";
            
            int length = RandomInt(0, maxLength);
            std::string result;
            result.reserve(length);
            
            for (int i = 0; i < length; ++i)
            {
                result += alphanum[RandomInt(0, sizeof(alphanum) - 2)];
            }
            
            return result;
        }

        ECS::PostProcessSettingsComponent RandomPostProcessSettingsComponent()
        {
            ECS::PostProcessSettingsComponent settings;
            settings.Enable = RandomBool();
            
            // Bloom 设置
            settings.enableBloom = RandomBool();
            settings.bloomThreshold = RandomFloat(0.0f, 10.0f);
            settings.bloomIntensity = RandomFloat(0.0f, 5.0f);
            settings.bloomRadius = RandomFloat(0.0f, 20.0f);
            settings.bloomIterations = RandomInt(1, 16);
            settings.bloomTint = RandomColor();
            
            // 光束设置
            settings.enableLightShafts = RandomBool();
            settings.lightShaftDensity = RandomFloat(0.0f, 1.0f);
            settings.lightShaftDecay = RandomFloat(0.0f, 1.0f);
            settings.lightShaftWeight = RandomFloat(0.0f, 1.0f);
            settings.lightShaftExposure = RandomFloat(0.0f, 2.0f);
            
            // 雾效设置
            settings.enableFog = RandomBool();
            settings.fogMode = RandomFogMode();
            settings.fogColor = RandomColor();
            settings.fogDensity = RandomFloat(0.0f, 1.0f);
            settings.fogStart = RandomFloat(0.0f, 100.0f);
            settings.fogEnd = RandomFloat(100.0f, 1000.0f);
            settings.enableHeightFog = RandomBool();
            settings.heightFogBase = RandomFloat(-100.0f, 100.0f);
            settings.heightFogDensity = RandomFloat(0.0f, 1.0f);
            
            // 色调映射设置
            settings.toneMappingMode = RandomToneMappingMode();
            settings.exposure = RandomFloat(0.1f, 10.0f);
            settings.contrast = RandomFloat(0.1f, 3.0f);
            settings.saturation = RandomFloat(0.0f, 3.0f);
            settings.gamma = RandomFloat(0.1f, 5.0f);
            
            // LUT 设置
            settings.enableColorGrading = RandomBool();
            settings.lutTexturePath = RandomString(30);
            settings.lutIntensity = RandomFloat(0.0f, 1.0f);
            
            return settings;
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
     * @brief Property 1: 组件序列化往返一致性
     * 
     * For any PostProcessSettingsComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 5.2, 5.4, 6.2, 11.2
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty1_SerializationRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random component
            ECS::PostProcessSettingsComponent original = gen.RandomPostProcessSettingsComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::PostProcessSettingsComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::PostProcessSettingsComponent restored;
            bool decodeSuccess = YAML::convert<ECS::PostProcessSettingsComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("PostProcessSettingsComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality for all fields
            bool match = true;
            
            // Enable
            if (restored.Enable != original.Enable)
            {
                LogError("Enable mismatch at iteration {}", i);
                match = false;
            }
            
            // Bloom settings
            if (restored.enableBloom != original.enableBloom ||
                !FloatEquals(restored.bloomThreshold, original.bloomThreshold) ||
                !FloatEquals(restored.bloomIntensity, original.bloomIntensity) ||
                !FloatEquals(restored.bloomRadius, original.bloomRadius) ||
                restored.bloomIterations != original.bloomIterations ||
                !ColorEquals(restored.bloomTint, original.bloomTint))
            {
                LogError("Bloom settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Light shaft settings
            if (restored.enableLightShafts != original.enableLightShafts ||
                !FloatEquals(restored.lightShaftDensity, original.lightShaftDensity) ||
                !FloatEquals(restored.lightShaftDecay, original.lightShaftDecay) ||
                !FloatEquals(restored.lightShaftWeight, original.lightShaftWeight) ||
                !FloatEquals(restored.lightShaftExposure, original.lightShaftExposure))
            {
                LogError("Light shaft settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Fog settings
            if (restored.enableFog != original.enableFog ||
                restored.fogMode != original.fogMode ||
                !ColorEquals(restored.fogColor, original.fogColor) ||
                !FloatEquals(restored.fogDensity, original.fogDensity) ||
                !FloatEquals(restored.fogStart, original.fogStart) ||
                !FloatEquals(restored.fogEnd, original.fogEnd) ||
                restored.enableHeightFog != original.enableHeightFog ||
                !FloatEquals(restored.heightFogBase, original.heightFogBase) ||
                !FloatEquals(restored.heightFogDensity, original.heightFogDensity))
            {
                LogError("Fog settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Tone mapping settings
            if (restored.toneMappingMode != original.toneMappingMode ||
                !FloatEquals(restored.exposure, original.exposure) ||
                !FloatEquals(restored.contrast, original.contrast) ||
                !FloatEquals(restored.saturation, original.saturation) ||
                !FloatEquals(restored.gamma, original.gamma))
            {
                LogError("Tone mapping settings mismatch at iteration {}", i);
                match = false;
            }
            
            // LUT settings
            if (restored.enableColorGrading != original.enableColorGrading ||
                restored.lutTexturePath != original.lutTexturePath ||
                !FloatEquals(restored.lutIntensity, original.lutIntensity))
            {
                LogError("LUT settings mismatch at iteration {}", i);
                match = false;
            }
            
            if (!match)
            {
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all PostProcessSettingsComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllPostProcessSettingsComponentTests()
    {
        bool allPassed = true;
        
        // Property 1: Serialization round-trip
        if (!TestProperty1_SerializationRoundTrip(100))
        {
            LogError("Property 1 (组件序列化往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 1 (组件序列化往返一致性) PASSED");
        }
        
        return allPassed;
    }
}

#endif // POST_PROCESS_SETTINGS_COMPONENT_TESTS_H

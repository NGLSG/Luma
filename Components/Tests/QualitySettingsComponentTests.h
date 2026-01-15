#ifndef QUALITY_SETTINGS_COMPONENT_TESTS_H
#define QUALITY_SETTINGS_COMPONENT_TESTS_H

/**
 * @file QualitySettingsComponentTests.h
 * @brief Property-based tests for QualitySettingsComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * QualitySettingsComponent serialization and property access.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 */

#include "../QualitySettingsComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace QualitySettingsTests
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

        ECS::QualityLevel RandomQualityLevel()
        {
            int type = RandomInt(0, 4);
            switch (type)
            {
                case 0: return ECS::QualityLevel::Low;
                case 1: return ECS::QualityLevel::Medium;
                case 2: return ECS::QualityLevel::High;
                case 3: return ECS::QualityLevel::Ultra;
                case 4: return ECS::QualityLevel::Custom;
                default: return ECS::QualityLevel::High;
            }
        }

        ECS::ShadowMethod RandomShadowMethod()
        {
            int type = RandomInt(0, 2);
            switch (type)
            {
                case 0: return ECS::ShadowMethod::Basic;
                case 1: return ECS::ShadowMethod::SDF;
                case 2: return ECS::ShadowMethod::ScreenSpace;
                default: return ECS::ShadowMethod::Basic;
            }
        }

        ECS::QualitySettingsComponent RandomQualitySettingsComponent()
        {
            ECS::QualitySettingsComponent settings;
            settings.Enable = RandomBool();
            
            // 质量等级
            settings.level = RandomQualityLevel();
            
            // 光照设置
            settings.maxLightsPerFrame = RandomInt(1, 256);
            settings.maxLightsPerPixel = RandomInt(1, 32);
            settings.enableAreaLights = RandomBool();
            settings.enableIndirectLighting = RandomBool();
            
            // 阴影设置
            settings.shadowMethod = RandomShadowMethod();
            settings.shadowMapResolution = RandomInt(256, 4096);
            settings.enableShadowCache = RandomBool();
            
            // 后处理设置
            settings.enableBloom = RandomBool();
            settings.enableLightShafts = RandomBool();
            settings.enableFog = RandomBool();
            settings.enableColorGrading = RandomBool();
            settings.renderScale = RandomFloat(0.25f, 2.0f);
            
            // 自动质量调整
            settings.enableAutoQuality = RandomBool();
            settings.targetFrameRate = RandomFloat(30.0f, 144.0f);
            settings.qualityAdjustThreshold = RandomFloat(1.0f, 30.0f);
            
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
     * @brief Property 1: 组件序列化往返一致性
     * 
     * For any QualitySettingsComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 9.4, 9.6
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
            ECS::QualitySettingsComponent original = gen.RandomQualitySettingsComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::QualitySettingsComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::QualitySettingsComponent restored;
            bool decodeSuccess = YAML::convert<ECS::QualitySettingsComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("QualitySettingsComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality for all fields
            bool match = true;
            
            // Enable
            if (restored.Enable != original.Enable)
            {
                LogError("Enable mismatch at iteration {}: expected {}, got {}", 
                         i, original.Enable, restored.Enable);
                match = false;
            }
            
            // Quality level
            if (restored.level != original.level)
            {
                LogError("level mismatch at iteration {}", i);
                match = false;
            }
            
            // Lighting settings
            if (restored.maxLightsPerFrame != original.maxLightsPerFrame ||
                restored.maxLightsPerPixel != original.maxLightsPerPixel ||
                restored.enableAreaLights != original.enableAreaLights ||
                restored.enableIndirectLighting != original.enableIndirectLighting)
            {
                LogError("Lighting settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Shadow settings
            if (restored.shadowMethod != original.shadowMethod ||
                restored.shadowMapResolution != original.shadowMapResolution ||
                restored.enableShadowCache != original.enableShadowCache)
            {
                LogError("Shadow settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Post-process settings
            if (restored.enableBloom != original.enableBloom ||
                restored.enableLightShafts != original.enableLightShafts ||
                restored.enableFog != original.enableFog ||
                restored.enableColorGrading != original.enableColorGrading ||
                !FloatEquals(restored.renderScale, original.renderScale))
            {
                LogError("Post-process settings mismatch at iteration {}", i);
                match = false;
            }
            
            // Auto quality settings
            if (restored.enableAutoQuality != original.enableAutoQuality ||
                !FloatEquals(restored.targetFrameRate, original.targetFrameRate) ||
                !FloatEquals(restored.qualityAdjustThreshold, original.qualityAdjustThreshold))
            {
                LogError("Auto quality settings mismatch at iteration {}", i);
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
     * @brief Test quality preset consistency
     * 
     * For any quality level preset, applying the preset and then serializing/deserializing
     * should produce consistent results.
     * 
     * Feature: 2d-lighting-enhancement
     * Validates: Requirements 9.2
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool TestPresetConsistency()
    {
        ECS::QualityLevel levels[] = {
            ECS::QualityLevel::Low,
            ECS::QualityLevel::Medium,
            ECS::QualityLevel::High,
            ECS::QualityLevel::Ultra,
            ECS::QualityLevel::Custom
        };
        
        for (auto level : levels)
        {
            // Get preset
            ECS::QualitySettingsComponent preset = ECS::QualitySettingsComponent::GetPreset(level);
            
            // Verify level is set correctly
            if (preset.level != level)
            {
                LogError("Preset level mismatch for level {}", static_cast<int>(level));
                return false;
            }
            
            // Serialize and deserialize
            YAML::Node node = YAML::convert<ECS::QualitySettingsComponent>::encode(preset);
            ECS::QualitySettingsComponent restored;
            bool decodeSuccess = YAML::convert<ECS::QualitySettingsComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("Preset decode failed for level {}", static_cast<int>(level));
                return false;
            }
            
            // Verify equality
            if (preset != restored)
            {
                LogError("Preset round-trip failed for level {}", static_cast<int>(level));
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all QualitySettingsComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllQualitySettingsComponentTests()
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
        
        // Preset consistency test
        if (!TestPresetConsistency())
        {
            LogError("Preset consistency test FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Preset consistency test PASSED");
        }
        
        return allPassed;
    }
}

#endif // QUALITY_SETTINGS_COMPONENT_TESTS_H

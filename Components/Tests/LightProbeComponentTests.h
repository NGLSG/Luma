#ifndef LIGHT_PROBE_COMPONENT_TESTS_H
#define LIGHT_PROBE_COMPONENT_TESTS_H

/**
 * @file LightProbeComponentTests.h
 * @brief Property-based tests for LightProbeComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * LightProbeComponent and LightProbeGridConfig serialization.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 */

#include "../LightProbeComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace LightProbeTests
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

        ECS::Vector2f RandomVector2f(float min = -1000.0f, float max = 1000.0f)
        {
            return ECS::Vector2f(
                RandomFloat(min, max),
                RandomFloat(min, max)
            );
        }

        ECS::Vector2i RandomVector2i(int min = 1, int max = 100)
        {
            return ECS::Vector2i(
                RandomInt(min, max),
                RandomInt(min, max)
            );
        }

        ECS::LightProbeComponent RandomLightProbeComponent()
        {
            ECS::LightProbeComponent probe;
            probe.Enable = RandomBool();
            probe.sampledColor = RandomColor();
            probe.sampledIntensity = RandomFloat(0.0f, 10.0f);
            probe.influenceRadius = RandomFloat(0.001f, 100.0f);
            probe.isBaked = RandomBool();
            probe.layerMask.value = RandomUInt32();
            return probe;
        }

        ECS::LightProbeGridConfig RandomLightProbeGridConfig()
        {
            ECS::LightProbeGridConfig config;
            config.gridOrigin = RandomVector2f(-1000.0f, 1000.0f);
            config.gridSize = RandomVector2f(1.0f, 1000.0f);
            config.probeCount = RandomVector2i(1, 100);
            config.updateFrequency = RandomFloat(0.001f, 10.0f);
            config.autoGenerate = RandomBool();
            return config;
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
     * @brief Helper function to compare Vector2f
     */
    inline bool Vector2fEquals(const ECS::Vector2f& a, const ECS::Vector2f& b, float epsilon = 1e-6f)
    {
        return FloatEquals(a.x, b.x, epsilon) &&
               FloatEquals(a.y, b.y, epsilon);
    }

    /**
     * @brief Property 1: 组件序列化往返一致性 (LightProbeComponent)
     * 
     * For any LightProbeComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 3.6
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty1_LightProbeSerializationRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random component
            ECS::LightProbeComponent original = gen.RandomLightProbeComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::LightProbeComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::LightProbeComponent restored;
            bool decodeSuccess = YAML::convert<ECS::LightProbeComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("LightProbeComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality
            if (restored.Enable != original.Enable ||
                !ColorEquals(restored.sampledColor, original.sampledColor) ||
                !FloatEquals(restored.sampledIntensity, original.sampledIntensity) ||
                !FloatEquals(restored.influenceRadius, original.influenceRadius) ||
                restored.isBaked != original.isBaked ||
                restored.layerMask.value != original.layerMask.value)
            {
                LogError("LightProbeComponent round-trip mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 1: 组件序列化往返一致性 (LightProbeGridConfig)
     * 
     * For any LightProbeGridConfig, serializing to YAML and then deserializing
     * should produce a config with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 3.6
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty1_GridConfigSerializationRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random config
            ECS::LightProbeGridConfig original = gen.RandomLightProbeGridConfig();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::LightProbeGridConfig>::encode(original);
            
            // Deserialize from YAML
            ECS::LightProbeGridConfig restored;
            bool decodeSuccess = YAML::convert<ECS::LightProbeGridConfig>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("LightProbeGridConfig decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality
            if (!Vector2fEquals(restored.gridOrigin, original.gridOrigin) ||
                !Vector2fEquals(restored.gridSize, original.gridSize) ||
                restored.probeCount != original.probeCount ||
                !FloatEquals(restored.updateFrequency, original.updateFrequency) ||
                restored.autoGenerate != original.autoGenerate)
            {
                LogError("LightProbeGridConfig round-trip mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all LightProbeComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllLightProbeComponentTests()
    {
        bool allPassed = true;
        
        // Property 1: LightProbeComponent Serialization round-trip
        if (!TestProperty1_LightProbeSerializationRoundTrip(100))
        {
            LogError("Property 1 (LightProbeComponent 组件序列化往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 1 (LightProbeComponent 组件序列化往返一致性) PASSED");
        }
        
        // Property 1: LightProbeGridConfig Serialization round-trip
        if (!TestProperty1_GridConfigSerializationRoundTrip(100))
        {
            LogError("Property 1 (LightProbeGridConfig 组件序列化往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 1 (LightProbeGridConfig 组件序列化往返一致性) PASSED");
        }
        
        return allPassed;
    }
}

#endif // LIGHT_PROBE_COMPONENT_TESTS_H

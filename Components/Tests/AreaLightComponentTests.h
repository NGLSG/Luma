#ifndef AREA_LIGHT_COMPONENT_TESTS_H
#define AREA_LIGHT_COMPONENT_TESTS_H

/**
 * @file AreaLightComponentTests.h
 * @brief Property-based tests for AreaLightComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * AreaLightComponent serialization and property access.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 */

#include "../AreaLightComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace AreaLightTests
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

        ECS::AttenuationType RandomAttenuationType()
        {
            int type = RandomInt(0, 2);
            switch (type)
            {
                case 0: return ECS::AttenuationType::Linear;
                case 1: return ECS::AttenuationType::Quadratic;
                case 2: return ECS::AttenuationType::InverseSquare;
                default: return ECS::AttenuationType::Quadratic;
            }
        }

        ECS::AreaLightShape RandomAreaLightShape()
        {
            int type = RandomInt(0, 1);
            switch (type)
            {
                case 0: return ECS::AreaLightShape::Rectangle;
                case 1: return ECS::AreaLightShape::Circle;
                default: return ECS::AreaLightShape::Rectangle;
            }
        }

        ECS::AreaLightComponent RandomAreaLightComponent()
        {
            ECS::AreaLightComponent light;
            light.Enable = RandomBool();
            light.color = RandomColor();
            light.intensity = RandomFloat(0.0f, 100.0f);
            light.shape = RandomAreaLightShape();
            light.width = RandomFloat(0.001f, 100.0f);
            light.height = RandomFloat(0.001f, 100.0f);
            light.radius = RandomFloat(0.001f, 1000.0f);
            light.attenuation = RandomAttenuationType();
            light.layerMask.value = RandomUInt32();
            light.priority = RandomInt(-1000, 1000);
            light.castShadows = RandomBool();
            light.shadowSoftness = RandomFloat(0.0f, 10.0f);
            return light;
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
     * For any AreaLightComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 1.3
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
            ECS::AreaLightComponent original = gen.RandomAreaLightComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::AreaLightComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::AreaLightComponent restored;
            bool decodeSuccess = YAML::convert<ECS::AreaLightComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("AreaLightComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality
            if (restored.Enable != original.Enable ||
                !ColorEquals(restored.color, original.color) ||
                !FloatEquals(restored.intensity, original.intensity) ||
                restored.shape != original.shape ||
                !FloatEquals(restored.width, original.width) ||
                !FloatEquals(restored.height, original.height) ||
                !FloatEquals(restored.radius, original.radius) ||
                restored.attenuation != original.attenuation ||
                restored.layerMask.value != original.layerMask.value ||
                restored.priority != original.priority ||
                restored.castShadows != original.castShadows ||
                !FloatEquals(restored.shadowSoftness, original.shadowSoftness))
            {
                LogError("AreaLightComponent round-trip mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all AreaLightComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllAreaLightComponentTests()
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

#endif // AREA_LIGHT_COMPONENT_TESTS_H

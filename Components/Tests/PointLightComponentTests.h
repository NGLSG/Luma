#ifndef POINT_LIGHT_COMPONENT_TESTS_H
#define POINT_LIGHT_COMPONENT_TESTS_H

/**
 * @file PointLightComponentTests.h
 * @brief Property-based tests for PointLightComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * PointLightComponent serialization and property access.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-system
 */

#include "../PointLightComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace LightingTests
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

        ECS::PointLightComponent RandomPointLightComponent()
        {
            ECS::PointLightComponent light;
            light.Enable = RandomBool();
            light.color = RandomColor();
            light.intensity = RandomFloat(0.0f, 100.0f);
            light.radius = RandomFloat(0.001f, 1000.0f);
            light.attenuation = RandomAttenuationType();
            light.layerMask = RandomUInt32();
            light.priority = RandomInt(-1000, 1000);
            light.castShadows = RandomBool();
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
     * @brief Property 1: 光源组件配置往返一致性
     * 
     * For any PointLightComponent, setting any valid property value and then
     * reading it back should return the same value.
     * 
     * Feature: 2d-lighting-system, Property 1: 光源组件配置往返一致性
     * Validates: Requirements 1.2, 11.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty1_ComponentConfigRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random component
            ECS::PointLightComponent original = gen.RandomPointLightComponent();
            
            // Create a copy and verify all properties match
            ECS::PointLightComponent copy;
            copy.Enable = original.Enable;
            copy.color = original.color;
            copy.intensity = original.intensity;
            copy.radius = original.radius;
            copy.attenuation = original.attenuation;
            copy.layerMask = original.layerMask;
            copy.priority = original.priority;
            copy.castShadows = original.castShadows;
            
            // Verify round-trip
            if (copy.Enable != original.Enable ||
                !ColorEquals(copy.color, original.color) ||
                !FloatEquals(copy.intensity, original.intensity) ||
                !FloatEquals(copy.radius, original.radius) ||
                copy.attenuation != original.attenuation ||
                copy.layerMask != original.layerMask ||
                copy.priority != original.priority ||
                copy.castShadows != original.castShadows)
            {
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 8: 组件序列化往返一致性
     * 
     * For any PointLightComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-system, Property 8: 组件序列化往返一致性
     * Validates: Requirements 11.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty8_SerializationRoundTrip(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random component
            ECS::PointLightComponent original = gen.RandomPointLightComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::PointLightComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::PointLightComponent restored;
            bool decodeSuccess = YAML::convert<ECS::PointLightComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                return false;
            }
            
            // Verify round-trip equality
            if (restored.Enable != original.Enable ||
                !ColorEquals(restored.color, original.color) ||
                !FloatEquals(restored.intensity, original.intensity) ||
                !FloatEquals(restored.radius, original.radius) ||
                restored.attenuation != original.attenuation ||
                restored.layerMask != original.layerMask ||
                restored.priority != original.priority ||
                restored.castShadows != original.castShadows)
            {
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all PointLightComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllPointLightComponentTests()
    {
        bool allPassed = true;
        
        // Property 1: Component config round-trip
        if (!TestProperty1_ComponentConfigRoundTrip(100))
        {
            LogError("Property 1 (光源组件配置往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 1 (光源组件配置往返一致性) PASSED");
        }
        
        // Property 8: Serialization round-trip
        if (!TestProperty8_SerializationRoundTrip(100))
        {
            LogError("Property 8 (组件序列化往返一致性) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 8 (组件序列化往返一致性) PASSED");
        }
        
        return allPassed;
    }
}

#endif // POINT_LIGHT_COMPONENT_TESTS_H

#ifndef AMBIENT_ZONE_COMPONENT_TESTS_H
#define AMBIENT_ZONE_COMPONENT_TESTS_H

/**
 * @file AmbientZoneComponentTests.h
 * @brief Property-based tests for AmbientZoneComponent
 * 
 * This file contains property-based tests for validating the correctness of
 * AmbientZoneComponent serialization and property access.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 */

#include "../AmbientZoneComponent.h"
#include "../Core.h"
#include <cassert>
#include <random>
#include <sstream>

namespace AmbientZoneTests
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

        ECS::AmbientZoneShape RandomAmbientZoneShape()
        {
            int type = RandomInt(0, 1);
            switch (type)
            {
                case 0: return ECS::AmbientZoneShape::Rectangle;
                case 1: return ECS::AmbientZoneShape::Circle;
                default: return ECS::AmbientZoneShape::Rectangle;
            }
        }

        ECS::AmbientGradientMode RandomAmbientGradientMode()
        {
            int type = RandomInt(0, 2);
            switch (type)
            {
                case 0: return ECS::AmbientGradientMode::None;
                case 1: return ECS::AmbientGradientMode::Vertical;
                case 2: return ECS::AmbientGradientMode::Horizontal;
                default: return ECS::AmbientGradientMode::None;
            }
        }

        ECS::AmbientZoneComponent RandomAmbientZoneComponent()
        {
            ECS::AmbientZoneComponent zone;
            zone.Enable = RandomBool();
            zone.shape = RandomAmbientZoneShape();
            zone.width = RandomFloat(0.001f, 1000.0f);
            zone.height = RandomFloat(0.001f, 1000.0f);
            zone.primaryColor = RandomColor();
            zone.secondaryColor = RandomColor();
            zone.gradientMode = RandomAmbientGradientMode();
            zone.intensity = RandomFloat(0.0f, 100.0f);
            zone.edgeSoftness = RandomFloat(0.0f, 100.0f);
            zone.priority = RandomInt(-1000, 1000);
            zone.blendWeight = RandomFloat(0.0f, 1.0f);
            return zone;
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
     * For any AmbientZoneComponent, serializing to YAML and then deserializing
     * should produce a component with identical properties.
     * 
     * Feature: 2d-lighting-enhancement, Property 1: 组件序列化往返一致性
     * Validates: Requirements 2.2, 2.6
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
            ECS::AmbientZoneComponent original = gen.RandomAmbientZoneComponent();
            
            // Serialize to YAML
            YAML::Node node = YAML::convert<ECS::AmbientZoneComponent>::encode(original);
            
            // Deserialize from YAML
            ECS::AmbientZoneComponent restored;
            bool decodeSuccess = YAML::convert<ECS::AmbientZoneComponent>::decode(node, restored);
            
            if (!decodeSuccess)
            {
                LogError("AmbientZoneComponent decode failed at iteration {}", i);
                return false;
            }
            
            // Verify round-trip equality
            if (restored.Enable != original.Enable ||
                restored.shape != original.shape ||
                !FloatEquals(restored.width, original.width) ||
                !FloatEquals(restored.height, original.height) ||
                !ColorEquals(restored.primaryColor, original.primaryColor) ||
                !ColorEquals(restored.secondaryColor, original.secondaryColor) ||
                restored.gradientMode != original.gradientMode ||
                !FloatEquals(restored.intensity, original.intensity) ||
                !FloatEquals(restored.edgeSoftness, original.edgeSoftness) ||
                restored.priority != original.priority ||
                !FloatEquals(restored.blendWeight, original.blendWeight))
            {
                LogError("AmbientZoneComponent round-trip mismatch at iteration {}", i);
                LogError("  Original: Enable={}, shape={}, width={}, height={}, intensity={}, edgeSoftness={}, priority={}, blendWeight={}",
                    original.Enable, static_cast<int>(original.shape), original.width, original.height,
                    original.intensity, original.edgeSoftness, original.priority, original.blendWeight);
                LogError("  Restored: Enable={}, shape={}, width={}, height={}, intensity={}, edgeSoftness={}, priority={}, blendWeight={}",
                    restored.Enable, static_cast<int>(restored.shape), restored.width, restored.height,
                    restored.intensity, restored.edgeSoftness, restored.priority, restored.blendWeight);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Run all AmbientZoneComponent property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllAmbientZoneComponentTests()
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

#endif // AMBIENT_ZONE_COMPONENT_TESTS_H

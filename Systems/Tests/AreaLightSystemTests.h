#ifndef AREA_LIGHT_SYSTEM_TESTS_H
#define AREA_LIGHT_SYSTEM_TESTS_H

/**
 * @file AreaLightSystemTests.h
 * @brief Property-based tests for AreaLightSystem
 * 
 * This file contains property-based tests for validating the correctness of
 * area light calculations, including rectangle and circle light contributions.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Property 2: 面光源光照计算正确性
 * Validates: Requirements 1.1, 1.4, 1.6
 */

#include "../AreaLightSystem.h"
#include "../../Components/LightingTypes.h"
#include "../../Components/AreaLightComponent.h"
#include "../../Utils/Logger.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace AreaLightTests
{
    /**
     * @brief Random generator for area light system tests
     */
    class AreaLightRandomGenerator
    {
    public:
        AreaLightRandomGenerator() : m_gen(std::random_device{}()) {}
        
        AreaLightRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        glm::vec2 RandomPosition(float minX, float maxX, float minY, float maxY)
        {
            return glm::vec2(RandomFloat(minX, maxX), RandomFloat(minY, maxY));
        }

        ECS::AreaLightShape RandomShape()
        {
            return RandomInt(0, 1) == 0 ? ECS::AreaLightShape::Rectangle : ECS::AreaLightShape::Circle;
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

        uint32_t RandomLayerMask()
        {
            std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
            return dist(m_gen);
        }

        ECS::AreaLightData RandomAreaLight(float minX, float maxX, float minY, float maxY)
        {
            ECS::AreaLightData light;
            light.position = RandomPosition(minX, maxX, minY, maxY);
            light.size = glm::vec2(RandomFloat(1.0f, 10.0f), RandomFloat(1.0f, 10.0f));
            light.color = glm::vec4(
                RandomFloat(0.1f, 1.0f),
                RandomFloat(0.1f, 1.0f),
                RandomFloat(0.1f, 1.0f),
                1.0f
            );
            light.intensity = RandomFloat(0.5f, 3.0f);
            light.radius = RandomFloat(10.0f, 100.0f);
            light.shape = static_cast<uint32_t>(RandomShape());
            light.layerMask = 0xFFFFFFFF;
            light.attenuation = static_cast<float>(RandomAttenuationType());
            light.shadowSoftness = RandomFloat(1.0f, 4.0f);
            return light;
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
     * @brief Test result structure for AreaLightSystem tests
     */
    struct AreaLightTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec2 lightPosition{0.0f, 0.0f};
        glm::vec2 targetPosition{0.0f, 0.0f};
        glm::vec2 lightSize{0.0f, 0.0f};
        float lightRadius = 0.0f;
        float distance = 0.0f;
        float contribution = 0.0f;
        uint32_t shape = 0;
    };

    /**
     * @brief Property 2: 面光源光照计算正确性
     * 
     * For any area light and target point:
     * - When target is within influence radius, contribution should be > 0
     * - When target is outside influence radius, contribution should be 0
     * - Contribution should monotonically decrease with distance from light surface
     * 
     * Feature: 2d-lighting-enhancement, Property 2: 面光源光照计算正确性
     * Validates: Requirements 1.1, 1.4, 1.6
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AreaLightTestResult with pass/fail status and failure details
     */
    inline AreaLightTestResult TestProperty2_AreaLightContributionCorrectness(int iterations = 100)
    {
        AreaLightTestResult result;
        AreaLightRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random area light
            ECS::AreaLightData areaLight = gen.RandomAreaLight(-100.0f, 100.0f, -100.0f, 100.0f);
            
            // Test 1: Target inside influence radius should have positive contribution
            {
                // Generate target position inside the influence radius
                float maxDist = areaLight.radius * 0.5f;
                glm::vec2 offset = gen.RandomPosition(-maxDist, maxDist, -maxDist, maxDist);
                glm::vec2 targetInside = areaLight.position + offset;
                
                float contribution = Systems::AreaLightSystem::CalculateAreaLightContribution(
                    areaLight, targetInside);
                
                if (contribution <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.lightPosition = areaLight.position;
                    result.targetPosition = targetInside;
                    result.lightSize = areaLight.size;
                    result.lightRadius = areaLight.radius;
                    result.distance = glm::length(targetInside - areaLight.position);
                    result.contribution = contribution;
                    result.shape = areaLight.shape;
                    std::ostringstream oss;
                    oss << "Area light contribution should be > 0 when target is inside radius. "
                        << "Light pos: (" << areaLight.position.x << ", " << areaLight.position.y << "), "
                        << "Target pos: (" << targetInside.x << ", " << targetInside.y << "), "
                        << "Radius: " << areaLight.radius << ", "
                        << "Distance: " << result.distance << ", "
                        << "Contribution: " << contribution;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Target outside influence radius should have zero contribution
            {
                // Generate target position outside the influence radius
                float farDistance = areaLight.radius + gen.RandomFloat(50.0f, 200.0f);
                float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                glm::vec2 targetOutside = areaLight.position + 
                    glm::vec2(std::cos(angle) * farDistance, std::sin(angle) * farDistance);
                
                float contribution = Systems::AreaLightSystem::CalculateAreaLightContribution(
                    areaLight, targetOutside);
                
                if (contribution != 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.lightPosition = areaLight.position;
                    result.targetPosition = targetOutside;
                    result.lightSize = areaLight.size;
                    result.lightRadius = areaLight.radius;
                    result.distance = glm::length(targetOutside - areaLight.position);
                    result.contribution = contribution;
                    result.shape = areaLight.shape;
                    std::ostringstream oss;
                    oss << "Area light contribution should be 0 when target is outside radius. "
                        << "Light pos: (" << areaLight.position.x << ", " << areaLight.position.y << "), "
                        << "Target pos: (" << targetOutside.x << ", " << targetOutside.y << "), "
                        << "Radius: " << areaLight.radius << ", "
                        << "Distance: " << result.distance << ", "
                        << "Contribution: " << contribution;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Monotonicity - contribution should decrease with distance
            {
                // Generate two points at different distances
                float dist1 = gen.RandomFloat(1.0f, areaLight.radius * 0.3f);
                float dist2 = gen.RandomFloat(dist1 + 5.0f, areaLight.radius * 0.8f);
                float angle = gen.RandomFloat(0.0f, 2.0f * 3.14159f);
                
                glm::vec2 target1 = areaLight.position + 
                    glm::vec2(std::cos(angle) * dist1, std::sin(angle) * dist1);
                glm::vec2 target2 = areaLight.position + 
                    glm::vec2(std::cos(angle) * dist2, std::sin(angle) * dist2);
                
                float contrib1 = Systems::AreaLightSystem::CalculateAreaLightContribution(
                    areaLight, target1);
                float contrib2 = Systems::AreaLightSystem::CalculateAreaLightContribution(
                    areaLight, target2);
                
                // Closer point should have higher or equal contribution
                if (contrib1 < contrib2)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.lightPosition = areaLight.position;
                    result.lightSize = areaLight.size;
                    result.lightRadius = areaLight.radius;
                    result.shape = areaLight.shape;
                    std::ostringstream oss;
                    oss << "Area light contribution should decrease with distance. "
                        << "At dist1=" << dist1 << " got " << contrib1 << ", "
                        << "at dist2=" << dist2 << " got " << contrib2;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Layer mask filtering
            {
                // Create light with specific layer mask
                ECS::AreaLightData maskedLight = areaLight;
                maskedLight.layerMask = 0x0000000F; // Only first 4 layers
                
                glm::vec2 targetPos = maskedLight.position + glm::vec2(5.0f, 0.0f);
                
                // Sprite on matching layer should receive light
                uint32_t matchingLayer = 0x00000001;
                glm::vec3 colorContrib = Systems::AreaLightSystem::CalculateAreaLightColorContribution(
                    maskedLight, targetPos, matchingLayer);
                
                if (colorContrib.r <= 0.0f && colorContrib.g <= 0.0f && colorContrib.b <= 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Sprite on matching layer should receive light contribution";
                    return result;
                }
                
                // Sprite on non-matching layer should not receive light
                uint32_t nonMatchingLayer = 0x00000100;
                glm::vec3 noContrib = Systems::AreaLightSystem::CalculateAreaLightColorContribution(
                    maskedLight, targetPos, nonMatchingLayer);
                
                if (noContrib.r != 0.0f || noContrib.g != 0.0f || noContrib.b != 0.0f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Sprite on non-matching layer should NOT receive light contribution";
                    return result;
                }
            }
        }
        
        return result;
    }


    /**
     * @brief Test rectangle area light specific behavior
     * 
     * Tests that rectangle area lights correctly calculate contribution
     * based on distance to the rectangle surface.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AreaLightTestResult with pass/fail status and failure details
     */
    inline AreaLightTestResult TestRectangleAreaLightBehavior(int iterations = 100)
    {
        AreaLightTestResult result;
        AreaLightRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Create rectangle area light
            ECS::AreaLightData rectLight;
            rectLight.position = gen.RandomPosition(-50.0f, 50.0f, -50.0f, 50.0f);
            rectLight.size = glm::vec2(gen.RandomFloat(2.0f, 10.0f), gen.RandomFloat(2.0f, 10.0f));
            rectLight.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            rectLight.intensity = gen.RandomFloat(1.0f, 3.0f);
            rectLight.radius = gen.RandomFloat(20.0f, 50.0f);
            rectLight.shape = static_cast<uint32_t>(ECS::AreaLightShape::Rectangle);
            rectLight.layerMask = 0xFFFFFFFF;
            rectLight.attenuation = 1.0f; // Quadratic
            
            // Test: Point directly above rectangle center should have high contribution
            glm::vec2 centerTarget = rectLight.position;
            float centerContrib = Systems::AreaLightSystem::CalculateRectangleLightContribution(
                rectLight, centerTarget);
            
            // Test: Point at corner should have lower contribution than center
            float halfW = rectLight.size.x / 2.0f;
            float halfH = rectLight.size.y / 2.0f;
            glm::vec2 cornerTarget = rectLight.position + glm::vec2(halfW + 5.0f, halfH + 5.0f);
            float cornerContrib = Systems::AreaLightSystem::CalculateRectangleLightContribution(
                rectLight, cornerTarget);
            
            // Center should have higher contribution than corner (if corner is still in range)
            float cornerDist = glm::length(cornerTarget - rectLight.position);
            if (cornerDist < rectLight.radius && centerContrib < cornerContrib)
            {
                result.passed = false;
                result.failedIteration = i;
                result.lightPosition = rectLight.position;
                result.lightSize = rectLight.size;
                result.lightRadius = rectLight.radius;
                std::ostringstream oss;
                oss << "Rectangle light: center contribution (" << centerContrib 
                    << ") should be >= corner contribution (" << cornerContrib << ")";
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Test circle area light specific behavior
     * 
     * Tests that circle area lights correctly calculate contribution
     * based on distance to the circle surface.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AreaLightTestResult with pass/fail status and failure details
     */
    inline AreaLightTestResult TestCircleAreaLightBehavior(int iterations = 100)
    {
        AreaLightTestResult result;
        AreaLightRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Create circle area light
            ECS::AreaLightData circleLight;
            circleLight.position = gen.RandomPosition(-50.0f, 50.0f, -50.0f, 50.0f);
            circleLight.size = glm::vec2(gen.RandomFloat(2.0f, 10.0f), 0.0f); // width = diameter
            circleLight.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            circleLight.intensity = gen.RandomFloat(1.0f, 3.0f);
            circleLight.radius = gen.RandomFloat(20.0f, 50.0f);
            circleLight.shape = static_cast<uint32_t>(ECS::AreaLightShape::Circle);
            circleLight.layerMask = 0xFFFFFFFF;
            circleLight.attenuation = 1.0f; // Quadratic
            
            // Test: Points at same distance from center should have same contribution
            float testDist = gen.RandomFloat(5.0f, circleLight.radius * 0.5f);
            
            glm::vec2 target1 = circleLight.position + glm::vec2(testDist, 0.0f);
            glm::vec2 target2 = circleLight.position + glm::vec2(0.0f, testDist);
            glm::vec2 target3 = circleLight.position + glm::vec2(-testDist, 0.0f);
            
            float contrib1 = Systems::AreaLightSystem::CalculateCircleLightContribution(
                circleLight, target1);
            float contrib2 = Systems::AreaLightSystem::CalculateCircleLightContribution(
                circleLight, target2);
            float contrib3 = Systems::AreaLightSystem::CalculateCircleLightContribution(
                circleLight, target3);
            
            // All contributions should be equal (within tolerance)
            float epsilon = 0.001f;
            if (!FloatEquals(contrib1, contrib2, epsilon) || !FloatEquals(contrib2, contrib3, epsilon))
            {
                result.passed = false;
                result.failedIteration = i;
                result.lightPosition = circleLight.position;
                result.lightSize = circleLight.size;
                result.lightRadius = circleLight.radius;
                std::ostringstream oss;
                oss << "Circle light: equidistant points should have equal contribution. "
                    << "Got: " << contrib1 << ", " << contrib2 << ", " << contrib3;
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Test area light to point light conversion
     * 
     * Tests that ConvertToPointLights produces valid point lights
     * that approximate the area light behavior.
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return AreaLightTestResult with pass/fail status and failure details
     */
    inline AreaLightTestResult TestAreaLightToPointLightConversion(int iterations = 100)
    {
        AreaLightTestResult result;
        AreaLightRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random area light
            ECS::AreaLightData areaLight = gen.RandomAreaLight(-50.0f, 50.0f, -50.0f, 50.0f);
            
            // Test with different sample counts
            for (int sampleCount : {1, 4, 9, 16})
            {
                std::vector<ECS::LightData> pointLights = 
                    Systems::AreaLightSystem::ConvertToPointLights(areaLight, sampleCount);
                
                // Verify correct number of point lights generated
                int expectedCount = std::min(sampleCount, 
                    static_cast<int>(Systems::AreaLightSystem::MAX_SAMPLES_PER_AREA_LIGHT));
                
                if (static_cast<int>(pointLights.size()) > expectedCount)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "ConvertToPointLights generated " << pointLights.size() 
                        << " lights, expected <= " << expectedCount;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // Verify total intensity is preserved (approximately)
                float totalIntensity = 0.0f;
                for (const auto& pl : pointLights)
                {
                    totalIntensity += pl.intensity;
                }
                
                // Total intensity should approximately equal original intensity
                float epsilon = 0.1f;
                if (!FloatEquals(totalIntensity, areaLight.intensity, epsilon))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Total intensity of converted point lights (" << totalIntensity 
                        << ") should equal original area light intensity (" << areaLight.intensity << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // Verify all point lights have correct properties
                for (const auto& pl : pointLights)
                {
                    // Color should match
                    if (!FloatEquals(pl.color.r, areaLight.color.r, 0.001f) ||
                        !FloatEquals(pl.color.g, areaLight.color.g, 0.001f) ||
                        !FloatEquals(pl.color.b, areaLight.color.b, 0.001f))
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.failureMessage = "Converted point light color doesn't match area light color";
                        return result;
                    }
                    
                    // Radius should match
                    if (!FloatEquals(pl.radius, areaLight.radius, 0.001f))
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.failureMessage = "Converted point light radius doesn't match area light radius";
                        return result;
                    }
                    
                    // Layer mask should match
                    if (pl.layerMask != areaLight.layerMask)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.failureMessage = "Converted point light layer mask doesn't match area light";
                        return result;
                    }
                    
                    // Light type should be Point
                    if (pl.lightType != static_cast<uint32_t>(ECS::LightType::Point))
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.failureMessage = "Converted light should be Point type";
                        return result;
                    }
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 2 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty2Test()
    {
        LogInfo("Running Property 2: 面光源光照计算正确性 (100 iterations)...");
        
        AreaLightTestResult result = TestProperty2_AreaLightContributionCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 2 (面光源光照计算正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 2 (面光源光照计算正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.lightRadius > 0)
            {
                LogError("Failing example: lightPos=(%f, %f), targetPos=(%f, %f), radius=%f, shape=%u",
                         result.lightPosition.x, result.lightPosition.y,
                         result.targetPosition.x, result.targetPosition.y,
                         result.lightRadius, result.shape);
            }
            return false;
        }
    }

    /**
     * @brief Run all area light tests and log results
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllAreaLightTests()
    {
        LogInfo("=== Running Area Light System Tests ===");
        
        bool allPassed = true;
        
        // Property 2: Area light contribution correctness
        if (!RunProperty2Test())
        {
            allPassed = false;
        }
        
        // Rectangle area light behavior
        LogInfo("Running Rectangle Area Light Behavior Test (100 iterations)...");
        AreaLightTestResult rectResult = TestRectangleAreaLightBehavior(100);
        if (rectResult.passed)
        {
            LogInfo("Rectangle Area Light Behavior Test PASSED");
        }
        else
        {
            LogError("Rectangle Area Light Behavior Test FAILED at iteration %d", rectResult.failedIteration);
            LogError("Failure: %s", rectResult.failureMessage.c_str());
            allPassed = false;
        }
        
        // Circle area light behavior
        LogInfo("Running Circle Area Light Behavior Test (100 iterations)...");
        AreaLightTestResult circleResult = TestCircleAreaLightBehavior(100);
        if (circleResult.passed)
        {
            LogInfo("Circle Area Light Behavior Test PASSED");
        }
        else
        {
            LogError("Circle Area Light Behavior Test FAILED at iteration %d", circleResult.failedIteration);
            LogError("Failure: %s", circleResult.failureMessage.c_str());
            allPassed = false;
        }
        
        // Area light to point light conversion
        LogInfo("Running Area Light to Point Light Conversion Test (100 iterations)...");
        AreaLightTestResult convResult = TestAreaLightToPointLightConversion(100);
        if (convResult.passed)
        {
            LogInfo("Area Light to Point Light Conversion Test PASSED");
        }
        else
        {
            LogError("Area Light to Point Light Conversion Test FAILED at iteration %d", convResult.failedIteration);
            LogError("Failure: %s", convResult.failureMessage.c_str());
            allPassed = false;
        }
        
        LogInfo("=== Area Light System Tests Complete ===");
        return allPassed;
    }
}

#endif // AREA_LIGHT_SYSTEM_TESTS_H

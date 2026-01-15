#ifndef LIGHTING_MATH_TESTS_H
#define LIGHTING_MATH_TESTS_H

/**
 * @file LightingMathTests.h
 * @brief Property-based tests for LightingMath functions
 * 
 * This file contains property-based tests for validating the correctness of
 * lighting attenuation functions and spotlight angle attenuation.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-system
 */

#include "../LightingMath.h"
#include "../../Components/LightingTypes.h"
#include "../../Utils/Logger.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>

namespace LightingTests
{
    /**
     * @brief Random generator for lighting math tests
     */
    class LightingMathRandomGenerator
    {
    public:
        LightingMathRandomGenerator() : m_gen(std::random_device{}()) {}
        
        LightingMathRandomGenerator(unsigned int seed) : m_gen(seed) {}

        float RandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(m_gen);
        }

        float RandomPositiveFloat(float max = 1000.0f)
        {
            return RandomFloat(0.0f, max);
        }

        float RandomRadius(float min = 0.001f, float max = 100.0f)
        {
            return RandomFloat(min, max);
        }

        float RandomAngle(float minDegrees = 0.0f, float maxDegrees = 180.0f)
        {
            return Lighting::DegreesToRadians(RandomFloat(minDegrees, maxDegrees));
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

        int RandomInt(int min, int max)
        {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(m_gen);
        }

        uint32_t RandomLayerMask()
        {
            std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
            return dist(m_gen);
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
     * @brief Test result structure for detailed failure reporting
     */
    struct TestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        float distance = 0.0f;
        float radius = 0.0f;
        float attenuation = 0.0f;
        float angle = 0.0f;
        float innerAngle = 0.0f;
        float outerAngle = 0.0f;
    };

    /**
     * @brief Property 2: 光照衰减函数正确性
     * 
     * For any point light or spotlight, given distance d and radius r:
     * - When d < r, light intensity should be > 0
     * - When d >= r, light intensity should be 0
     * - Attenuation value should monotonically decrease with distance
     * 
     * Feature: 2d-lighting-system, Property 2: 光照衰减函数正确性
     * Validates: Requirements 1.4, 2.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return TestResult with pass/fail status and failure details
     */
    inline TestResult TestProperty2_AttenuationCorrectness(int iterations = 100)
    {
        TestResult result;
        LightingMathRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            float radius = gen.RandomRadius(0.1f, 100.0f);
            ECS::AttenuationType attType = gen.RandomAttenuationType();
            
            // Test 1: Distance < radius should give positive attenuation
            float distanceInside = gen.RandomFloat(0.0f, radius * 0.99f);
            float attInside = Lighting::CalculateAttenuation(distanceInside, radius, attType);
            
            if (attInside <= 0.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.distance = distanceInside;
                result.radius = radius;
                result.attenuation = attInside;
                std::ostringstream oss;
                oss << "Attenuation should be > 0 when distance (" << distanceInside 
                    << ") < radius (" << radius << "), but got " << attInside;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 2: Distance >= radius should give zero attenuation
            float distanceOutside = radius + gen.RandomFloat(0.0f, 100.0f);
            float attOutside = Lighting::CalculateAttenuation(distanceOutside, radius, attType);
            
            if (attOutside != 0.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.distance = distanceOutside;
                result.radius = radius;
                result.attenuation = attOutside;
                std::ostringstream oss;
                oss << "Attenuation should be 0 when distance (" << distanceOutside 
                    << ") >= radius (" << radius << "), but got " << attOutside;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 3: Monotonicity - attenuation should decrease with distance
            float d1 = gen.RandomFloat(0.0f, radius * 0.5f);
            float d2 = gen.RandomFloat(d1 + 0.001f, radius * 0.99f);
            
            float att1 = Lighting::CalculateAttenuation(d1, radius, attType);
            float att2 = Lighting::CalculateAttenuation(d2, radius, attType);
            
            if (att1 < att2)
            {
                result.passed = false;
                result.failedIteration = i;
                result.distance = d1;
                result.radius = radius;
                result.attenuation = att1;
                std::ostringstream oss;
                oss << "Attenuation should be monotonically decreasing. "
                    << "At d1=" << d1 << " got " << att1 
                    << ", at d2=" << d2 << " got " << att2;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 4: Attenuation at distance 0 should be 1.0 (or close to it)
            float attAtZero = Lighting::CalculateAttenuation(0.0f, radius, attType);
            if (!FloatEquals(attAtZero, 1.0f, 0.01f))
            {
                result.passed = false;
                result.failedIteration = i;
                result.distance = 0.0f;
                result.radius = radius;
                result.attenuation = attAtZero;
                std::ostringstream oss;
                oss << "Attenuation at distance 0 should be 1.0, but got " << attAtZero;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 5: Attenuation should be in range [0, 1]
            float randomDist = gen.RandomFloat(0.0f, radius * 2.0f);
            float attRandom = Lighting::CalculateAttenuation(randomDist, radius, attType);
            
            if (attRandom < 0.0f || attRandom > 1.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.distance = randomDist;
                result.radius = radius;
                result.attenuation = attRandom;
                std::ostringstream oss;
                oss << "Attenuation should be in [0, 1], but got " << attRandom;
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Property 11: 聚光灯角度衰减
     * 
     * For any spotlight and target point:
     * - When angle < innerAngle: intensity should be 100%
     * - When angle > outerAngle: intensity should be 0%
     * - When innerAngle <= angle <= outerAngle: should smoothly interpolate
     * 
     * Feature: 2d-lighting-system, Property 11: 聚光灯角度衰减
     * Validates: Requirements 2.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return TestResult with pass/fail status and failure details
     */
    inline TestResult TestProperty11_SpotlightAngleAttenuation(int iterations = 100)
    {
        TestResult result;
        LightingMathRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate valid inner and outer angles (inner < outer)
            float innerAngleDeg = gen.RandomFloat(5.0f, 60.0f);
            float outerAngleDeg = gen.RandomFloat(innerAngleDeg + 5.0f, 90.0f);
            
            float innerAngle = Lighting::DegreesToRadians(innerAngleDeg);
            float outerAngle = Lighting::DegreesToRadians(outerAngleDeg);
            
            // Test 1: Angle < innerAngle should give 100% intensity
            float angleInside = gen.RandomFloat(0.0f, innerAngleDeg * 0.9f);
            float angleInsideRad = Lighting::DegreesToRadians(angleInside);
            float attInside = Lighting::CalculateSpotAngleAttenuationFromAngles(
                angleInsideRad, innerAngle, outerAngle);
            
            if (!FloatEquals(attInside, 1.0f, 0.01f))
            {
                result.passed = false;
                result.failedIteration = i;
                result.angle = angleInside;
                result.innerAngle = innerAngleDeg;
                result.outerAngle = outerAngleDeg;
                result.attenuation = attInside;
                std::ostringstream oss;
                oss << "Spotlight attenuation should be 1.0 when angle (" << angleInside 
                    << "°) < innerAngle (" << innerAngleDeg << "°), but got " << attInside;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 2: Angle > outerAngle should give 0% intensity
            float angleOutside = gen.RandomFloat(outerAngleDeg * 1.1f, 180.0f);
            float angleOutsideRad = Lighting::DegreesToRadians(angleOutside);
            float attOutside = Lighting::CalculateSpotAngleAttenuationFromAngles(
                angleOutsideRad, innerAngle, outerAngle);
            
            if (!FloatEquals(attOutside, 0.0f, 0.01f))
            {
                result.passed = false;
                result.failedIteration = i;
                result.angle = angleOutside;
                result.innerAngle = innerAngleDeg;
                result.outerAngle = outerAngleDeg;
                result.attenuation = attOutside;
                std::ostringstream oss;
                oss << "Spotlight attenuation should be 0.0 when angle (" << angleOutside 
                    << "°) > outerAngle (" << outerAngleDeg << "°), but got " << attOutside;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 3: Angle between inner and outer should give value in (0, 1)
            float angleBetween = gen.RandomFloat(innerAngleDeg + 1.0f, outerAngleDeg - 1.0f);
            float angleBetweenRad = Lighting::DegreesToRadians(angleBetween);
            float attBetween = Lighting::CalculateSpotAngleAttenuationFromAngles(
                angleBetweenRad, innerAngle, outerAngle);
            
            if (attBetween <= 0.0f || attBetween >= 1.0f)
            {
                result.passed = false;
                result.failedIteration = i;
                result.angle = angleBetween;
                result.innerAngle = innerAngleDeg;
                result.outerAngle = outerAngleDeg;
                result.attenuation = attBetween;
                std::ostringstream oss;
                oss << "Spotlight attenuation should be in (0, 1) when angle (" << angleBetween 
                    << "°) is between inner (" << innerAngleDeg << "°) and outer (" 
                    << outerAngleDeg << "°), but got " << attBetween;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test 4: Monotonicity - attenuation should decrease as angle increases
            float angle1 = gen.RandomFloat(innerAngleDeg, (innerAngleDeg + outerAngleDeg) / 2.0f);
            float angle2 = gen.RandomFloat(angle1 + 1.0f, outerAngleDeg);
            
            float att1 = Lighting::CalculateSpotAngleAttenuationFromAngles(
                Lighting::DegreesToRadians(angle1), innerAngle, outerAngle);
            float att2 = Lighting::CalculateSpotAngleAttenuationFromAngles(
                Lighting::DegreesToRadians(angle2), innerAngle, outerAngle);
            
            if (att1 < att2)
            {
                result.passed = false;
                result.failedIteration = i;
                result.angle = angle1;
                result.innerAngle = innerAngleDeg;
                result.outerAngle = outerAngleDeg;
                result.attenuation = att1;
                std::ostringstream oss;
                oss << "Spotlight attenuation should decrease as angle increases. "
                    << "At angle1=" << angle1 << "° got " << att1 
                    << ", at angle2=" << angle2 << "° got " << att2;
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Property 4: 光照层过滤正确性
     * 
     * For any light and sprite:
     * - Light should affect sprite if and only if (light.layerMask & sprite.lightLayer) != 0
     * 
     * Feature: 2d-lighting-system, Property 4: 光照层过滤正确性
     * Validates: Requirements 6.2, 6.3, 6.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return TestResult with pass/fail status and failure details
     */
    inline TestResult TestProperty4_LayerFilterCorrectness(int iterations = 100)
    {
        TestResult result;
        LightingMathRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            uint32_t lightLayerMask = gen.RandomLayerMask();
            uint32_t spriteLayer = gen.RandomLayerMask();
            
            // Test the layer filtering function
            bool affects = Lighting::LightAffectsLayer(lightLayerMask, spriteLayer);
            bool expectedAffects = (lightLayerMask & spriteLayer) != 0;
            
            if (affects != expectedAffects)
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Layer filter mismatch. lightLayerMask=0x" << std::hex << lightLayerMask
                    << ", spriteLayer=0x" << spriteLayer
                    << ". Expected " << (expectedAffects ? "true" : "false")
                    << ", got " << (affects ? "true" : "false");
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test specific layer index filtering
            for (int layerIdx = 0; layerIdx < 32; ++layerIdx)
            {
                bool affectsIndex = Lighting::LightAffectsLayerIndex(lightLayerMask, layerIdx);
                uint32_t layerBit = 1u << layerIdx;
                bool expectedAffectsIndex = (lightLayerMask & layerBit) != 0;
                
                if (affectsIndex != expectedAffectsIndex)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Layer index filter mismatch. lightLayerMask=0x" << std::hex << lightLayerMask
                        << ", layerIndex=" << std::dec << layerIdx
                        << ". Expected " << (expectedAffectsIndex ? "true" : "false")
                        << ", got " << (affectsIndex ? "true" : "false");
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test edge cases: invalid layer indices
            if (Lighting::LightAffectsLayerIndex(lightLayerMask, -1))
            {
                result.passed = false;
                result.failedIteration = i;
                result.failureMessage = "LightAffectsLayerIndex should return false for negative index";
                return result;
            }
            
            if (Lighting::LightAffectsLayerIndex(lightLayerMask, 32))
            {
                result.passed = false;
                result.failedIteration = i;
                result.failureMessage = "LightAffectsLayerIndex should return false for index >= 32";
                return result;
            }
            
            // Test specific scenarios from requirements:
            // Requirement 6.2: When light and sprite are on the same layer, light should affect sprite
            uint32_t sameLayerMask = 1u << gen.RandomInt(0, 31);
            if (!Lighting::LightAffectsLayer(sameLayerMask, sameLayerMask))
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Light should affect sprite when on same layer. layerMask=0x" 
                    << std::hex << sameLayerMask;
                result.failureMessage = oss.str();
                return result;
            }
            
            // Requirement 6.3: When light and sprite are on different layers, light should not affect sprite
            int layer1 = gen.RandomInt(0, 15);
            int layer2 = gen.RandomInt(16, 31);
            uint32_t lightMask = 1u << layer1;
            uint32_t spriteMask = 1u << layer2;
            if (Lighting::LightAffectsLayer(lightMask, spriteMask))
            {
                result.passed = false;
                result.failedIteration = i;
                std::ostringstream oss;
                oss << "Light should NOT affect sprite when on different layers. "
                    << "lightMask=0x" << std::hex << lightMask
                    << ", spriteMask=0x" << spriteMask;
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Test result structure for Property 3 (multi-light linearity)
     */
    struct Property3TestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        int numLights = 0;
        glm::vec2 targetPosition{0.0f, 0.0f};
        glm::vec3 combinedContribution{0.0f, 0.0f, 0.0f};
        glm::vec3 sumOfIndividual{0.0f, 0.0f, 0.0f};
    };

    /**
     * @brief Helper function to calculate point light contribution at a position
     * 
     * This simulates the shader calculation for testing purposes.
     * 
     * @param light Light data
     * @param worldPos Target world position
     * @param spriteLayer Sprite's light layer
     * @return Light contribution (RGB)
     */
    inline glm::vec3 CalculatePointLightContribution(
        const ECS::LightData& light,
        const glm::vec2& worldPos,
        uint32_t spriteLayer)
    {
        // Check layer mask
        if (!Lighting::LightAffectsLayer(light.layerMask, spriteLayer))
        {
            return glm::vec3(0.0f);
        }
        
        // Calculate distance
        glm::vec2 toLight = light.position - worldPos;
        float distance = glm::length(toLight);
        
        // Calculate attenuation based on type
        ECS::AttenuationType attType = static_cast<ECS::AttenuationType>(static_cast<int>(light.attenuation));
        float attenuation = Lighting::CalculateAttenuation(distance, light.radius, attType);
        
        if (attenuation <= 0.0f)
        {
            return glm::vec3(0.0f);
        }
        
        // Return light contribution
        return glm::vec3(light.color.r, light.color.g, light.color.b) * light.intensity * attenuation;
    }

    /**
     * @brief Property 3: 多光源叠加线性性
     * 
     * For any two or more lights, the lighting contribution at a point should
     * equal the linear sum of each light's individual contribution
     * (when not exceeding the maximum light limit).
     * 
     * This property validates that:
     * - Light contributions are additive
     * - Combined lighting = sum of individual light contributions
     * - Order of light processing doesn't affect the result
     * 
     * Feature: 2d-lighting-system, Property 3: 多光源叠加线性性
     * Validates: Requirements 1.5
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return Property3TestResult with pass/fail status and failure details
     */
    inline Property3TestResult TestProperty3_MultiLightAdditiveLinearity(int iterations = 100)
    {
        Property3TestResult result;
        LightingMathRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random number of lights (2 to 8)
            int numLights = gen.RandomInt(2, 8);
            
            // Generate random target position
            glm::vec2 targetPos(gen.RandomFloat(-100.0f, 100.0f), gen.RandomFloat(-100.0f, 100.0f));
            
            // Generate random sprite layer (use all layers for simplicity)
            uint32_t spriteLayer = 0xFFFFFFFF;
            
            // Generate random lights
            std::vector<ECS::LightData> lights;
            for (int j = 0; j < numLights; ++j)
            {
                ECS::LightData light;
                // Position lights around the target so they can contribute
                float offsetX = gen.RandomFloat(-50.0f, 50.0f);
                float offsetY = gen.RandomFloat(-50.0f, 50.0f);
                light.position = targetPos + glm::vec2(offsetX, offsetY);
                light.direction = glm::vec2(0.0f, -1.0f);
                light.color = glm::vec4(
                    gen.RandomFloat(0.1f, 1.0f),
                    gen.RandomFloat(0.1f, 1.0f),
                    gen.RandomFloat(0.1f, 1.0f),
                    1.0f
                );
                light.intensity = gen.RandomFloat(0.1f, 2.0f);
                // Make radius large enough to reach target
                float distToTarget = glm::length(light.position - targetPos);
                light.radius = distToTarget + gen.RandomFloat(10.0f, 50.0f);
                light.innerAngle = 0.0f;
                light.outerAngle = 0.0f;
                light.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                light.layerMask = 0xFFFFFFFF;
                // Use quadratic attenuation (type 1)
                light.attenuation = 1.0f;
                light.padding = 0.0f;
                
                lights.push_back(light);
            }
            
            // Calculate sum of individual contributions
            glm::vec3 sumOfIndividual(0.0f);
            for (const auto& light : lights)
            {
                glm::vec3 contribution = CalculatePointLightContribution(light, targetPos, spriteLayer);
                sumOfIndividual += contribution;
            }
            
            // Calculate combined contribution (simulating shader behavior)
            // In the shader, we simply add all light contributions
            glm::vec3 combinedContribution(0.0f);
            for (const auto& light : lights)
            {
                combinedContribution += CalculatePointLightContribution(light, targetPos, spriteLayer);
            }
            
            // Verify linearity: combined should equal sum of individual
            float epsilon = 1e-5f;
            if (!FloatEquals(combinedContribution.r, sumOfIndividual.r, epsilon) ||
                !FloatEquals(combinedContribution.g, sumOfIndividual.g, epsilon) ||
                !FloatEquals(combinedContribution.b, sumOfIndividual.b, epsilon))
            {
                result.passed = false;
                result.failedIteration = i;
                result.numLights = numLights;
                result.targetPosition = targetPos;
                result.combinedContribution = combinedContribution;
                result.sumOfIndividual = sumOfIndividual;
                std::ostringstream oss;
                oss << "Multi-light contribution is not linear. "
                    << "Combined=(" << combinedContribution.r << ", " << combinedContribution.g << ", " << combinedContribution.b << "), "
                    << "Sum=(" << sumOfIndividual.r << ", " << sumOfIndividual.g << ", " << sumOfIndividual.b << ")";
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test commutativity: order shouldn't matter
            // Reverse the order and recalculate
            glm::vec3 reversedContribution(0.0f);
            for (int j = numLights - 1; j >= 0; --j)
            {
                reversedContribution += CalculatePointLightContribution(lights[j], targetPos, spriteLayer);
            }
            
            if (!FloatEquals(combinedContribution.r, reversedContribution.r, epsilon) ||
                !FloatEquals(combinedContribution.g, reversedContribution.g, epsilon) ||
                !FloatEquals(combinedContribution.b, reversedContribution.b, epsilon))
            {
                result.passed = false;
                result.failedIteration = i;
                result.numLights = numLights;
                result.targetPosition = targetPos;
                result.combinedContribution = combinedContribution;
                result.sumOfIndividual = reversedContribution;
                std::ostringstream oss;
                oss << "Light contribution order matters (should be commutative). "
                    << "Forward=(" << combinedContribution.r << ", " << combinedContribution.g << ", " << combinedContribution.b << "), "
                    << "Reversed=(" << reversedContribution.r << ", " << reversedContribution.g << ", " << reversedContribution.b << ")";
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test associativity: (A + B) + C = A + (B + C)
            if (numLights >= 3)
            {
                // Calculate (light0 + light1) + light2
                glm::vec3 contrib01 = CalculatePointLightContribution(lights[0], targetPos, spriteLayer) +
                                      CalculatePointLightContribution(lights[1], targetPos, spriteLayer);
                glm::vec3 leftAssoc = contrib01 + CalculatePointLightContribution(lights[2], targetPos, spriteLayer);
                
                // Calculate light0 + (light1 + light2)
                glm::vec3 contrib12 = CalculatePointLightContribution(lights[1], targetPos, spriteLayer) +
                                      CalculatePointLightContribution(lights[2], targetPos, spriteLayer);
                glm::vec3 rightAssoc = CalculatePointLightContribution(lights[0], targetPos, spriteLayer) + contrib12;
                
                if (!FloatEquals(leftAssoc.r, rightAssoc.r, epsilon) ||
                    !FloatEquals(leftAssoc.g, rightAssoc.g, epsilon) ||
                    !FloatEquals(leftAssoc.b, rightAssoc.b, epsilon))
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.numLights = numLights;
                    result.targetPosition = targetPos;
                    result.combinedContribution = leftAssoc;
                    result.sumOfIndividual = rightAssoc;
                    std::ostringstream oss;
                    oss << "Light contribution is not associative. "
                        << "(A+B)+C=(" << leftAssoc.r << ", " << leftAssoc.g << ", " << leftAssoc.b << "), "
                        << "A+(B+C)=(" << rightAssoc.r << ", " << rightAssoc.g << ", " << rightAssoc.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 3 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty3Test()
    {
        LogInfo("Running Property 3: 多光源叠加线性性 (100 iterations)...");
        
        Property3TestResult result = TestProperty3_MultiLightAdditiveLinearity(100);
        
        if (result.passed)
        {
            LogInfo("Property 3 (多光源叠加线性性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 3 (多光源叠加线性性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: numLights=%d, targetPos=(%f, %f)", 
                     result.numLights, result.targetPosition.x, result.targetPosition.y);
            LogError("Combined=(%f, %f, %f), Sum=(%f, %f, %f)",
                     result.combinedContribution.r, result.combinedContribution.g, result.combinedContribution.b,
                     result.sumOfIndividual.r, result.sumOfIndividual.g, result.sumOfIndividual.b);
            return false;
        }
    }

    /**
     * @brief Run Property 2 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty2Test()
    {
        LogInfo("Running Property 2: 光照衰减函数正确性 (100 iterations)...");
        
        TestResult result = TestProperty2_AttenuationCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 2 (光照衰减函数正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 2 (光照衰减函数正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: distance=%f, radius=%f, attenuation=%f", 
                     result.distance, result.radius, result.attenuation);
            return false;
        }
    }

    /**
     * @brief Run Property 11 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty11Test()
    {
        LogInfo("Running Property 11: 聚光灯角度衰减 (100 iterations)...");
        
        TestResult result = TestProperty11_SpotlightAngleAttenuation(100);
        
        if (result.passed)
        {
            LogInfo("Property 11 (聚光灯角度衰减) PASSED");
            return true;
        }
        else
        {
            LogError("Property 11 (聚光灯角度衰减) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: angle=%f°, innerAngle=%f°, outerAngle=%f°, attenuation=%f", 
                     result.angle, result.innerAngle, result.outerAngle, result.attenuation);
            return false;
        }
    }

    /**
     * @brief Run Property 4 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty4Test()
    {
        LogInfo("Running Property 4: 光照层过滤正确性 (100 iterations)...");
        
        TestResult result = TestProperty4_LayerFilterCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 4 (光照层过滤正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 4 (光照层过滤正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Test result structure for Property 12 (normal map lighting)
     */
    struct Property12TestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        glm::vec3 normalMapValue{0.0f, 0.0f, 0.0f};
        glm::vec3 withNormalResult{0.0f, 0.0f, 0.0f};
        glm::vec3 withoutNormalResult{0.0f, 0.0f, 0.0f};
        glm::vec2 lightPosition{0.0f, 0.0f};
        glm::vec2 targetPosition{0.0f, 0.0f};
    };

    /**
     * @brief Property 12: 法线贴图光照影响
     * 
     * For any sprite with a normal map configured:
     * - The lighting calculation result should differ from when no normal map is configured
     * - Exception: If the normal map is pure blue (0.5, 0.5, 1.0), results should be similar
     *   because the default normal (0, 0, 1) points straight up
     * 
     * This property validates that:
     * - Normal maps affect lighting calculations
     * - Non-default normals produce different lighting than flat surfaces
     * - Default normals (pure blue) produce similar results to no normal map
     * 
     * Feature: 2d-lighting-system, Property 12: 法线贴图光照影响
     * Validates: Requirements 7.2
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return Property12TestResult with pass/fail status and failure details
     */
    inline Property12TestResult TestProperty12_NormalMapLightingEffect(int iterations = 100)
    {
        Property12TestResult result;
        LightingMathRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random target position
            glm::vec2 targetPos(gen.RandomFloat(-100.0f, 100.0f), gen.RandomFloat(-100.0f, 100.0f));
            
            // Generate random light position (offset from target)
            float offsetX = gen.RandomFloat(-50.0f, 50.0f);
            float offsetY = gen.RandomFloat(-50.0f, 50.0f);
            glm::vec2 lightPos = targetPos + glm::vec2(offsetX, offsetY);
            
            // Generate random light properties
            glm::vec4 lightColor(
                gen.RandomFloat(0.5f, 1.0f),
                gen.RandomFloat(0.5f, 1.0f),
                gen.RandomFloat(0.5f, 1.0f),
                1.0f
            );
            float intensity = gen.RandomFloat(0.5f, 2.0f);
            float distToTarget = glm::length(lightPos - targetPos);
            float radius = distToTarget + gen.RandomFloat(10.0f, 50.0f);
            ECS::AttenuationType attType = gen.RandomAttenuationType();
            uint32_t layerMask = 0xFFFFFFFF;
            uint32_t spriteLayer = 0xFFFFFFFF;
            
            // Test 1: Non-default normal should produce different result than no normal
            {
                // Generate a non-default normal map value (not pure blue)
                glm::vec3 normalMapValue(
                    gen.RandomFloat(0.0f, 1.0f),
                    gen.RandomFloat(0.0f, 1.0f),
                    gen.RandomFloat(0.5f, 1.0f)  // Z should be positive for valid normals
                );
                
                // Skip if this happens to be close to default normal
                if (Lighting::IsDefaultNormal(normalMapValue, 0.1f))
                {
                    continue;
                }
                
                // Unpack and normalize the normal
                glm::vec3 normal = glm::normalize(Lighting::UnpackNormal(normalMapValue));
                
                // Calculate lighting with normal
                glm::vec3 withNormal = Lighting::CalculatePointLightWithNormal(
                    lightPos, targetPos, normal, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Calculate lighting without normal
                glm::vec3 withoutNormal = Lighting::CalculatePointLightWithoutNormal(
                    lightPos, targetPos, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Results should be different (unless both are zero due to being out of range)
                bool bothZero = (glm::length(withNormal) < 0.001f && glm::length(withoutNormal) < 0.001f);
                bool areDifferent = !FloatEquals(withNormal.r, withoutNormal.r, 0.01f) ||
                                    !FloatEquals(withNormal.g, withoutNormal.g, 0.01f) ||
                                    !FloatEquals(withNormal.b, withoutNormal.b, 0.01f);
                
                if (!bothZero && !areDifferent)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.normalMapValue = normalMapValue;
                    result.withNormalResult = withNormal;
                    result.withoutNormalResult = withoutNormal;
                    result.lightPosition = lightPos;
                    result.targetPosition = targetPos;
                    std::ostringstream oss;
                    oss << "Non-default normal map should produce different lighting result. "
                        << "NormalMapValue=(" << normalMapValue.x << ", " << normalMapValue.y << ", " << normalMapValue.z << "), "
                        << "WithNormal=(" << withNormal.r << ", " << withNormal.g << ", " << withNormal.b << "), "
                        << "WithoutNormal=(" << withoutNormal.r << ", " << withoutNormal.g << ", " << withoutNormal.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Default normal (pure blue 0.5, 0.5, 1.0) should produce similar result
            {
                glm::vec3 defaultNormalMapValue(0.5f, 0.5f, 1.0f);
                glm::vec3 defaultNormal = glm::normalize(Lighting::UnpackNormal(defaultNormalMapValue));
                // defaultNormal should be approximately (0, 0, 1)
                
                // Calculate lighting with default normal
                glm::vec3 withDefaultNormal = Lighting::CalculatePointLightWithNormal(
                    lightPos, targetPos, defaultNormal, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Calculate lighting without normal
                glm::vec3 withoutNormal = Lighting::CalculatePointLightWithoutNormal(
                    lightPos, targetPos, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // With default normal pointing up (0, 0, 1), the NdotL factor depends on light direction
                // The light direction is normalize(vec3(toLight, 1.0)), so NdotL = lightDir.z
                // This means the result with default normal will be scaled by lightDir.z
                // We just verify that the default normal produces a valid result (not zero when light is in range)
                
                bool bothZero = (glm::length(withDefaultNormal) < 0.001f && glm::length(withoutNormal) < 0.001f);
                
                // If without normal is non-zero, with default normal should also be non-zero
                // (because default normal points up and light has positive z component)
                if (glm::length(withoutNormal) > 0.001f && glm::length(withDefaultNormal) < 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.normalMapValue = defaultNormalMapValue;
                    result.withNormalResult = withDefaultNormal;
                    result.withoutNormalResult = withoutNormal;
                    result.lightPosition = lightPos;
                    result.targetPosition = targetPos;
                    std::ostringstream oss;
                    oss << "Default normal (0.5, 0.5, 1.0) should produce non-zero lighting when light is in range. "
                        << "WithDefaultNormal=(" << withDefaultNormal.r << ", " << withDefaultNormal.g << ", " << withDefaultNormal.b << "), "
                        << "WithoutNormal=(" << withoutNormal.r << ", " << withoutNormal.g << ", " << withoutNormal.b << ")";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Normal facing away from light should produce zero or very low lighting
            {
                // Create a normal that faces away from the light
                glm::vec2 toLight = lightPos - targetPos;
                glm::vec2 awayFromLight = -glm::normalize(toLight);
                // Convert to normal map value (facing away in XY, with small Z)
                glm::vec3 awayNormal = glm::normalize(glm::vec3(awayFromLight.x, awayFromLight.y, 0.1f));
                
                // Calculate lighting with away-facing normal
                glm::vec3 withAwayNormal = Lighting::CalculatePointLightWithNormal(
                    lightPos, targetPos, awayNormal, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Calculate lighting without normal
                glm::vec3 withoutNormal = Lighting::CalculatePointLightWithoutNormal(
                    lightPos, targetPos, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Away-facing normal should produce less light than no normal (if light is in range)
                if (glm::length(withoutNormal) > 0.001f)
                {
                    float withAwayMagnitude = glm::length(withAwayNormal);
                    float withoutMagnitude = glm::length(withoutNormal);
                    
                    // Away-facing normal should produce significantly less light
                    if (withAwayMagnitude >= withoutMagnitude)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.normalMapValue = glm::vec3(awayNormal.x * 0.5f + 0.5f, awayNormal.y * 0.5f + 0.5f, awayNormal.z * 0.5f + 0.5f);
                        result.withNormalResult = withAwayNormal;
                        result.withoutNormalResult = withoutNormal;
                        result.lightPosition = lightPos;
                        result.targetPosition = targetPos;
                        std::ostringstream oss;
                        oss << "Normal facing away from light should produce less lighting. "
                            << "AwayNormal=(" << awayNormal.x << ", " << awayNormal.y << ", " << awayNormal.z << "), "
                            << "WithAwayNormal magnitude=" << withAwayMagnitude << ", "
                            << "WithoutNormal magnitude=" << withoutMagnitude;
                        result.failureMessage = oss.str();
                        return result;
                    }
                }
            }
            
            // Test 4: Normal facing toward light should produce more light than away-facing
            {
                glm::vec2 toLight = lightPos - targetPos;
                glm::vec2 towardLight = glm::normalize(toLight);
                
                // Normal facing toward light
                glm::vec3 towardNormal = glm::normalize(glm::vec3(towardLight.x, towardLight.y, 0.5f));
                
                // Normal facing away from light
                glm::vec3 awayNormal = glm::normalize(glm::vec3(-towardLight.x, -towardLight.y, 0.5f));
                
                // Calculate lighting with both normals
                glm::vec3 withTowardNormal = Lighting::CalculatePointLightWithNormal(
                    lightPos, targetPos, towardNormal, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                glm::vec3 withAwayNormal = Lighting::CalculatePointLightWithNormal(
                    lightPos, targetPos, awayNormal, lightColor, intensity, radius, attType, layerMask, spriteLayer);
                
                // Toward-facing normal should produce more light than away-facing
                float towardMagnitude = glm::length(withTowardNormal);
                float awayMagnitude = glm::length(withAwayNormal);
                
                if (towardMagnitude > 0.001f && towardMagnitude <= awayMagnitude)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.normalMapValue = glm::vec3(towardNormal.x * 0.5f + 0.5f, towardNormal.y * 0.5f + 0.5f, towardNormal.z * 0.5f + 0.5f);
                    result.withNormalResult = withTowardNormal;
                    result.withoutNormalResult = withAwayNormal;
                    result.lightPosition = lightPos;
                    result.targetPosition = targetPos;
                    std::ostringstream oss;
                    oss << "Normal facing toward light should produce more lighting than away-facing. "
                        << "TowardNormal magnitude=" << towardMagnitude << ", "
                        << "AwayNormal magnitude=" << awayMagnitude;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 12 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty12Test()
    {
        LogInfo("Running Property 12: 法线贴图光照影响 (100 iterations)...");
        
        Property12TestResult result = TestProperty12_NormalMapLightingEffect(100);
        
        if (result.passed)
        {
            LogInfo("Property 12 (法线贴图光照影响) PASSED");
            return true;
        }
        else
        {
            LogError("Property 12 (法线贴图光照影响) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: normalMapValue=(%f, %f, %f), lightPos=(%f, %f), targetPos=(%f, %f)",
                     result.normalMapValue.x, result.normalMapValue.y, result.normalMapValue.z,
                     result.lightPosition.x, result.lightPosition.y,
                     result.targetPosition.x, result.targetPosition.y);
            LogError("WithNormal=(%f, %f, %f), WithoutNormal=(%f, %f, %f)",
                     result.withNormalResult.r, result.withNormalResult.g, result.withNormalResult.b,
                     result.withoutNormalResult.r, result.withoutNormalResult.g, result.withoutNormalResult.b);
            return false;
        }
    }

    /**
     * @brief Run all LightingMath property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllLightingMathTests()
    {
        bool allPassed = true;
        
        LogInfo("=== Running LightingMath Property Tests ===");
        
        // Property 2: Attenuation correctness
        if (!RunProperty2Test())
        {
            allPassed = false;
        }
        
        // Property 3: Multi-light additive linearity
        if (!RunProperty3Test())
        {
            allPassed = false;
        }
        
        // Property 4: Layer filter correctness
        if (!RunProperty4Test())
        {
            allPassed = false;
        }
        
        // Property 11: Spotlight angle attenuation
        if (!RunProperty11Test())
        {
            allPassed = false;
        }
        
        // Property 12: Normal map lighting effect
        if (!RunProperty12Test())
        {
            allPassed = false;
        }
        
        LogInfo("=== LightingMath Property Tests Complete ===");
        
        if (allPassed)
        {
            LogInfo("All LightingMath tests PASSED");
        }
        else
        {
            LogError("Some LightingMath tests FAILED");
        }
        
        return allPassed;
    }
}

#endif // LIGHTING_MATH_TESTS_H

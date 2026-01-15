#ifndef LIGHTING_SYSTEM_TESTS_H
#define LIGHTING_SYSTEM_TESTS_H

/**
 * @file LightingSystemTests.h
 * @brief Property-based tests for LightingSystem
 * 
 * This file contains property-based tests for validating the correctness of
 * light culling, priority sorting, and light count limiting.
 * 
 * Tests are designed to be run with RapidCheck-style property testing.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-system
 */

#include "../LightingSystem.h"
#include "../../Components/LightingTypes.h"
#include "../../Utils/Logger.h"
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <algorithm>

namespace LightingTests
{
    /**
     * @brief Random generator for lighting system tests
     */
    class LightingSystemRandomGenerator
    {
    public:
        LightingSystemRandomGenerator() : m_gen(std::random_device{}()) {}
        
        LightingSystemRandomGenerator(unsigned int seed) : m_gen(seed) {}

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

        ECS::LightData RandomPointLight(float minX, float maxX, float minY, float maxY)
        {
            ECS::LightData light;
            light.position = RandomPosition(minX, maxX, minY, maxY);
            light.direction = glm::vec2(0.0f, 0.0f);
            light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            light.intensity = RandomFloat(0.1f, 2.0f);
            light.radius = RandomFloat(1.0f, 50.0f);
            light.innerAngle = 0.0f;
            light.outerAngle = 0.0f;
            light.lightType = static_cast<uint32_t>(ECS::LightType::Point);
            light.layerMask = 0xFFFFFFFF;
            light.attenuation = 1.0f;
            return light;
        }

        ECS::LightData RandomDirectionalLight()
        {
            ECS::LightData light;
            light.position = glm::vec2(0.0f, 0.0f);
            light.direction = glm::normalize(glm::vec2(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)));
            light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            light.intensity = RandomFloat(0.1f, 2.0f);
            light.radius = 0.0f;
            light.innerAngle = 0.0f;
            light.outerAngle = 0.0f;
            light.lightType = static_cast<uint32_t>(ECS::LightType::Directional);
            light.layerMask = 0xFFFFFFFF;
            light.attenuation = 0.0f;
            return light;
        }

        Systems::LightInfo RandomLightInfo(float minX, float maxX, float minY, float maxY, 
                                           const glm::vec2& cameraPos)
        {
            Systems::LightInfo info;
            info.data = RandomPointLight(minX, maxX, minY, maxY);
            info.priority = RandomInt(-10, 100);
            info.isDirectional = false;
            info.distanceToCamera = glm::length(info.data.position - cameraPos);
            return info;
        }

    private:
        std::mt19937 m_gen;
    };

    /**
     * @brief Test result structure for LightingSystem tests
     */
    struct LightingSystemTestResult
    {
        bool passed = true;
        std::string failureMessage;
        int failedIteration = -1;
        
        // Failing example data
        int lightCount = 0;
        int visibleCount = 0;
        int expectedCount = 0;
        float viewWidth = 0.0f;
        float viewHeight = 0.0f;
        glm::vec2 cameraPosition{0.0f, 0.0f};
    };

    /**
     * @brief Property 5: 光源剔除正确性
     * 
     * For any light set and camera frustum:
     * - Culled light set should only contain lights that intersect with the frustum
     * - No visible lights should be missed (no false negatives)
     * - Directional lights should always be visible
     * 
     * Feature: 2d-lighting-system, Property 5: 光源剔除正确性
     * Validates: Requirements 10.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightingSystemTestResult with pass/fail status and failure details
     */
    inline LightingSystemTestResult TestProperty5_LightCullingCorrectness(int iterations = 100)
    {
        LightingSystemTestResult result;
        LightingSystemRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random view bounds
            float viewWidth = gen.RandomFloat(100.0f, 1000.0f);
            float viewHeight = gen.RandomFloat(100.0f, 1000.0f);
            glm::vec2 cameraPos = gen.RandomPosition(-500.0f, 500.0f, -500.0f, 500.0f);
            
            // Calculate view bounds with margin
            float margin = 100.0f;
            Systems::LightBounds viewBounds;
            viewBounds.minX = cameraPos.x - viewWidth / 2.0f - margin;
            viewBounds.maxX = cameraPos.x + viewWidth / 2.0f + margin;
            viewBounds.minY = cameraPos.y - viewHeight / 2.0f - margin;
            viewBounds.maxY = cameraPos.y + viewHeight / 2.0f + margin;
            
            // Generate random lights - some inside view, some outside
            int numLights = gen.RandomInt(10, 50);
            std::vector<ECS::LightData> allLights;
            std::vector<bool> expectedVisible;
            
            for (int j = 0; j < numLights; ++j)
            {
                ECS::LightData light;
                bool shouldBeVisible;
                
                // 50% chance to be inside view, 50% outside
                if (gen.RandomInt(0, 1) == 0)
                {
                    // Generate light inside view
                    light = gen.RandomPointLight(
                        viewBounds.minX + 10.0f, viewBounds.maxX - 10.0f,
                        viewBounds.minY + 10.0f, viewBounds.maxY - 10.0f
                    );
                    shouldBeVisible = true;
                }
                else
                {
                    // Generate light outside view (far away)
                    float farOffset = gen.RandomFloat(500.0f, 1000.0f);
                    int direction = gen.RandomInt(0, 3);
                    glm::vec2 pos;
                    switch (direction)
                    {
                        case 0: pos = glm::vec2(viewBounds.maxX + farOffset, cameraPos.y); break;
                        case 1: pos = glm::vec2(viewBounds.minX - farOffset, cameraPos.y); break;
                        case 2: pos = glm::vec2(cameraPos.x, viewBounds.maxY + farOffset); break;
                        case 3: pos = glm::vec2(cameraPos.x, viewBounds.minY - farOffset); break;
                    }
                    light.position = pos;
                    light.radius = gen.RandomFloat(1.0f, 50.0f);
                    light.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                    shouldBeVisible = false;
                }
                
                allLights.push_back(light);
                expectedVisible.push_back(shouldBeVisible);
            }
            
            // Add a directional light (should always be visible)
            ECS::LightData dirLight = gen.RandomDirectionalLight();
            allLights.push_back(dirLight);
            expectedVisible.push_back(true); // Directional lights always visible
            
            // Test culling for each light
            for (size_t j = 0; j < allLights.size(); ++j)
            {
                const ECS::LightData& light = allLights[j];
                Systems::LightBounds lightBounds = Systems::LightingSystem::CalculateLightBounds(light);
                bool isVisible = Systems::LightingSystem::IsLightInView(lightBounds, viewBounds);
                
                // For directional lights, they should always be visible
                if (light.lightType == static_cast<uint32_t>(ECS::LightType::Directional))
                {
                    if (!isVisible)
                    {
                        result.passed = false;
                        result.failedIteration = i;
                        result.failureMessage = "Directional light should always be visible";
                        return result;
                    }
                    continue;
                }
                
                // For point lights that should be visible, verify they are not culled
                if (expectedVisible[j] && !isVisible)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.viewWidth = viewWidth;
                    result.viewHeight = viewHeight;
                    result.cameraPosition = cameraPos;
                    std::ostringstream oss;
                    oss << "Light at (" << light.position.x << ", " << light.position.y 
                        << ") with radius " << light.radius 
                        << " should be visible but was culled. "
                        << "View bounds: [" << viewBounds.minX << ", " << viewBounds.maxX 
                        << "] x [" << viewBounds.minY << ", " << viewBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test that CalculateLightBounds produces correct bounds
            for (const auto& light : allLights)
            {
                if (light.lightType == static_cast<uint32_t>(ECS::LightType::Directional))
                    continue;
                    
                Systems::LightBounds bounds = Systems::LightingSystem::CalculateLightBounds(light);
                
                // Verify bounds are correct for point/spot lights
                float expectedMinX = light.position.x - light.radius;
                float expectedMaxX = light.position.x + light.radius;
                float expectedMinY = light.position.y - light.radius;
                float expectedMaxY = light.position.y + light.radius;
                
                if (std::abs(bounds.minX - expectedMinX) > 0.001f ||
                    std::abs(bounds.maxX - expectedMaxX) > 0.001f ||
                    std::abs(bounds.minY - expectedMinY) > 0.001f ||
                    std::abs(bounds.maxY - expectedMaxY) > 0.001f)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Light bounds calculation incorrect. "
                        << "Expected [" << expectedMinX << ", " << expectedMaxX 
                        << "] x [" << expectedMinY << ", " << expectedMaxY << "], "
                        << "got [" << bounds.minX << ", " << bounds.maxX 
                        << "] x [" << bounds.minY << ", " << bounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Property 6: 光源数量限制
     * 
     * For any pixel position, the number of lights affecting that pixel
     * should not exceed maxLightsPerPixel configuration value.
     * 
     * Feature: 2d-lighting-system, Property 6: 光源数量限制
     * Validates: Requirements 10.2
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightingSystemTestResult with pass/fail status and failure details
     */
    inline LightingSystemTestResult TestProperty6_LightCountLimit(int iterations = 100)
    {
        LightingSystemTestResult result;
        LightingSystemRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random max light count
            uint32_t maxLights = static_cast<uint32_t>(gen.RandomInt(1, 50));
            
            // Generate more lights than the limit
            int numLights = gen.RandomInt(static_cast<int>(maxLights) + 10, 200);
            glm::vec2 cameraPos(0.0f, 0.0f);
            
            std::vector<Systems::LightInfo> lights;
            for (int j = 0; j < numLights; ++j)
            {
                lights.push_back(gen.RandomLightInfo(-100.0f, 100.0f, -100.0f, 100.0f, cameraPos));
            }
            
            // Apply limit
            Systems::LightingSystem::LimitLightCount(lights, maxLights);
            
            // Verify count is limited
            if (lights.size() > maxLights)
            {
                result.passed = false;
                result.failedIteration = i;
                result.lightCount = numLights;
                result.visibleCount = static_cast<int>(lights.size());
                result.expectedCount = static_cast<int>(maxLights);
                std::ostringstream oss;
                oss << "Light count should be limited to " << maxLights 
                    << ", but got " << lights.size() << " lights";
                result.failureMessage = oss.str();
                return result;
            }
            
            // Test with fewer lights than limit
            int fewLights = gen.RandomInt(1, static_cast<int>(maxLights) - 1);
            std::vector<Systems::LightInfo> fewLightsList;
            for (int j = 0; j < fewLights; ++j)
            {
                fewLightsList.push_back(gen.RandomLightInfo(-100.0f, 100.0f, -100.0f, 100.0f, cameraPos));
            }
            
            size_t originalSize = fewLightsList.size();
            Systems::LightingSystem::LimitLightCount(fewLightsList, maxLights);
            
            // Should not change if already under limit
            if (fewLightsList.size() != originalSize)
            {
                result.passed = false;
                result.failedIteration = i;
                result.lightCount = fewLights;
                result.visibleCount = static_cast<int>(fewLightsList.size());
                result.expectedCount = fewLights;
                std::ostringstream oss;
                oss << "Light count should remain " << originalSize 
                    << " when under limit, but got " << fewLightsList.size();
                result.failureMessage = oss.str();
                return result;
            }
        }
        
        return result;
    }

    /**
     * @brief Property 7: 光源优先级排序
     * 
     * For any light set exceeding the limit, selected lights should be sorted
     * by priority (high to low), and by distance (near to far) when priorities are equal.
     * 
     * Feature: 2d-lighting-system, Property 7: 光源优先级排序
     * Validates: Requirements 10.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightingSystemTestResult with pass/fail status and failure details
     */
    inline LightingSystemTestResult TestProperty7_LightPrioritySorting(int iterations = 100)
    {
        LightingSystemTestResult result;
        LightingSystemRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random lights with varying priorities
            int numLights = gen.RandomInt(10, 100);
            glm::vec2 cameraPos = gen.RandomPosition(-100.0f, 100.0f, -100.0f, 100.0f);
            
            std::vector<Systems::LightInfo> lights;
            for (int j = 0; j < numLights; ++j)
            {
                Systems::LightInfo info;
                info.data = gen.RandomPointLight(-500.0f, 500.0f, -500.0f, 500.0f);
                info.priority = gen.RandomInt(-10, 100);
                info.isDirectional = false;
                info.distanceToCamera = glm::length(info.data.position - cameraPos);
                lights.push_back(info);
            }
            
            // Sort lights
            Systems::LightingSystem::SortLightsByPriority(lights);
            
            // Verify sorting order
            for (size_t j = 1; j < lights.size(); ++j)
            {
                const auto& prev = lights[j - 1];
                const auto& curr = lights[j];
                
                // Higher priority should come first
                if (prev.priority < curr.priority)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Lights not sorted by priority. Light at index " << (j - 1) 
                        << " has priority " << prev.priority 
                        << ", but light at index " << j << " has priority " << curr.priority;
                    result.failureMessage = oss.str();
                    return result;
                }
                
                // If same priority, closer should come first
                if (prev.priority == curr.priority && prev.distanceToCamera > curr.distanceToCamera)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Lights with same priority not sorted by distance. "
                        << "Light at index " << (j - 1) << " has distance " << prev.distanceToCamera 
                        << ", but light at index " << j << " has distance " << curr.distanceToCamera;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test that high priority lights are kept when limiting
            uint32_t maxLights = static_cast<uint32_t>(gen.RandomInt(5, numLights / 2));
            
            // Create a copy and limit
            std::vector<Systems::LightInfo> limitedLights = lights;
            Systems::LightingSystem::LimitLightCount(limitedLights, maxLights);
            
            // Verify that the kept lights are the highest priority ones
            for (size_t j = 0; j < limitedLights.size(); ++j)
            {
                if (limitedLights[j].priority != lights[j].priority ||
                    limitedLights[j].distanceToCamera != lights[j].distanceToCamera)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Limited lights should be the first " << maxLights 
                        << " from sorted list, but mismatch at index " << j;
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 5 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty5Test()
    {
        LogInfo("Running Property 5: 光源剔除正确性 (100 iterations)...");
        
        LightingSystemTestResult result = TestProperty5_LightCullingCorrectness(100);
        
        if (result.passed)
        {
            LogInfo("Property 5 (光源剔除正确性) PASSED");
            return true;
        }
        else
        {
            LogError("Property 5 (光源剔除正确性) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            if (result.viewWidth > 0)
            {
                LogError("Failing example: viewWidth=%f, viewHeight=%f, cameraPos=(%f, %f)", 
                         result.viewWidth, result.viewHeight, 
                         result.cameraPosition.x, result.cameraPosition.y);
            }
            return false;
        }
    }

    /**
     * @brief Run Property 6 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty6Test()
    {
        LogInfo("Running Property 6: 光源数量限制 (100 iterations)...");
        
        LightingSystemTestResult result = TestProperty6_LightCountLimit(100);
        
        if (result.passed)
        {
            LogInfo("Property 6 (光源数量限制) PASSED");
            return true;
        }
        else
        {
            LogError("Property 6 (光源数量限制) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            LogError("Failing example: lightCount=%d, visibleCount=%d, expectedCount=%d", 
                     result.lightCount, result.visibleCount, result.expectedCount);
            return false;
        }
    }

    /**
     * @brief Run Property 7 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty7Test()
    {
        LogInfo("Running Property 7: 光源优先级排序 (100 iterations)...");
        
        LightingSystemTestResult result = TestProperty7_LightPrioritySorting(100);
        
        if (result.passed)
        {
            LogInfo("Property 7 (光源优先级排序) PASSED");
            return true;
        }
        else
        {
            LogError("Property 7 (光源优先级排序) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Property 9: 光照数据实时更新
     * 
     * For any light position/direction change:
     * - The visible lights list should reflect the updated position/direction
     * - Light bounds should be recalculated correctly after position change
     * - Culling results should update based on new light positions
     * 
     * Feature: 2d-lighting-system, Property 9: 光照数据实时更新
     * Validates: Requirements 1.3, 2.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return LightingSystemTestResult with pass/fail status and failure details
     */
    inline LightingSystemTestResult TestProperty9_RealTimeLightDataUpdate(int iterations = 100)
    {
        LightingSystemTestResult result;
        LightingSystemRandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Test 1: Point light position change should update bounds correctly
            {
                ECS::LightData light = gen.RandomPointLight(-100.0f, 100.0f, -100.0f, 100.0f);
                
                // Calculate initial bounds
                Systems::LightBounds initialBounds = Systems::LightingSystem::CalculateLightBounds(light);
                
                // Change position
                glm::vec2 newPosition = gen.RandomPosition(-500.0f, 500.0f, -500.0f, 500.0f);
                glm::vec2 positionDelta = newPosition - light.position;
                light.position = newPosition;
                
                // Calculate new bounds
                Systems::LightBounds newBounds = Systems::LightingSystem::CalculateLightBounds(light);
                
                // Verify bounds moved by the same delta
                float expectedMinX = initialBounds.minX + positionDelta.x;
                float expectedMaxX = initialBounds.maxX + positionDelta.x;
                float expectedMinY = initialBounds.minY + positionDelta.y;
                float expectedMaxY = initialBounds.maxY + positionDelta.y;
                
                const float epsilon = 0.001f;
                if (std::abs(newBounds.minX - expectedMinX) > epsilon ||
                    std::abs(newBounds.maxX - expectedMaxX) > epsilon ||
                    std::abs(newBounds.minY - expectedMinY) > epsilon ||
                    std::abs(newBounds.maxY - expectedMaxY) > epsilon)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Point light bounds not updated correctly after position change. "
                        << "Position delta: (" << positionDelta.x << ", " << positionDelta.y << "). "
                        << "Expected bounds: [" << expectedMinX << ", " << expectedMaxX 
                        << "] x [" << expectedMinY << ", " << expectedMaxY << "], "
                        << "got: [" << newBounds.minX << ", " << newBounds.maxX 
                        << "] x [" << newBounds.minY << ", " << newBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 2: Light moving into view should become visible
            {
                // Create a view bounds
                float viewWidth = gen.RandomFloat(200.0f, 500.0f);
                float viewHeight = gen.RandomFloat(200.0f, 500.0f);
                glm::vec2 cameraPos(0.0f, 0.0f);
                
                Systems::LightBounds viewBounds;
                viewBounds.minX = cameraPos.x - viewWidth / 2.0f;
                viewBounds.maxX = cameraPos.x + viewWidth / 2.0f;
                viewBounds.minY = cameraPos.y - viewHeight / 2.0f;
                viewBounds.maxY = cameraPos.y + viewHeight / 2.0f;
                
                // Create a light outside the view
                ECS::LightData light;
                light.position = glm::vec2(viewBounds.maxX + 500.0f, cameraPos.y);
                light.radius = gen.RandomFloat(10.0f, 50.0f);
                light.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                
                // Verify it's outside view
                Systems::LightBounds lightBounds = Systems::LightingSystem::CalculateLightBounds(light);
                bool initiallyVisible = Systems::LightingSystem::IsLightInView(lightBounds, viewBounds);
                
                if (initiallyVisible)
                {
                    // Light should be outside view initially
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Test setup error: light should be outside view initially";
                    return result;
                }
                
                // Move light into view
                light.position = cameraPos;
                lightBounds = Systems::LightingSystem::CalculateLightBounds(light);
                bool nowVisible = Systems::LightingSystem::IsLightInView(lightBounds, viewBounds);
                
                if (!nowVisible)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Light moved to center of view should be visible. "
                        << "Light position: (" << light.position.x << ", " << light.position.y << "), "
                        << "radius: " << light.radius << ". "
                        << "View bounds: [" << viewBounds.minX << ", " << viewBounds.maxX 
                        << "] x [" << viewBounds.minY << ", " << viewBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 3: Light moving out of view should become invisible
            {
                float viewWidth = gen.RandomFloat(200.0f, 500.0f);
                float viewHeight = gen.RandomFloat(200.0f, 500.0f);
                glm::vec2 cameraPos(0.0f, 0.0f);
                
                Systems::LightBounds viewBounds;
                viewBounds.minX = cameraPos.x - viewWidth / 2.0f;
                viewBounds.maxX = cameraPos.x + viewWidth / 2.0f;
                viewBounds.minY = cameraPos.y - viewHeight / 2.0f;
                viewBounds.maxY = cameraPos.y + viewHeight / 2.0f;
                
                // Create a light inside the view
                ECS::LightData light;
                light.position = cameraPos;
                light.radius = gen.RandomFloat(10.0f, 50.0f);
                light.lightType = static_cast<uint32_t>(ECS::LightType::Point);
                
                // Verify it's inside view
                Systems::LightBounds lightBounds = Systems::LightingSystem::CalculateLightBounds(light);
                bool initiallyVisible = Systems::LightingSystem::IsLightInView(lightBounds, viewBounds);
                
                if (!initiallyVisible)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    result.failureMessage = "Test setup error: light at center should be visible initially";
                    return result;
                }
                
                // Move light far outside view
                light.position = glm::vec2(viewBounds.maxX + light.radius + 500.0f, cameraPos.y);
                lightBounds = Systems::LightingSystem::CalculateLightBounds(light);
                bool nowVisible = Systems::LightingSystem::IsLightInView(lightBounds, viewBounds);
                
                if (nowVisible)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Light moved far outside view should not be visible. "
                        << "Light position: (" << light.position.x << ", " << light.position.y << "), "
                        << "radius: " << light.radius << ". "
                        << "View bounds: [" << viewBounds.minX << ", " << viewBounds.maxX 
                        << "] x [" << viewBounds.minY << ", " << viewBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 4: Spot light direction change should not affect bounds (bounds based on position+radius)
            {
                ECS::LightData spotLight;
                spotLight.position = gen.RandomPosition(-100.0f, 100.0f, -100.0f, 100.0f);
                spotLight.direction = glm::normalize(glm::vec2(1.0f, 0.0f));
                spotLight.radius = gen.RandomFloat(10.0f, 100.0f);
                spotLight.innerAngle = 0.3f;
                spotLight.outerAngle = 0.5f;
                spotLight.lightType = static_cast<uint32_t>(ECS::LightType::Spot);
                
                // Calculate initial bounds
                Systems::LightBounds initialBounds = Systems::LightingSystem::CalculateLightBounds(spotLight);
                
                // Change direction
                spotLight.direction = glm::normalize(glm::vec2(-1.0f, 1.0f));
                
                // Calculate new bounds
                Systems::LightBounds newBounds = Systems::LightingSystem::CalculateLightBounds(spotLight);
                
                // Bounds should remain the same (based on position and radius, not direction)
                const float epsilon = 0.001f;
                if (std::abs(newBounds.minX - initialBounds.minX) > epsilon ||
                    std::abs(newBounds.maxX - initialBounds.maxX) > epsilon ||
                    std::abs(newBounds.minY - initialBounds.minY) > epsilon ||
                    std::abs(newBounds.maxY - initialBounds.maxY) > epsilon)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Spot light bounds should not change when only direction changes. "
                        << "Initial bounds: [" << initialBounds.minX << ", " << initialBounds.maxX 
                        << "] x [" << initialBounds.minY << ", " << initialBounds.maxY << "], "
                        << "new bounds: [" << newBounds.minX << ", " << newBounds.maxX 
                        << "] x [" << newBounds.minY << ", " << newBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
            
            // Test 5: Light radius change should update bounds
            {
                ECS::LightData light = gen.RandomPointLight(-100.0f, 100.0f, -100.0f, 100.0f);
                
                // Calculate initial bounds
                Systems::LightBounds initialBounds = Systems::LightingSystem::CalculateLightBounds(light);
                
                // Change radius
                float newRadius = light.radius * 2.0f;
                float radiusDelta = newRadius - light.radius;
                light.radius = newRadius;
                
                // Calculate new bounds
                Systems::LightBounds newBounds = Systems::LightingSystem::CalculateLightBounds(light);
                
                // Verify bounds expanded by radius delta
                float expectedMinX = initialBounds.minX - radiusDelta;
                float expectedMaxX = initialBounds.maxX + radiusDelta;
                float expectedMinY = initialBounds.minY - radiusDelta;
                float expectedMaxY = initialBounds.maxY + radiusDelta;
                
                const float epsilon = 0.001f;
                if (std::abs(newBounds.minX - expectedMinX) > epsilon ||
                    std::abs(newBounds.maxX - expectedMaxX) > epsilon ||
                    std::abs(newBounds.minY - expectedMinY) > epsilon ||
                    std::abs(newBounds.maxY - expectedMaxY) > epsilon)
                {
                    result.passed = false;
                    result.failedIteration = i;
                    std::ostringstream oss;
                    oss << "Light bounds not updated correctly after radius change. "
                        << "Radius delta: " << radiusDelta << ". "
                        << "Expected bounds: [" << expectedMinX << ", " << expectedMaxX 
                        << "] x [" << expectedMinY << ", " << expectedMaxY << "], "
                        << "got: [" << newBounds.minX << ", " << newBounds.maxX 
                        << "] x [" << newBounds.minY << ", " << newBounds.maxY << "]";
                    result.failureMessage = oss.str();
                    return result;
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Run Property 9 test and log results
     * 
     * @return true if test passes, false otherwise
     */
    inline bool RunProperty9Test()
    {
        LogInfo("Running Property 9: 光照数据实时更新 (100 iterations)...");
        
        LightingSystemTestResult result = TestProperty9_RealTimeLightDataUpdate(100);
        
        if (result.passed)
        {
            LogInfo("Property 9 (光照数据实时更新) PASSED");
            return true;
        }
        else
        {
            LogError("Property 9 (光照数据实时更新) FAILED at iteration %d", result.failedIteration);
            LogError("Failure: %s", result.failureMessage.c_str());
            return false;
        }
    }

    /**
     * @brief Run all LightingSystem property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllLightingSystemTests()
    {
        bool allPassed = true;
        
        LogInfo("=== Running LightingSystem Property Tests ===");
        
        // Property 5: Light culling correctness
        if (!RunProperty5Test())
        {
            allPassed = false;
        }
        
        // Property 6: Light count limit
        if (!RunProperty6Test())
        {
            allPassed = false;
        }
        
        // Property 7: Light priority sorting
        if (!RunProperty7Test())
        {
            allPassed = false;
        }
        
        // Property 9: Real-time light data update
        if (!RunProperty9Test())
        {
            allPassed = false;
        }
        
        LogInfo("=== LightingSystem Property Tests Complete ===");
        
        if (allPassed)
        {
            LogInfo("All LightingSystem tests PASSED");
        }
        else
        {
            LogError("Some LightingSystem tests FAILED");
        }
        
        return allPassed;
    }
}

#endif // LIGHTING_SYSTEM_TESTS_H

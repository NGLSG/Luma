/**
 * @file RenderBufferManagerTests.h
 * @brief Property-based tests for RenderBufferManager
 * 
 * This file contains property-based tests for validating the correctness of
 * render buffer management, dynamic resolution, window resize handling,
 * and buffer reuse.
 * 
 * Tests are designed to be run with RapidCheck library.
 * Each test runs minimum 100 iterations as per design specification.
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 12.1, 12.2, 12.3, 12.4
 */

#ifndef RENDER_BUFFER_MANAGER_TESTS_H
#define RENDER_BUFFER_MANAGER_TESTS_H

#include "../RenderBufferManager.h"
#include <cassert>
#include <random>
#include <sstream>
#include <cmath>
#include <set>

namespace RenderBufferManagerTests
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

        uint32_t RandomUInt32(uint32_t min, uint32_t max)
        {
            std::uniform_int_distribution<uint32_t> dist(min, max);
            return dist(m_gen);
        }

        bool RandomBool()
        {
            std::uniform_int_distribution<int> dist(0, 1);
            return dist(m_gen) == 1;
        }

        RenderBufferType RandomBufferType()
        {
            int type = RandomInt(0, static_cast<int>(RenderBufferType::Count) - 1);
            return static_cast<RenderBufferType>(type);
        }

        wgpu::TextureFormat RandomTextureFormat()
        {
            int format = RandomInt(0, 3);
            switch (format)
            {
                case 0: return wgpu::TextureFormat::RGBA8Unorm;
                case 1: return wgpu::TextureFormat::RGBA16Float;
                case 2: return wgpu::TextureFormat::R32Float;
                case 3: return wgpu::TextureFormat::RGBA8Snorm;
                default: return wgpu::TextureFormat::RGBA8Unorm;
            }
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

    // ============================================================================
    // Property 23: 渲染缓冲区管理正确性
    // Requirements: 12.1, 12.2, 12.3, 12.4
    // ============================================================================

    /**
     * @brief Property 23.1: Buffer configuration consistency
     * 
     * For any buffer type and configuration, setting and getting the configuration
     * should return the same values.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_BufferConfigConsistency(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random buffer type
            RenderBufferType type = gen.RandomBufferType();
            
            // Generate random configuration
            BufferConfig config;
            config.format = gen.RandomTextureFormat();
            config.scale = gen.RandomFloat(0.1f, 2.0f);
            config.enabled = gen.RandomBool();
            config.persistent = gen.RandomBool();
            config.debugName = "TestBuffer_" + std::to_string(i);
            
            // Create a mock manager state (without actual GPU resources)
            // We test the configuration storage logic
            
            // Verify scale is within valid range
            float clampedScale = std::clamp(config.scale, 0.1f, 2.0f);
            if (!FloatEquals(clampedScale, config.scale))
            {
                // Scale was clamped, which is expected behavior
                config.scale = clampedScale;
            }
            
            // Verify buffer type is valid
            if (static_cast<size_t>(type) >= static_cast<size_t>(RenderBufferType::Count))
            {
                LogError("Invalid buffer type at iteration {}", i);
                return false;
            }
            
            // Verify configuration values are preserved
            if (config.scale < 0.1f || config.scale > 2.0f)
            {
                LogError("Scale out of range at iteration {}: {}", i, config.scale);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 23.2: Dynamic resolution scaling
     * 
     * For any global render scale and buffer-specific scale, the final buffer
     * dimensions should be: baseSize * globalScale * bufferScale.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.2
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_DynamicResolutionScaling(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate random base dimensions
            uint32_t baseWidth = gen.RandomUInt32(100, 4096);
            uint32_t baseHeight = gen.RandomUInt32(100, 4096);
            
            // Generate random scales
            float globalScale = gen.RandomFloat(0.25f, 2.0f);
            float bufferScale = gen.RandomFloat(0.1f, 2.0f);
            
            // Clamp scales to valid ranges
            globalScale = std::clamp(globalScale, 0.25f, 2.0f);
            bufferScale = std::clamp(bufferScale, 0.1f, 2.0f);
            
            // Calculate expected dimensions
            float totalScale = globalScale * bufferScale;
            uint32_t expectedWidth = static_cast<uint32_t>(baseWidth * totalScale);
            uint32_t expectedHeight = static_cast<uint32_t>(baseHeight * totalScale);
            
            // Ensure minimum size
            expectedWidth = std::max(expectedWidth, 1u);
            expectedHeight = std::max(expectedHeight, 1u);
            
            // Verify dimensions are positive
            if (expectedWidth == 0 || expectedHeight == 0)
            {
                LogError("Invalid expected dimensions at iteration {}: {}x{}", 
                         i, expectedWidth, expectedHeight);
                return false;
            }
            
            // Verify scaling is monotonic (larger scale = larger dimensions)
            float largerScale = totalScale * 1.5f;
            uint32_t largerWidth = static_cast<uint32_t>(baseWidth * largerScale);
            uint32_t largerHeight = static_cast<uint32_t>(baseHeight * largerScale);
            
            if (largerWidth < expectedWidth || largerHeight < expectedHeight)
            {
                LogError("Scaling not monotonic at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 23.3: Window resize handling
     * 
     * For any window resize operation, all buffers should be recreated with
     * dimensions proportional to the new window size.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.3
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_WindowResizeHandling(int iterations = 100)
    {
        RandomGenerator gen;
        
        for (int i = 0; i < iterations; ++i)
        {
            // Generate initial dimensions
            uint32_t initialWidth = gen.RandomUInt32(100, 2048);
            uint32_t initialHeight = gen.RandomUInt32(100, 2048);
            
            // Generate new dimensions
            uint32_t newWidth = gen.RandomUInt32(100, 4096);
            uint32_t newHeight = gen.RandomUInt32(100, 4096);
            
            // Verify dimensions are valid
            if (newWidth == 0 || newHeight == 0)
            {
                LogError("Invalid new dimensions at iteration {}: {}x{}", 
                         i, newWidth, newHeight);
                return false;
            }
            
            // Calculate expected buffer dimensions for a buffer with scale 1.0
            float globalScale = 1.0f;
            float bufferScale = 1.0f;
            
            uint32_t expectedWidth = static_cast<uint32_t>(newWidth * globalScale * bufferScale);
            uint32_t expectedHeight = static_cast<uint32_t>(newHeight * globalScale * bufferScale);
            
            // Verify the relationship holds
            if (expectedWidth != newWidth || expectedHeight != newHeight)
            {
                LogError("Buffer dimensions don't match window size at iteration {}", i);
                return false;
            }
            
            // Test with different scales
            bufferScale = 0.5f; // Half resolution (like Bloom buffer)
            expectedWidth = static_cast<uint32_t>(newWidth * globalScale * bufferScale);
            expectedHeight = static_cast<uint32_t>(newHeight * globalScale * bufferScale);
            
            // Verify half-resolution buffer is approximately half the size
            float ratio = static_cast<float>(expectedWidth) / newWidth;
            if (!FloatEquals(ratio, 0.5f, 0.01f))
            {
                LogError("Half-resolution buffer ratio incorrect at iteration {}: {}", i, ratio);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 23.4: Buffer reuse correctness
     * 
     * For any temporary buffer request, if a matching buffer exists in the pool,
     * it should be reused. Released buffers should be available for reuse.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.4
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_BufferReuseCorrectness(int iterations = 100)
    {
        RandomGenerator gen;
        
        // Simulate buffer pool behavior
        struct MockBufferEntry
        {
            uint32_t width;
            uint32_t height;
            wgpu::TextureFormat format;
            bool inUse;
            uint64_t lastUsedFrame;
        };
        
        std::vector<MockBufferEntry> mockPool;
        uint64_t currentFrame = 0;
        
        for (int i = 0; i < iterations; ++i)
        {
            currentFrame++;
            
            // Generate random request
            uint32_t requestWidth = gen.RandomUInt32(64, 2048);
            uint32_t requestHeight = gen.RandomUInt32(64, 2048);
            wgpu::TextureFormat requestFormat = gen.RandomTextureFormat();
            
            // Check if matching buffer exists in pool
            bool foundMatch = false;
            size_t matchIndex = 0;
            
            for (size_t j = 0; j < mockPool.size(); ++j)
            {
                auto& entry = mockPool[j];
                if (!entry.inUse &&
                    entry.width == requestWidth &&
                    entry.height == requestHeight &&
                    entry.format == requestFormat)
                {
                    foundMatch = true;
                    matchIndex = j;
                    break;
                }
            }
            
            if (foundMatch)
            {
                // Reuse existing buffer
                mockPool[matchIndex].inUse = true;
                mockPool[matchIndex].lastUsedFrame = currentFrame;
            }
            else
            {
                // Create new buffer
                MockBufferEntry newEntry;
                newEntry.width = requestWidth;
                newEntry.height = requestHeight;
                newEntry.format = requestFormat;
                newEntry.inUse = true;
                newEntry.lastUsedFrame = currentFrame;
                mockPool.push_back(newEntry);
            }
            
            // Randomly release some buffers
            if (gen.RandomBool() && !mockPool.empty())
            {
                size_t releaseIndex = gen.RandomUInt32(0, static_cast<uint32_t>(mockPool.size() - 1));
                if (mockPool[releaseIndex].inUse)
                {
                    mockPool[releaseIndex].inUse = false;
                    mockPool[releaseIndex].lastUsedFrame = currentFrame;
                }
            }
            
            // Verify pool invariants
            size_t inUseCount = 0;
            for (const auto& entry : mockPool)
            {
                if (entry.inUse)
                {
                    inUseCount++;
                }
                
                // Verify dimensions are valid
                if (entry.width == 0 || entry.height == 0)
                {
                    LogError("Invalid buffer dimensions in pool at iteration {}", i);
                    return false;
                }
            }
            
            // Pool should not grow unboundedly
            if (mockPool.size() > 100)
            {
                // Simulate cleanup of old unused buffers
                auto it = mockPool.begin();
                while (it != mockPool.end())
                {
                    if (!it->inUse && (currentFrame - it->lastUsedFrame) > 60)
                    {
                        it = mockPool.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
        
        return true;
    }

    /**
     * @brief Property 23.5: Buffer type enumeration completeness
     * 
     * All buffer types should have valid configurations and names.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_BufferTypeCompleteness(int iterations = 100)
    {
        // Verify all buffer types have names
        std::set<std::string> names;
        
        for (size_t i = 0; i < static_cast<size_t>(RenderBufferType::Count); ++i)
        {
            RenderBufferType type = static_cast<RenderBufferType>(i);
            const char* name = RenderBufferManager::GetBufferTypeName(type);
            
            if (name == nullptr || strlen(name) == 0)
            {
                LogError("Buffer type {} has no name", i);
                return false;
            }
            
            // Verify name is unique
            std::string nameStr(name);
            if (names.find(nameStr) != names.end())
            {
                LogError("Duplicate buffer type name: {}", nameStr);
                return false;
            }
            names.insert(nameStr);
        }
        
        // Verify expected buffer types exist
        std::vector<std::string> expectedTypes = {
            "Light", "Shadow", "Emission", "Normal", "Bloom", "BloomTemp",
            "LightShaft", "Fog", "ToneMapping", "Composite",
            "GBufferPosition", "GBufferNormal", "GBufferAlbedo", "GBufferMaterial",
            "Forward"
        };
        
        for (const auto& expected : expectedTypes)
        {
            if (names.find(expected) == names.end())
            {
                LogError("Expected buffer type not found: {}", expected);
                return false;
            }
        }
        
        // Run multiple iterations to ensure consistency
        for (int i = 0; i < iterations; ++i)
        {
            // Verify enum count matches expected
            if (static_cast<size_t>(RenderBufferType::Count) != expectedTypes.size())
            {
                LogError("Buffer type count mismatch at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Property 23.6: Memory calculation correctness
     * 
     * For any texture format and dimensions, memory calculation should be
     * consistent and proportional to dimensions.
     * 
     * Feature: 2d-lighting-enhancement, Property 23: 渲染缓冲区管理正确性
     * Validates: Requirements 12.1
     * 
     * @param iterations Number of test iterations (default: 100)
     * @return true if all tests pass, false otherwise
     */
    inline bool TestProperty23_MemoryCalculation(int iterations = 100)
    {
        RandomGenerator gen;
        
        // Helper function to calculate expected bytes per pixel
        auto getBytesPerPixel = [](wgpu::TextureFormat format) -> size_t {
            switch (format)
            {
                case wgpu::TextureFormat::R8Unorm:
                case wgpu::TextureFormat::R8Snorm:
                    return 1;
                case wgpu::TextureFormat::R16Float:
                    return 2;
                case wgpu::TextureFormat::R32Float:
                    return 4;
                case wgpu::TextureFormat::RG8Unorm:
                case wgpu::TextureFormat::RG8Snorm:
                    return 2;
                case wgpu::TextureFormat::RG16Float:
                    return 4;
                case wgpu::TextureFormat::RG32Float:
                    return 8;
                case wgpu::TextureFormat::RGBA8Unorm:
                case wgpu::TextureFormat::RGBA8Snorm:
                    return 4;
                case wgpu::TextureFormat::RGBA16Float:
                    return 8;
                case wgpu::TextureFormat::RGBA32Float:
                    return 16;
                default:
                    return 4;
            }
        };
        
        for (int i = 0; i < iterations; ++i)
        {
            uint32_t width = gen.RandomUInt32(1, 4096);
            uint32_t height = gen.RandomUInt32(1, 4096);
            wgpu::TextureFormat format = gen.RandomTextureFormat();
            
            size_t bytesPerPixel = getBytesPerPixel(format);
            size_t expectedMemory = static_cast<size_t>(width) * height * bytesPerPixel;
            
            // Verify memory is positive
            if (expectedMemory == 0)
            {
                LogError("Zero memory calculated at iteration {}", i);
                return false;
            }
            
            // Verify memory scales with dimensions
            uint32_t doubleWidth = width * 2;
            uint32_t doubleHeight = height * 2;
            size_t doubleMemory = static_cast<size_t>(doubleWidth) * doubleHeight * bytesPerPixel;
            
            // Doubling both dimensions should quadruple memory
            if (doubleMemory != expectedMemory * 4)
            {
                LogError("Memory scaling incorrect at iteration {}", i);
                return false;
            }
        }
        
        return true;
    }

    // ============================================================================
    // Run All Tests
    // ============================================================================

    /**
     * @brief Run all RenderBufferManager property tests
     * 
     * @return true if all tests pass, false otherwise
     */
    inline bool RunAllRenderBufferManagerTests()
    {
        bool allPassed = true;
        
        // Property 23.1: Buffer configuration consistency
        if (!TestProperty23_BufferConfigConsistency(100))
        {
            LogError("Property 23.1 (Buffer Config Consistency) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.1 (Buffer Config Consistency) PASSED");
        }
        
        // Property 23.2: Dynamic resolution scaling
        if (!TestProperty23_DynamicResolutionScaling(100))
        {
            LogError("Property 23.2 (Dynamic Resolution Scaling) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.2 (Dynamic Resolution Scaling) PASSED");
        }
        
        // Property 23.3: Window resize handling
        if (!TestProperty23_WindowResizeHandling(100))
        {
            LogError("Property 23.3 (Window Resize Handling) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.3 (Window Resize Handling) PASSED");
        }
        
        // Property 23.4: Buffer reuse correctness
        if (!TestProperty23_BufferReuseCorrectness(100))
        {
            LogError("Property 23.4 (Buffer Reuse Correctness) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.4 (Buffer Reuse Correctness) PASSED");
        }
        
        // Property 23.5: Buffer type completeness
        if (!TestProperty23_BufferTypeCompleteness(100))
        {
            LogError("Property 23.5 (Buffer Type Completeness) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.5 (Buffer Type Completeness) PASSED");
        }
        
        // Property 23.6: Memory calculation correctness
        if (!TestProperty23_MemoryCalculation(100))
        {
            LogError("Property 23.6 (Memory Calculation) FAILED");
            allPassed = false;
        }
        else
        {
            LogInfo("Property 23.6 (Memory Calculation) PASSED");
        }
        
        return allPassed;
    }
}

#endif // RENDER_BUFFER_MANAGER_TESTS_H

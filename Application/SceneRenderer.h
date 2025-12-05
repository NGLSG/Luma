#ifndef SCENERENDERER_H
#define SCENERENDERER_H
#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <cstring>
#include "RenderComponent.h"
#include "include/core/SkImage.h"
enum class TextAlignment;
struct RenderPacket;
struct FastSpriteBatchKey
{
    uintptr_t imagePtr; 
    uintptr_t materialPtr; 
    uint32_t colorValue; 
    uint16_t settings; 
    uint32_t srcL; 
    uint32_t srcT; 
    uint32_t srcR; 
    uint32_t srcB; 
    uint32_t ppuBits; 
    int32_t zIndex;   
    size_t precomputedHash; 
    FastSpriteBatchKey() = default;
    FastSpriteBatchKey(SkImage* image, const Material* material,
                       const ECS::Color& color, ECS::FilterQuality filterQuality,
                       ECS::WrapMode wrapMode, const SkRect& srcRect,
                       float ppuScaleFactor, int z)
        : imagePtr(reinterpret_cast<uintptr_t>(image))
          , materialPtr(reinterpret_cast<uintptr_t>(material))
          , colorValue(packColor(color))
          , settings(packSettings(filterQuality, wrapMode))
          , srcL(floatBits(srcRect.fLeft))
          , srcT(floatBits(srcRect.fTop))
          , srcR(floatBits(srcRect.fRight))
          , srcB(floatBits(srcRect.fBottom))
          , ppuBits(floatBits(ppuScaleFactor))
          , zIndex(static_cast<int32_t>(z))
    {
        precomputedHash = computeHash();
    }
    bool operator==(const FastSpriteBatchKey& other) const
    {
        return imagePtr == other.imagePtr &&
            materialPtr == other.materialPtr &&
            colorValue == other.colorValue &&
            settings == other.settings &&
            srcL == other.srcL && srcT == other.srcT &&
            srcR == other.srcR && srcB == other.srcB &&
            ppuBits == other.ppuBits &&
            zIndex == other.zIndex;
    }
private:
    uint32_t packColor(const ECS::Color& color) const
    {
        return (static_cast<uint32_t>(color.r * 255) << 24) |
            (static_cast<uint32_t>(color.g * 255) << 16) |
            (static_cast<uint32_t>(color.b * 255) << 8) |
            static_cast<uint32_t>(color.a * 255);
    }
    uint16_t packSettings(ECS::FilterQuality filterQuality, ECS::WrapMode wrapMode) const
    {
        return (static_cast<uint16_t>(filterQuality) << 8) | static_cast<uint16_t>(wrapMode);
    }
    static uint32_t floatBits(float v)
    {
        uint32_t bits;
        static_assert(sizeof(float) == sizeof(uint32_t));
        std::memcpy(&bits, &v, sizeof(uint32_t));
        return bits;
    }
    size_t computeHash() const
    {
        size_t hash = imagePtr;
        hash ^= materialPtr + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= colorValue + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= settings + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= srcL + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= srcT + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= srcR + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= srcB + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= ppuBits + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= static_cast<size_t>(zIndex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};
struct FastTextBatchKey
{
    uintptr_t typefacePtr; 
    uint32_t fontSizeInt; 
    uint32_t colorValue; 
    uint8_t alignment; 
    int32_t zIndex;    
    size_t precomputedHash; 
    FastTextBatchKey() = default;
    FastTextBatchKey(SkTypeface* typeface, float fontSize,
                     TextAlignment alignment, const ECS::Color& color, int z)
        : typefacePtr(reinterpret_cast<uintptr_t>(typeface))
          , fontSizeInt(*reinterpret_cast<const uint32_t*>(&fontSize))
          , colorValue(packColor(color))
          , alignment(static_cast<uint8_t>(alignment))
          , zIndex(static_cast<int32_t>(z))
    {
        precomputedHash = computeHash();
    }
    bool operator==(const FastTextBatchKey& other) const
    {
        return typefacePtr == other.typefacePtr &&
            fontSizeInt == other.fontSizeInt &&
            colorValue == other.colorValue &&
            alignment == other.alignment &&
            zIndex == other.zIndex;
    }
private:
    uint32_t packColor(const ECS::Color& color) const
    {
        return (static_cast<uint32_t>(color.r * 255) << 24) |
            (static_cast<uint32_t>(color.g * 255) << 16) |
            (static_cast<uint32_t>(color.b * 255) << 8) |
            static_cast<uint32_t>(color.a * 255);
    }
    size_t computeHash() const
    {
        size_t hash = typefacePtr;
        hash ^= fontSizeInt + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= colorValue + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= alignment + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= static_cast<size_t>(zIndex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};
namespace std
{
    template <>
    struct hash<FastSpriteBatchKey>
    {
        size_t operator()(const FastSpriteBatchKey& key) const noexcept
        {
            return key.precomputedHash;
        }
    };
    template <>
    struct hash<FastTextBatchKey>
    {
        size_t operator()(const FastTextBatchKey& key) const noexcept
        {
            return key.precomputedHash;
        }
    };
}
template <typename T>
class FrameArena
{
private:
    std::vector<T> m_data; 
    size_t m_currentIndex = 0; 
public:
    FrameArena(size_t initialCapacity = 16384)
    {
        m_data.reserve(initialCapacity);
    }
    T* Allocate(size_t count)
    {
        if (m_currentIndex + count > m_data.capacity())
        {
            size_t newCapacity = std::max(m_currentIndex + count, m_data.capacity() * 3 / 2);
            m_data.reserve(newCapacity);
        }
        if (m_currentIndex + count > m_data.size())
        {
            m_data.resize(m_currentIndex + count);
        }
        T* ptr = &m_data[m_currentIndex];
        m_currentIndex += count;
        return ptr;
    }
    void Reserve(size_t total)
    {
        if (total > m_data.capacity())
        {
            m_data.reserve(total);
        }
    }
    void Reverse()
    {
        m_data.clear();
        m_currentIndex = 0;
    }
};
class SceneRenderer
{
public:
    SceneRenderer() = default;
    void Extract(entt::registry& registry, std::vector<RenderPacket>& outQueue);
    static void ExtractToRenderableManager(entt::registry& registry);
    struct BatchGroup
    {
        std::vector<RenderableTransform> transforms; 
        SkRect sourceRect; 
        int zIndex; 
        uint64_t sortKey = 0; 
        int filterQuality; 
        int wrapMode; 
        float ppuScaleFactor; 
        std::vector<std::string> texts; 
        SkImage* image = nullptr; 
        const Material* material = nullptr; 
        ECS::Color color; 
        SkTypeface* typeface = nullptr; 
        float fontSize = 0.0f; 
        TextAlignment alignment; 
    };
private:
    std::unique_ptr<FrameArena<RenderableTransform>> m_transformArena = std::make_unique<FrameArena<
        RenderableTransform>>(100000); 
    std::unique_ptr<FrameArena<std::string>> m_textArena = std::make_unique<FrameArena<std::string>>(4096);
    std::unordered_map<FastSpriteBatchKey, size_t> m_spriteGroupIndices; 
    std::unordered_map<FastTextBatchKey, size_t> m_textGroupIndices; 
    std::vector<BatchGroup> m_spriteBatchGroups; 
    std::vector<BatchGroup> m_textBatchGroups; 
};
#endif

#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <type_traits>
#include <unordered_map>

#include "RenderComponent.h"
#include "include/core/SkImage.h"

/**
 * @brief 文本对齐方式枚举。
 */
enum class TextAlignment;

/**
 * @brief 渲染数据包结构体。
 */
struct RenderPacket;

/**
 * @brief 用于快速精灵批处理的键。
 *
 * 该结构体封装了精灵渲染所需的关键属性，用于在批处理中进行分组和查找。
 */
struct FastSpriteBatchKey
{
    uintptr_t imagePtr; ///< 图像指针的整数表示。
    uintptr_t materialPtr; ///< 材质指针的整数表示。
    uint32_t colorValue; ///< 颜色值的打包表示。
    uint16_t settings; ///< 过滤质量和环绕模式的打包设置。

    size_t precomputedHash; ///< 预计算的哈希值。

    /**
     * @brief 默认构造函数。
     */
    FastSpriteBatchKey() = default;

    /**
     * @brief 构造函数，根据提供的渲染属性创建批处理键。
     *
     * @param image 图像对象指针。
     * @param material 材质对象指针。
     * @param color 颜色值。
     * @param filterQuality 过滤质量。
     * @param wrapMode 环绕模式。
     */
    FastSpriteBatchKey(SkImage* image, const Material* material,
                       const ECS::Color& color, ECS::FilterQuality filterQuality,
                       ECS::WrapMode wrapMode)
        : imagePtr(reinterpret_cast<uintptr_t>(image))
          , materialPtr(reinterpret_cast<uintptr_t>(material))
          , colorValue(packColor(color))
          , settings(packSettings(filterQuality, wrapMode))
    {
        precomputedHash = computeHash();
    }

    /**
     * @brief 比较两个FastSpriteBatchKey是否相等。
     *
     * @param other 另一个FastSpriteBatchKey对象。
     * @return 如果所有成员都相等则返回true，否则返回false。
     */
    bool operator==(const FastSpriteBatchKey& other) const
    {
        return imagePtr == other.imagePtr &&
            materialPtr == other.materialPtr &&
            colorValue == other.colorValue &&
            settings == other.settings;
    }

private:
    /**
     * @brief 将ECS::Color打包成一个32位整数。
     *
     * @param color 要打包的颜色。
     * @return 打包后的32位颜色整数。
     */
    uint32_t packColor(const ECS::Color& color) const
    {
        return (static_cast<uint32_t>(color.r * 255) << 24) |
            (static_cast<uint32_t>(color.g * 255) << 16) |
            (static_cast<uint32_t>(color.b * 255) << 8) |
            static_cast<uint32_t>(color.a * 255);
    }

    /**
     * @brief 将过滤质量和环绕模式打包成一个16位整数。
     *
     * @param filterQuality 过滤质量枚举值。
     * @param wrapMode 环绕模式枚举值。
     * @return 打包后的16位设置整数。
     */
    uint16_t packSettings(ECS::FilterQuality filterQuality, ECS::WrapMode wrapMode) const
    {
        return (static_cast<uint16_t>(filterQuality) << 8) | static_cast<uint16_t>(wrapMode);
    }

    /**
     * @brief 计算当前键的哈希值。
     *
     * @return 计算出的哈希值。
     */
    size_t computeHash() const
    {
        size_t hash = imagePtr;
        hash ^= materialPtr + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= colorValue + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= settings + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

/**
 * @brief 用于快速文本批处理的键。
 *
 * 该结构体封装了文本渲染所需的关键属性，用于在批处理中进行分组和查找。
 */
struct FastTextBatchKey
{
    uintptr_t typefacePtr; ///< 字体指针的整数表示。
    uint32_t fontSizeInt; ///< 字体大小的整数表示。
    uint32_t colorValue; ///< 颜色值的打包表示。
    uint8_t alignment; ///< 对齐方式的打包表示。
    size_t precomputedHash; ///< 预计算的哈希值。

    /**
     * @brief 默认构造函数。
     */
    FastTextBatchKey() = default;

    /**
     * @brief 构造函数，根据提供的文本渲染属性创建批处理键。
     *
     * @param typeface 字体对象指针。
     * @param fontSize 字体大小。
     * @param alignment 文本对齐方式。
     * @param color 颜色值。
     */
    FastTextBatchKey(SkTypeface* typeface, float fontSize,
                     TextAlignment alignment, const ECS::Color& color)
        : typefacePtr(reinterpret_cast<uintptr_t>(typeface))
          , fontSizeInt(*reinterpret_cast<const uint32_t*>(&fontSize))
          , colorValue(packColor(color))
          , alignment(static_cast<uint8_t>(alignment))
    {
        precomputedHash = computeHash();
    }

    /**
     * @brief 比较两个FastTextBatchKey是否相等。
     *
     * @param other 另一个FastTextBatchKey对象。
     * @return 如果所有成员都相等则返回true，否则返回false。
     */
    bool operator==(const FastTextBatchKey& other) const
    {
        return typefacePtr == other.typefacePtr &&
            fontSizeInt == other.fontSizeInt &&
            colorValue == other.colorValue &&
            alignment == other.alignment;
    }

private:
    /**
     * @brief 将ECS::Color打包成一个32位整数。
     *
     * @param color 要打包的颜色。
     * @return 打包后的32位颜色整数。
     */
    uint32_t packColor(const ECS::Color& color) const
    {
        return (static_cast<uint32_t>(color.r * 255) << 24) |
            (static_cast<uint32_t>(color.g * 255) << 16) |
            (static_cast<uint32_t>(color.b * 255) << 8) |
            static_cast<uint32_t>(color.a * 255);
    }

    /**
     * @brief 计算当前键的哈希值。
     *
     * @return 计算出的哈希值。
     */
    size_t computeHash() const
    {
        size_t hash = typefacePtr;
        hash ^= fontSizeInt + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= colorValue + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= alignment + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

namespace std
{
    /**
     * @brief FastSpriteBatchKey的哈希特化。
     */
    template <>
    struct hash<FastSpriteBatchKey>
    {
        /**
         * @brief 计算FastSpriteBatchKey的哈希值。
         *
         * @param key 要计算哈希的FastSpriteBatchKey对象。
         * @return 预计算的哈希值。
         */
        size_t operator()(const FastSpriteBatchKey& key) const noexcept
        {
            return key.precomputedHash;
        }
    };

    /**
     * @brief FastTextBatchKey的哈希特化。
     */
    template <>
    struct hash<FastTextBatchKey>
    {
        /**
         * @brief 计算FastTextBatchKey的哈希值。
         *
         * @param key 要计算哈希的FastTextBatchKey对象。
         * @return 预计算的哈希值。
         */
        size_t operator()(const FastTextBatchKey& key) const noexcept
        {
            return key.precomputedHash;
        }
    };
}

/**
 * @brief 一个简单的帧竞技场（Frame Arena）内存分配器。
 *
 * 该分配器用于在每一帧中快速分配和重置内存，适用于临时对象的存储。
 *
 * @tparam T 存储在竞技场中的数据类型。
 */
template <typename T>
class FrameArena
{
private:
    std::vector<T> m_data; ///< 存储数据的向量。
    size_t m_currentIndex = 0; ///< 当前分配的索引。

public:
    /**
     * @brief 构造函数，初始化竞技场。
     *
     * @param initialCapacity 竞技场的初始容量。
     */
    FrameArena(size_t initialCapacity = 16384)
    {
        m_data.reserve(initialCapacity);
    }

    /**
     * @brief 分配指定数量的T类型对象。
     *
     * 如果当前容量不足，会自动扩容。
     *
     * @param count 要分配的对象数量。
     * @return 指向分配内存起始位置的指针。
     */
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

    /**
     * @brief 重置竞技场，使其可以重新分配内存。
     *
     * 如果T不是平凡可析构类型，会调用已分配对象的析构函数。
     */
    void Reverse()
    {
        // Reset arena for the new frame while preserving capacity.
        // Use clear() so non-trivial types are properly destroyed and will be
        // reconstructed on the next Allocate() via resize().
        m_data.clear();
        m_currentIndex = 0;
    }
};

/**
 * @brief 场景渲染器类。
 *
 * 负责从ECS注册表中提取渲染相关组件，并组织成渲染数据包。
 */
class SceneRenderer
{
public:
    /**
     * @brief 默认构造函数。
     */
    SceneRenderer() = default;

    /**
     * @brief 从ECS注册表中提取渲染数据，并填充到输出队列中。
     *
     * @param registry ECS注册表实例。
     * @param outQueue 用于存储提取出的渲染数据包的向量。
     */
    void Extract(entt::registry& registry, std::vector<RenderPacket>& outQueue);

    static void ExtractToRenderableManager(entt::registry& registry);
    /**
     * @brief 批处理组结构体。
     *
     * 包含了一组具有相同渲染属性的渲染对象。
     */
    struct BatchGroup
    {
        std::vector<RenderableTransform> transforms; ///< 变换组件列表。
        SkRect sourceRect; ///< 源矩形。
        int zIndex; ///< Z轴深度索引。
        int filterQuality; ///< 过滤质量。
        int wrapMode; ///< 环绕模式。
        float ppuScaleFactor; ///< 每像素单位缩放因子。
        std::vector<std::string> texts; ///< 文本字符串列表。
        SkImage* image = nullptr; ///< 图像指针。
        const Material* material = nullptr; ///< 材质指针。
        ECS::Color color; ///< 颜色。
        SkTypeface* typeface = nullptr; ///< 字体指针。
        float fontSize = 0.0f; ///< 字体大小。
        TextAlignment alignment; ///< 文本对齐方式。
    };

private:
    std::unique_ptr<FrameArena<RenderableTransform>> m_transformArena = std::make_unique<FrameArena<
        RenderableTransform>>(100000); ///< 用于存储可渲染变换的帧竞技场。
    std::unique_ptr<FrameArena<std::string>> m_textArena = std::make_unique<FrameArena<std::string>>(4096);///< 用于存储文本字符串的帧竞技场。

    std::unordered_map<FastSpriteBatchKey, size_t> m_spriteGroupIndices; ///< 精灵批处理键到组索引的映射。
    std::unordered_map<FastTextBatchKey, size_t> m_textGroupIndices; ///< 文本批处理键到组索引的映射。


    std::vector<BatchGroup> m_spriteBatchGroups; ///< 精灵批处理组的向量。
    std::vector<BatchGroup> m_textBatchGroups; ///< 文本批处理组的向量。
};

#endif

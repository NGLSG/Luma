#ifndef META_H
#define META_H


#pragma once
#include <string>
#include <vector>
#include "../Utils/Guid.h"

/**
 * @brief 图像过滤模式。
 */
enum class ImageFilter
{
    Point,      ///< 点过滤。
    Linear,     ///< 线性过滤。
};

/**
 * @brief 图像环绕模式。
 */
enum class ImageWrapMode
{
    ClampToEdge,    ///< 钳制到边缘。
    Repeat,         ///< 重复。
    MirroredRepeat, ///< 镜像重复。
};

/**
 * @brief 图像压缩模式。
 */
enum class ImageCompression
{
    None,   ///< 无压缩。
    DXT1,   ///< DXT1 压缩。
    DXT5,   ///< DXT5 压缩。
    ASTC,   ///< ASTC 压缩。
};

/**
 * @brief 基础元数据结构。
 * 包含资源的通用信息，如GUID、依赖项、名称、哈希和文件路径。
 */
struct Meta
{
    Guid guid;                  ///< 资源的全局唯一标识符。

    std::vector<Guid> dependencies; ///< 资源的依赖项列表。
    std::string name;           ///< 资源的名称。
    std::string hash;           ///< 资源的哈希值。
    std::string filePath;       ///< 资源的文件路径。
    virtual ~Meta() = default;  ///< 默认虚析构函数。
};

/**
 * @brief 图像元数据结构。
 * 继承自基础元数据，并添加了图像特有的属性，如宽度、高度、过滤模式、环绕模式、压缩模式和图像数据。
 */
struct ImageMeta : public Meta
{
    int width;                  ///< 图像的宽度。
    int height;                 ///< 图像的高度。

    ImageFilter filter = ImageFilter::Point;            ///< 图像的过滤模式。
    ImageWrapMode wrapMode = ImageWrapMode::ClampToEdge;///< 图像的环绕模式。
    ImageCompression compression = ImageCompression::None;///< 图像的压缩模式。
    std::vector<uint8_t> data;  ///< 图像的原始数据。
};

#endif
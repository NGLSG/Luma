#ifndef SPRITE_H
#define SPRITE_H

#include <memory>
#include <string_view>
#include <vector>
#include <tuple>

#include "Meta.h"
#include "../Utils/Guid.h"
#include "include/core/SkImage.h"
#include "include/gpu/graphite/Recorder.h"

/**
 * @brief 表示一个精灵（Sprite）对象，通常用于渲染图像。
 *
 * 精灵封装了图像数据以及相关的加载和访问方法。
 */
class Sprite
{
public:
    /**
     * @brief 从文件路径加载一个精灵。
     *
     * @param recorder 用于图形操作的记录器。
     * @param filePath 图像文件的路径。
     * @param filter 图像过滤模式。
     * @param wrapMode 图像环绕模式。
     * @return 一个指向新加载精灵的唯一指针。如果加载失败，可能返回空指针。
     */
    static std::unique_ptr<Sprite> LoadFromFile(skgpu::graphite::Recorder* recorder, std::string_view filePath,
                                                ImageFilter filter = ImageFilter::Linear,
                                                ImageWrapMode wrapMode = ImageWrapMode::ClampToEdge);

    /**
     * @brief 从资源ID加载一个精灵。
     *
     * @param recorder 用于图形操作的记录器。
     * @param uid 资源的唯一标识符。
     * @return 一个指向新加载精灵的唯一指针。如果加载失败，可能返回空指针。
     */
    static std::unique_ptr<Sprite> LoadFromResource(skgpu::graphite::Recorder* recorder, const std::string& uid);

    /**
     * @brief 获取精灵的底层SkImage图像。
     *
     * @return 一个指向SkImage的智能指针。
     */
    sk_sp<SkImage> GetImage() const { return image; }

    /**
     * @brief 隐式转换为SkImage智能指针。
     *
     * 允许Sprite对象在需要SkImage的地方直接使用。
     * @return 一个指向SkImage的智能指针。
     */
    operator sk_sp<SkImage>() const { return image; }

    /**
     * @brief 获取一个默认的精灵实例。
     *
     * @return 一个指向默认精灵的共享指针。
     */
    static std::shared_ptr<Sprite> Default();

private:
    sk_sp<SkImage> image; ///< 精灵的底层SkImage图像。
    Guid uid; ///< 精灵的唯一标识符。
    ImageFilter filter; ///< 图像过滤模式。
    ImageWrapMode wrapMode; ///< 图像环绕模式。
};

#endif
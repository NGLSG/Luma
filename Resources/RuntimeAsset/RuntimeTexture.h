#ifndef LUMAENGINE_RUNTIMETEXTURE_H
#define LUMAENGINE_RUNTIMETEXTURE_H

#include "IRuntimeAsset.h"
#include "TextureImporterSettings.h"
#include "include/core/SkImage.h"
#include "Nut/TextureA.h"

/**
 * @brief 运行时纹理类，继承自运行时资产接口。
 *
 * 该类封装了一个Skia图像对象和其相关的导入设置，代表了在运行时可用的纹理资源。
 */
class RuntimeTexture : public IRuntimeAsset
{
private:
    sk_sp<SkImage> m_image; ///< Skia图像对象。
    TextureImporterSettings m_importSettings; ///< 纹理导入设置。
    std::shared_ptr<Nut::TextureA> m_nutTexture; ///< Nut图形纹理对象。

public:
    /**
     * @brief 默认构造函数。
     */
    RuntimeTexture() = default;

    /**
     * @brief 构造函数，用于创建运行时纹理实例。
     * @param sourceGuid 源资产的全局唯一标识符。
     * @param image Skia图像对象。
     * @param importSettings 纹理导入设置。
     * @param nutTexture Nut图形纹理对象。
     */
    RuntimeTexture(const Guid& sourceGuid, sk_sp<SkImage> image, TextureImporterSettings importSettings = {},
                   std::shared_ptr<Nut::TextureA>&& nutTexture = nullptr)
        : m_image(std::move(image)), m_importSettings(std::move(importSettings)), m_nutTexture(nutTexture)
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取纹理导入设置。
     * @return 纹理导入设置的常量引用。
     */
    const TextureImporterSettings& getImportSettings() const
    {
        return m_importSettings;
    }

    /**
     * @brief 获取Skia图像对象。
     * @return Skia图像对象的常量引用。
     */
    const sk_sp<SkImage>& getImage() const
    {
        return m_image;
    }

    const std::shared_ptr<Nut::TextureA>& getNutTexture() const
    {
        return m_nutTexture;
    }
};
#endif

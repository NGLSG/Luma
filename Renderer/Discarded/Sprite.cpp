#include "../Sprite.h"
#include "../Utils/Path.h"
#include "../Utils/Utils.h"
#include "include/core/SkData.h"
#include "include/gpu/graphite/Image.h"
#include "include/gpu/graphite/Image.h"
#include <stdexcept>

#include "ResourceManager.h"
#include "include/core/SkPixmap.h"


namespace
{
    ImageMeta CreateImageMeta(Guid uid, const std::string_view& path, sk_sp<SkData> encodedData,
                              ImageFilter filter, ImageWrapMode wrapMode)
    {
        ImageMeta meta;
        meta.guid = uid;

        meta.filter = filter;
        meta.wrapMode = wrapMode;
        meta.filePath = path;
        meta.name = Path::GetFileNameWithoutExtension(path);


        meta.data.assign(encodedData->bytes(), encodedData->bytes() + encodedData->size());

        meta.hash = Utils::GetHashFromFile(path.data());
        return meta;
    }
}


std::unique_ptr<Sprite> Sprite::LoadFromFile(skgpu::graphite::Recorder* recorder, std::string_view filePath,
                                             ImageFilter filter, ImageWrapMode wrapMode)
{
    if (!Path::Exists(filePath))
    {
        return nullptr;
    }


    sk_sp<SkData> encodedData = SkData::MakeFromFileName(filePath.data());
    if (!encodedData)
    {
        throw std::runtime_error("无法从文件加载数据: " + std::string(filePath));
    }


    sk_sp<SkImage> cpuImage = SkImages::DeferredFromEncodedData(encodedData);
    if (!cpuImage)
    {
        throw std::runtime_error("无法解码图像文件: " + std::string(filePath));
    }


    const bool needsMipmap = (filter == ImageFilter::Linear);
    const SkImage::RequiredProperties props = {.fMipmapped = needsMipmap};
    sk_sp<SkImage> gpuImage = SkImages::TextureFromImage(recorder, cpuImage.get(), props);
    if (!gpuImage)
    {
        throw std::runtime_error("无法从 CPU 图像创建 GPU 纹理: " + std::string(filePath));
    }


    auto sprite = std::make_unique<Sprite>();
    sprite->image = std::move(gpuImage);
    sprite->uid = Guid::NewGUID();
    sprite->filter = filter;
    sprite->wrapMode = wrapMode;


    ImageMeta meta = CreateImageMeta(sprite->uid, filePath, encodedData, filter, wrapMode);
    ResourceManager::GetInstance().AddResource(meta);

    return sprite;
}

std::unique_ptr<Sprite> Sprite::LoadFromResource(skgpu::graphite::Recorder* recorder, const std::string& uid)
{
    const ImageMeta* meta = ResourceManager::GetInstance().GetResource<ImageMeta>(uid);
    if (!meta)
    {
        return nullptr;
    }


    sk_sp<SkData> encodedData = SkData::MakeWithoutCopy(meta->data.data(), meta->data.size());


    sk_sp<SkImage> cpuImage = SkImages::DeferredFromEncodedData(encodedData);
    if (!cpuImage)
    {
        throw std::runtime_error("无法从资源缓存解码图像数据。");
    }


    const bool needsMipmap = (meta->filter == ImageFilter::Linear);
    const SkImage::RequiredProperties props = {.fMipmapped = needsMipmap};
    sk_sp<SkImage> gpuImage = SkImages::TextureFromImage(recorder, cpuImage.get(), props);
    if (!gpuImage)
    {
        throw std::runtime_error("无法从资源创建 GPU 纹理。");
    }


    auto sprite = std::make_unique<Sprite>();
    sprite->image = std::move(gpuImage);
    sprite->uid = meta->guid;
    sprite->filter = meta->filter;
    sprite->wrapMode = meta->wrapMode;

    return sprite;
}

std::shared_ptr<Sprite> Sprite::Default()
{
    static std::shared_ptr<Sprite> defaultSprite = []
    {
        auto sprite = std::make_shared<Sprite>();


        SkImageInfo info = SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        uint32_t whitePixel = 0xFFFFFFFF;
        SkPixmap pixmap(info, &whitePixel, info.minRowBytes());

        sprite->image = SkImages::RasterFromPixmap(pixmap, nullptr, nullptr);
        if (!sprite->image)
        {
            return std::shared_ptr<Sprite>(nullptr);
        }

        sprite->uid = Guid::NewGUID();
        sprite->filter = ImageFilter::Point;
        sprite->wrapMode = ImageWrapMode::ClampToEdge;

        return sprite;
    }();

    return defaultSprite;
}

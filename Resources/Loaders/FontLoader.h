#ifndef FONTLOADER_H
#define FONTLOADER_H

#include "IAssetLoader.h"
#include "yaml-cpp/yaml.h"
#include "include/core/SkData.h"
#include "include/core/SkTypeface.h"

/**
 * @brief 字体加载器，用于加载 SkTypeface 类型的字体资源。
 *
 * 该类实现了 IAssetLoader 接口，负责从各种来源加载字体数据并将其转换为 SkTypeface 对象。
 */
class FontLoader : IAssetLoader<SkTypeface>
{
private:
    /**
     * @brief 根据字体资源的元数据获取字体数据。
     * @param fontMetadata 字体资源的元数据。
     * @return 智能指针，指向字体数据。
     */
    static sk_sp<SkData> GetFontData(const AssetMetadata& fontMetadata);

public:
    /**
     * @brief 构造一个新的 FontLoader 实例。
     */
    FontLoader() = default;

    /**
     * @brief 根据资产元数据加载字体资源。
     * @param metadata 字体资源的元数据。
     * @return 智能指针，指向加载的字体类型对象。
     */
    sk_sp<SkTypeface> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载字体资源。
     * @param guid 字体资源的全局唯一标识符。
     * @return 智能指针，指向加载的字体类型对象。
     */
    sk_sp<SkTypeface> LoadAsset(const Guid& guid) override;
};

#endif
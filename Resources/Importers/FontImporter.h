#ifndef FONTIMPORTER_H
#define FONTIMPORTER_H

#include "IAssetImporter.h"
#include "include/core/SkData.h"
#include "include/core/SkTypeface.h"

/**
 * @brief 字体资产导入器。
 *
 * 负责处理字体文件的导入和重新导入，继承自IAssetImporter。
 */
class FontImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的字体文件扩展名列表。
     *
     * @return 支持的扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的字体资产。
     *
     * @param assetPath 字体资产的路径。
     * @return 导入后的资产元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入已存在的字体资产。
     *
     * @param metadata 需要重新导入的资产元数据。
     * @return 重新导入后的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;

private:
    // 私有方法不需要Doxygen注释，因为它们不是接口的一部分。
    void ExtractFontInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode);
};

#endif
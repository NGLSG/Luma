#ifndef TEXTUREIMPORTER_H
#define TEXTUREIMPORTER_H

#include "IAssetImporter.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"

/**
 * @brief 纹理导入器，用于导入纹理资产。
 *
 * 该类实现了IAssetImporter接口，专门处理纹理文件的导入和重新导入。
 */
class TextureImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 支持的文件扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的纹理资产。
     *
     * @param assetPath 纹理资产的路径。
     * @return 导入后的资产元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入已存在的纹理资产。
     *
     * @param metadata 需要重新导入的资产元数据。
     * @return 重新导入后的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;

private:
    // 私有方法，根据要求不添加Doxygen注释。
    void ExtractTextureInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode);
};


#endif
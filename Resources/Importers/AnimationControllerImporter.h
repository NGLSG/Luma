#ifndef ANIMATIONCONTROLLERIMPORTER_H
#define ANIMATIONCONTROLLERIMPORTER_H
#include "IAssetImporter.h"

/**
 * @brief 动画控制器资产导入器。
 *
 * 该类负责导入和重新导入动画控制器相关的资产文件。
 */
class AnimationControllerImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     * @return 支持的文件扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 从指定路径导入资产。
     * @param assetPath 资产文件的路径。
     * @return 导入资产的元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入资产。
     * @param metadata 待重新导入资产的现有元数据。
     * @return 重新导入后资产的元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};


#endif
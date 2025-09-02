#ifndef ANIMATIONCLIPIMPORTER_H
#define ANIMATIONCLIPIMPORTER_H
#include "IAssetImporter.h"

/**
 * @brief 动画剪辑资源导入器。
 *
 * 该类负责导入和重新导入动画剪辑资源文件，并提供其支持的文件扩展名列表。
 */
class AnimationClipImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     * @return 包含支持文件扩展名的字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 从指定路径导入资源。
     * @param assetPath 资源文件的路径。
     * @return 导入资源的元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入一个资源。
     * @param metadata 需要重新导入资源的元数据。
     * @return 重新导入资源的更新元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};


#endif
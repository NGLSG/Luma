#ifndef LUMAENGINE_TILEIMPORTER_H
#define LUMAENGINE_TILEIMPORTER_H

#include "IAssetImporter.h"

/**
 * @brief 瓦片资源导入器。
 *
 * 负责导入和管理瓦片相关的资源文件。
 */
class TileImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 支持的文件扩展名列表的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 从指定路径导入资源。
     *
     * @param assetPath 待导入资源的路径。
     * @return 导入资源的元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 根据现有元数据重新导入资源。
     *
     * @param metadata 待重新导入资源的现有元数据。
     * @return 重新导入资源的更新元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};

#endif
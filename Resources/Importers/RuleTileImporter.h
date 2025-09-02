#ifndef LUMAENGINE_RULETILEIMPORTER_H
#define LUMAENGINE_RULETILEIMPORTER_H

#include "IAssetImporter.h"

/**
 * @brief 规则瓦片资源导入器。
 *
 * 负责导入和管理规则瓦片（Rule Tile）类型的资源。
 */
class RuleTileImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 包含所有支持文件扩展名的字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 从指定路径导入资源。
     *
     * @param assetPath 待导入资源的完整文件路径。
     * @return 导入资源的元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 根据现有元数据重新导入资源。
     *
     * @param metadata 待重新导入资源的现有元数据。
     * @return 重新导入后更新的资源元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};


#endif
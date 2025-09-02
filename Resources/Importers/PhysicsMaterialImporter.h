#ifndef PHYSICSMATERIALIMPORTER_H
#define PHYSICSMATERIALIMPORTER_H
#include "IAssetImporter.h"

/**
 * @brief 物理材质导入器。
 *
 * 负责导入和重新导入物理材质相关的资产文件。
 */
class PhysicsMaterialImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 支持的文件扩展名列表的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的资产文件。
     *
     * @param assetPath 资产文件的路径。
     * @return 导入资产的元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入指定的资产。
     *
     * @param metadata 需要重新导入的资产元数据。
     * @return 重新导入后更新的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};


#endif
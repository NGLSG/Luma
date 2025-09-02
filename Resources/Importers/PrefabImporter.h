/**
 * @file PrefabImporter.h
 * @brief 预制件导入器头文件。
 */

#ifndef PREFABIMPORTER_H
#define PREFABIMPORTER_H


#include "IAssetImporter.h"

/**
 * @brief 预制件导入器类。
 *
 * 负责导入和重新导入预制件（Prefab）类型的资产。
 */
class PrefabImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 返回一个包含支持扩展名的字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的资产。
     *
     * @param assetPath 资产文件的路径。
     * @return 返回导入后的资产元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入已存在的资产。
     *
     * @param metadata 需要重新导入的资产的元数据。
     * @return 返回重新导入后的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};

#endif
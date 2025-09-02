#ifndef SCENEIMPORTER_H
#define SCENEIMPORTER_H


#include "IAssetImporter.h"

/**
 * @brief 场景导入器，用于导入场景资产。
 *
 * 该类实现了IAssetImporter接口，专门负责处理场景文件的导入和重新导入。
 */
class SceneImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 支持的文件扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的场景资产。
     *
     * @param assetPath 资产文件的文件系统路径。
     * @return 导入后生成的资产元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入已存在的场景资产。
     *
     * @param metadata 需要重新导入的资产的元数据。
     * @return 重新导入后更新的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};

#endif
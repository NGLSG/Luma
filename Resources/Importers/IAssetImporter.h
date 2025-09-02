#ifndef I_ASSET_IMPORTER_H
#define I_ASSET_IMPORTER_H
#include <fstream>

#include "AssetMetadata.h"
/**
 * @brief 资产导入器接口。
 *
 * 定义了所有资产导入器必须实现的通用接口。
 */
class IAssetImporter
{
public:
    /**
     * @brief 虚析构函数。
     *
     * 确保派生类对象能正确销毁。
     */
    virtual ~IAssetImporter();

    /**
     * @brief 获取当前导入器支持的文件扩展名列表。
     *
     * @return 一个包含所有支持扩展名（例如 ".png", ".fbx"）的字符串向量的常量引用。
     */
    virtual const std::vector<std::string>& GetSupportedExtensions() const = 0;

    /**
     * @brief 导入指定路径的资产。
     *
     * @param assetPath 待导入资产的完整文件路径。
     * @return 导入成功后生成的资产元数据。
     */
    virtual AssetMetadata Import(const std::filesystem::path& assetPath) = 0;

    /**
     * @brief 根据现有元数据重新导入资产。
     *
     * @param metadata 包含资产路径和现有信息的元数据。
     * @return 重新导入后更新的资产元数据。
     */
    virtual AssetMetadata Reimport(const AssetMetadata& metadata) = 0;

    /**
     * @brief 将资产元数据写入文件。
     *
     * 元数据文件通常与资产文件同名，但带有 ".meta" 扩展名。
     * @param metadata 要写入的资产元数据。
     */
    virtual void WriteMetadata(const AssetMetadata& metadata);

    /**
     * @brief 从指定路径的元数据文件加载资产元数据。
     *
     * @param metaFilePath 元数据文件的完整路径。
     * @return 从文件中加载的资产元数据。如果加载失败，返回一个空的 AssetMetadata 对象。
     */
    virtual AssetMetadata LoadMetadata(const std::filesystem::path& metaFilePath);
};
#endif
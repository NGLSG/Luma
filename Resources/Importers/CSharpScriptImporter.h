#ifndef CSHARPSCRIPTIMPORTER_H
#define CSHARPSCRIPTIMPORTER_H


#include "IAssetImporter.h"

/**
 * @brief C# 脚本导入器，用于处理和导入 C# 脚本文件。
 *
 * 该类实现了 IAssetImporter 接口，专门负责导入和重新导入 C# 脚本资产。
 */
class CSharpScriptImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 支持的文件扩展名列表的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的资产。
     *
     * @param assetPath 资产文件的路径。
     * @return 导入后的资产元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入给定的资产元数据。
     *
     * @param metadata 需要重新导入的资产元数据。
     * @return 重新导入后的资产元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;

private:
    /**
     * @brief 从 C# 脚本文件中提取脚本信息。
     *
     * @param assetPath C# 脚本文件的路径。
     * @param settingsNode 用于存储提取到的脚本信息的 YAML 节点。
     */
    void ExtractScriptInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode);
};


#endif
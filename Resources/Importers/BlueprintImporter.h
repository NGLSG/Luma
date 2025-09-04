#ifndef LUMAENGINE_BLUEPRINTIMPORTER_H
#define LUMAENGINE_BLUEPRINTIMPORTER_H

#include "IAssetImporter.h"

/**
 * @brief 蓝图资产导入器。
 *
 * 负责导入 .blueprint 文件，并将其解析生成对应的C#脚本。
 */
class BlueprintImporter : public IAssetImporter
{
public:
    const std::vector<std::string>& GetSupportedExtensions() const override;
    AssetMetadata Import(const std::filesystem::path& assetPath) override;
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};

#endif // LUMAENGINE_BLUEPRINTIMPORTER_H
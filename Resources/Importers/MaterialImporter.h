#pragma once

#include "IAssetImporter.h"
#include "../../Data/MaterialData.h"
#include "../../Renderer/RenderComponent.h"
#include <memory>

class AssetManager;

/**
 * @brief 材质资产导入器。
 *
 * 负责导入和重新导入材质相关的资产文件。
 */
class MaterialImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * @return 一个包含所有支持扩展名（如 ".mat"）的字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的资产文件。
     *
     * @param assetPath 资产文件的文件系统路径。
     * @return 包含导入资产元数据的结构体。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 根据现有元数据重新导入资产。
     *
     * 当资产文件发生变化时，可以使用此方法重新加载其内容。
     *
     * @param metadata 包含要重新导入资产信息的元数据。
     * @return 包含更新后资产元数据的结构体。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};
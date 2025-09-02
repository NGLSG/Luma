#ifndef LUMAENGINE_TILESETIMPORTER_H
#define LUMAENGINE_TILESETIMPORTER_H

#pragma once
#include "IAssetImporter.h"

/**
 * @brief 负责导入瓦片集资源的类。
 *
 * TilesetImporter 实现了 IAssetImporter 接口，用于处理瓦片集（Tileset）文件的导入和重新导入。
 */
class TilesetImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的文件扩展名列表。
     *
     * 此方法返回一个字符串向量，其中包含 TilesetImporter 能够处理的所有文件扩展名。
     * @return 支持的文件扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 从指定路径导入瓦片集资源。
     *
     * 此方法根据给定的文件系统路径导入一个瓦片集资源，并生成相应的资源元数据。
     * @param assetPath 待导入资源的路径。
     * @return 导入后生成的资源元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 根据现有元数据重新导入瓦片集资源。
     *
     * 此方法使用已有的资源元数据来重新导入瓦片集资源，通常用于更新或刷新资源。
     * @param metadata 待重新导入资源的现有元数据。
     * @return 重新导入后更新的资源元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};


#endif
#ifndef ASSETPACKER_H
#define ASSETPACKER_H
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include "../Resources/AssetMetadata.h"

/**
 * @brief 资产打包器，用于将资产打包或解包。
 *
 * 该类提供静态方法来处理资产的打包和解包操作，
 * 包括将资产元数据打包到指定路径，以及从包清单中解包元数据。
 */
class AssetPacker
{
public:
    /**
     * @brief 将资产数据库打包到指定输出路径。
     *
     * @param assetDatabase 资产元数据数据库，键为资产路径，值为资产元数据。
     * @param outputPath 打包文件的输出路径。
     * @param maxChunks 最大分块数量，用于将大文件分割成小块。
     * @return 如果打包成功则返回 true，否则返回 false。
     */
    static bool Pack(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                     const std::filesystem::path& outputPath,
                     int maxChunks = 8);

    /**
     * @brief 从指定包清单文件解包资产元数据。
     *
     * @param packageManifestPath 包清单文件的路径。
     * @return 解包后的资产元数据数据库，键为资产路径，值为资产元数据。
     */
    static std::unordered_map<std::string, AssetMetadata> Unpack(const std::filesystem::path& packageManifestPath);
};
#endif
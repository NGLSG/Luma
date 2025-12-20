#ifndef ASSETPACKER_H
#define ASSETPACKER_H
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include "../Utils/Guid.h"
#include "../Resources/AssetMetadata.h"

/**
 * @brief 资产索引条目,用于快速查找资产在打包文件中的位置
 */
struct AssetIndexEntry
{
    std::string guid;           ///< 资产的全局唯一标识符
    size_t offset;              ///< 资产数据在文件中的偏移量
    size_t size;                ///< 资产数据的大小(字节)
};

/**
 * @brief Addressables 索引,用于按地址或分组快速查找资产
 */
struct AddressablesIndex
{
    std::unordered_map<std::string, Guid> addressToGuid;
    std::unordered_map<std::string, std::vector<Guid>> groupToGuids;
};

/**
 * @brief 资产打包器,用于将资产打包或解包
 *
 * 该类提供静态方法来处理资产的打包和解包操作,
 * 包括将资产元数据打包到指定路径,以及从包清单中解包元数据
 */
class AssetPacker
{
public:
    /**
     * @brief 将资产数据库打包到指定输出路径
     *
     * @param assetDatabase 资产元数据数据库,键为资产路径,值为资产元数据
     * @param outputPath 打包文件的输出路径
     * @param maxChunks 最大分块数量,用于将大文件分割成小块
     * @return 如果打包成功则返回 true,否则返回 false
     */
    static bool Pack(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                     const std::filesystem::path& outputPath,
                     int maxChunks = 8);

    /**
     * @brief 从指定包清单文件解包资产元数据
     *
     * @param packageManifestPath 包清单文件的路径
     * @return 解包后的资产元数据数据库,键为资产路径,值为资产元数据
     */
    static std::unordered_map<std::string, AssetMetadata> Unpack(const std::filesystem::path& packageManifestPath);

    /**
     * @brief 从包中仅加载资产索引(不加载完整元数据)
     *
     * @param packageManifestPath 包清单文件的路径
     * @return 资产索引映射,键为GUID字符串,值为索引条目
     */
    static std::unordered_map<std::string, AssetIndexEntry> LoadIndex(const std::filesystem::path& packageManifestPath);

    /**
     * @brief 从包中加载单个资产的元数据
     *
     * @param packageManifestPath 包清单文件的路径
     * @param indexEntry 资产的索引条目
     * @return 资产元数据
     */
    static AssetMetadata LoadSingleAsset(const std::filesystem::path& packageManifestPath,
                                        const AssetIndexEntry& indexEntry);

    /**
     * @brief 读取 Addressables 索引
     *
     * @param packageManifestPath 包清单文件的路径
     * @param outIndex 输出 Addressables 索引
     * @return 如果索引存在且读取成功返回 true,否则返回 false
     */
    static bool TryLoadAddressablesIndex(const std::filesystem::path& packageManifestPath, AddressablesIndex& outIndex);

private:
    static bool SaveAddressablesIndex(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                                      const std::filesystem::path& outputPath);
    static std::vector<unsigned char> ReadChunkFile(const std::filesystem::path& chunkPath);
    static std::vector<unsigned char> LoadPackageData(const std::filesystem::path& packageManifestPath);
};
#endif

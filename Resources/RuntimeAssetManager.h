#ifndef RUNTIMEASSETMANAGER_H
#define RUNTIMEASSETMANAGER_H

#include "IAssetManager.h"

/**
 * @brief 运行时资产管理器，负责管理和查询游戏中的资产。
 *
 * 该类实现了 IAssetManager 接口，提供了在运行时访问和管理资产的功能。
 */
class RuntimeAssetManager : public IAssetManager
{
public:
    /**
     * @brief 构造函数，初始化运行时资产管理器。
     * @param packageManifestPath 包清单文件的路径。
     */
    explicit RuntimeAssetManager(const std::filesystem::path& packageManifestPath);

    /**
     * @brief 析构函数。
     */
    ~RuntimeAssetManager() override = default;

    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产的名称。
     * @param guid 资产的全局唯一标识符。
     * @return 资产的名称字符串。
     */
    std::string GetAssetName(const Guid& guid) const override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产的元数据。
     * @param guid 资产的全局唯一标识符。
     * @return 指向资产元数据的指针，如果未找到则可能为 nullptr。
     */
    const AssetMetadata* GetMetadata(const Guid& guid) const override;

    /**
     * @brief 根据资产路径获取资产的元数据。
     * @param assetPath 资产的文件系统路径。
     * @return 指向资产元数据的指针，如果未找到则可能为 nullptr。
     */
    const AssetMetadata* GetMetadata(const std::filesystem::path& assetPath) const override;

    /**
     * @brief 获取所有资产的数据库。
     * @return 包含所有资产元数据的无序映射的常量引用。
     */
    const std::unordered_map<std::string, AssetMetadata>& GetAssetDatabase() const override;

    /**
     * @brief 获取资产的根路径。
     * @return 资产根路径的常量引用。
     */
    const std::filesystem::path& GetAssetsRootPath() const override;

private:
    std::unordered_map<std::string, AssetMetadata> m_assetDatabase; ///< 存储所有资产元数据的数据库。
    std::filesystem::path m_dummyPath; ///< 一个占位符路径，可能用于某些内部操作。
};


#endif
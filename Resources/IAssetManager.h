/**
 * @file IAssetManager.h
 * @brief 定义了资产管理器接口。
 */
#ifndef IASSETMANAGER_H
#define IASSETMANAGER_H

#include "AssetMetadata.h"
#include "../Utils/Guid.h"
#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

/**
 * @brief 资产管理器接口。
 *
 * 定义了管理和访问资产的抽象接口，包括获取资产信息、元数据以及更新和重新导入资产的功能。
 */
class IAssetManager
{
public:
    /**
     * @brief 虚析构函数。
     *
     * 确保派生类实例能够正确销毁。
     */
    virtual ~IAssetManager() = default;

    /**
     * @brief 获取指定GUID的资产名称。
     * @param guid 资产的全局唯一标识符。
     * @return 资产的名称。
     */
    virtual std::string GetAssetName(const Guid& guid) const = 0;

    /**
     * @brief 根据GUID获取资产的元数据。
     * @param guid 资产的全局唯一标识符。
     * @return 指向资产元数据的常量指针，如果未找到则为nullptr。
     */
    virtual const AssetMetadata* GetMetadata(const Guid& guid) const = 0;

    /**
     * @brief 根据文件系统路径获取资产的元数据。
     * @param assetPath 资产的文件系统路径。
     * @return 指向资产元数据的常量指针，如果未找到则为nullptr。
     */
    virtual const AssetMetadata* GetMetadata(const std::filesystem::path& assetPath) const = 0;

    /**
     * @brief 获取整个资产数据库。
     * @return 资产数据库的常量引用，其中键通常是资产路径或名称，值是资产元数据。
     */
    virtual const std::unordered_map<std::string, AssetMetadata>& GetAssetDatabase() const = 0;

    /**
     * @brief 获取资产的根目录路径。
     * @return 资产根目录路径的常量引用。
     */
    virtual const std::filesystem::path& GetAssetsRootPath() const = 0;

    /**
     * @brief 更新资产管理器的内部状态。
     * @param deltaTime 自上次更新以来经过的时间。
     */
    virtual void Update(float deltaTime)
    {
    }

    /**
     * @brief 根据提供的元数据重新导入资产。
     * @param metadata 要重新导入的资产的元数据。
     */
    virtual void ReImport(const AssetMetadata& metadata)
    {
    }
};
#endif
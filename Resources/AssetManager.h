#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include "AssetMetadata.h"
#include "../Utils/Guid.h"
#include "../Utils/LazySingleton.h"
#include "Importers/IAssetImporter.h"

#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>

#pragma once

#include "IAssetManager.h"
#include "../Utils/LazySingleton.h"
#include <memory>
#include "../Data/EngineContext.h"

/**
 * @brief 资产管理器类。
 *
 * 负责管理游戏或应用程序中的所有资产，包括加载、获取元数据、重新导入等操作。
 * 这是一个懒汉式单例模式的实现。
 */
class AssetManager : public LazySingleton<AssetManager>
{
public:
    friend class LazySingleton<AssetManager>;
    /**
     * @brief 析构函数。
     *
     * 负责清理资产管理器占用的资源。
     */
    ~AssetManager() override;
    /**
     * @brief 禁用拷贝构造函数。
     *
     * 防止资产管理器对象被拷贝。
     */
    AssetManager(const AssetManager&) = delete;
    /**
     * @brief 禁用赋值运算符。
     *
     * 防止资产管理器对象被赋值。
     */
    AssetManager& operator=(const AssetManager&) = delete;

    /**
     * @brief 初始化资产管理器。
     *
     * @param mode 应用程序的运行模式。
     * @param path 资产的根目录路径。
     */
    void Initialize(ApplicationMode mode, const std::filesystem::path& path);

    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产名称。
     *
     * @param guid 资产的全局唯一标识符。
     * @return 资产的名称字符串。
     */
    std::string GetAssetName(const Guid& guid) const;
    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产元数据。
     *
     * @param guid 资产的全局唯一标识符。
     * @return 指向资产元数据的常量指针，如果未找到则为 nullptr。
     */
    const AssetMetadata* GetMetadata(const Guid& guid) const;
    /**
     * @brief 根据资产路径获取资产元数据。
     *
     * @param assetPath 资产的文件系统路径。
     * @return 指向资产元数据的常量指针，如果未找到则为 nullptr。
     */
    const AssetMetadata* GetMetadata(const std::filesystem::path& assetPath) const;
    /**
     * @brief 获取所有资产的数据库。
     *
     * @return 包含所有资产元数据的无序映射的常量引用。
     */
    const std::unordered_map<std::string, AssetMetadata>& GetAssetDatabase() const;
    /**
     * @brief 获取资产的根目录路径。
     *
     * @return 资产根目录路径的常量引用。
     */
    const std::filesystem::path& GetAssetsRootPath() const;
    /**
     * @brief 重新导入指定的资产。
     *
     * @param metadata 要重新导入的资产的元数据。
     */
    const void ReImport(const AssetMetadata& metadata);
    /**
     * @brief 更新资产管理器。
     *
     * @param deltaTime 自上次更新以来的时间间隔。
     */
    void Update(float deltaTime);
    /**
     * @brief 关闭资产管理器。
     *
     * 释放所有资源并执行必要的清理工作。
     */
    void Shutdown();

private:
    /**
     * @brief 私有默认构造函数。
     *
     * 确保资产管理器只能通过单例模式的 `GetInstance()` 方法创建。
     */
    AssetManager() = default;

    std::unique_ptr<IAssetManager> m_implementation; ///< 资产管理器接口的实现。
};
#endif
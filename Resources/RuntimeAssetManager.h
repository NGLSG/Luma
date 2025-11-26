#ifndef RUNTIMEASSETMANAGER_H
#define RUNTIMEASSETMANAGER_H

#include "IAssetManager.h"
#include "AssetPacker.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <utility>
#include <vector>
#include <memory>

class IAssetImporter;

/**
 * @brief 运行时资产管理器，负责管理和查询游戏中的资产
 *
 * 该类实现了 IAssetManager 接口，提供了在运行时访问和管理资产的功能
 * 使用懒加载策略，只在需要时加载完整的资产元数据
 * 可手动触发后台预加载所有资产以提升后续访问性能
 */
class RuntimeAssetManager : public IAssetManager
{
public:
    /**
     * @brief 构造函数，初始化运行时资产管理器
     * @param packageManifestPath 包清单文件的路径
     */
    explicit RuntimeAssetManager(const std::filesystem::path& packageManifestPath);

    /**
     * @brief 析构函数
     */
    ~RuntimeAssetManager() override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产的名称
     * @param guid 资产的全局唯一标识符
     * @return 资产的名称字符串
     */
    std::string GetAssetName(const Guid& guid) const override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 获取资产的元数据
     * @param guid 资产的全局唯一标识符
     * @return 指向资产元数据的指针，如果未找到则可能为 nullptr
     */
    const AssetMetadata* GetMetadata(const Guid& guid) const override;

    /**
     * @brief 根据资产路径获取资产的元数据
     * @param assetPath 资产的文件系统路径
     * @return 指向资产元数据的指针，如果未找到则可能为 nullptr
     */
    const AssetMetadata* GetMetadata(const std::filesystem::path& assetPath) const override;

    /**
     * @brief 获取所有资产的数据库
     * @return 包含所有资产元数据的无序映射的常量引用
     */
    const std::unordered_map<std::string, AssetMetadata>& GetAssetDatabase() const override;

    /**
     * @brief 获取资产的根路径
     * @return 资产根路径的常量引用
     */
    const std::filesystem::path& GetAssetsRootPath() const override;

    /**
     * @brief 手动启动后台预加载
     * @return 如果成功启动返回 true，如果已在运行或已完成返回 false
     */
    bool StartPreload() override;

    /**
     * @brief 停止后台预加载
     */
    void StopPreload() override;

    /**
     * @brief 获取预加载进度
     * @return pair<总数, 已处理数>
     */
    std::pair<int, int> GetPreloadProgress() const override;

    /**
     * @brief 检查预加载是否完成
     * @return 如果预加载完成返回 true，否则返回 false
     */
    bool IsPreloadComplete() const override;

    /**
     * @brief 检查预加载是否正在运行
     * @return 如果预加载正在运行返回 true，否则返回 false
     */
    bool IsPreloadRunning() const override;

    /**
     * @brief 根据资产路径加载资产并返回其GUID
     * @param assetPath 资产的文件系统路径
     * @return 资产的GUID
     */
    Guid LoadAsset(const std::filesystem::path& assetPath) override;

    /**
     * @brief 启动 shader 预热
     * 
     * Runtime 模式下从打包的 ShaderRegistry.yaml 加载并预热所有 shader
     * 
     * @return 如果成功启动返回 true，如果已在运行或已完成返回 false
     */
    bool StartPreWarmingShader() override;

    /**
     * @brief 停止 shader 预热
     */
    void StopPreWarmingShader() override;

    /**
     * @brief 获取 shader 预热进度
     * @return pair<总数, 已处理数>
     */
    std::pair<int, int> GetPreWarmingProgress() const override;

    /**
     * @brief 检查 shader 预热是否完成
     * @return 如果预热完成返回 true，否则返回 false
     */
    bool IsPreWarmingComplete() const override;

    /**
     * @brief 检查 shader 预热是否正在运行
     * @return 如果预热正在运行返回 true，否则返回 false
     */
    bool IsPreWarmingRunning() const override;

private:
    /**
     * @brief 懒加载指定GUID的资产元数据
     * @param guid 资产的GUID字符串
     * @return 指向资产元数据的指针
     */
    const AssetMetadata* lazyLoadMetadata(const std::string& guid) const;

    /**
     * @brief 后台预加载线程函数
     * @param threadIndex 线程索引
     * @param totalThreads 总线程数
     */
    void preloadWorker(size_t threadIndex, size_t totalThreads);

    /**
     * @brief Shader 预热工作线程函数
     */
    void preWarmingWorker();

    /**
     * @brief 注册所有可用的资产导入器
     */
    void registerImporters();

    /**
     * @brief 为给定文件路径查找合适的资产导入器
     * @param filePath 资产文件的路径
     * @return 指向找到的导入器的指针，如果未找到则为nullptr
     */
    IAssetImporter* findImporterFor(const std::filesystem::path& filePath);

private:
    std::filesystem::path m_packageManifestPath; ///< 包清单文件路径
    std::unordered_map<std::string, AssetIndexEntry> m_assetIndex; ///< 资产索引映射
    mutable std::unordered_map<std::string, AssetMetadata> m_assetCache; ///< 已加载的资产缓存
    mutable std::unordered_map<std::string, Guid> m_pathToGuid; ///< 路径到GUID的映射缓存
    mutable std::mutex m_cacheMutex; ///< 缓存互斥锁，用于线程安全
    std::filesystem::path m_dummyPath; ///< 占位符路径

    std::vector<std::thread> m_preloadThreads; ///< 后台预加载线程池
    std::atomic<bool> m_preloadRunning; ///< 预加载线程运行标志
    std::atomic<bool> m_preloadComplete; ///< 预加载完成标志
    std::atomic<int> m_preloadedCount; ///< 已预加载的资产数量

    std::vector<std::pair<std::string, AssetIndexEntry>> m_indexEntries; ///< 索引条目列表，用于分配任务

    std::vector<std::unique_ptr<IAssetImporter>> m_importers; ///< 注册的资产导入器列表

    // Shader 预热相关
    std::thread m_preWarmingThread; ///< Shader 预热线程
    std::atomic<bool> m_preWarmingRunning; ///< Shader 预热运行标志
    std::atomic<bool> m_preWarmingComplete; ///< Shader 预热完成标志
    std::atomic<int> m_preWarmingTotal; ///< Shader 总数
    std::atomic<int> m_preWarmingLoaded; ///< 已加载的 Shader 数量
};

#endif
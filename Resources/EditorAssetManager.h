#ifndef EDITORASSETMANAGER_H
#define EDITORASSETMANAGER_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "../Utils/Guid.h"
#include "AssetMetadata.h"
#include "IAssetManager.h"

class IAssetImporter;

/**
 * @brief 扫描结果结构体，用于存储资产扫描过程中发现的元数据和路径到GUID的映射。
 */
struct ScanResult
{
    std::unordered_map<std::string, AssetMetadata> guidToMeta; ///< GUID到资产元数据的映射。
    std::unordered_map<std::string, Guid> pathToGuid; ///< 资产路径到GUID的映射。
};

/**
 * @brief 编辑器资产管理器，负责管理项目中的所有资产，包括扫描、导入和提供资产元数据。
 *
 * 继承自IAssetManager，提供编辑器特有的资产管理功能。
 */
class EditorAssetManager : public IAssetManager
{
public:
    /**
     * @brief 构造函数，初始化编辑器资产管理器。
     * @param projectRootPath 项目的根路径。
     */
    explicit EditorAssetManager(const std::filesystem::path& projectRootPath);

    /**
     * @brief 析构函数，清理资源。
     */
    ~EditorAssetManager() override;

    // 禁用拷贝构造和赋值，禁用移动构造和赋值，确保单例或唯一实例行为。
    EditorAssetManager(const EditorAssetManager&) = delete;
    EditorAssetManager& operator=(const EditorAssetManager&) = delete;
    EditorAssetManager(EditorAssetManager&&) = delete;
    EditorAssetManager& operator=(EditorAssetManager&&) = delete;

    /**
     * @brief 更新资产管理器的状态。
     * @param deltaTime 自上次更新以来的时间间隔。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 根据GUID获取资产名称。
     * @param guid 资产的全局唯一标识符。
     * @return 资产的名称字符串。
     */
    std::string GetAssetName(const Guid& guid) const override;

    /**
     * @brief 根据GUID获取资产的元数据。
     * @param guid 资产的全局唯一标识符。
     * @return 指向资产元数据的指针，如果未找到则为nullptr。
     */
    const AssetMetadata* GetMetadata(const Guid& guid) const override;

    /**
     * @brief 执行初始资产扫描。
     *
     * 扫描项目资产目录，发现并处理所有资产文件。
     */
    void InitialScan();

    /**
     * @brief 根据资产路径获取资产的元数据。
     * @param assetPath 资产的完整文件系统路径。
     * @return 指向资产元数据的指针，如果未找到则为nullptr。
     */
    const AssetMetadata* GetMetadata(const std::filesystem::path& assetPath) const override;

    /**
     * @brief 获取当前资产数据库的只读视图。
     * @return 包含所有资产元数据的映射。
     */
    const std::unordered_map<std::string, AssetMetadata>& GetAssetDatabase() const override;

    /**
     * @brief 获取资产根目录的路径。
     * @return 资产根目录的路径。
     */
    const std::filesystem::path& GetAssetsRootPath() const override;

    /**
     * @brief 重新导入指定的资产。
     * @param metadata 需要重新导入的资产的元数据。
     */
    void ReImport(const AssetMetadata& metadata) override;
    /**
     * @brief 根据资产路径加载资产并返回其GUID
     * @param assetPath 资产的文件系统路径
     * @return 资产的GUID
     */
    Guid LoadAsset(const std::filesystem::path& assetPath) override;

private:
    /**
     * @brief 注册所有可用的资产导入器。
     */
    void RegisterImporters();

    /**
     * @brief 为给定文件路径查找合适的资产导入器。
     * @param filePath 资产文件的路径。
     * @return 指向找到的导入器的指针，如果未找到则为nullptr。
     */
    IAssetImporter* FindImporterFor(const std::filesystem::path& filePath);

    /**
     * @brief 扫描目录的任务函数，用于在单独的线程中执行。
     */
    void ScanDirectoryTask();

    /**
     * @brief 处理单个资产文件。
     * @param assetPath 资产文件的路径。
     * @param result 存储扫描结果的结构体。
     * @param resultMutex 保护扫描结果的互斥锁。
     */
    void ProcessAssetFile(const std::filesystem::path& assetPath, ScanResult& result, std::mutex& resultMutex);

    /**
     * @brief 工作线程循环，从任务队列中取出并执行任务。
     */
    void WorkerLoop();

    /**
     * @brief 扫描并注册所有 shader 资产到 ShaderRegistry
     */
    void RegisterShadersToRegistry();

    /**
     * @brief 烘焙所有 shader（Editor 模式预热）
     * @return 如果成功启动返回 true
     */
    bool StartPreWarmingShader() override;

    /**
     * @brief 停止 shader 预热
     */
    void StopPreWarmingShader() override;

    /**
     * @brief 获取 shader 预热进度
     */
    std::pair<int, int> GetPreWarmingProgress() const override;

    /**
     * @brief 检查 shader 预热是否完成
     */
    bool IsPreWarmingComplete() const override;

    /**
     * @brief 检查 shader 预热是否正在运行
     */
    bool IsPreWarmingRunning() const override;

private:
    std::filesystem::path m_assetsRoot; ///< 资产的根目录路径。
    std::vector<std::unique_ptr<IAssetImporter>> m_importers; ///< 注册的资产导入器列表。

    std::unordered_map<std::string, AssetMetadata> m_guidToMeta; ///< GUID到资产元数据的映射数据库。
    std::unordered_map<std::string, Guid> m_pathToGuid; ///< 资产路径到GUID的映射数据库。
    mutable std::mutex m_dbMutex; ///< 保护资产数据库的互斥锁。

    std::atomic<bool> m_isScanning = false; ///< 指示当前是否正在进行资产扫描。
    float m_rescanTimer = 0.0f; ///< 重新扫描计时器。
    static constexpr float RESCAN_INTERVAL = 0.f; ///< 重新扫描的时间间隔。

    std::unique_ptr<ScanResult> m_scanResult; ///< 存储最新扫描结果的智能指针。
    std::mutex m_scanResultMutex; ///< 保护扫描结果的互斥锁。

    std::vector<std::jthread> m_workerThreads; ///< 用于并行处理资产的工作线程。
    std::queue<std::function<void()>> m_taskQueue; ///< 存储待执行任务的队列。
    std::mutex m_taskQueueMutex; ///< 保护任务队列的互斥锁。
    std::condition_variable m_taskCondition; ///< 用于工作线程等待任务的条件变量。
    std::atomic<bool> m_stopThreads = false; ///< 原子标志，指示是否停止所有工作线程。

    // Shader 预热相关
    std::thread m_preWarmingThread; ///< Shader 预热线程
    std::atomic<bool> m_preWarmingRunning{false}; ///< Shader 预热运行标志
    std::atomic<bool> m_preWarmingComplete{false}; ///< Shader 预热完成标志
    std::atomic<int> m_preWarmingTotal{0}; ///< Shader 总数
    std::atomic<int> m_preWarmingLoaded{0}; ///< 已加载的 Shader 数量
};

#endif

#ifndef LUMAENGINE_SHADERREGISTRY_H
#define LUMAENGINE_SHADERREGISTRY_H

#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include <future>
#include "../../Components/AssetHandle.h"
#include "../../Utils/LazySingleton.h"

namespace Nut
{
    /**
     * @brief Shader 资产信息
     * 只存储 GUID，其他信息通过 AssetManager 获取
     */
    struct ShaderAssetInfo
    {
        AssetHandle assetHandle;       ///< 资产句柄（包含 GUID）
        bool isLoaded = false;         ///< 是否已加载到 GPU
        
        ShaderAssetInfo() = default;
        explicit ShaderAssetInfo(const AssetHandle& handle)
            : assetHandle(handle) {}
    };

    /**
     * @brief 预热状态
     */
    struct PreWarmingState
    {
        int total = 0;           ///< 总shader数量
        int loaded = 0;          ///< 已加载数量
        bool isRunning = false;  ///< 是否正在预热
        bool isComplete = false; ///< 是否已完成
    };

    /**
     * @brief Shader 注册表
     * 管理所有 shader 资产的引用，支持序列化和预热
     * 
     * 功能：
     * - 存储 shader 资产引用
     * - 序列化/反序列化到磁盘
     * - 异步预热（加载所有 shader 到 GPU）
     * - 轮询进度状态
     */
    class ShaderRegistry : public LazySingleton<ShaderRegistry>
    {
        friend class LazySingleton<ShaderRegistry>;
        
    public:
        
        /**
         * @brief 注册一个 shader 资产
         * @param assetHandle 资产句柄（包含 GUID）
         */
        void RegisterShader(const AssetHandle& assetHandle);
        
        /**
         * @brief 取消注册一个 shader 资产
         * @param assetHandle 资产句柄
         */
        void UnregisterShader(const AssetHandle& assetHandle);
        
        /**
         * @brief 清空所有注册的 shader
         */
        void Clear();
        
        /**
         * @brief 获取所有注册的 shader 资产
         * @return Shader 资产信息列表
         */
        std::vector<ShaderAssetInfo> GetAllShaders() const;
        
        /**
         * @brief 获取注册的 shader 数量
         * @return Shader 数量
         */
        size_t GetShaderCount() const;
        
        /**
         * @brief 检查 shader 是否已注册
         * @param assetHandle 资产句柄
         * @return 如果已注册返回 true
         */
        bool IsRegistered(const AssetHandle& assetHandle) const;
        
        /**
         * @brief 获取 shader 信息
         * @param assetHandle 资产句柄
         * @return Shader 信息指针，如果不存在返回 nullptr
         */
        const ShaderAssetInfo* GetShaderInfo(const AssetHandle& assetHandle) const;
        
        /**
         * @brief 标记 shader 为已加载
         * @param assetHandle 资产句柄
         */
        void MarkAsLoaded(const AssetHandle& assetHandle);
        
        /**
         * @brief 序列化到文件
         * @param filePath 文件路径
         * @return 成功返回 true
         */
        bool SaveToFile(const std::string& filePath) const;
        
        /**
         * @brief 从文件反序列化
         * @param filePath 文件路径
         * @return 成功返回 true
         */
        bool LoadFromFile(const std::string& filePath);
        
        /**
         * @brief 开始异步预热所有 shader
         * 
         * 工作流程：
         * 1. 将所有 shader 注册到 ShaderModuleRegistry
         * 2. 加载每个 shader 到 GPU 并触发缓存
         * 3. 通过 GetPreWarmingState() 轮询进度
         */
        void StartPreWarmingAsync();
        
        /**
         * @brief 同步预热所有 shader（阻塞直到完成）
         */
        void PreWarming();
        
        /**
         * @brief 获取预热状态（用于轮询）
         * @return 当前预热状态
         */
        PreWarmingState GetPreWarmingState() const;
        
        /**
         * @brief 检查预热是否正在运行
         */
        bool IsPreWarmingRunning() const;
        
        /**
         * @brief 检查预热是否已完成
         */
        bool IsPreWarmingComplete() const;
        
        /**
         * @brief 请求停止预热
         * 预热会在当前shader处理完后停止
         */
        void StopPreWarming();
        
    private:
        ShaderRegistry();
        ~ShaderRegistry() = default;
        ShaderRegistry(const ShaderRegistry&) = delete;
        ShaderRegistry& operator=(const ShaderRegistry&) = delete;
        
        void PreWarmingImpl();
        
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, ShaderAssetInfo> m_shaders;
        
        // 预热状态（原子操作保证线程安全）
        std::atomic<int> m_preWarmingTotal{0};
        std::atomic<int> m_preWarmingLoaded{0};
        std::atomic<bool> m_preWarmingRunning{false};
        std::atomic<bool> m_preWarmingComplete{false};
        std::atomic<bool> m_preWarmingStopRequested{false};
    };

} // namespace Nut

#endif //LUMAENGINE_SHADERREGISTRY_H

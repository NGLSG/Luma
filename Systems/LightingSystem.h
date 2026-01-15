#ifndef LIGHTING_SYSTEM_H
#define LIGHTING_SYSTEM_H

/**
 * @file LightingSystem.h
 * @brief 2D 光照系统
 * 
 * 负责管理光源、计算光照和渲染阴影。
 * 与现有的 WebGPU 渲染管线无缝集成。
 * 
 * Feature: 2d-lighting-system
 * Requirements: 1.1, 2.1, 3.1, 10.1, 10.2, 10.3, 8.3
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>

// Forward declaration
namespace Systems { class AreaLightSystem; }

namespace Systems
{
    /**
     * @brief 光源剔除边界框
     * 
     * 用于视锥剔除的 AABB 边界框
     */
    struct LightBounds
    {
        float minX;
        float minY;
        float maxX;
        float maxY;

        /**
         * @brief 检查两个边界框是否相交
         */
        bool Intersects(const LightBounds& other) const
        {
            return !(maxX < other.minX || minX > other.maxX ||
                     maxY < other.minY || minY > other.maxY);
        }
    };

    /**
     * @brief 光源信息结构（用于排序和剔除）
     */
    struct LightInfo
    {
        ECS::LightData data;           ///< 光源数据
        int priority;                   ///< 优先级
        float distanceToCamera;         ///< 到相机的距离
        bool isDirectional;             ///< 是否为方向光
    };

    /**
     * @brief 2D 光照系统
     * 
     * 该系统继承自 ISystem 接口，负责：
     * - 收集场景中的所有光源
     * - 执行视锥剔除
     * - 按优先级排序光源
     * - 更新 GPU 光照缓冲区
     */
    class LightingSystem : public ISystem
    {
    public:
        /// 每帧最大光源数量
        static constexpr uint32_t MAX_LIGHTS_PER_FRAME = 128;

        /**
         * @brief 默认构造函数
         */
        LightingSystem();

        /**
         * @brief 析构函数
         */
        ~LightingSystem() override = default;

        /**
         * @brief 系统创建时调用
         * 
         * 初始化光照系统所需的资源，包括 GPU 缓冲区。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * 
         * 收集光源、执行剔除、排序并更新 GPU 缓冲区。
         * 
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * 
         * 清理光照系统占用的资源。
         * 
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 光照数据访问 ====================

        /**
         * @brief 获取可见光源列表
         * @return 可见光源数据的常量引用
         */
        const std::vector<ECS::LightData>& GetVisibleLights() const { return m_visibleLights; }

        /**
         * @brief 获取当前光源数量
         * @return 可见光源数量
         */
        uint32_t GetLightCount() const { return static_cast<uint32_t>(m_visibleLights.size()); }

        /**
         * @brief 获取光照设置
         * @return 光照设置的常量引用
         */
        const ECS::LightingSettingsData& GetSettings() const { return m_settings; }

        /**
         * @brief 获取光照全局数据（GPU 格式）
         * @return 光照全局数据
         */
        ECS::LightingGlobalData GetGlobalData() const;

        // ==================== 光源管理 ====================

        /**
         * @brief 设置环境光
         * @param color 环境光颜色
         * @param intensity 环境光强度
         */
        void SetAmbientLight(const ECS::Color& color, float intensity);

        /**
         * @brief 启用/禁用阴影
         * @param enable 是否启用阴影
         */
        void EnableShadows(bool enable);

        /**
         * @brief 设置每像素最大光源数
         * @param maxLights 最大光源数
         */
        void SetMaxLightsPerPixel(int maxLights);

        // ==================== 调试 ====================

        /**
         * @brief 设置调试模式
         * @param enable 是否启用调试模式
         */
        void SetDebugMode(bool enable) { m_debugMode = enable; }

        /**
         * @brief 获取调试模式状态
         * @return 是否处于调试模式
         */
        bool IsDebugMode() const { return m_debugMode; }

        // ==================== 优化控制 ====================

        /**
         * @brief 启用/禁用分帧更新
         * @param enable 是否启用分帧更新
         */
        void SetFrameDistributedUpdateEnabled(bool enable) { m_enableFrameDistributedUpdate = enable; }

        /**
         * @brief 获取分帧更新是否启用
         * @return 是否启用分帧更新
         */
        bool IsFrameDistributedUpdateEnabled() const { return m_enableFrameDistributedUpdate; }

        /**
         * @brief 强制下一帧更新所有光照数据
         */
        void ForceUpdate() { m_forceUpdate = true; }

        /**
         * @brief 检查光照数据是否为脏（需要更新）
         * @return true 如果需要更新
         */
        bool IsDirty() const { return m_isDirty; }

        /**
         * @brief 获取分帧更新进度
         * @return 进度值 [0, 1]，1 表示完成
         */
        float GetFrameUpdateProgress() const;

        // ==================== GPU 缓冲区访问 ====================

        /**
         * @brief 获取光照数据缓冲区
         * @return 光照数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetLightBuffer() const { return m_lightBuffer; }

        /**
         * @brief 获取光照全局数据缓冲区
         * @return 光照全局数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetGlobalBuffer() const { return m_globalBuffer; }

        // ==================== 面光源集成 ====================

        /**
         * @brief 设置面光源系统引用
         * @param areaLightSystem 面光源系统指针
         */
        void SetAreaLightSystem(AreaLightSystem* areaLightSystem) { m_areaLightSystem = areaLightSystem; }

        /**
         * @brief 获取面光源系统引用
         * @return 面光源系统指针
         */
        AreaLightSystem* GetAreaLightSystem() const { return m_areaLightSystem; }

        /**
         * @brief 获取面光源数据缓冲区
         * @return 面光源数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetAreaLightBuffer() const { return m_areaLightBuffer; }

        /**
         * @brief 获取面光源全局数据缓冲区
         * @return 面光源全局数据缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetAreaLightGlobalBuffer() const { return m_areaLightGlobalBuffer; }

        /**
         * @brief 获取当前面光源数量
         * @return 面光源数量
         */
        uint32_t GetAreaLightCount() const;

        // ==================== 阴影方法控制 (Requirements: 7.5) ====================

        /**
         * @brief 设置阴影计算方法
         * @param method 阴影方法（Basic, SDF, ScreenSpace）
         * 
         * 运行时切换阴影方法，确保平滑过渡无跳变。
         * Requirements: 7.5
         */
        void SetShadowMethod(ECS::ShadowMethod method);

        /**
         * @brief 获取当前阴影计算方法
         * @return 当前阴影方法
         */
        ECS::ShadowMethod GetShadowMethod() const;

        /**
         * @brief 检查是否支持指定的阴影方法
         * @param method 阴影方法
         * @return 是否支持
         */
        bool IsShadowMethodSupported(ECS::ShadowMethod method) const;

        // ==================== 静态工具函数（用于测试） ====================

        /**
         * @brief 计算光源的边界框
         * @param light 光源数据
         * @return 光源边界框
         */
        static LightBounds CalculateLightBounds(const ECS::LightData& light);

        /**
         * @brief 检查光源是否在视锥内
         * @param lightBounds 光源边界框
         * @param viewBounds 视锥边界框
         * @return true 如果光源在视锥内
         */
        static bool IsLightInView(const LightBounds& lightBounds, const LightBounds& viewBounds);

        /**
         * @brief 按优先级和距离排序光源
         * @param lights 光源信息列表
         */
        static void SortLightsByPriority(std::vector<LightInfo>& lights);

        /**
         * @brief 限制光源数量
         * @param lights 光源信息列表
         * @param maxCount 最大数量
         */
        static void LimitLightCount(std::vector<LightInfo>& lights, uint32_t maxCount);

    private:
        /**
         * @brief 收集场景中的所有光源
         * @param scene 运行时场景
         */
        void CollectLights(RuntimeScene* scene);

        /**
         * @brief 执行视锥剔除
         * @param cameraPosition 相机位置
         * @param viewWidth 视图宽度
         * @param viewHeight 视图高度
         */
        void CullLights(const glm::vec2& cameraPosition, float viewWidth, float viewHeight);

        /**
         * @brief 更新 GPU 光照缓冲区
         */
        void UpdateLightBuffer();

        /**
         * @brief 从场景获取光照设置
         * @param scene 运行时场景
         */
        void UpdateSettingsFromScene(RuntimeScene* scene);

        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateBuffers(EngineContext& engineCtx);

        /**
         * @brief 更新面光源 GPU 缓冲区
         */
        void UpdateAreaLightBuffer();

    private:
        std::vector<LightInfo> m_allLights;              ///< 所有收集到的光源
        std::vector<ECS::LightData> m_visibleLights;     ///< 可见光源数据
        ECS::LightingSettingsData m_settings;            ///< 光照设置

        std::shared_ptr<Nut::Buffer> m_lightBuffer;      ///< GPU 光源数据缓冲区
        std::shared_ptr<Nut::Buffer> m_globalBuffer;     ///< GPU 全局光照数据缓冲区

        // ==================== 面光源集成 ====================
        
        AreaLightSystem* m_areaLightSystem = nullptr;    ///< 面光源系统引用
        std::shared_ptr<Nut::Buffer> m_areaLightBuffer;  ///< GPU 面光源数据缓冲区
        std::shared_ptr<Nut::Buffer> m_areaLightGlobalBuffer; ///< GPU 面光源全局数据缓冲区

        bool m_debugMode = false;                        ///< 调试模式标志
        bool m_buffersCreated = false;                   ///< 缓冲区是否已创建
        bool m_isDirty = true;                           ///< 光照数据是否需要更新

        // ==================== 脏标记优化 ====================
        
        /// 上一帧的光源数据哈希值（用于检测变化）
        size_t m_lastLightDataHash = 0;
        
        /// 上一帧的光源数量
        uint32_t m_lastLightCount = 0;
        
        /// 上一帧的设置哈希值
        size_t m_lastSettingsHash = 0;
        
        /// 强制更新标志（用于首次更新或配置变化）
        bool m_forceUpdate = true;

        // ==================== 分帧更新优化 ====================
        
        /// 每帧最大更新光源数量（用于分帧更新）
        static constexpr uint32_t MAX_LIGHTS_PER_FRAME_UPDATE = 32;
        
        /// 当前分帧更新的起始索引
        uint32_t m_frameUpdateStartIndex = 0;
        
        /// 是否启用分帧更新
        bool m_enableFrameDistributedUpdate = true;
        
        /// 待更新的光源数据缓存
        std::vector<ECS::LightData> m_pendingLightUpdates;
        
        /// 分帧更新是否完成
        bool m_frameUpdateComplete = true;

        // ==================== 私有辅助函数 ====================
        
        /**
         * @brief 计算光源数据的哈希值
         * @return 哈希值
         */
        size_t CalculateLightDataHash() const;
        
        /**
         * @brief 计算设置的哈希值
         * @return 哈希值
         */
        size_t CalculateSettingsHash() const;
        
        /**
         * @brief 检查光照数据是否发生变化
         * @return true 如果数据发生变化
         */
        bool HasLightDataChanged() const;
        
        /**
         * @brief 执行分帧更新
         * @return true 如果本帧更新完成
         */
        bool PerformFrameDistributedUpdate();
    };
}

#endif // LIGHTING_SYSTEM_H

#ifndef SHADOW_RENDERER_H
#define SHADOW_RENDERER_H

/**
 * @file ShadowRenderer.h
 * @brief 2D 阴影渲染器
 * 
 * 负责管理阴影投射器、生成阴影贴图和渲染 2D 阴影。
 * 使用射线投射算法生成阴影体。
 * 支持 SDF 阴影、屏幕空间阴影和阴影缓存。
 * 
 * Feature: 2d-lighting-system, 2d-lighting-enhancement
 * Requirements: 5.1, 5.3, 7.1, 7.2, 7.3, 7.4, 7.5, 7.6
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Components/ShadowCasterComponent.h"
#include "../Renderer/Nut/Buffer.h"
#include "../Renderer/Nut/TextureA.h"
#include "../Renderer/Nut/RenderTarget.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Systems
{
    /**
     * @brief 阴影投射器信息
     * 
     * 存储阴影投射器的几何信息和变换数据
     */
    struct ShadowCasterInfo
    {
        std::vector<glm::vec2> vertices;    ///< 阴影投射器顶点（世界坐标）
        glm::vec2 position;                  ///< 世界位置
        float opacity;                       ///< 阴影不透明度
        bool selfShadow;                     ///< 是否自阴影
        entt::entity entity;                 ///< 关联的实体
    };

    /**
     * @brief 阴影边缘结构
     * 
     * 表示阴影投射器的一条边
     */
    struct ShadowEdge
    {
        glm::vec2 start;    ///< 边的起点
        glm::vec2 end;      ///< 边的终点
    };

    /**
     * @brief 阴影体顶点数据（GPU 传输用）
     */
    struct alignas(16) ShadowVertex
    {
        glm::vec2 position;     ///< 顶点位置
        glm::vec2 lightDir;     ///< 光源方向（用于软阴影）
        float opacity;          ///< 阴影不透明度
        float padding[3];       ///< 对齐填充
    };

    /**
     * @brief 光源阴影数据
     * 
     * 存储单个光源的阴影贴图和相关数据
     */
    struct LightShadowData
    {
        Nut::TextureAPtr shadowMap;         ///< 阴影贴图
        glm::vec2 lightPosition;            ///< 光源位置
        float lightRadius;                  ///< 光源半径
        bool isDirty;                       ///< 是否需要更新
    };

    /**
     * @brief 阴影边缘数据（GPU 传输用，与 shader 对齐）
     */
    struct alignas(16) GPUShadowEdge
    {
        glm::vec2 start;        ///< 边的起点
        glm::vec2 end;          ///< 边的终点
        glm::vec2 boundsMin;    ///< 投射器包围盒最小点
        glm::vec2 boundsMax;    ///< 投射器包围盒最大点
        uint32_t selfShadow;    ///< 是否允许自阴影 (0 = false, 1 = true)
        float opacity;          ///< 阴影不透明度
        float padding[2];       ///< 对齐填充
    };

    /**
     * @brief 阴影参数（GPU uniform）
     */
    struct alignas(16) ShadowParams
    {
        uint32_t edgeCount;         ///< 边缘数量
        float shadowSoftness;       ///< 软阴影程度
        float shadowBias;           ///< 阴影偏移
        float padding;              ///< 对齐填充
    };

    /**
     * @brief 2D 阴影渲染器
     * 
     * 该类负责：
     * - 收集场景中的阴影投射器
     * - 为每个光源生成阴影贴图
     * - 使用 2D 射线投射算法计算阴影
     */
    class ShadowRenderer : public ISystem
    {
    public:
        /// 默认阴影贴图分辨率
        static constexpr uint32_t DEFAULT_SHADOW_MAP_RESOLUTION = 1024;
        /// 最大阴影投射器数量
        static constexpr uint32_t MAX_SHADOW_CASTERS = 64;
        /// 阴影射线数量（用于软阴影）
        static constexpr uint32_t SHADOW_RAY_COUNT = 360;

        /**
         * @brief 默认构造函数
         */
        ShadowRenderer();

        /**
         * @brief 析构函数
         */
        ~ShadowRenderer() override = default;

        /**
         * @brief 系统创建时调用
         * @param scene 指向当前运行时场景的指针
         * @param engineCtx 引擎上下文
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         * @param scene 指向当前运行时场景的指针
         * @param deltaTime 自上一帧以来的时间间隔（秒）
         * @param engineCtx 引擎上下文
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         * @param scene 指向当前运行时场景的指针
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 阴影数据访问 ====================

        /**
         * @brief 获取阴影投射器列表
         * @return 阴影投射器信息的常量引用
         */
        const std::vector<ShadowCasterInfo>& GetShadowCasters() const { return m_shadowCasters; }

        /**
         * @brief 获取阴影投射器数量
         * @return 阴影投射器数量
         */
        uint32_t GetShadowCasterCount() const { return static_cast<uint32_t>(m_shadowCasters.size()); }

        /**
         * @brief 获取指定光源的阴影贴图
         * @param lightIndex 光源索引
         * @return 阴影贴图纹理指针，如果不存在则返回 nullptr
         */
        Nut::TextureAPtr GetShadowMap(uint32_t lightIndex) const;

        /**
         * @brief 获取合并的阴影贴图（用于着色器采样）
         * @return 合并阴影贴图纹理指针
         */
        Nut::TextureAPtr GetCombinedShadowMap() const { return m_combinedShadowMap; }

        // ==================== GPU 缓冲区访问 ====================

        /**
         * @brief 获取阴影边缘缓冲区
         * @return 阴影边缘缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetEdgeBuffer() const { return m_edgeBuffer; }

        /**
         * @brief 获取阴影参数缓冲区
         * @return 阴影参数缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetParamsBuffer() const { return m_paramsBuffer; }

        /**
         * @brief 获取阴影边缘数量
         * @return 边缘数量
         */
        uint32_t GetEdgeCount() const { return static_cast<uint32_t>(m_allEdges.size()); }

        /**
         * @brief 获取所有阴影边缘
         * @return 阴影边缘列表的常量引用
         */
        const std::vector<ShadowEdge>& GetAllEdges() const { return m_allEdges; }

        /**
         * @brief 获取单例实例
         * @return ShadowRenderer 单例引用
         */
        static ShadowRenderer* GetInstance() { return s_instance; }

        // ==================== 配置 ====================

        /**
         * @brief 设置阴影贴图分辨率
         * @param resolution 分辨率（像素）
         */
        void SetShadowMapResolution(uint32_t resolution);

        /**
         * @brief 获取阴影贴图分辨率
         * @return 当前阴影贴图分辨率
         */
        uint32_t GetShadowMapResolution() const { return m_shadowMapResolution; }

        /**
         * @brief 设置软阴影程度
         * @param softness 软阴影程度 [0, 1]
         */
        void SetShadowSoftness(float softness);

        /**
         * @brief 获取软阴影程度
         * @return 当前软阴影程度
         */
        float GetShadowSoftness() const { return m_shadowSoftness; }

        /**
         * @brief 启用/禁用阴影
         * @param enable 是否启用阴影
         */
        void SetEnabled(bool enable) { m_enabled = enable; }

        /**
         * @brief 获取阴影是否启用
         * @return 阴影是否启用
         */
        bool IsEnabled() const { return m_enabled; }

        // ==================== 阴影方法切换 (Requirements: 7.5) ====================

        /**
         * @brief 设置阴影计算方法
         * @param method 阴影方法
         */
        void SetShadowMethod(ECS::ShadowMethod method);

        /**
         * @brief 获取当前阴影计算方法
         * @return 当前阴影方法
         */
        ECS::ShadowMethod GetShadowMethod() const { return m_shadowMethod; }

        /**
         * @brief 检查是否支持指定的阴影方法
         * @param method 阴影方法
         * @return 是否支持
         */
        bool IsShadowMethodSupported(ECS::ShadowMethod method) const;

        // ==================== SDF 阴影 (Requirements: 7.3, 7.4) ====================

        /**
         * @brief 为阴影投射器生成 SDF 数据
         * @param caster 阴影投射器组件
         * @param vertices 世界坐标顶点
         * @return 生成的 SDF 数据
         */
        static ECS::SDFData GenerateSDF(
            const ECS::ShadowCasterComponent& caster,
            const std::vector<glm::vec2>& vertices);

        /**
         * @brief 计算点到多边形的有符号距离
         * @param point 查询点
         * @param vertices 多边形顶点
         * @return 有符号距离（负值表示在内部）
         */
        static float CalculateSignedDistance(
            const glm::vec2& point,
            const std::vector<glm::vec2>& vertices);

        /**
         * @brief 计算点到线段的距离
         * @param point 查询点
         * @param lineStart 线段起点
         * @param lineEnd 线段终点
         * @return 距离
         */
        static float PointToSegmentDistance(
            const glm::vec2& point,
            const glm::vec2& lineStart,
            const glm::vec2& lineEnd);

        /**
         * @brief 使用 SDF 计算软阴影
         * @param point 查询点
         * @param lightPos 光源位置
         * @param sdfData SDF 数据
         * @param softness 软阴影程度
         * @return 阴影因子 [0, 1]，0 = 完全照亮，1 = 完全阴影
         */
        static float CalculateSDFShadow(
            const glm::vec2& point,
            const glm::vec2& lightPos,
            const ECS::SDFData& sdfData,
            float softness);

        /**
         * @brief 获取 SDF 缓冲区
         * @return SDF 缓冲区的共享指针
         */
        std::shared_ptr<Nut::Buffer> GetSDFBuffer() const { return m_sdfBuffer; }

        // ==================== 屏幕空间阴影 (Requirements: 7.2) ====================

        /**
         * @brief 计算屏幕空间阴影
         * @param screenPos 屏幕坐标
         * @param lightScreenPos 光源屏幕坐标
         * @param depthBuffer 深度缓冲区
         * @return 阴影因子 [0, 1]
         */
        static float CalculateScreenSpaceShadow(
            const glm::vec2& screenPos,
            const glm::vec2& lightScreenPos,
            const std::vector<float>& depthBuffer,
            int width, int height);

        // ==================== 阴影缓存 (Requirements: 7.6) ====================

        /**
         * @brief 启用/禁用阴影缓存
         * @param enable 是否启用
         */
        void SetShadowCacheEnabled(bool enable) { m_shadowCacheEnabled = enable; }

        /**
         * @brief 获取阴影缓存是否启用
         * @return 是否启用
         */
        bool IsShadowCacheEnabled() const { return m_shadowCacheEnabled; }

        /**
         * @brief 使所有阴影缓存失效
         */
        void InvalidateAllShadowCaches();

        /**
         * @brief 使指定实体的阴影缓存失效
         * @param entity 实体
         */
        void InvalidateShadowCache(entt::entity entity);

        /**
         * @brief 处理待失效的缓存
         * @param scene 运行时场景
         */
        void ProcessCacheInvalidations(RuntimeScene* scene);

        /**
         * @brief 获取缓存命中率
         * @return 缓存命中率 [0, 1]
         */
        float GetCacheHitRate() const;

        /**
         * @brief 获取缓存命中次数
         * @return 缓存命中次数
         */
        uint64_t GetCacheHits() const { return m_cacheHits; }

        /**
         * @brief 获取缓存未命中次数
         * @return 缓存未命中次数
         */
        uint64_t GetCacheMisses() const { return m_cacheMisses; }

        /**
         * @brief 重置缓存统计
         */
        void ResetCacheStatistics();

        /**
         * @brief 获取当前帧号
         * @return 帧号
         */
        uint64_t GetCurrentFrame() const { return m_currentFrame; }

        // ==================== 静态工具函数（用于测试） ====================

        /**
         * @brief 从 ShadowCasterComponent 生成顶点
         * @param caster 阴影投射器组件
         * @param position 世界位置
         * @param scale 缩放
         * @param rotation 旋转角度（弧度）
         * @return 世界坐标顶点列表
         */
        static std::vector<glm::vec2> GenerateVertices(
            const ECS::ShadowCasterComponent& caster,
            const glm::vec2& position,
            const glm::vec2& scale,
            float rotation);

        /**
         * @brief 从顶点列表提取边缘
         * @param vertices 顶点列表
         * @return 边缘列表
         */
        static std::vector<ShadowEdge> ExtractEdges(const std::vector<glm::vec2>& vertices);

        /**
         * @brief 计算射线与边缘的交点
         * @param rayOrigin 射线起点
         * @param rayDir 射线方向（归一化）
         * @param edge 边缘
         * @param outT 输出参数：射线参数 t
         * @return 是否相交
         */
        static bool RayEdgeIntersection(
            const glm::vec2& rayOrigin,
            const glm::vec2& rayDir,
            const ShadowEdge& edge,
            float& outT);

        /**
         * @brief 检查点是否在阴影中
         * @param point 要检查的点
         * @param lightPos 光源位置
         * @param edges 所有阴影边缘
         * @return true 如果点在阴影中
         */
        static bool IsPointInShadow(
            const glm::vec2& point,
            const glm::vec2& lightPos,
            const std::vector<ShadowEdge>& edges);

        /**
         * @brief 生成矩形顶点
         * @param size 矩形尺寸
         * @return 顶点列表（局部坐标）
         */
        static std::vector<glm::vec2> GenerateRectangleVertices(const glm::vec2& size);

        /**
         * @brief 生成圆形顶点
         * @param radius 圆形半径
         * @param segments 分段数
         * @return 顶点列表（局部坐标）
         */
        static std::vector<glm::vec2> GenerateCircleVertices(float radius, uint32_t segments = 16);

    private:
        /**
         * @brief 收集场景中的阴影投射器
         * @param scene 运行时场景
         */
        void CollectShadowCasters(RuntimeScene* scene);

        /**
         * @brief 为光源渲染阴影贴图
         * @param lightPos 光源位置
         * @param lightRadius 光源半径
         * @param lightIndex 光源索引
         */
        void RenderShadowMapForLight(
            const glm::vec2& lightPos,
            float lightRadius,
            uint32_t lightIndex);

        /**
         * @brief 更新合并阴影贴图
         */
        void UpdateCombinedShadowMap();

        /**
         * @brief 创建阴影贴图纹理
         * @param engineCtx 引擎上下文
         */
        void CreateShadowMapTextures(EngineContext& engineCtx);

        /**
         * @brief 创建 GPU 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateGPUBuffers(EngineContext& engineCtx);

        /**
         * @brief 更新 GPU 缓冲区
         */
        void UpdateGPUBuffers();

        /**
         * @brief 变换顶点到世界坐标
         * @param localVertices 局部坐标顶点
         * @param position 位置
         * @param scale 缩放
         * @param rotation 旋转角度（弧度）
         * @return 世界坐标顶点
         */
        static std::vector<glm::vec2> TransformVertices(
            const std::vector<glm::vec2>& localVertices,
            const glm::vec2& position,
            const glm::vec2& scale,
            float rotation);

    private:
        std::vector<ShadowCasterInfo> m_shadowCasters;          ///< 阴影投射器列表
        std::vector<ShadowEdge> m_allEdges;                     ///< 所有阴影边缘
        
        std::unordered_map<uint32_t, LightShadowData> m_lightShadowData;  ///< 光源阴影数据
        Nut::TextureAPtr m_combinedShadowMap;                   ///< 合并阴影贴图

        std::shared_ptr<Nut::Buffer> m_shadowVertexBuffer;      ///< 阴影顶点缓冲区
        std::shared_ptr<Nut::Buffer> m_edgeBuffer;              ///< 阴影边缘缓冲区（GPU）
        std::shared_ptr<Nut::Buffer> m_paramsBuffer;            ///< 阴影参数缓冲区（GPU）
        std::shared_ptr<Nut::Buffer> m_sdfBuffer;               ///< SDF 数据缓冲区（GPU）
        
        uint32_t m_shadowMapResolution = DEFAULT_SHADOW_MAP_RESOLUTION;  ///< 阴影贴图分辨率
        float m_shadowSoftness = 1.0f;                          ///< 软阴影程度
        bool m_enabled = true;                                  ///< 是否启用阴影
        bool m_texturesCreated = false;                         ///< 纹理是否已创建
        bool m_buffersCreated = false;                          ///< GPU 缓冲区是否已创建

        // 阴影方法相关
        ECS::ShadowMethod m_shadowMethod = ECS::ShadowMethod::Basic;  ///< 当前阴影方法
        ECS::ShadowMethod m_previousShadowMethod = ECS::ShadowMethod::Basic;  ///< 上一帧的阴影方法

        // 阴影缓存相关
        bool m_shadowCacheEnabled = true;                       ///< 是否启用阴影缓存
        uint64_t m_currentFrame = 0;                            ///< 当前帧号
        uint64_t m_cacheHits = 0;                               ///< 缓存命中次数
        uint64_t m_cacheMisses = 0;                             ///< 缓存未命中次数
        std::vector<entt::entity> m_entitiesToInvalidate;       ///< 待失效的实体列表

        EngineContext* m_engineCtx = nullptr;                   ///< 引擎上下文缓存
        
        static ShadowRenderer* s_instance;                      ///< 单例实例

        // ==================== 私有方法 ====================

        /**
         * @brief 更新 SDF 数据
         * @param scene 运行时场景
         */
        void UpdateSDFData(RuntimeScene* scene);

        /**
         * @brief 更新阴影缓存
         * @param scene 运行时场景
         */
        void UpdateShadowCaches(RuntimeScene* scene);

        /**
         * @brief 创建 SDF 缓冲区
         * @param engineCtx 引擎上下文
         */
        void CreateSDFBuffer(EngineContext& engineCtx);
    };
}

#endif // SHADOW_RENDERER_H

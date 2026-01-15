#ifndef LIGHTMAP_BAKER_H
#define LIGHTMAP_BAKER_H

/**
 * @file LightmapBaker.h
 * @brief 光照贴图烘焙器
 * 
 * 负责将静态光源烘焙到纹理，用于优化静态光照的渲染性能。
 * 烘焙后的光照贴图可以直接采样，避免实时计算静态光源的光照。
 * 
 * Feature: 2d-lighting-system
 * Requirements: 10.4
 */

#include "../Components/LightingTypes.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "Nut/TextureA.h"

// 前向声明
class RuntimeScene;
struct EngineContext;

namespace Systems
{
    /**
     * @brief 烘焙配置结构体
     * 
     * 定义光照贴图烘焙的参数
     */
    struct LightmapBakeConfig
    {
        uint32_t resolution = 512;              ///< 光照贴图分辨率（像素）
        float worldWidth = 100.0f;              ///< 世界空间宽度
        float worldHeight = 100.0f;             ///< 世界空间高度
        glm::vec2 worldOrigin = {0.0f, 0.0f};   ///< 世界空间原点
        uint32_t samplesPerPixel = 4;           ///< 每像素采样数（用于抗锯齿）
        bool includeAmbient = true;             ///< 是否包含环境光
        bool includeShadows = true;             ///< 是否包含阴影

        /**
         * @brief 比较运算符
         */
        bool operator==(const LightmapBakeConfig& other) const
        {
            return resolution == other.resolution &&
                   worldWidth == other.worldWidth &&
                   worldHeight == other.worldHeight &&
                   worldOrigin == other.worldOrigin &&
                   samplesPerPixel == other.samplesPerPixel &&
                   includeAmbient == other.includeAmbient &&
                   includeShadows == other.includeShadows;
        }

        bool operator!=(const LightmapBakeConfig& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 烘焙进度回调类型
     * 
     * @param progress 进度值 [0, 1]
     * @param message 进度消息
     */
    using BakeProgressCallback = std::function<void(float progress, const std::string& message)>;

    /**
     * @brief 烘焙结果结构体
     */
    struct LightmapBakeResult
    {
        bool success = false;                   ///< 烘焙是否成功
        std::string errorMessage;               ///< 错误消息（如果失败）
        Nut::TextureAPtr lightmap;              ///< 烘焙的光照贴图
        uint32_t bakedLightCount = 0;           ///< 烘焙的光源数量
        float bakeTimeMs = 0.0f;                ///< 烘焙耗时（毫秒）
    };

    /**
     * @brief 静态光源信息
     * 
     * 用于标记哪些光源应该被烘焙
     */
    struct StaticLightInfo
    {
        ECS::LightData lightData;               ///< 光源数据
        bool isStatic = true;                   ///< 是否为静态光源
        entt::entity entity = entt::null;       ///< 关联的实体
    };


    /**
     * @brief 光照贴图烘焙器
     * 
     * 该类负责：
     * - 收集场景中的静态光源
     * - 计算静态光照并烘焙到纹理
     * - 管理烘焙的光照贴图
     * - 支持增量烘焙和完整烘焙
     */
    class LightmapBaker
    {
    public:
        /// 默认光照贴图分辨率
        static constexpr uint32_t DEFAULT_LIGHTMAP_RESOLUTION = 512;
        /// 最大光照贴图分辨率
        static constexpr uint32_t MAX_LIGHTMAP_RESOLUTION = 4096;
        /// 最小光照贴图分辨率
        static constexpr uint32_t MIN_LIGHTMAP_RESOLUTION = 64;

        /**
         * @brief 默认构造函数
         */
        LightmapBaker();

        /**
         * @brief 析构函数
         */
        ~LightmapBaker();

        // ==================== 烘焙操作 ====================

        /**
         * @brief 烘焙静态光照贴图
         * 
         * 收集场景中所有标记为静态的光源，计算光照并烘焙到纹理。
         * 
         * @param scene 运行时场景
         * @param engineCtx 引擎上下文
         * @param config 烘焙配置
         * @param progressCallback 进度回调（可选）
         * @return 烘焙结果
         */
        LightmapBakeResult BakeLightmap(
            RuntimeScene* scene,
            EngineContext& engineCtx,
            const LightmapBakeConfig& config,
            BakeProgressCallback progressCallback = nullptr);

        /**
         * @brief 从光源列表烘焙光照贴图
         * 
         * 直接从提供的光源列表烘焙，不从场景收集。
         * 
         * @param lights 静态光源列表
         * @param engineCtx 引擎上下文
         * @param config 烘焙配置
         * @param ambientColor 环境光颜色
         * @param ambientIntensity 环境光强度
         * @param progressCallback 进度回调（可选）
         * @return 烘焙结果
         */
        LightmapBakeResult BakeLightmapFromLights(
            const std::vector<StaticLightInfo>& lights,
            EngineContext& engineCtx,
            const LightmapBakeConfig& config,
            const ECS::Color& ambientColor = ECS::Color(0.1f, 0.1f, 0.15f, 1.0f),
            float ambientIntensity = 0.2f,
            BakeProgressCallback progressCallback = nullptr);

        /**
         * @brief 清除烘焙的光照贴图
         */
        void ClearLightmap();

        /**
         * @brief 检查是否有有效的烘焙光照贴图
         * @return true 如果有有效的光照贴图
         */
        bool HasValidLightmap() const { return m_lightmap != nullptr; }

        // ==================== 光照贴图访问 ====================

        /**
         * @brief 获取烘焙的光照贴图
         * @return 光照贴图纹理指针
         */
        Nut::TextureAPtr GetLightmap() const { return m_lightmap; }

        /**
         * @brief 获取当前烘焙配置
         * @return 烘焙配置
         */
        const LightmapBakeConfig& GetConfig() const { return m_currentConfig; }

        /**
         * @brief 获取烘焙的光源数量
         * @return 光源数量
         */
        uint32_t GetBakedLightCount() const { return m_bakedLightCount; }

        // ==================== 配置 ====================

        /**
         * @brief 设置默认烘焙配置
         * @param config 烘焙配置
         */
        void SetDefaultConfig(const LightmapBakeConfig& config) { m_defaultConfig = config; }

        /**
         * @brief 获取默认烘焙配置
         * @return 默认烘焙配置
         */
        const LightmapBakeConfig& GetDefaultConfig() const { return m_defaultConfig; }

        // ==================== 静态工具函数 ====================

        /**
         * @brief 计算单个像素的光照
         * 
         * @param worldPos 世界坐标位置
         * @param lights 光源列表
         * @param ambientColor 环境光颜色
         * @param ambientIntensity 环境光强度
         * @return 光照颜色（RGBA）
         */
        static glm::vec4 CalculatePixelLighting(
            const glm::vec2& worldPos,
            const std::vector<StaticLightInfo>& lights,
            const ECS::Color& ambientColor,
            float ambientIntensity);

        /**
         * @brief 计算点光源对某点的光照贡献
         * 
         * @param worldPos 世界坐标位置
         * @param lightData 光源数据
         * @return 光照贡献（RGB）
         */
        static glm::vec3 CalculatePointLightContribution(
            const glm::vec2& worldPos,
            const ECS::LightData& lightData);

        /**
         * @brief 计算聚光灯对某点的光照贡献
         * 
         * @param worldPos 世界坐标位置
         * @param lightData 光源数据
         * @return 光照贡献（RGB）
         */
        static glm::vec3 CalculateSpotLightContribution(
            const glm::vec2& worldPos,
            const ECS::LightData& lightData);

        /**
         * @brief 计算方向光对某点的光照贡献
         * 
         * @param lightData 光源数据
         * @return 光照贡献（RGB）
         */
        static glm::vec3 CalculateDirectionalLightContribution(
            const ECS::LightData& lightData);

        /**
         * @brief 将世界坐标转换为纹理坐标
         * 
         * @param worldPos 世界坐标
         * @param config 烘焙配置
         * @return 纹理坐标 [0, 1]
         */
        static glm::vec2 WorldToTextureCoord(
            const glm::vec2& worldPos,
            const LightmapBakeConfig& config);

        /**
         * @brief 将纹理坐标转换为世界坐标
         * 
         * @param texCoord 纹理坐标 [0, 1]
         * @param config 烘焙配置
         * @return 世界坐标
         */
        static glm::vec2 TextureCoordToWorld(
            const glm::vec2& texCoord,
            const LightmapBakeConfig& config);

    private:
        /**
         * @brief 收集场景中的静态光源
         * @param scene 运行时场景
         * @return 静态光源列表
         */
        std::vector<StaticLightInfo> CollectStaticLights(RuntimeScene* scene);

        /**
         * @brief 创建光照贴图纹理
         * @param engineCtx 引擎上下文
         * @param resolution 分辨率
         * @return 是否成功
         */
        bool CreateLightmapTexture(EngineContext& engineCtx, uint32_t resolution);

        /**
         * @brief 执行 CPU 端光照计算
         * @param lights 光源列表
         * @param config 烘焙配置
         * @param ambientColor 环境光颜色
         * @param ambientIntensity 环境光强度
         * @param progressCallback 进度回调
         * @return 像素数据（RGBA8）
         */
        std::vector<uint8_t> ComputeLightingCPU(
            const std::vector<StaticLightInfo>& lights,
            const LightmapBakeConfig& config,
            const ECS::Color& ambientColor,
            float ambientIntensity,
            BakeProgressCallback progressCallback);

        /**
         * @brief 上传像素数据到 GPU 纹理
         * @param engineCtx 引擎上下文
         * @param pixelData 像素数据
         * @param width 宽度
         * @param height 高度
         * @return 是否成功
         */
        bool UploadToGPU(
            EngineContext& engineCtx,
            const std::vector<uint8_t>& pixelData,
            uint32_t width,
            uint32_t height);

    private:
        Nut::TextureAPtr m_lightmap;                ///< 烘焙的光照贴图
        LightmapBakeConfig m_currentConfig;         ///< 当前烘焙配置
        LightmapBakeConfig m_defaultConfig;         ///< 默认烘焙配置
        uint32_t m_bakedLightCount = 0;             ///< 烘焙的光源数量
        bool m_isInitialized = false;               ///< 是否已初始化
    };
}

#endif // LIGHTMAP_BAKER_H

#pragma once

#include "NutContext.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "TextureA.h"
#include "RenderPass.h"
#include <memory>
#include <vector>
#include <array>

namespace Nut {

/**
 * @brief 精灵顶点结构
 */
struct SpriteVertex {
    float position[2];   // x, y
    float texCoord[2];   // u, v
    float color[4];      // r, g, b, a
};

/**
 * @brief 精灵uniform数据
 */
struct SpriteUniforms {
    float viewProjection[16];  // 4x4 矩阵
    float modelTransform[16];  // 4x4 矩阵
};

/**
 * @brief 精灵渲染器
 * 
 * 负责批量渲染2D精灵，使用WGSL着色器
 */
class SpriteRenderer {
public:
    /**
     * @brief 构造函数
     * @param context NutContext图形上下文
     */
    explicit SpriteRenderer(const std::shared_ptr<NutContext>& context);
    ~SpriteRenderer();

    /**
     * @brief 初始化渲染器
     * @param colorFormat 颜色目标格式
     * @return 是否初始化成功
     */
    bool Initialize(wgpu::TextureFormat colorFormat = wgpu::TextureFormat::BGRA8Unorm);

    /**
     * @brief 开始渲染批次
     * @param renderPass 渲染通道
     * @param viewProjectionMatrix 视图投影矩阵（4x4，行主序）
     */
    void BeginBatch(RenderPass& renderPass, const float* viewProjectionMatrix);

    /**
     * @brief 绘制单个精灵
     * @param texture 纹理
     * @param position 位置 (x, y)
     * @param size 大小 (width, height)
     * @param texCoordMin 纹理坐标最小值 (u, v)
     * @param texCoordMax 纹理坐标最大值 (u, v)
     * @param color 颜色 (r, g, b, a)
     * @param rotation 旋转角度（弧度）
     * @param pivot 旋转中心 (x, y)，相对于精灵中心
     */
    void DrawSprite(
        const TextureA& texture,
        const float position[2],
        const float size[2],
        const float texCoordMin[2],
        const float texCoordMax[2],
        const float color[4],
        float rotation = 0.0f,
        const float pivot[2] = nullptr
    );

    /**
     * @brief 结束渲染批次并提交
     */
    void EndBatch();

    /**
     * @brief 获取渲染管线
     */
    RenderPipeline* GetPipeline() const { return m_pipeline.get(); }

    /**
     * @brief 检查渲染器是否有效
     */
    bool IsValid() const { return m_pipeline != nullptr; }

private:
    /**
     * @brief 刷新当前批次
     */
    void Flush();

    /**
     * @brief 创建四边形顶点
     */
    void CreateQuadVertices(
        const float position[2],
        const float size[2],
        const float texCoordMin[2],
        const float texCoordMax[2],
        const float color[4],
        float rotation,
        const float pivot[2]
    );

    std::shared_ptr<NutContext> m_context;
    std::unique_ptr<RenderPipeline> m_pipeline;
    ShaderModule m_shaderModule;
    Sampler m_sampler;

    // 批次数据
    static constexpr size_t MAX_SPRITES_PER_BATCH = 1000;
    static constexpr size_t VERTICES_PER_SPRITE = 6; // 2 triangles
    std::vector<SpriteVertex> m_vertices;
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_uniformBuffer;

    // 当前批次状态
    RenderPass* m_currentRenderPass = nullptr;
    const TextureA* m_currentTexture = nullptr;
    SpriteUniforms m_currentUniforms;
    bool m_inBatch = false;
};

} // namespace Nut

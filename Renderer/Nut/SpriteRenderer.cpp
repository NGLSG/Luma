#include "SpriteRenderer.h"
#include "Logger.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

namespace Nut {

SpriteRenderer::SpriteRenderer(const std::shared_ptr<NutContext>& context)
    : m_context(context)
{
    m_vertices.reserve(MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE);
}

SpriteRenderer::~SpriteRenderer() = default;

bool SpriteRenderer::Initialize(wgpu::TextureFormat colorFormat)
{
    if (!m_context)
    {
        LogError("SpriteRenderer::Initialize - NutContext is null");
        return false;
    }

    
    std::string shaderPath = "Renderer/Nut/Shaders/sprite.wgsl";
    std::ifstream file(shaderPath);
    if (!file.is_open())
    {
        LogError("SpriteRenderer::Initialize - Failed to open shader file: {}", shaderPath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderCode = buffer.str();
    file.close();

    m_shaderModule = ShaderModule(shaderCode, m_context);
    if (!m_shaderModule.Get())
    {
        LogError("SpriteRenderer::Initialize - Failed to create shader module");
        return false;
    }

    
    VertexBufferLayout vertexLayout;
    vertexLayout.arrayStride = sizeof(SpriteVertex);
    vertexLayout.stepMode = VertexStepMode::Vertex;
    
    vertexLayout.attributes.push_back(
        VertexAttribute().SetLocation(0).SetFormat(VertexFormat::Float32x2).SetOffset(offsetof(SpriteVertex, position))
    );
    vertexLayout.attributes.push_back(
        VertexAttribute().SetLocation(1).SetFormat(VertexFormat::Float32x2).SetOffset(offsetof(SpriteVertex, texCoord))
    );
    vertexLayout.attributes.push_back(
        VertexAttribute().SetLocation(2).SetFormat(VertexFormat::Float32x4).SetOffset(offsetof(SpriteVertex, color))
    );

    
    VertexState vertexState({vertexLayout}, m_shaderModule, "vs_main");

    
    ColorTargetState colorTarget;
    colorTarget.SetFormat(colorFormat);
    
    
    wgpu::BlendComponent blendComponent;
    blendComponent.operation = wgpu::BlendOperation::Add;
    blendComponent.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendComponent.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    
    wgpu::BlendState blendState;
    blendState.color = blendComponent;
    blendState.alpha = blendComponent;
    
    colorTarget.SetBlendState(&blendState);
    
    FragmentState fragmentState({colorTarget}, m_shaderModule, "fs_main");

    
    RenderPipelineDescriptor pipelineDesc{
        .vertex = vertexState,
        .fragment = fragmentState,
        .shaderModule = m_shaderModule,
        .context = m_context
    };

    m_pipeline = std::make_unique<RenderPipeline>(pipelineDesc);
    if (!m_pipeline->Get())
    {
        LogError("SpriteRenderer::Initialize - Failed to create render pipeline");
        return false;
    }

    
    m_sampler.SetMagFilter(FilterMode::Linear)
            .SetMinFilter(FilterMode::Linear)
            .SetWrapModeU(WrapMode::ClampToEdge)
            .SetWrapModeV(WrapMode::ClampToEdge);
    m_sampler.Build(m_context);

    
    BufferLayout _vertexLayout;
    _vertexLayout.size = MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE * sizeof(SpriteVertex);
    _vertexLayout.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    _vertexLayout.mapped = false;
    m_vertexBuffer = Buffer::Create(_vertexLayout, m_context);

    BufferLayout uniformLayout;
    uniformLayout.size = sizeof(SpriteUniforms);
    uniformLayout.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniformLayout.mapped = false;
    m_uniformBuffer = Buffer::Create(uniformLayout, m_context);

    LogInfo("SpriteRenderer initialized successfully");
    return true;
}

void SpriteRenderer::BeginBatch(RenderPass& renderPass, const float* viewProjectionMatrix)
{
    if (m_inBatch)
    {
        LogWarn("SpriteRenderer::BeginBatch - Already in batch, call EndBatch first");
        return;
    }

    m_currentRenderPass = &renderPass;
    m_inBatch = true;
    m_vertices.clear();
    m_currentTexture = nullptr;

    
    std::memcpy(m_currentUniforms.viewProjection, viewProjectionMatrix, sizeof(float) * 16);
    
    
    std::memset(m_currentUniforms.modelTransform, 0, sizeof(float) * 16);
    m_currentUniforms.modelTransform[0] = 1.0f;
    m_currentUniforms.modelTransform[5] = 1.0f;
    m_currentUniforms.modelTransform[10] = 1.0f;
    m_currentUniforms.modelTransform[15] = 1.0f;
}

void SpriteRenderer::DrawSprite(
    const TextureA& texture,
    const float position[2],
    const float size[2],
    const float texCoordMin[2],
    const float texCoordMax[2],
    const float color[4],
    float rotation,
    const float pivot[2])
{
    if (!m_inBatch)
    {
        LogWarn("SpriteRenderer::DrawSprite - Not in batch, call BeginBatch first");
        return;
    }

    
    if (m_currentTexture != &texture || m_vertices.size() + VERTICES_PER_SPRITE > MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE)
    {
        if (!m_vertices.empty())
        {
            Flush();
        }
        m_currentTexture = &texture;
    }

    
    CreateQuadVertices(position, size, texCoordMin, texCoordMax, color, rotation, pivot);
}

void SpriteRenderer::EndBatch()
{
    if (!m_inBatch)
    {
        LogWarn("SpriteRenderer::EndBatch - Not in batch");
        return;
    }

    
    if (!m_vertices.empty())
    {
        Flush();
    }

    m_inBatch = false;
    m_currentRenderPass = nullptr;
    m_currentTexture = nullptr;
}

void SpriteRenderer::Flush()
{
    if (m_vertices.empty() || !m_currentRenderPass || !m_currentTexture)
    {
        return;
    }

    
    m_vertexBuffer->WriteBuffer(m_vertices.data(), m_vertices.size() * sizeof(SpriteVertex));

    
    m_uniformBuffer->WriteBuffer(&m_currentUniforms, sizeof(SpriteUniforms));

    
    m_currentRenderPass->SetPipeline(*m_pipeline);

    
    m_pipeline->SetBinding(0, 0, *m_uniformBuffer);
    m_pipeline->SetBinding(0, 1, m_sampler);
    m_pipeline->SetBinding(0, 2, *m_currentTexture);

    
    m_pipeline->BuildBindings(m_context);
    m_pipeline->ForeachGroup([this](size_t groupIdx, BindGroup& group)
    {
        m_currentRenderPass->SetBindGroup(groupIdx, group);
    });

    
    m_currentRenderPass->SetVertexBuffer(0, *m_vertexBuffer);

    
    m_currentRenderPass->Draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);

    
    m_vertices.clear();
}

void SpriteRenderer::CreateQuadVertices(
    const float position[2],
    const float size[2],
    const float texCoordMin[2],
    const float texCoordMax[2],
    const float color[4],
    float rotation,
    const float pivot[2])
{
    
    float halfWidth = size[0] * 0.5f;
    float halfHeight = size[1] * 0.5f;

    float pivotX = pivot ? pivot[0] : 0.0f;
    float pivotY = pivot ? pivot[1] : 0.0f;

    
    float corners[4][2] = {
        {-halfWidth - pivotX, -halfHeight - pivotY}, 
        { halfWidth - pivotX, -halfHeight - pivotY}, 
        { halfWidth - pivotX,  halfHeight - pivotY}, 
        {-halfWidth - pivotX,  halfHeight - pivotY}  
    };

    
    float cosTheta = std::cos(rotation);
    float sinTheta = std::sin(rotation);

    SpriteVertex vertices[4];
    for (int i = 0; i < 4; i++)
    {
        float x = corners[i][0];
        float y = corners[i][1];

        
        float rotatedX = x * cosTheta - y * sinTheta;
        float rotatedY = x * sinTheta + y * cosTheta;

        
        vertices[i].position[0] = rotatedX + position[0];
        vertices[i].position[1] = rotatedY + position[1];

        
        std::memcpy(vertices[i].color, color, sizeof(float) * 4);
    }

    
    vertices[0].texCoord[0] = texCoordMin[0]; vertices[0].texCoord[1] = texCoordMax[1]; 
    vertices[1].texCoord[0] = texCoordMax[0]; vertices[1].texCoord[1] = texCoordMax[1]; 
    vertices[2].texCoord[0] = texCoordMax[0]; vertices[2].texCoord[1] = texCoordMin[1]; 
    vertices[3].texCoord[0] = texCoordMin[0]; vertices[3].texCoord[1] = texCoordMin[1]; 

    
    m_vertices.push_back(vertices[0]);
    m_vertices.push_back(vertices[1]);
    m_vertices.push_back(vertices[2]);

    m_vertices.push_back(vertices[0]);
    m_vertices.push_back(vertices[2]);
    m_vertices.push_back(vertices[3]);
}

} 

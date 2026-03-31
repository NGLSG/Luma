#ifndef PIXEL_WORLD_H
#define PIXEL_WORLD_H

#include <cstdint>
#include <memory>
#include <vector>
#include "../../Renderer/Nut/NutContext.h"
#include "../../Renderer/Nut/Pipeline.h"
#include "../../Renderer/Nut/Buffer.h"
#include "../../Renderer/Nut/BindGroup.h"

struct PixelType
{
    enum Value : uint32_t
    {
        Air = 0, Sand = 1, Water = 2, Stone = 3,
        Fire = 4, Steam = 5, Oil = 6, Lava = 7
    };
};

struct GPUPixel
{
    uint32_t pixel_type;
    uint32_t color;
    float lifetime;
    float velocity_y;
};

struct SimParams
{
    uint32_t width;
    uint32_t height;
    uint32_t frame;
    float dt;
};

class LUMA_API PixelWorld
{
public:
    PixelWorld(uint32_t width, uint32_t height);

    void Initialize(std::shared_ptr<Nut::NutContext> ctx);
    void Step(float dt);
    void SetPixel(int x, int y, PixelType::Value type);
    uint32_t GetPixel(int x, int y) const;
    wgpu::Texture GetRenderTexture() const;
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    void UploadDirtyPixels();

private:
    static uint32_t DefaultColor(PixelType::Value type);

    uint32_t m_width, m_height;
    std::shared_ptr<Nut::NutContext> m_ctx;

    wgpu::Buffer m_bufferA;
    wgpu::Buffer m_bufferB;
    wgpu::Buffer m_paramsBuffer;
    wgpu::Buffer m_stagingBuffer;

    wgpu::ComputePipeline m_pipeline;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroupA;
    wgpu::BindGroup m_bindGroupB;

    wgpu::Texture m_renderTexture;

    uint32_t m_frame = 0;
    bool m_pingPong = false;

    std::vector<GPUPixel> m_cpuBuffer;
    bool m_cpuDirty = false;
};

#endif

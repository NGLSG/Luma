#include "PixelWorld.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

static std::string LoadShaderFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

PixelWorld::PixelWorld(uint32_t width, uint32_t height)
    : m_width(width), m_height(height)
{
    m_cpuBuffer.resize(static_cast<size_t>(width) * height, GPUPixel{0, 0, 0.0f, 0.0f});
}

uint32_t PixelWorld::DefaultColor(PixelType::Value type)
{
    switch (type)
    {
    case PixelType::Sand:  return 0xFFC8B464;
    case PixelType::Water: return 0xFF6464C8;
    case PixelType::Stone: return 0xFF808080;
    case PixelType::Fire:  return 0xFF3264FF;
    case PixelType::Steam: return 0xFFC8C8C8;
    case PixelType::Oil:   return 0xFF325050;
    case PixelType::Lava:  return 0xFF0040FF;
    default:               return 0x00000000;
    }
}

void PixelWorld::Initialize(std::shared_ptr<Nut::NutContext> ctx)
{
    m_ctx = std::move(ctx);
    auto& device = m_ctx->GetWGPUDevice();
    const size_t pixelCount = static_cast<size_t>(m_width) * m_height;
    const size_t bufferSize = pixelCount * sizeof(GPUPixel);

    wgpu::BufferDescriptor bufDesc{};
    bufDesc.size = bufferSize;
    bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
    bufDesc.label = "PixelWorld_BufferA";
    m_bufferA = device.CreateBuffer(&bufDesc);
    bufDesc.label = "PixelWorld_BufferB";
    m_bufferB = device.CreateBuffer(&bufDesc);

    wgpu::BufferDescriptor paramsDesc{};
    paramsDesc.size = sizeof(SimParams);
    paramsDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    paramsDesc.label = "PixelWorld_Params";
    m_paramsBuffer = device.CreateBuffer(&paramsDesc);

    wgpu::BufferDescriptor stagingDesc{};
    stagingDesc.size = bufferSize;
    stagingDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    stagingDesc.label = "PixelWorld_Staging";
    m_stagingBuffer = device.CreateBuffer(&stagingDesc);

    device.GetQueue().WriteBuffer(m_bufferA, 0, m_cpuBuffer.data(), bufferSize);
    device.GetQueue().WriteBuffer(m_bufferB, 0, m_cpuBuffer.data(), bufferSize);

    std::string shaderCode = LoadShaderFile("Shaders/pixel_physics.wgsl");
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode.c_str();
    wgpu::ShaderModuleDescriptor smDesc{};
    smDesc.nextInChain = &wgslDesc;
    smDesc.label = "pixel_physics";
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&smDesc);

    wgpu::BindGroupLayoutEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Compute;
    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    entries[0].buffer.minBindingSize = bufferSize;

    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Compute;
    entries[1].buffer.type = wgpu::BufferBindingType::Storage;
    entries[1].buffer.minBindingSize = bufferSize;

    entries[2].binding = 2;
    entries[2].visibility = wgpu::ShaderStage::Compute;
    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;
    entries[2].buffer.minBindingSize = sizeof(SimParams);

    wgpu::BindGroupLayoutDescriptor bglDesc{};
    bglDesc.entryCount = 3;
    bglDesc.entries = entries;
    m_bindGroupLayout = device.CreateBindGroupLayout(&bglDesc);

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &m_bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&plDesc);

    wgpu::ComputePipelineDescriptor cpDesc{};
    cpDesc.layout = pipelineLayout;
    cpDesc.compute.module = shaderModule;
    cpDesc.compute.entryPoint = "main";
    cpDesc.label = "PixelPhysics";
    m_pipeline = device.CreateComputePipeline(&cpDesc);

    auto makeBG = [&](wgpu::Buffer& input, wgpu::Buffer& output) -> wgpu::BindGroup
    {
        wgpu::BindGroupEntry bgEntries[3] = {};
        bgEntries[0].binding = 0;
        bgEntries[0].buffer = input;
        bgEntries[0].size = bufferSize;

        bgEntries[1].binding = 1;
        bgEntries[1].buffer = output;
        bgEntries[1].size = bufferSize;

        bgEntries[2].binding = 2;
        bgEntries[2].buffer = m_paramsBuffer;
        bgEntries[2].size = sizeof(SimParams);

        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout = m_bindGroupLayout;
        bgDesc.entryCount = 3;
        bgDesc.entries = bgEntries;
        return device.CreateBindGroup(&bgDesc);
    };

    m_bindGroupA = makeBG(m_bufferA, m_bufferB);
    m_bindGroupB = makeBG(m_bufferB, m_bufferA);

    wgpu::TextureDescriptor texDesc{};
    texDesc.size = {m_width, m_height, 1};
    texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    texDesc.label = "PixelWorld_RenderTex";
    m_renderTexture = device.CreateTexture(&texDesc);
}

void PixelWorld::Step(float dt)
{
    if (!m_ctx) return;
    auto& device = m_ctx->GetWGPUDevice();
    auto queue = m_ctx->GetWGPUQueue();

    if (m_cpuDirty)
    {
        UploadDirtyPixels();
    }

    SimParams params{m_width, m_height, m_frame, dt};
    queue.WriteBuffer(m_paramsBuffer, 0, &params, sizeof(SimParams));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    wgpu::ComputePassDescriptor passDesc{};
    passDesc.label = "PixelPhysics_Step";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&passDesc);
    pass.SetPipeline(m_pipeline);
    pass.SetBindGroup(0, m_pingPong ? m_bindGroupB : m_bindGroupA);
    pass.DispatchWorkgroups(
        static_cast<uint32_t>(std::ceil(static_cast<float>(m_width) / 16.0f)),
        static_cast<uint32_t>(std::ceil(static_cast<float>(m_height) / 16.0f))
    );
    pass.End();

    wgpu::Buffer& outputBuf = m_pingPong ? m_bufferA : m_bufferB;

    wgpu::TexelCopyBufferLayout srcLayout{};
    srcLayout.bytesPerRow = m_width * sizeof(uint32_t);
    srcLayout.rowsPerImage = m_height;

    wgpu::TexelCopyBufferInfo bufInfo{};
    bufInfo.buffer = outputBuf;
    bufInfo.layout = srcLayout;
    bufInfo.layout.offset = offsetof(GPUPixel, color);

    wgpu::TexelCopyTextureInfo texInfo{};
    texInfo.texture = m_renderTexture;

    wgpu::Extent3D copySize = {m_width, m_height, 1};

    wgpu::CommandBuffer cmd = encoder.Finish();
    queue.Submit(1, &cmd);

    m_pingPong = !m_pingPong;
    m_frame++;
}

void PixelWorld::SetPixel(int x, int y, PixelType::Value type)
{
    if (x < 0 || x >= static_cast<int>(m_width) || y < 0 || y >= static_cast<int>(m_height))
        return;

    size_t i = static_cast<size_t>(y) * m_width + x;
    m_cpuBuffer[i] = GPUPixel{static_cast<uint32_t>(type), DefaultColor(type), 0.0f, 0.0f};
    m_cpuDirty = true;
}

uint32_t PixelWorld::GetPixel(int x, int y) const
{
    if (x < 0 || x >= static_cast<int>(m_width) || y < 0 || y >= static_cast<int>(m_height))
        return 0;
    return m_cpuBuffer[static_cast<size_t>(y) * m_width + x].pixel_type;
}

wgpu::Texture PixelWorld::GetRenderTexture() const
{
    return m_renderTexture;
}

void PixelWorld::UploadDirtyPixels()
{
    if (!m_ctx || !m_cpuDirty) return;
    auto queue = m_ctx->GetWGPUQueue();
    const size_t bufferSize = static_cast<size_t>(m_width) * m_height * sizeof(GPUPixel);

    wgpu::Buffer& currentInput = m_pingPong ? m_bufferB : m_bufferA;
    queue.WriteBuffer(currentInput, 0, m_cpuBuffer.data(), bufferSize);
    m_cpuDirty = false;
}

#include "ChunkedPixelWorld.h"
#include <cmath>
#include <algorithm>

ChunkedPixelWorld::ChunkedPixelWorld() = default;

void ChunkedPixelWorld::Initialize(std::shared_ptr<Nut::NutContext> ctx)
{
    m_ctx = std::move(ctx);
    const int diameter = m_activeRadius * 2 + 1;
    const uint32_t simWidth = static_cast<uint32_t>(diameter * CHUNK_SIZE);
    const uint32_t simHeight = simWidth;

    m_activeSimulation = std::make_unique<PixelWorld>(simWidth, simHeight);
    m_activeSimulation->Initialize(m_ctx);
    m_initialized = true;
}

uint64_t ChunkedPixelWorld::ChunkKey(int cx, int cy)
{
    auto ux = static_cast<uint32_t>(cx);
    auto uy = static_cast<uint32_t>(cy);
    return (static_cast<uint64_t>(ux) << 32) | static_cast<uint64_t>(uy);
}

void ChunkedPixelWorld::WorldToChunk(int wx, int wy, int& cx, int& cy, int& lx, int& ly)
{
    cx = (wx >= 0) ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE;
    cy = (wy >= 0) ? wy / CHUNK_SIZE : (wy - CHUNK_SIZE + 1) / CHUNK_SIZE;
    lx = wx - cx * CHUNK_SIZE;
    ly = wy - cy * CHUNK_SIZE;
}

ChunkedPixelWorld::PixelChunk* ChunkedPixelWorld::GetOrCreateChunk(int cx, int cy)
{
    uint64_t key = ChunkKey(cx, cy);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end())
        return it->second.get();

    auto chunk = std::make_unique<PixelChunk>();
    chunk->chunkX = cx;
    chunk->chunkY = cy;
    chunk->pixels.resize(static_cast<size_t>(CHUNK_SIZE) * CHUNK_SIZE, GPUPixel{0, 0, 0.0f, 0.0f});
    chunk->dirty = true;
    GenerateChunkTerrain(*chunk);

    auto* ptr = chunk.get();
    m_chunks[key] = std::move(chunk);
    return ptr;
}

const ChunkedPixelWorld::PixelChunk* ChunkedPixelWorld::GetChunk(int cx, int cy) const
{
    auto it = m_chunks.find(ChunkKey(cx, cy));
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

void ChunkedPixelWorld::GenerateChunkTerrain(PixelChunk& chunk)
{
    for (int ly = 0; ly < CHUNK_SIZE; ly++)
    {
        int wy = chunk.chunkY * CHUNK_SIZE + ly;
        for (int lx = 0; lx < CHUNK_SIZE; lx++)
        {
            size_t i = static_cast<size_t>(ly) * CHUNK_SIZE + lx;
            if (wy > 0)
            {
                chunk.pixels[i] = GPUPixel{PixelType::Stone, 0xFF808080, 0.0f, 0.0f};
            }
            else
            {
                chunk.pixels[i] = GPUPixel{PixelType::Air, 0, 0.0f, 0.0f};
            }
        }
    }
}

void ChunkedPixelWorld::SetViewCenter(float worldX, float worldY, float pixelScale)
{
    int newCX = static_cast<int>(std::floor(worldX / (CHUNK_SIZE * pixelScale)));
    int newCY = static_cast<int>(std::floor(worldY / (CHUNK_SIZE * pixelScale)));

    if (newCX != m_viewCenterChunkX || newCY != m_viewCenterChunkY)
    {
        if (m_initialized)
            SyncGPUToChunks();

        m_viewCenterChunkX = newCX;
        m_viewCenterChunkY = newCY;

        for (auto& [_, chunk] : m_chunks)
            chunk->active = false;

        for (int dy = -m_activeRadius; dy <= m_activeRadius; dy++)
        {
            for (int dx = -m_activeRadius; dx <= m_activeRadius; dx++)
            {
                auto* chunk = GetOrCreateChunk(newCX + dx, newCY + dy);
                chunk->active = true;
            }
        }

        if (m_initialized)
            SyncChunksToGPU();
    }
}

void ChunkedPixelWorld::SyncChunksToGPU()
{
    if (!m_activeSimulation) return;

    const int diameter = m_activeRadius * 2 + 1;
    const int startCX = m_viewCenterChunkX - m_activeRadius;
    const int startCY = m_viewCenterChunkY - m_activeRadius;

    for (int dy = 0; dy < diameter; dy++)
    {
        for (int dx = 0; dx < diameter; dx++)
        {
            int cx = startCX + dx;
            int cy = startCY + dy;
            auto* chunk = GetOrCreateChunk(cx, cy);

            for (int ly = 0; ly < CHUNK_SIZE; ly++)
            {
                for (int lx = 0; lx < CHUNK_SIZE; lx++)
                {
                    int simX = dx * CHUNK_SIZE + lx;
                    int simY = dy * CHUNK_SIZE + ly;
                    size_t chunkIdx = static_cast<size_t>(ly) * CHUNK_SIZE + lx;
                    auto& p = chunk->pixels[chunkIdx];
                    m_activeSimulation->SetPixel(simX, simY, static_cast<PixelType::Value>(p.pixel_type));
                }
            }
        }
    }
}

void ChunkedPixelWorld::SyncGPUToChunks()
{
    if (!m_activeSimulation) return;

    const int diameter = m_activeRadius * 2 + 1;
    const int startCX = m_viewCenterChunkX - m_activeRadius;
    const int startCY = m_viewCenterChunkY - m_activeRadius;

    for (int dy = 0; dy < diameter; dy++)
    {
        for (int dx = 0; dx < diameter; dx++)
        {
            int cx = startCX + dx;
            int cy = startCY + dy;
            auto* chunk = GetOrCreateChunk(cx, cy);

            for (int ly = 0; ly < CHUNK_SIZE; ly++)
            {
                for (int lx = 0; lx < CHUNK_SIZE; lx++)
                {
                    int simX = dx * CHUNK_SIZE + lx;
                    int simY = dy * CHUNK_SIZE + ly;
                    size_t chunkIdx = static_cast<size_t>(ly) * CHUNK_SIZE + lx;
                    uint32_t type = m_activeSimulation->GetPixel(simX, simY);
                    chunk->pixels[chunkIdx].pixel_type = type;
                }
            }
        }
    }
}

void ChunkedPixelWorld::Step(float dt)
{
    if (!m_activeSimulation || !m_initialized) return;
    m_activeSimulation->Step(dt);
}

void ChunkedPixelWorld::SetPixel(int worldPixelX, int worldPixelY, PixelType::Value type)
{
    int cx, cy, lx, ly;
    WorldToChunk(worldPixelX, worldPixelY, cx, cy, lx, ly);

    auto* chunk = GetOrCreateChunk(cx, cy);
    size_t i = static_cast<size_t>(ly) * CHUNK_SIZE + lx;
    chunk->pixels[i] = GPUPixel{static_cast<uint32_t>(type), PixelWorld::DefaultColor(type), 0.0f, 0.0f};
    chunk->dirty = true;

    if (chunk->active && m_activeSimulation)
    {
        int startCX = m_viewCenterChunkX - m_activeRadius;
        int startCY = m_viewCenterChunkY - m_activeRadius;
        int simX = (cx - startCX) * CHUNK_SIZE + lx;
        int simY = (cy - startCY) * CHUNK_SIZE + ly;
        m_activeSimulation->SetPixel(simX, simY, type);
    }
}

uint32_t ChunkedPixelWorld::GetPixel(int worldPixelX, int worldPixelY) const
{
    int cx, cy, lx, ly;
    WorldToChunk(worldPixelX, worldPixelY, cx, cy, lx, ly);

    auto* chunk = GetChunk(cx, cy);
    if (!chunk) return PixelType::Air;
    return chunk->pixels[static_cast<size_t>(ly) * CHUNK_SIZE + lx].pixel_type;
}

wgpu::Texture ChunkedPixelWorld::GetRenderTexture() const
{
    return m_activeSimulation ? m_activeSimulation->GetRenderTexture() : wgpu::Texture{};
}

uint32_t ChunkedPixelWorld::GetActiveWidth() const
{
    return m_activeSimulation ? m_activeSimulation->GetWidth() : 0;
}

uint32_t ChunkedPixelWorld::GetActiveHeight() const
{
    return m_activeSimulation ? m_activeSimulation->GetHeight() : 0;
}

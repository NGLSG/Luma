#ifndef CHUNKED_PIXEL_WORLD_H
#define CHUNKED_PIXEL_WORLD_H

#include "PixelWorld.h"
#include <unordered_map>
#include <memory>
#include <cstdint>

class LUMA_API ChunkedPixelWorld
{
public:
    static constexpr int CHUNK_SIZE = 128;

    ChunkedPixelWorld();

    void Initialize(std::shared_ptr<Nut::NutContext> ctx);
    void SetViewCenter(float worldX, float worldY, float pixelScale);
    void Step(float dt);

    void SetPixel(int worldPixelX, int worldPixelY, PixelType::Value type);
    uint32_t GetPixel(int worldPixelX, int worldPixelY) const;

    wgpu::Texture GetRenderTexture() const;
    uint32_t GetActiveWidth() const;
    uint32_t GetActiveHeight() const;

private:
    struct PixelChunk
    {
        int chunkX, chunkY;
        std::vector<GPUPixel> pixels;
        bool dirty = true;
        bool active = false;
    };

    std::unordered_map<uint64_t, std::unique_ptr<PixelChunk>> m_chunks;
    std::unique_ptr<PixelWorld> m_activeSimulation;
    std::shared_ptr<Nut::NutContext> m_ctx;

    int m_viewCenterChunkX = 0;
    int m_viewCenterChunkY = 0;
    int m_activeRadius = 3;
    bool m_initialized = false;

    static uint64_t ChunkKey(int cx, int cy);
    static void WorldToChunk(int wx, int wy, int& cx, int& cy, int& lx, int& ly);
    PixelChunk* GetOrCreateChunk(int cx, int cy);
    const PixelChunk* GetChunk(int cx, int cy) const;
    void SyncChunksToGPU();
    void SyncGPUToChunks();
    void GenerateChunkTerrain(PixelChunk& chunk);
};

#endif

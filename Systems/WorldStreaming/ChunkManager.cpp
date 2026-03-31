#include "ChunkManager.h"
#include <vector>
#include <cmath>

namespace WorldStreaming
{
    void ChunkManager::SetViewCenter(float worldX, float worldY)
    {
        m_viewCenter = WorldToChunk(worldX, worldY);
    }

    void ChunkManager::Update()
    {
        for (int dy = -m_loadRadius; dy <= m_loadRadius; ++dy)
        {
            for (int dx = -m_loadRadius; dx <= m_loadRadius; ++dx)
            {
                ChunkCoord cc{m_viewCenter.x + dx, m_viewCenter.y + dy};
                if (m_loadedChunks.contains(cc))
                    continue;

                auto chunk = std::make_unique<Chunk>();
                chunk->Initialize(cc);
                if (m_onChunkLoad)
                    m_onChunkLoad(*chunk);
                m_loadedChunks[cc] = std::move(chunk);
            }
        }

        std::vector<ChunkCoord> toUnload;
        for (auto& [coord, chunk] : m_loadedChunks)
        {
            int dx = coord.x - m_viewCenter.x;
            int dy = coord.y - m_viewCenter.y;
            if (dx * dx + dy * dy > m_unloadRadius * m_unloadRadius)
                toUnload.push_back(coord);
        }

        for (auto& coord : toUnload)
        {
            if (m_onChunkUnload)
                m_onChunkUnload(*m_loadedChunks[coord]);
            m_loadedChunks.erase(coord);
        }
    }

    Chunk* ChunkManager::GetChunk(ChunkCoord coord)
    {
        auto it = m_loadedChunks.find(coord);
        return it != m_loadedChunks.end() ? it->second.get() : nullptr;
    }

    Chunk* ChunkManager::GetChunkAt(float worldX, float worldY)
    {
        return GetChunk(WorldToChunk(worldX, worldY));
    }

    uint16_t ChunkManager::GetTileAt(float worldX, float worldY)
    {
        ChunkCoord cc;
        int lx, ly;
        WorldToLocal(worldX, worldY, cc, lx, ly);
        auto* chunk = GetChunk(cc);
        return chunk ? chunk->GetTile(lx, ly) : 0;
    }

    void ChunkManager::SetTileAt(float worldX, float worldY, uint16_t tileId)
    {
        ChunkCoord cc;
        int lx, ly;
        WorldToLocal(worldX, worldY, cc, lx, ly);
        auto* chunk = GetChunk(cc);
        if (chunk)
            chunk->SetTile(lx, ly, tileId);
    }

    ChunkCoord ChunkManager::WorldToChunk(float worldX, float worldY) const
    {
        float chunkWorld = m_tileSize * Chunk::SIZE;
        return {
            static_cast<int32_t>(std::floor(worldX / chunkWorld)),
            static_cast<int32_t>(std::floor(worldY / chunkWorld))
        };
    }

    void ChunkManager::WorldToLocal(float worldX, float worldY,
                                     ChunkCoord& outChunk, int& outLocalX, int& outLocalY) const
    {
        float chunkWorld = m_tileSize * Chunk::SIZE;
        outChunk = WorldToChunk(worldX, worldY);

        float localWorldX = worldX - outChunk.x * chunkWorld;
        float localWorldY = worldY - outChunk.y * chunkWorld;
        outLocalX = static_cast<int>(localWorldX / m_tileSize);
        outLocalY = static_cast<int>(localWorldY / m_tileSize);

        if (outLocalX < 0) outLocalX = 0;
        if (outLocalX >= Chunk::SIZE) outLocalX = Chunk::SIZE - 1;
        if (outLocalY < 0) outLocalY = 0;
        if (outLocalY >= Chunk::SIZE) outLocalY = Chunk::SIZE - 1;
    }
}

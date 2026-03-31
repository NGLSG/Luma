#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <cmath>

namespace WorldStreaming
{
    class ChunkManager
    {
    public:
        void SetViewCenter(float worldX, float worldY);
        void Update();
        Chunk* GetChunk(ChunkCoord coord);
        Chunk* GetChunkAt(float worldX, float worldY);
        uint16_t GetTileAt(float worldX, float worldY);
        void SetTileAt(float worldX, float worldY, uint16_t tileId);
        void SetLoadRadius(int chunks) { m_loadRadius = chunks; }
        void SetUnloadRadius(int chunks) { m_unloadRadius = chunks; }
        void SetTileSize(float size) { m_tileSize = size; }
        float GetTileSize() const { return m_tileSize; }

        void SetOnChunkLoad(std::function<void(Chunk&)> callback) { m_onChunkLoad = std::move(callback); }
        void SetOnChunkUnload(std::function<void(Chunk&)> callback) { m_onChunkUnload = std::move(callback); }

        int GetLoadedChunkCount() const { return static_cast<int>(m_loadedChunks.size()); }

    private:
        ChunkCoord WorldToChunk(float worldX, float worldY) const;
        void WorldToLocal(float worldX, float worldY, ChunkCoord& outChunk, int& outLocalX, int& outLocalY) const;

        std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_loadedChunks;
        ChunkCoord m_viewCenter{};
        int m_loadRadius = 4;
        int m_unloadRadius = 6;
        float m_tileSize = 16.0f;
        std::function<void(Chunk&)> m_onChunkLoad;
        std::function<void(Chunk&)> m_onChunkUnload;
    };
}

#endif

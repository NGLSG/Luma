#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>
#include <vector>
#include <functional>

namespace WorldStreaming
{
    struct ChunkCoord
    {
        int32_t x = 0;
        int32_t y = 0;
        bool operator==(const ChunkCoord&) const = default;
    };

    struct ChunkCoordHash
    {
        size_t operator()(const ChunkCoord& c) const
        {
            size_t h1 = std::hash<int32_t>{}(c.x);
            size_t h2 = std::hash<int32_t>{}(c.y);
            return h1 ^ (h2 << 1);
        }
    };

    class Chunk
    {
    public:
        static constexpr int SIZE = 64;

        ChunkCoord coord{};
        bool dirty = false;
        bool loaded = false;

        void Initialize(ChunkCoord c)
        {
            coord = c;
            tiles.assign(SIZE * SIZE, 0);
            dirty = false;
            loaded = true;
        }

        uint16_t GetTile(int localX, int localY) const
        {
            if (localX < 0 || localX >= SIZE || localY < 0 || localY >= SIZE)
                return 0;
            return tiles[localY * SIZE + localX];
        }

        void SetTile(int localX, int localY, uint16_t tileId)
        {
            if (localX < 0 || localX >= SIZE || localY < 0 || localY >= SIZE)
                return;
            tiles[localY * SIZE + localX] = tileId;
            dirty = true;
        }

        std::vector<uint16_t>& GetTiles() { return tiles; }
        const std::vector<uint16_t>& GetTiles() const { return tiles; }

    private:
        std::vector<uint16_t> tiles;
    };
}

#endif

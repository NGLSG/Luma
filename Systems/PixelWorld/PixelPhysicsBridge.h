#ifndef PIXEL_PHYSICS_BRIDGE_H
#define PIXEL_PHYSICS_BRIDGE_H

#include "PixelWorld.h"
#include <box2d/box2d.h>
#include <vector>
#include <unordered_map>

struct PixelChunkKey
{
    int cx, cy;
    bool operator==(const PixelChunkKey& o) const { return cx == o.cx && cy == o.cy; }
};

struct PixelChunkKeyHash
{
    size_t operator()(const PixelChunkKey& k) const
    {
        return std::hash<int>()(k.cx) ^ (std::hash<int>()(k.cy) << 16);
    }
};

struct PixelChunkBody
{
    b2BodyId body = b2_nullBodyId;
    b2ChainId chain = b2_nullChainId;
};

class LUMA_API PixelPhysicsBridge
{
public:
    static constexpr int CHUNK_SIZE = 32;

    static void SyncPixelWorldToPhysics(PixelWorld& world, b2WorldId physicsWorld, float pixelScale);

    static void ApplyRigidbodyDamage(PixelWorld& world, float worldX, float worldY, float radius, float pixelScale);

    static bool IsPixelSolid(const PixelWorld& world, float worldX, float worldY, float pixelScale);

    static b2ChainId CreateChainFromPixels(PixelWorld& world, b2BodyId body,
                                           int startX, int startY, int regionW, int regionH,
                                           float pixelScale);

private:
    static bool IsSolidPixel(uint32_t pixelType);
    static std::vector<b2Vec2> TraceEdges(PixelWorld& world, int startX, int startY, int regionW, int regionH, float pixelScale);

    static std::unordered_map<PixelChunkKey, PixelChunkBody, PixelChunkKeyHash> s_chunkBodies;
};

#endif

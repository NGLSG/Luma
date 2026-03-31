#include "PixelPhysicsBridge.h"
#include <cmath>
#include <algorithm>

std::unordered_map<PixelChunkKey, PixelChunkBody, PixelChunkKeyHash> PixelPhysicsBridge::s_chunkBodies;

bool PixelPhysicsBridge::IsSolidPixel(uint32_t pixelType)
{
    return pixelType == PixelType::Sand
        || pixelType == PixelType::Stone;
}

bool PixelPhysicsBridge::IsPixelSolid(const PixelWorld& world, float worldX, float worldY, float pixelScale)
{
    if (pixelScale <= 0.0f) return false;
    int px = static_cast<int>(worldX / pixelScale);
    int py = static_cast<int>(worldY / pixelScale);
    if (px < 0 || py < 0 || px >= (int)world.GetWidth() || py >= (int)world.GetHeight())
        return false;
    return IsSolidPixel(world.GetPixel(px, py));
}

void PixelPhysicsBridge::ApplyRigidbodyDamage(PixelWorld& world, float worldX, float worldY, float radius, float pixelScale)
{
    if (pixelScale <= 0.0f) return;

    int centerX = static_cast<int>(worldX / pixelScale);
    int centerY = static_cast<int>(worldY / pixelScale);
    int pixelRadius = static_cast<int>(std::ceil(radius / pixelScale));

    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());

    for (int dy = -pixelRadius; dy <= pixelRadius; ++dy)
    {
        for (int dx = -pixelRadius; dx <= pixelRadius; ++dx)
        {
            if (dx * dx + dy * dy > pixelRadius * pixelRadius)
                continue;

            int px = centerX + dx;
            int py = centerY + dy;
            if (px < 0 || py < 0 || px >= w || py >= h)
                continue;

            if (IsSolidPixel(world.GetPixel(px, py)))
            {
                world.SetPixel(px, py, PixelType::Air);
            }
        }
    }
}

std::vector<b2Vec2> PixelPhysicsBridge::TraceEdges(PixelWorld& world, int startX, int startY,
                                                    int regionW, int regionH, float pixelScale)
{
    std::vector<b2Vec2> points;
    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());

    auto isSolid = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= w || y >= h) return false;
        return IsSolidPixel(world.GetPixel(x, y));
    };

    int endX = std::min(startX + regionW, w);
    int endY = std::min(startY + regionH, h);

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            if (!isSolid(x, y)) continue;

            bool topExposed    = !isSolid(x, y - 1);
            bool bottomExposed = !isSolid(x, y + 1);
            bool leftExposed   = !isSolid(x - 1, y);
            bool rightExposed  = !isSolid(x + 1, y);

            if (!topExposed && !bottomExposed && !leftExposed && !rightExposed)
                continue;

            float wx = x * pixelScale;
            float wy = y * pixelScale;

            if (topExposed)
            {
                points.push_back({wx, wy});
                points.push_back({wx + pixelScale, wy});
            }
            if (bottomExposed)
            {
                points.push_back({wx + pixelScale, wy + pixelScale});
                points.push_back({wx, wy + pixelScale});
            }
            if (leftExposed)
            {
                points.push_back({wx, wy + pixelScale});
                points.push_back({wx, wy});
            }
            if (rightExposed)
            {
                points.push_back({wx + pixelScale, wy});
                points.push_back({wx + pixelScale, wy + pixelScale});
            }
        }
    }

    return points;
}

b2ChainId PixelPhysicsBridge::CreateChainFromPixels(PixelWorld& world, b2BodyId body,
                                                     int startX, int startY,
                                                     int regionW, int regionH, float pixelScale)
{
    auto edges = TraceEdges(world, startX, startY, regionW, regionH, pixelScale);
    if (edges.size() < 4)
        return b2_nullChainId;

    b2ChainDef chainDef = b2DefaultChainDef();
    chainDef.points = edges.data();
    chainDef.count = static_cast<int>(edges.size());
    chainDef.isLoop = true;

    return b2CreateChain(body, &chainDef);
}

void PixelPhysicsBridge::SyncPixelWorldToPhysics(PixelWorld& world, b2WorldId physicsWorld, float pixelScale)
{
    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());
    int chunksX = (w + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int chunksY = (h + CHUNK_SIZE - 1) / CHUNK_SIZE;

    for (int cy = 0; cy < chunksY; ++cy)
    {
        for (int cx = 0; cx < chunksX; ++cx)
        {
            PixelChunkKey key{cx, cy};

            auto it = s_chunkBodies.find(key);
            if (it != s_chunkBodies.end())
            {
                if (B2_IS_NON_NULL(it->second.chain))
                    b2DestroyChain(it->second.chain);
                if (B2_IS_NON_NULL(it->second.body))
                    b2DestroyBody(it->second.body);
                s_chunkBodies.erase(it);
            }

            int startX = cx * CHUNK_SIZE;
            int startY = cy * CHUNK_SIZE;
            int regionW = std::min(CHUNK_SIZE, w - startX);
            int regionH = std::min(CHUNK_SIZE, h - startY);

            bool hasSolid = false;
            for (int y = startY; y < startY + regionH && !hasSolid; ++y)
                for (int x = startX; x < startX + regionW && !hasSolid; ++x)
                    hasSolid = IsSolidPixel(world.GetPixel(x, y));

            if (!hasSolid) continue;

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_staticBody;
            bodyDef.position = {0.0f, 0.0f};
            b2BodyId body = b2CreateBody(physicsWorld, &bodyDef);

            b2ChainId chain = CreateChainFromPixels(world, body, startX, startY, regionW, regionH, pixelScale);

            if (B2_IS_NON_NULL(chain))
            {
                s_chunkBodies[key] = {body, chain};
            }
            else
            {
                b2DestroyBody(body);
            }
        }
    }
}

#include "TerrainGenerator.h"
#include "NoiseGenerator.h"
#include "../PixelWorld/PixelWorld.h"
#include "../WorldStreaming/Chunk.h"

int TerrainGenerator::SurfaceHeight(int worldX, const TerrainProfile& profile)
{
    float n = NoiseGenerator::FBM(
        (float)worldX * profile.surfaceFrequency, 0.0f,
        4, 2.0f, 0.5f, profile.seed);
    return profile.surfaceBaseY + (int)(n * profile.surfaceAmplitude);
}

bool TerrainGenerator::IsCave(int worldX, int worldY, const TerrainProfile& profile)
{
    float n = NoiseGenerator::Cellular(
        (float)worldX * profile.caveFrequency,
        (float)worldY * profile.caveFrequency,
        profile.seed + 999);
    return n < profile.caveThreshold;
}

void TerrainGenerator::GenerateForPixelWorld(PixelWorld& world, const TerrainProfile& profile)
{
    int w = (int)world.GetWidth();
    int h = (int)world.GetHeight();

    for (int x = 0; x < w; ++x)
    {
        int surfY = SurfaceHeight(x, profile);

        for (int y = 0; y < h; ++y)
        {
            if (y < surfY)
            {
                world.SetPixel(x, y, PixelType::Air);
                continue;
            }

            int depth = y - surfY;

            if (depth < 3)
            {
                world.SetPixel(x, y, PixelType::Sand);
                continue;
            }

            if (IsCave(x, y, profile))
            {
                world.SetPixel(x, y, PixelType::Air);
                continue;
            }

            // Deep liquid pools
            if (depth > 80)
            {
                float pool = NoiseGenerator::Perlin(
                    (float)x * 0.08f, (float)y * 0.08f, profile.seed + 777);
                if (pool > 0.75f)
                {
                    world.SetPixel(x, y, (depth > 120) ? PixelType::Lava : PixelType::Water);
                    continue;
                }
            }

            world.SetPixel(x, y, PixelType::Stone);
        }
    }
}

void TerrainGenerator::GenerateForChunk(WorldStreaming::Chunk& chunk, const TerrainProfile& profile)
{
    int ox = chunk.coord.x * WorldStreaming::Chunk::SIZE;
    int oy = chunk.coord.y * WorldStreaming::Chunk::SIZE;

    for (int lx = 0; lx < WorldStreaming::Chunk::SIZE; ++lx)
    {
        int wx = ox + lx;
        int surfY = SurfaceHeight(wx, profile);

        for (int ly = 0; ly < WorldStreaming::Chunk::SIZE; ++ly)
        {
            int wy = oy + ly;

            if (wy < surfY)
            {
                chunk.SetTile(lx, ly, PixelType::Air);
                continue;
            }

            int depth = wy - surfY;

            if (depth < 3)
            {
                chunk.SetTile(lx, ly, PixelType::Sand);
                continue;
            }

            if (IsCave(wx, wy, profile))
            {
                chunk.SetTile(lx, ly, PixelType::Air);
                continue;
            }

            if (depth > 80)
            {
                float pool = NoiseGenerator::Perlin(
                    (float)wx * 0.08f, (float)wy * 0.08f, profile.seed + 777);
                if (pool > 0.75f)
                {
                    chunk.SetTile(lx, ly, (depth > 120) ? PixelType::Lava : PixelType::Water);
                    continue;
                }
            }

            chunk.SetTile(lx, ly, PixelType::Stone);
        }
    }
}

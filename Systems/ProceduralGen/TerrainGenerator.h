#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <cstdint>

class PixelWorld;
namespace WorldStreaming { class Chunk; }

struct TerrainProfile
{
    int surfaceBaseY = 256;
    float surfaceAmplitude = 40.0f;
    float surfaceFrequency = 0.02f;
    float caveThreshold = 0.4f;
    float caveFrequency = 0.05f;
    int seed = 42;
};

class TerrainGenerator
{
public:
    static void GenerateForPixelWorld(PixelWorld& world, const TerrainProfile& profile);
    static void GenerateForChunk(WorldStreaming::Chunk& chunk, const TerrainProfile& profile);

private:
    static int SurfaceHeight(int worldX, const TerrainProfile& profile);
    static bool IsCave(int worldX, int worldY, const TerrainProfile& profile);
};

#endif

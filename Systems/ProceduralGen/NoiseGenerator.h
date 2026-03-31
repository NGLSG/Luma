#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H

#include <cstdint>

class NoiseGenerator
{
public:
    static float Perlin(float x, float y, int seed = 0);
    static float FBM(float x, float y, int octaves = 4, float lacunarity = 2.0f,
                     float persistence = 0.5f, int seed = 0);
    static float Cellular(float x, float y, int seed = 0);

private:
    static float Grad(int hash, float x, float y);
    static float Fade(float t);
    static float Lerp(float a, float b, float t);
    static int Hash2D(int x, int y, int seed);
};

#endif

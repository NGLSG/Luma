#include "NoiseGenerator.h"
#include <cmath>
#include <cfloat>

static constexpr int PERM[] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

int NoiseGenerator::Hash2D(int x, int y, int seed)
{
    int n = x + y * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
}

float NoiseGenerator::Fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float NoiseGenerator::Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float NoiseGenerator::Grad(int hash, float x, float y)
{
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float NoiseGenerator::Perlin(float x, float y, int seed)
{
    int xi = (int)std::floor(x) & 255;
    int yi = (int)std::floor(y) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = Fade(xf);
    float v = Fade(yf);

    int aa = PERM[(PERM[(xi + seed) & 255] + yi) & 255];
    int ab = PERM[(PERM[(xi + seed) & 255] + yi + 1) & 255];
    int ba = PERM[(PERM[(xi + 1 + seed) & 255] + yi) & 255];
    int bb = PERM[(PERM[(xi + 1 + seed) & 255] + yi + 1) & 255];

    float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u);
    float x2 = Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u);

    return (Lerp(x1, x2, v) + 1.0f) * 0.5f;
}

float NoiseGenerator::FBM(float x, float y, int octaves, float lacunarity,
                          float persistence, int seed)
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < octaves; ++i)
    {
        total += Perlin(x * frequency, y * frequency, seed + i) * amplitude;
        maxVal += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxVal;
}

float NoiseGenerator::Cellular(float x, float y, int seed)
{
    int xi = (int)std::floor(x);
    int yi = (int)std::floor(y);
    float minDist = FLT_MAX;

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int cx = xi + dx;
            int cy = yi + dy;
            int h = Hash2D(cx, cy, seed);
            float fx = cx + (float)(h & 0xffff) / 65535.0f;
            float fy = cy + (float)((h >> 16) & 0xffff) / 65535.0f;
            float dist = (x - fx) * (x - fx) + (y - fy) * (y - fy);
            if (dist < minDist) minDist = dist;
        }
    }

    return std::sqrt(minDist);
}

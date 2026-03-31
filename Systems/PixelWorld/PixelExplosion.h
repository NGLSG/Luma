#ifndef PIXEL_EXPLOSION_H
#define PIXEL_EXPLOSION_H

#include "PixelWorld.h"

struct ExplosionParams
{
    float worldX = 0.0f;
    float worldY = 0.0f;
    float radius = 32.0f;
    float force = 1.0f;
    bool createFire = true;
    bool createDebris = true;
};

class PixelExplosion
{
public:
    static void Explode(PixelWorld& world, const ExplosionParams& params, float pixelScale);
    static void DestroyRect(PixelWorld& world, int startX, int startY, int w, int h);
    static void FillCircle(PixelWorld& world, int cx, int cy, int radius, PixelType::Value type);
    static void DestroyLine(PixelWorld& world, int x0, int y0, int x1, int y1, int thickness);
};

#endif

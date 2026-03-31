#include "PixelExplosion.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace
{
    bool IsSolid(uint32_t t)
    {
        return t == PixelType::Sand || t == PixelType::Stone;
    }

    bool IsFlammable(uint32_t t)
    {
        return t == PixelType::Oil;
    }
}

void PixelExplosion::Explode(PixelWorld& world, const ExplosionParams& params, float pixelScale)
{
    int cx = static_cast<int>(params.worldX / pixelScale);
    int cy = static_cast<int>(params.worldY / pixelScale);
    int pr = static_cast<int>(params.radius / pixelScale);
    if (pr < 1) pr = 1;

    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());

    for (int dy = -pr; dy <= pr; ++dy)
    {
        for (int dx = -pr; dx <= pr; ++dx)
        {
            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || px >= w || py < 0 || py >= h)
                continue;

            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist > static_cast<float>(pr))
                continue;

            uint32_t cur = world.GetPixel(px, py);

            if (params.createFire && IsFlammable(cur))
            {
                world.SetPixel(px, py, PixelType::Fire);
                continue;
            }

            if (cur != PixelType::Air)
            {
                float edgeRatio = dist / static_cast<float>(pr);
                if (params.createDebris && edgeRatio > 0.7f && IsSolid(cur))
                {
                    if ((std::rand() & 3) == 0)
                    {
                        int ox = px + (dx > 0 ? 2 : -2);
                        int oy = py + (dy > 0 ? 2 : -2);
                        if (ox >= 0 && ox < w && oy >= 0 && oy < h &&
                            world.GetPixel(ox, oy) == PixelType::Air)
                        {
                            world.SetPixel(ox, oy, PixelType::Sand);
                        }
                    }
                }
                world.SetPixel(px, py, PixelType::Air);
            }
        }
    }

    if (params.createFire)
    {
        int fireR = std::max(1, pr / 3);
        for (int dy = -fireR; dy <= fireR; ++dy)
        {
            for (int dx = -fireR; dx <= fireR; ++dx)
            {
                if (dx * dx + dy * dy > fireR * fireR) continue;
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < w && py >= 0 && py < h &&
                    world.GetPixel(px, py) == PixelType::Air &&
                    (std::rand() % 3) == 0)
                {
                    world.SetPixel(px, py, PixelType::Fire);
                }
            }
        }
    }
}

void PixelExplosion::DestroyRect(PixelWorld& world, int startX, int startY, int w, int h)
{
    int ww = static_cast<int>(world.GetWidth());
    int wh = static_cast<int>(world.GetHeight());
    int ex = std::min(startX + w, ww);
    int ey = std::min(startY + h, wh);
    int sx = std::max(startX, 0);
    int sy = std::max(startY, 0);

    for (int y = sy; y < ey; ++y)
        for (int x = sx; x < ex; ++x)
            world.SetPixel(x, y, PixelType::Air);
}

void PixelExplosion::FillCircle(PixelWorld& world, int cx, int cy, int radius, PixelType::Value type)
{
    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());
    int r2 = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (dx * dx + dy * dy > r2) continue;
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < w && py >= 0 && py < h)
                world.SetPixel(px, py, type);
        }
    }
}

void PixelExplosion::DestroyLine(PixelWorld& world, int x0, int y0, int x1, int y1, int thickness)
{
    int w = static_cast<int>(world.GetWidth());
    int h = static_cast<int>(world.GetHeight());
    int half = thickness / 2;

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int cx = x0, cy = y0;
    for (;;)
    {
        for (int ty = -half; ty <= half; ++ty)
        {
            for (int tx = -half; tx <= half; ++tx)
            {
                int px = cx + tx;
                int py = cy + ty;
                if (px >= 0 && px < w && py >= 0 && py < h)
                    world.SetPixel(px, py, PixelType::Air);
            }
        }

        if (cx == x1 && cy == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
}

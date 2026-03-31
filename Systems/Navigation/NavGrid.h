#ifndef NAVGRID_H
#define NAVGRID_H

#include "../../Components/Core.h"
#include <vector>
#include <cstdint>
#include <utility>

namespace Navigation
{
    struct NavGrid
    {
        int width = 0;
        int height = 0;
        float cellSize = 32.0f;
        std::vector<uint8_t> walkable;
        ECS::Vector2f origin{0.0f, 0.0f};

        NavGrid() = default;

        NavGrid(int w, int h, float cs, ECS::Vector2f org = {0.0f, 0.0f})
            : width(w), height(h), cellSize(cs), origin(org),
              walkable(static_cast<size_t>(w * h), 1)
        {
        }

        bool InBounds(int x, int y) const
        {
            return x >= 0 && x < width && y >= 0 && y < height;
        }

        bool IsWalkable(int x, int y) const
        {
            if (!InBounds(x, y)) return false;
            return walkable[static_cast<size_t>(y * width + x)] != 0;
        }

        void SetWalkable(int x, int y, bool v)
        {
            if (!InBounds(x, y)) return;
            walkable[static_cast<size_t>(y * width + x)] = v ? 1 : 0;
        }

        ECS::Vector2f GridToWorld(int x, int y) const
        {
            return {
                origin.x + (static_cast<float>(x) + 0.5f) * cellSize,
                origin.y + (static_cast<float>(y) + 0.5f) * cellSize
            };
        }

        std::pair<int, int> WorldToGrid(ECS::Vector2f pos) const
        {
            int gx = static_cast<int>((pos.x - origin.x) / cellSize);
            int gy = static_cast<int>((pos.y - origin.y) / cellSize);
            return {gx, gy};
        }
    };
}

#endif

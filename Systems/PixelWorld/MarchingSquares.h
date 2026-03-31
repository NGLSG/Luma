#ifndef MARCHING_SQUARES_H
#define MARCHING_SQUARES_H

#include <vector>
#include "Components/Core.h"

class PixelWorld;

struct Contour
{
    std::vector<ECS::Vector2f> points;
    bool closed = true;
};

class MarchingSquares
{
public:
    static std::vector<Contour> Extract(
        const PixelWorld& world,
        int startX, int startY,
        int regionW, int regionH,
        float pixelScale,
        int step = 2
    );

    static Contour Simplify(const Contour& contour, float tolerance);

private:
    static bool IsSolid(uint32_t pixelType);
    static float PerpendicularDistance(const ECS::Vector2f& p,
                                      const ECS::Vector2f& a,
                                      const ECS::Vector2f& b);
    static void DouglasPeucker(const std::vector<ECS::Vector2f>& pts,
                               int start, int end,
                               float tolerance,
                               std::vector<bool>& keep);
};

#endif

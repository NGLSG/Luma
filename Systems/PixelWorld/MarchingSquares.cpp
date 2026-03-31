#include "MarchingSquares.h"
#include "PixelWorld.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

bool MarchingSquares::IsSolid(uint32_t pixelType)
{
    return pixelType == PixelType::Stone ||
           pixelType == PixelType::Sand ||
           pixelType == PixelType::Lava;
}

std::vector<Contour> MarchingSquares::Extract(
    const PixelWorld& world,
    int startX, int startY,
    int regionW, int regionH,
    float pixelScale,
    int step)
{
    if (step < 1) step = 1;

    int gridW = regionW / step;
    int gridH = regionH / step;
    if (gridW < 2 || gridH < 2)
        return {};

    auto sample = [&](int gx, int gy) -> bool
    {
        int wx = startX + gx * step;
        int wy = startY + gy * step;
        return IsSolid(world.GetPixel(wx, wy));
    };

    // edge key: pack two grid-coord pairs into uint64
    auto edgeKey = [&](int x1, int y1, int x2, int y2) -> uint64_t
    {
        if (x1 > x2 || (x1 == x2 && y1 > y2))
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        uint64_t a = (static_cast<uint64_t>(static_cast<uint16_t>(x1)) << 48) |
                     (static_cast<uint64_t>(static_cast<uint16_t>(y1)) << 32);
        uint64_t b = (static_cast<uint64_t>(static_cast<uint16_t>(x2)) << 16) |
                      static_cast<uint64_t>(static_cast<uint16_t>(y2));
        return a | b;
    };

    // edge segments: midpoint of grid edges
    struct Segment
    {
        ECS::Vector2f a, b;
    };

    // Marching squares: for each cell, generate 0-2 segments
    //   corners: TL=bit3, TR=bit2, BR=bit1, BL=bit0
    //   midpoints: top(T), right(R), bottom(B), left(L)
    std::vector<Segment> segments;

    for (int gy = 0; gy < gridH - 1; gy++)
    {
        for (int gx = 0; gx < gridW - 1; gx++)
        {
            bool tl = sample(gx, gy);
            bool tr = sample(gx + 1, gy);
            bool br = sample(gx + 1, gy + 1);
            bool bl = sample(gx, gy + 1);

            int caseIdx = (tl ? 8 : 0) | (tr ? 4 : 0) | (br ? 2 : 0) | (bl ? 1 : 0);

            if (caseIdx == 0 || caseIdx == 15)
                continue;

            float cx = static_cast<float>(startX + gx * step);
            float cy = static_cast<float>(startY + gy * step);
            float s = static_cast<float>(step);

            ECS::Vector2f T = {(cx + cx + s) * 0.5f * pixelScale, cy * pixelScale};
            ECS::Vector2f R = {(cx + s) * pixelScale, (cy + cy + s) * 0.5f * pixelScale};
            ECS::Vector2f B = {(cx + cx + s) * 0.5f * pixelScale, (cy + s) * pixelScale};
            ECS::Vector2f L = {cx * pixelScale, (cy + cy + s) * 0.5f * pixelScale};

            switch (caseIdx)
            {
            case 1:  segments.push_back({L, B}); break;
            case 2:  segments.push_back({B, R}); break;
            case 3:  segments.push_back({L, R}); break;
            case 4:  segments.push_back({T, R}); break;
            case 5:  segments.push_back({L, T}); segments.push_back({B, R}); break;
            case 6:  segments.push_back({T, B}); break;
            case 7:  segments.push_back({L, T}); break;
            case 8:  segments.push_back({T, L}); break;
            case 9:  segments.push_back({T, B}); break;
            case 10: segments.push_back({T, R}); segments.push_back({L, B}); break;
            case 11: segments.push_back({T, R}); break;
            case 12: segments.push_back({L, R}); break;
            case 13: segments.push_back({B, R}); break;
            case 14: segments.push_back({L, B}); break;
            default: break;
            }
        }
    }

    if (segments.empty())
        return {};

    // Chain segments into contours
    // Build adjacency: point -> list of segment indices
    auto pointKey = [](const ECS::Vector2f& p) -> uint64_t
    {
        uint32_t px = *reinterpret_cast<const uint32_t*>(&p.x);
        uint32_t py = *reinterpret_cast<const uint32_t*>(&p.y);
        return (static_cast<uint64_t>(px) << 32) | py;
    };

    std::unordered_multimap<uint64_t, size_t> adjacency;
    for (size_t i = 0; i < segments.size(); i++)
    {
        adjacency.emplace(pointKey(segments[i].a), i);
        adjacency.emplace(pointKey(segments[i].b), i);
    }

    std::vector<bool> used(segments.size(), false);
    std::vector<Contour> contours;

    for (size_t i = 0; i < segments.size(); i++)
    {
        if (used[i])
            continue;

        Contour contour;
        used[i] = true;
        contour.points.push_back(segments[i].a);
        contour.points.push_back(segments[i].b);

        // Extend forward
        bool extended = true;
        while (extended)
        {
            extended = false;
            uint64_t key = pointKey(contour.points.back());
            auto range = adjacency.equal_range(key);
            for (auto it = range.first; it != range.second; ++it)
            {
                size_t si = it->second;
                if (used[si])
                    continue;

                used[si] = true;
                auto& seg = segments[si];
                if (pointKey(seg.a) == key)
                    contour.points.push_back(seg.b);
                else
                    contour.points.push_back(seg.a);
                extended = true;
                break;
            }
        }

        // Extend backward
        extended = true;
        while (extended)
        {
            extended = false;
            uint64_t key = pointKey(contour.points.front());
            auto range = adjacency.equal_range(key);
            for (auto it = range.first; it != range.second; ++it)
            {
                size_t si = it->second;
                if (used[si])
                    continue;

                used[si] = true;
                auto& seg = segments[si];
                if (pointKey(seg.a) == key)
                    contour.points.insert(contour.points.begin(), seg.b);
                else
                    contour.points.insert(contour.points.begin(), seg.a);
                extended = true;
                break;
            }
        }

        // Check if closed
        if (contour.points.size() >= 3)
        {
            auto& first = contour.points.front();
            auto& last = contour.points.back();
            float dx = first.x - last.x;
            float dy = first.y - last.y;
            contour.closed = (dx * dx + dy * dy) < (pixelScale * pixelScale * 0.01f);
        }

        if (contour.points.size() >= 3)
            contours.push_back(std::move(contour));
    }

    return contours;
}

float MarchingSquares::PerpendicularDistance(const ECS::Vector2f& p,
                                            const ECS::Vector2f& a,
                                            const ECS::Vector2f& b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-12f)
    {
        float ex = p.x - a.x;
        float ey = p.y - a.y;
        return std::sqrt(ex * ex + ey * ey);
    }
    float area = std::abs(dx * (a.y - p.y) - (a.x - p.x) * dy);
    return area / std::sqrt(lenSq);
}

void MarchingSquares::DouglasPeucker(const std::vector<ECS::Vector2f>& pts,
                                     int start, int end,
                                     float tolerance,
                                     std::vector<bool>& keep)
{
    if (end - start < 2)
        return;

    float maxDist = 0.0f;
    int maxIdx = start;
    for (int i = start + 1; i < end; i++)
    {
        float d = PerpendicularDistance(pts[i], pts[start], pts[end]);
        if (d > maxDist)
        {
            maxDist = d;
            maxIdx = i;
        }
    }

    if (maxDist > tolerance)
    {
        keep[maxIdx] = true;
        DouglasPeucker(pts, start, maxIdx, tolerance, keep);
        DouglasPeucker(pts, maxIdx, end, tolerance, keep);
    }
}

Contour MarchingSquares::Simplify(const Contour& contour, float tolerance)
{
    if (contour.points.size() < 3)
        return contour;

    int n = static_cast<int>(contour.points.size());
    std::vector<bool> keep(n, false);
    keep[0] = true;
    keep[n - 1] = true;

    DouglasPeucker(contour.points, 0, n - 1, tolerance, keep);

    Contour result;
    result.closed = contour.closed;
    for (int i = 0; i < n; i++)
    {
        if (keep[i])
            result.points.push_back(contour.points[i]);
    }
    return result;
}

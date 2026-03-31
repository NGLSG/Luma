#include "Pathfinder.h"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace Navigation
{
    namespace
    {
        struct Node
        {
            int x, y;
            float g, f;
        };

        struct NodeCmp
        {
            bool operator()(const Node& a, const Node& b) const { return a.f > b.f; }
        };

        inline int PackKey(int x, int y) { return y * 0x10000 + x; }

        inline float Heuristic(int ax, int ay, int bx, int by)
        {
            float dx = static_cast<float>(ax - bx);
            float dy = static_cast<float>(ay - by);
            return std::sqrt(dx * dx + dy * dy);
        }

        static constexpr int DX8[] = {-1, 0, 1, 0, -1, -1, 1, 1};
        static constexpr int DY8[] = {0, -1, 0, 1, -1, 1, -1, 1};
        static constexpr int DX4[] = {-1, 0, 1, 0};
        static constexpr int DY4[] = {0, -1, 0, 1};
        static constexpr float SQRT2 = 1.41421356f;
    }

    PathResult Pathfinder::FindPath(const NavGrid& grid, const PathRequest& request)
    {
        PathResult result;
        auto [sx, sy] = grid.WorldToGrid(request.start);
        auto [ex, ey] = grid.WorldToGrid(request.end);

        if (!grid.InBounds(sx, sy) || !grid.InBounds(ex, ey))
            return result;
        if (!grid.IsWalkable(sx, sy) || !grid.IsWalkable(ex, ey))
            return result;

        if (sx == ex && sy == ey)
        {
            result.found = true;
            result.waypoints.push_back(grid.GridToWorld(ex, ey));
            return result;
        }

        const int dirCount = request.allowDiagonal ? 8 : 4;
        const int* dx = request.allowDiagonal ? DX8 : DX4;
        const int* dy = request.allowDiagonal ? DY8 : DY4;

        std::priority_queue<Node, std::vector<Node>, NodeCmp> open;
        std::unordered_map<int, float> gScore;
        std::unordered_map<int, int> cameFrom;

        int startKey = PackKey(sx, sy);
        int endKey = PackKey(ex, ey);

        gScore[startKey] = 0.0f;
        open.push({sx, sy, 0.0f, Heuristic(sx, sy, ex, ey)});

        bool pathFound = false;

        while (!open.empty())
        {
            Node cur = open.top();
            open.pop();

            int curKey = PackKey(cur.x, cur.y);
            if (curKey == endKey)
            {
                pathFound = true;
                break;
            }

            if (cur.g > gScore[curKey])
                continue;

            for (int d = 0; d < dirCount; ++d)
            {
                int nx = cur.x + dx[d];
                int ny = cur.y + dy[d];

                if (!grid.IsWalkable(nx, ny))
                    continue;

                if (d >= 4)
                {
                    if (!grid.IsWalkable(cur.x + dx[d], cur.y) ||
                        !grid.IsWalkable(cur.x, cur.y + dy[d]))
                        continue;
                }

                float moveCost = (d >= 4) ? SQRT2 : 1.0f;
                float ng = cur.g + moveCost;
                int nKey = PackKey(nx, ny);

                auto it = gScore.find(nKey);
                if (it == gScore.end() || ng < it->second)
                {
                    gScore[nKey] = ng;
                    cameFrom[nKey] = curKey;
                    open.push({nx, ny, ng, ng + Heuristic(nx, ny, ex, ey)});
                }
            }
        }

        if (!pathFound)
            return result;

        result.found = true;
        std::vector<std::pair<int, int>> gridPath;
        int traceKey = endKey;
        while (traceKey != startKey)
        {
            int ty = traceKey / 0x10000;
            int tx = traceKey - ty * 0x10000;
            gridPath.push_back({tx, ty});
            traceKey = cameFrom[traceKey];
        }
        std::reverse(gridPath.begin(), gridPath.end());

        result.waypoints.reserve(gridPath.size());
        for (auto& [gx, gy] : gridPath)
            result.waypoints.push_back(grid.GridToWorld(gx, gy));

        return result;
    }
}

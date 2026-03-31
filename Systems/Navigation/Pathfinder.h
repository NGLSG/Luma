#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "NavGrid.h"
#include "../../Components/Core.h"
#include <vector>

namespace Navigation
{
    struct PathRequest
    {
        ECS::Vector2f start;
        ECS::Vector2f end;
        bool allowDiagonal = true;
    };

    struct PathResult
    {
        bool found = false;
        std::vector<ECS::Vector2f> waypoints;
    };

    class Pathfinder
    {
    public:
        static PathResult FindPath(const NavGrid& grid, const PathRequest& request);
    };
}

#endif

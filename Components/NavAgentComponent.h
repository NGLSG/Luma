#ifndef NAVAGENTCOMPONENT_H
#define NAVAGENTCOMPONENT_H

#include "IComponent.h"
#include "Core.h"
#include <vector>

namespace ECS
{
    struct NavAgentComponent : IComponent
    {
        Vector2f destination{0.0f, 0.0f};
        float speed = 100.0f;
        std::vector<Vector2f> path;
        int currentWaypointIndex = 0;
        bool hasArrived = true;
        bool isPathRequested = false;
    };
}

#endif

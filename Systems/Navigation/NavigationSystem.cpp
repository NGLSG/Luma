#include "NavigationSystem.h"
#include "../../Components/NavAgentComponent.h"
#include "../../Components/Transform.h"
#include "../../Resources/RuntimeAsset/RuntimeScene.h"
#include <cmath>

namespace Systems
{
    void NavigationSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
    }

    void NavigationSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (m_grid.width == 0 || m_grid.height == 0)
            return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::NavAgentComponent, ECS::TransformComponent>();

        for (auto entity : view)
        {
            auto& agent = view.get<ECS::NavAgentComponent>(entity);
            auto& transform = view.get<ECS::TransformComponent>(entity);

            if (!agent.Enable)
                continue;

            if (agent.isPathRequested)
            {
                Navigation::PathRequest req;
                req.start = transform.position;
                req.end = agent.destination;
                req.allowDiagonal = true;

                auto result = Navigation::Pathfinder::FindPath(m_grid, req);
                agent.path = std::move(result.waypoints);
                agent.currentWaypointIndex = 0;
                agent.hasArrived = !result.found || agent.path.empty();
                agent.isPathRequested = false;
            }

            if (agent.hasArrived || agent.path.empty())
                continue;

            if (agent.currentWaypointIndex >= static_cast<int>(agent.path.size()))
            {
                agent.hasArrived = true;
                continue;
            }

            ECS::Vector2f target = agent.path[agent.currentWaypointIndex];
            ECS::Vector2f diff = target - transform.position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            float step = agent.speed * deltaTime;

            if (dist <= step)
            {
                transform.position = target;
                agent.currentWaypointIndex++;

                if (agent.currentWaypointIndex >= static_cast<int>(agent.path.size()))
                    agent.hasArrived = true;
            }
            else
            {
                ECS::Vector2f dir{diff.x / dist, diff.y / dist};
                transform.position.x += dir.x * step;
                transform.position.y += dir.y * step;
            }
        }
    }

    void NavigationSystem::OnDestroy(RuntimeScene* scene)
    {
    }

    void NavigationSystem::SetGrid(const Navigation::NavGrid& grid)
    {
        m_grid = grid;
    }

    Navigation::NavGrid& NavigationSystem::GetGrid()
    {
        return m_grid;
    }
}

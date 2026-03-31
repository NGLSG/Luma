#ifndef NAVIGATIONSYSTEM_H
#define NAVIGATIONSYSTEM_H

#include "../ISystem.h"
#include "NavGrid.h"
#include "Pathfinder.h"

namespace Systems
{
    class NavigationSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
        void OnDestroy(RuntimeScene* scene) override;

        void SetGrid(const Navigation::NavGrid& grid);
        Navigation::NavGrid& GetGrid();

    private:
        Navigation::NavGrid m_grid;
    };
}

#endif

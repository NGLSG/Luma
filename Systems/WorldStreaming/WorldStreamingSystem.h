#ifndef WORLDSTREAMINGSYSTEM_H
#define WORLDSTREAMINGSYSTEM_H

#include "../ISystem.h"

namespace Systems
{
    class WorldStreamingSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
        void OnDestroy(RuntimeScene* scene) override;
    };
}

#endif

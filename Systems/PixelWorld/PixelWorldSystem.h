#ifndef PIXEL_WORLD_SYSTEM_H
#define PIXEL_WORLD_SYSTEM_H

#include "../ISystem.h"
#include <memory>

namespace Nut { class NutContext; }

namespace Systems
{
    class PixelWorldSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
        void OnDestroy(RuntimeScene* scene) override;

    private:
        std::shared_ptr<Nut::NutContext> m_nutContext;
    };
}

#endif

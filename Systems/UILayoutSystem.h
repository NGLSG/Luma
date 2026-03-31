#ifndef UI_LAYOUT_SYSTEM_H
#define UI_LAYOUT_SYSTEM_H

#include "ISystem.h"
#include <entt/entt.hpp>

namespace Systems
{
    class UILayoutSystem final : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        void LayoutEntity(entt::registry& registry, entt::entity entity);
    };
}

#endif

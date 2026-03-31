#ifndef SUBSCENE_COMPONENT_H
#define SUBSCENE_COMPONENT_H

#include "IComponent.h"
#include "../Utils/Guid.h"

namespace ECS
{
    struct SubSceneComponent : IComponent
    {
        Guid sceneGuid;
        bool autoLoad = false;
        bool isLoaded = false;
        float loadDistance = 0.0f;
        bool additive = true;
    };
}

#endif

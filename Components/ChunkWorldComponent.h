#ifndef CHUNKWORLDCOMPONENT_H
#define CHUNKWORLDCOMPONENT_H

#include "IComponent.h"
#include "../Systems/WorldStreaming/ChunkManager.h"
#include <memory>

namespace ECS
{
    struct ChunkWorldComponent : IComponent
    {
        int loadRadius = 4;
        int unloadRadius = 6;
        float tileSize = 16.0f;
        std::shared_ptr<WorldStreaming::ChunkManager> chunkManager;
    };
}

#endif

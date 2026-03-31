#ifndef PIXEL_WORLD_COMPONENT_H
#define PIXEL_WORLD_COMPONENT_H

#include "IComponent.h"
#include <memory>

class PixelWorld;

namespace ECS
{
    struct PixelWorldComponent : IComponent
    {
        uint32_t worldWidth = 512;
        uint32_t worldHeight = 512;
        float pixelScale = 1.0f;
        bool paused = false;
        std::shared_ptr<PixelWorld> world;
    };
}

namespace YAML
{
    template <>
    struct convert<ECS::PixelWorldComponent>
    {
        static Node encode(const ECS::PixelWorldComponent& rhs)
        {
            Node node;
            node["Enable"] = rhs.Enable;
            node["worldWidth"] = rhs.worldWidth;
            node["worldHeight"] = rhs.worldHeight;
            node["pixelScale"] = rhs.pixelScale;
            node["paused"] = rhs.paused;
            return node;
        }

        static bool decode(const Node& node, ECS::PixelWorldComponent& rhs)
        {
            if (!node.IsMap()) return false;
            if (node["Enable"]) rhs.Enable = node["Enable"].as<bool>();
            if (node["worldWidth"]) rhs.worldWidth = node["worldWidth"].as<uint32_t>();
            if (node["worldHeight"]) rhs.worldHeight = node["worldHeight"].as<uint32_t>();
            if (node["pixelScale"]) rhs.pixelScale = node["pixelScale"].as<float>();
            if (node["paused"]) rhs.paused = node["paused"].as<bool>();
            return true;
        }
    };
}

#endif

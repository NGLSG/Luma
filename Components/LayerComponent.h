#ifndef LAYERCOMPONENT_H
#define LAYERCOMPONENT_H

#include "IComponent.h"
#include "yaml-cpp/node/node.h"
#include "ComponentRegistry.h"
#include "Utils/LayerMask.h"
#include <cstdint>

namespace ECS
{
    /**
     * @brief 层组件，用于标识实体所属的层（类似 Unity 的 Layer，但支持多选）
     *
     * 层用于控制渲染、物理碰撞、光照等系统的交互。
     * 每个 GameObject 可以属于多个层（0-31），使用 LayerMask 位掩码表示。
     */
    struct LayerComponent : IComponent
    {
        /// 层掩码，支持多层选择
        LayerMask layers;

        /**
         * @brief 默认构造函数，默认为 Layer 0 (Default)
         */
        LayerComponent() : layers(LayerMask::Only(0)) {}

        /**
         * @brief 构造函数，使用给定的层索引初始化（单层）
         * @param layerIndex 层索引 (0-31)
         */
        explicit LayerComponent(int layerIndex) : layers(LayerMask::Only(layerIndex)) {}

        /**
         * @brief 构造函数，使用给定的层掩码初始化
         * @param mask 层掩码
         */
        explicit LayerComponent(LayerMask mask) : layers(mask) {}

        /**
         * @brief 获取层掩码
         * @return 层掩码值
         */
        uint32_t GetLayerMask() const
        {
            return layers.value;
        }

        /**
         * @brief 检查是否在指定层
         * @param layer 层索引 (0-31)
         * @return 是否在该层
         */
        bool IsInLayer(int layer) const
        {
            return layers.Contains(layer);
        }

        /**
         * @brief 检查是否与另一个层掩码有交集
         * @param other 另一个层掩码
         * @return 是否有交集
         */
        bool Intersects(const LayerMask& other) const
        {
            return layers.Intersects(other);
        }
    };
}

namespace YAML
{
    template <>
    struct convert<ECS::LayerComponent>
    {
        static Node encode(const ECS::LayerComponent& comp)
        {
            Node node;
            node["layers"] = comp.layers.value;
            return node;
        }

        static bool decode(const Node& node, ECS::LayerComponent& comp)
        {
            if (!node.IsMap())
                return false;
            // 兼容旧格式 (单层 int)
            if (node["layer"])
            {
                int layer = node["layer"].as<int>();
                if (layer < 0) layer = 0;
                if (layer > 31) layer = 31;
                comp.layers = LayerMask::Only(layer);
                return true;
            }
            // 新格式 (LayerMask)
            if (node["layers"])
            {
                comp.layers.value = node["layers"].as<uint32_t>();
                return true;
            }
            return false;
        }
    };
}

// 注册 LayerComponent 到组件注册系统
REGISTRY
{
    Registry_<ECS::LayerComponent>("LayerComponent")
        .SetNonRemovable()
    .SetHidden()
        .property("layers", &ECS::LayerComponent::layers);
}

#endif

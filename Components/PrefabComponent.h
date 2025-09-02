#ifndef PREFABCOMPONENT_H
#define PREFABCOMPONENT_H
#include "ComponentRegistry.h"
#include "IComponent.h"

namespace ECS
{
    /**
     * @brief 预制组件。
     */
    struct PrefabComponent : public IComponent
    {
        ///< 源预制资源句柄。
        AssetHandle sourcePrefab = AssetHandle(AssetType::Prefab);
    };
}

namespace YAML
{
    template <>
    struct convert<ECS::PrefabComponent>
    {
        /**
         * @brief 将PrefabComponent编码为YAML节点。
         * @param comp 要编码的PrefabComponent组件。
         * @return 编码后的YAML节点。
         */
        static Node encode(const ECS::PrefabComponent& comp)
        {
            Node node;
            node["sourcePrefab"] = comp.sourcePrefab.assetGuid;
            return node;
        }

        /**
         * @brief 将YAML节点解码为PrefabComponent。
         * @param node 要解码的YAML节点。
         * @param comp 解码后的PrefabComponent组件。
         * @return 解码是否成功。
         */
        static bool decode(const Node& node, ECS::PrefabComponent& comp)
        {
            if (!node.IsMap())
                return false;

            comp.sourcePrefab.assetGuid = node["sourcePrefab"].as<Guid>();
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::PrefabComponent>("PrefabComponent")
        .SetHidden()
        .property("sourcePrefab", &ECS::PrefabComponent::sourcePrefab);
}
#endif
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "IData.h"
#include "../Utils/Guid.h"
#include "../Components/AssetHandle.h"
#include "include/core/SkRefCnt.h"

namespace Data
{
    /**
     * @brief 表示预制体中的一个节点。
     *
     * 一个预制体节点可以包含名称、本地唯一标识符、对源预制体的引用、
     * 附加的组件数据以及子节点列表。
     */
    struct PrefabNode
    {
        mutable Guid localGuid; ///< 节点的本地唯一标识符。
        std::string name;       ///< 节点的名称。

        AssetHandle prefabSource = AssetHandle(AssetType::Prefab); ///< 如果此节点是另一个预制体的实例，则指向其源预制体的句柄。

        std::unordered_map<std::string, YAML::Node> components; ///< 附加到此节点上的组件数据，以组件类型字符串为键。
        std::vector<PrefabNode> children;                       ///< 此节点的子节点列表。
    };

    /**
     * @brief 表示一个完整的预制体数据。
     *
     * 继承自 IData，提供预制体数据的基本结构和类型信息。
     */
    struct PrefabData : public IData<PrefabData>
    {
    public:
        friend class IData<PrefabData>;

        PrefabNode root; ///< 预制体的根节点。

    private:
        static constexpr const char* TypeName = "prefab"; ///< 预制体数据类型的名称字符串。
    };
} // namespace Data

namespace YAML
{
    /**
     * @brief YAML 库用于 Data::PrefabNode 类型的转换器特化。
     *
     * 允许 YAML 库将 PrefabNode 对象编码为 YAML 节点，以及从 YAML 节点解码为 PrefabNode 对象。
     */
    template <>
    struct convert<Data::PrefabNode>
    {
        /**
         * @brief 将 Data::PrefabNode 对象编码为 YAML 节点。
         * @param rhs 要编码的 PrefabNode 对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const Data::PrefabNode& rhs)
        {
            Node node;
            node["localGuid"] = rhs.localGuid;
            node["name"] = rhs.name;
            if (rhs.prefabSource.Valid())
            {
                node["prefabSource"] = rhs.prefabSource;
            }
            node["components"] = rhs.components;
            node["children"] = rhs.children;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 Data::PrefabNode 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 PrefabNode 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Data::PrefabNode& rhs)
        {
            if (!node.IsMap() || !node["localGuid"])
                return false;

            rhs.localGuid = node["localGuid"].as<Guid>();
            rhs.name = node["name"].as<std::string>("");
            if (node["prefabSource"])
            {
                rhs.prefabSource = node["prefabSource"].as<AssetHandle>();
            }
            if (node["components"])
            {
                rhs.components = node["components"].as<std::unordered_map<std::string, YAML::Node>>();
            }
            if (node["children"])
            {
                rhs.children = node["children"].as<std::vector<Data::PrefabNode>>();
            }
            return true;
        }
    };

    /**
     * @brief YAML 库用于 Data::PrefabData 类型的转换器特化。
     *
     * 允许 YAML 库将 PrefabData 对象编码为 YAML 节点，以及从 YAML 节点解码为 PrefabData 对象。
     */
    template <>
    struct convert<Data::PrefabData>
    {
        /**
         * @brief 将 Data::PrefabData 对象编码为 YAML 节点。
         * @param rhs 要编码的 PrefabData 对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const Data::PrefabData& rhs)
        {
            Node node;

            node = rhs.root;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 Data::PrefabData 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 PrefabData 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Data::PrefabData& rhs)
        {
            if (!node.IsMap())
                return false;

            rhs.root = node.as<Data::PrefabNode>();
            return true;
        }
    };
} // namespace YAML
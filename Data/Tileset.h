#ifndef LUMAENGINE_TILESET_H
#define LUMAENGINE_TILESET_H


#include <vector>
#include <yaml-cpp/yaml.h>

#include "AssetHandle.h"
#include "IData.h"

/**
 * @brief 表示瓦片集的数据结构。
 *
 * 继承自 Data::IData，用于数据管理系统。
 */
struct TilesetData : public Data::IData<TilesetData>
{
    std::vector<AssetHandle> tiles; ///< 瓦片集中的所有瓦片的资产句柄列表。
};


namespace YAML
{
    /**
     * @brief 为 TilesetData 提供 YAML 序列化和反序列化转换器。
     *
     * 允许将 TilesetData 对象编码为 YAML 节点，或从 YAML 节点解码为 TilesetData 对象。
     */
    template <>
    struct convert<TilesetData>
    {
        /**
         * @brief 将 TilesetData 对象编码为 YAML 节点。
         * @param rhs 要编码的 TilesetData 对象。
         * @return 表示 TilesetData 对象的 YAML 节点。
         */
        static Node encode(const TilesetData& rhs)
        {
            Node node;
            node["tiles"] = rhs.tiles;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 TilesetData 对象。
         * @param node 包含 TilesetData 数据的 YAML 节点。
         * @param rhs 用于存储解码结果的 TilesetData 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, TilesetData& rhs)
        {
            if (!node.IsMap() || !node["tiles"]) return false;
            rhs.tiles = node["tiles"].as<std::vector<AssetHandle>>();
            return true;
        }
    };
}
#endif
#ifndef LUMAENGINE_RULETILE_H
#define LUMAENGINE_RULETILE_H


#pragma once

#include "Tile.h"
#include <array>
#include <vector>

/**
 * @brief 定义了邻居瓦片的规则类型。
 */
enum class NeighborRule {
    DontCare,       ///< 不关心邻居瓦片的状态。
    MustBeThis,     ///< 邻居瓦片必须是指定的类型。
    MustNotBeThis   ///< 邻居瓦片不能是指定的类型。
};

/**
 * @brief 定义了一个瓦片规则，包含满足规则时生成的结果瓦片句柄和周围邻居的规则。
 */
struct Rule
{
    AssetHandle resultTileHandle;           ///< 满足规则时生成的结果瓦片的资源句柄。
    std::array<NeighborRule, 8> neighbors;  ///< 定义了周围8个邻居瓦片的规则。
};

/**
 * @brief 规则瓦片资产的数据结构，用于存储默认瓦片和一系列规则。
 */
struct RuleTileAssetData : Data::IData<RuleTileAssetData>
{
    AssetHandle defaultTileHandle;  ///< 默认瓦片的资源句柄。
    std::vector<Rule> rules;        ///< 瓦片规则的集合。
};


namespace YAML
{
    /**
     * @brief YAML::Node 到 NeighborRule 枚举类型的转换特化。
     */
    template <>
    struct convert<NeighborRule>
    {
        /**
         * @brief 将 NeighborRule 枚举编码为 YAML::Node。
         * @param rule 要编码的 NeighborRule 枚举值。
         * @return 编码后的 YAML::Node。
         */
        static Node encode(const NeighborRule& rule)
        {
            Node node;
            node = static_cast<int>(rule);
            return node;
        }

        /**
         * @brief 将 YAML::Node 解码为 NeighborRule 枚举。
         * @param node 要解码的 YAML::Node。
         * @param rule 解码后的 NeighborRule 枚举值。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, NeighborRule& rule)
        {
            if (!node.IsScalar())
                return false;
            int value = node.as<int>();
            if (value < 0 || value > 2)
                return false;
            rule = static_cast<NeighborRule>(value);
            return true;
        }
    };

    /**
     * @brief YAML::Node 到 Rule 结构体的转换特化。
     */
    template <>
    struct convert<Rule>
    {
        /**
         * @brief 将 Rule 结构体编码为 YAML::Node。
         * @param rule 要编码的 Rule 结构体。
         * @return 编码后的 YAML::Node。
         */
        static Node encode(const Rule& rule)
        {
            Node node;
            node["resultTileHandle"] = rule.resultTileHandle;
            node["neighbors"] = rule.neighbors;
            return node;
        }

        /**
         * @brief 将 YAML::Node 解码为 Rule 结构体。
         * @param node 要解码的 YAML::Node。
         * @param rule 解码后的 Rule 结构体。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Rule& rule)
        {
            if (!node.IsMap())
                return false;

            if (!node["resultTileHandle"] || !node["neighbors"])
                return false;

            rule.resultTileHandle = node["resultTileHandle"].as<AssetHandle>();
            rule.neighbors = node["neighbors"].as<std::array<NeighborRule, 8>>();
            return true;
        }
    };

    /**
     * @brief YAML::Node 到 RuleTileAssetData 结构体的转换特化。
     */
    template <>
    struct convert<RuleTileAssetData>
    {
        /**
         * @brief 将 RuleTileAssetData 结构体编码为 YAML::Node。
         * @param rhs 要编码的 RuleTileAssetData 结构体。
         * @return 编码后的 YAML::Node。
         */
        static Node encode(const RuleTileAssetData& rhs)
        {
            Node node;
            node["defaultTileHandle"] = rhs.defaultTileHandle;
            node["rules"] = rhs.rules;
            return node;
        }

        /**
         * @brief 将 YAML::Node 解码为 RuleTileAssetData 结构体。
         * @param node 要解码的 YAML::Node。
         * @param rhs 解码后的 RuleTileAssetData 结构体。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, RuleTileAssetData& rhs)
        {
            if (!node.IsMap())
                return false;

            if (node["defaultTileHandle"])
                rhs.defaultTileHandle = node["defaultTileHandle"].as<AssetHandle>();
            if (node["rules"])
                rhs.rules = node["rules"].as<std::vector<Rule>>();
            return true;
        }
    };
}
#endif
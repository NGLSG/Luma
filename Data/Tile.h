#pragma once

#include "Components/Sprite.h"
#include <variant>
#include <yaml-cpp/yaml.h>

/**
 * @brief 定义瓦片的类型。
 */
enum class TileType {
    Sprite, ///< 精灵瓦片类型。
    Prefab  ///< 预制体瓦片类型。
};

/**
 * @brief 存储精灵瓦片的数据。
 */
struct SpriteTileData
{
    AssetHandle textureHandle; ///< 纹理资源的句柄。
    ECS::RectF sourceRect = {0.0f, 0.0f, 0.0f, 0.0f}; ///< 纹理的源矩形区域。
    ECS::Color color = ECS::Colors::White; ///< 瓦片的颜色。
    ECS::FilterQuality filterQuality = ECS::FilterQuality::Bilinear; ///< 纹理过滤质量。
    ECS::WrapMode wrapMode = ECS::WrapMode::Clamp; ///< 纹理环绕模式。
};

/**
 * @brief 存储预制体瓦片的数据。
 */
struct PrefabTileData
{
    AssetHandle prefabHandle; ///< 预制体资源的句柄。
};

/// @brief 瓦片资产数据的变体类型，可以是精灵瓦片数据或预制体瓦片数据。
using TileAssetData = std::variant<SpriteTileData, PrefabTileData>;

namespace YAML
{
    /**
     * @brief YAML 转换器，用于将 SpriteTileData 对象编码和解码为 YAML 节点。
     * @tparam SpriteTileData 要转换的类型。
     */
    template <>
    struct convert<SpriteTileData>
    {
        /**
         * @brief 将 SpriteTileData 对象编码为 YAML 节点。
         * @param rhs 要编码的 SpriteTileData 对象。
         * @return 表示 SpriteTileData 对象的 YAML 节点。
         */
        static Node encode(const SpriteTileData& rhs)
        {
            Node node;
            node["textureHandle"] = rhs.textureHandle;
            node["sourceRect"] = rhs.sourceRect;
            node["color"] = rhs.color;
            node["filterQuality"] = static_cast<int>(rhs.filterQuality);
            node["wrapMode"] = static_cast<int>(rhs.wrapMode);
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 SpriteTileData 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后存储 SpriteTileData 对象的引用。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, SpriteTileData& rhs)
        {
            if (!node.IsMap() || !node["textureHandle"] || !node["sourceRect"] || !node["color"])
                return false;
            rhs.textureHandle = node["textureHandle"].as<AssetHandle>();
            rhs.sourceRect = node["sourceRect"].as<ECS::RectF>();
            rhs.color = node["color"].as<ECS::Color>();
            if (node["filterQuality"])
                rhs.filterQuality = static_cast<ECS::FilterQuality>(node["filterQuality"].as<int>());
            if (node["wrapMode"])
                rhs.wrapMode = static_cast<ECS::WrapMode>(node["wrapMode"].as<int>());
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于将 PrefabTileData 对象编码和解码为 YAML 节点。
     * @tparam PrefabTileData 要转换的类型。
     */
    template <>
    struct convert<PrefabTileData>
    {
        /**
         * @brief 将 PrefabTileData 对象编码为 YAML 节点。
         * @param rhs 要编码的 PrefabTileData 对象。
         * @return 表示 PrefabTileData 对象的 YAML 节点。
         */
        static Node encode(const PrefabTileData& rhs)
        {
            Node node;
            node["prefabHandle"] = rhs.prefabHandle;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 PrefabTileData 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后存储 PrefabTileData 对象的引用。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, PrefabTileData& rhs)
        {
            if (!node.IsMap() || !node["prefabHandle"])
            {
                return false;
            }
            rhs.prefabHandle = node["prefabHandle"].as<AssetHandle>();
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于将 TileAssetData 对象编码和解码为 YAML 节点。
     * @tparam TileAssetData 要转换的类型。
     */
    template <>
    struct convert<TileAssetData>
    {
        /**
         * @brief 将 TileAssetData 对象编码为 YAML 节点。
         * @param rhs 要编码的 TileAssetData 对象。
         * @return 表示 TileAssetData 对象的 YAML 节点。
         */
        static Node encode(const TileAssetData& rhs)
        {
            Node node;
            std::visit([&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, SpriteTileData>)
                {
                    node["Type"] = "Sprite";
                    node["Data"] = arg;
                }
                else if constexpr (std::is_same_v<T, PrefabTileData>)
                {
                    node["Type"] = "Prefab";
                    node["Data"] = arg;
                }
            }, rhs);
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 TileAssetData 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后存储 TileAssetData 对象的引用。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, TileAssetData& rhs)
        {
            if (!node.IsMap() || !node["Type"]) return false;
            std::string type = node["Type"].as<std::string>();
            if (type == "Sprite") rhs = node["Data"].as<SpriteTileData>();
            else if (type == "Prefab") rhs = node["Data"].as<PrefabTileData>();
            else return false;
            return true;
        }
    };
}
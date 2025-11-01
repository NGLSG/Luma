#ifndef LUMAENGINE_TILEMAPCOMPONENT_H
#define LUMAENGINE_TILEMAPCOMPONENT_H
#include "IComponent.h"
#include "Sprite.h"


#include "IComponent.h"
#include <unordered_map>
#include <cmath>

#include "RuleTile.h"
#include "Tile.h"

namespace ECS
{
    /**
     * @brief 用于计算Vector2i哈希值的结构体。
     */
    struct Vector2iHash
    {
        /**
         * @brief 计算给定Vector2i的哈希值。
         * @param v 要计算哈希值的Vector2i对象。
         * @return 计算出的哈希值。
         */
        std::size_t operator()(const Vector2i& v) const
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };

    /**
     * @brief 用于计算Vector2f哈希值的结构体。
     * @note 注意：此哈希函数内部使用了float哈希，但输入参数为Vector2i。
     */
    struct Vector2fHash
    {
        /**
         * @brief 计算给定Vector2f的哈希值。
         * @param v 要计算哈希值的Vector2f对象。
         * @return 计算出的哈希值。
         */
        std::size_t operator()(const Vector2f& v) const
        {
            // 使用量化以减少浮点误差对哈希的影响
            const float q = 1e-3f;
            int xi = static_cast<int>(std::round(v.x / q));
            int yi = static_cast<int>(std::round(v.y / q));
            return std::hash<int>()(xi) ^ (std::hash<int>()(yi) << 1);
        }
    };

    /**
     * @brief 表示一个已解析的瓦片信息。
     */
    struct ResolvedTile
    {
        AssetHandle sourceTileAsset; ///< 源瓦片资源的句柄。
        TileAssetData data; ///< 瓦片资产数据。
    };

    /**
     * @brief 瓦片地图组件，用于存储瓦片数据和配置。
     */
    struct TilemapComponent : public IComponent
    {
        Vector2f cellSize = {100.0f, 100.0f}; ///< 瓦片单元格的大小。

        std::unordered_map<Vector2i, AssetHandle, Vector2iHash> normalTiles; ///< 存储普通瓦片的映射，键为瓦片位置，值为瓦片资产句柄。
        std::unordered_map<Vector2i, AssetHandle, Vector2iHash> ruleTiles; ///< 存储规则瓦片的映射，键为瓦片位置，值为瓦片资产句柄。


        std::unordered_map<Vector2i, ResolvedTile, Vector2iHash> runtimeTileCache; ///< 运行时瓦片缓存，键为瓦片位置，值为已解析的瓦片信息。
        std::unordered_map<Vector2i, Guid, Vector2iHash> instantiatedPrefabs; ///< 已实例化的预制体映射，键为瓦片位置，值为预制体的全局唯一标识符。
    };

    /**
     * @brief 瓦片地图渲染器组件，用于控制瓦片地图的渲染属性。
     */
    struct TilemapRendererComponent : public IComponent
    {
        int zIndex = 0; ///< 渲染层级，用于控制渲染顺序。
        AssetHandle materialHandle; ///< 渲染材质的资产句柄。


        sk_sp<Material> material = nullptr; ///< 渲染使用的材质对象。


        /**
         * @brief 表示一个已水合（hydrated）的精灵瓦片。
         */
        struct HydratedSpriteTile
        {
            sk_sp<RuntimeTexture> image; ///< 精灵瓦片的运行时纹理。
            SkRect sourceRect; ///< 纹理中精灵的源矩形区域。
            Color color; ///< 精灵的颜色。
            FilterQuality filterQuality; ///< 纹理过滤质量。
            WrapMode wrapMode; ///< 纹理环绕模式。
        };


        std::unordered_map<Guid, HydratedSpriteTile> hydratedSpriteTiles; ///< 已水合的精灵瓦片映射，键为GUID，值为水合精灵瓦片数据。


        /**
         * @brief 表示一个已水合的规则。
         */
        struct HydratedRule
        {
            AssetHandle resultTileHandle; ///< 规则结果瓦片的资产句柄。
            std::array<NeighborRule, 8> neighbors; ///< 邻居规则数组。
        };

        /**
         * @brief 表示一个已水合的规则瓦片。
         */
        struct HydratedRuleTile
        {
            AssetHandle defaultTileHandle; ///< 默认瓦片的资产句柄。
            std::vector<HydratedRule> rules; ///< 规则列表。
        };


        std::unordered_map<Guid, HydratedRuleTile> hydratedRuleTiles; ///< 已水合的规则瓦片映射，键为GUID，值为水合规则瓦片数据。
    };
}

namespace YAML
{
    /**
     * @brief YAML转换器，用于ECS::TilemapComponent的序列化和反序列化。
     * @tparam ECS::TilemapComponent 要转换的瓦片地图组件类型。
     */
    template <>
    struct convert<ECS::TilemapComponent>
    {
        /**
         * @brief 将ECS::TilemapComponent对象编码为YAML节点。
         * @param rhs 要编码的ECS::TilemapComponent对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const ECS::TilemapComponent& rhs)
        {
            Node node;
            node["cellSize"] = rhs.cellSize;
            node["normalTiles"] = rhs.normalTiles;
            node["ruleTiles"] = rhs.ruleTiles;
            return node;
        }

        /**
         * @brief 从YAML节点解码ECS::TilemapComponent对象。
         * @param node 包含瓦片地图组件数据的YAML节点。
         * @param rhs 要填充的ECS::TilemapComponent对象。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::TilemapComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.cellSize = node["cellSize"].as<ECS::Vector2f>(ECS::Vector2f{100.0f, 100.0f});
            if (node["normalTiles"])
                rhs.normalTiles = node["normalTiles"].as<std::unordered_map<
                    ECS::Vector2i, AssetHandle, ECS::Vector2iHash>>();
            if (node["ruleTiles"])
                rhs.ruleTiles = node["ruleTiles"].as<std::unordered_map<
                    ECS::Vector2i, AssetHandle, ECS::Vector2iHash>>();
            return true;
        }
    };

    /**
     * @brief YAML转换器，用于ECS::TilemapRendererComponent的序列化和反序列化。
     * @tparam ECS::TilemapRendererComponent 要转换的瓦片地图渲染器组件类型。
     */
    template <>
    struct convert<ECS::TilemapRendererComponent>
    {
        /**
         * @brief 将ECS::TilemapRendererComponent对象编码为YAML节点。
         * @param rhs 要编码的ECS::TilemapRendererComponent对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const ECS::TilemapRendererComponent& rhs)
        {
            Node node;
            node["zIndex"] = rhs.zIndex;
            node["materialHandle"] = rhs.materialHandle;
            return node;
        }

        /**
         * @brief 从YAML节点解码ECS::TilemapRendererComponent对象。
         * @param node 包含瓦片地图渲染器组件数据的YAML节点。
         * @param rhs 要填充的ECS::TilemapRendererComponent对象。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::TilemapRendererComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.zIndex = node["zIndex"].as<int>(0);
            rhs.materialHandle = node["materialHandle"].as<AssetHandle>();
            return true;
        }
    };
}


REGISTRY
{
    Registry_<ECS::TilemapComponent>("TilemapComponent")
        .property("cellSize", &ECS::TilemapComponent::cellSize)
        .property("normalTiles", &ECS::TilemapComponent::normalTiles, false)
        .property("ruleTiles", &ECS::TilemapComponent::ruleTiles, false);

    Registry_<ECS::TilemapRendererComponent>("TilemapRendererComponent")
        .property("zIndex", &ECS::TilemapRendererComponent::zIndex)
        .property("materialHandle", &ECS::TilemapRendererComponent::materialHandle);
}
#endif

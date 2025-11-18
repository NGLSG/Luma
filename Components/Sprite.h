#ifndef SPRITE_H
#define SPRITE_H

#include "Core.h"
#include "IComponent.h"
#include "AssetHandle.h"
#include <include/core/SkImage.h>
#include <include/core/SkRefCnt.h>
#include "../Renderer/RenderComponent.h"
#include "ComponentRegistry.h"
#include "RuntimeAsset/RuntimeTexture.h"
#include "RuntimeAsset/RuntimeWGSLMaterial.h"

namespace ECS
{
    /**
     * @brief 精灵组件，用于表示游戏中的可渲染精灵。
     *
     * 包含精灵的纹理、材质、裁剪区域、颜色和渲染层级等信息。
     */
    struct SpriteComponent : public IComponent
    {
        AssetHandle textureHandle = AssetHandle(AssetType::Texture); ///< 纹理资源的句柄。
        AssetHandle materialHandle = AssetHandle(AssetType::Material); ///< 材质资源的句柄。
        RectF sourceRect = {0.0f, 0.0f, 0.0f, 0.0f}; ///< 纹理中用于渲染的源矩形区域。
        Color color = Colors::White; ///< 精灵的颜色。
        int zIndex = 0; ///< 精灵的Z轴渲染顺序，值越大越靠前。

        /**
         * @brief 构造一个新的精灵组件。
         * @param initialTextureHandle 初始纹理资源的句柄。
         * @param initialColor 初始颜色，默认为白色。
         */
        SpriteComponent(const AssetHandle& initialTextureHandle, const Color& initialColor = Colors::White)
            : textureHandle(initialTextureHandle), color(initialColor)
        {
        }

        /**
         * @brief 默认构造函数。
         */
        SpriteComponent() = default;


        sk_sp<RuntimeTexture> image = nullptr; ///< 运行时纹理对象。
        sk_sp<Material> material = nullptr; ///< 运行时材质对象（SkSL - 已弃用）。
        sk_sp<RuntimeWGSLMaterial> wgslMaterial = nullptr; ///< 运行时WGSL材质对象（推荐）。
        AssetHandle lastSpriteHandle = AssetHandle(AssetType::Texture); ///< 上次使用的精灵纹理句柄，用于检测变化。
        AssetHandle lastMaterialHandle = AssetHandle(AssetType::Material); ///< 上次使用的材质句柄，用于检测变化。
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于序列化和反序列化 ECS::SpriteComponent。
     */
    template <>
    struct convert<ECS::SpriteComponent>
    {
        /**
         * @brief 将 SpriteComponent 对象编码为 YAML 节点。
         * @param sprite 要编码的精灵组件。
         * @return 表示精灵组件的 YAML 节点。
         */
        static Node encode(const ECS::SpriteComponent& sprite)
        {
            Node node;
            node["textureHandle"] = sprite.textureHandle;
            if (sprite.materialHandle.Valid())
                node["materialHandle"] = sprite.materialHandle;
            node["sourceRect"] = sprite.sourceRect;
            node["color"] = sprite.color;
            node["zIndex"] = sprite.zIndex;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 SpriteComponent 对象。
         * @param node 包含精灵组件数据的 YAML 节点。
         * @param sprite 要填充的精灵组件对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::SpriteComponent& sprite)
        {
            if (!node.IsMap()) return false;

            if (!node["textureHandle"] ||
                !node["sourceRect"] || !node["color"])
            {
                LogError("SpriteComponent: Missing required fields in YAML node.");
                throw std::runtime_error("SpriteComponent: Missing required fields in YAML node.");
                return false;
            }
            sprite.textureHandle = node["textureHandle"].as<AssetHandle>();
            if (node["materialHandle"].IsDefined())
            {
                sprite.materialHandle = node["materialHandle"].as<AssetHandle>();
            }
            else
            {
                sprite.materialHandle = AssetHandle();
            }
            sprite.sourceRect = node["sourceRect"].as<ECS::RectF>();
            sprite.color = node["color"].as<ECS::Color>();
            sprite.zIndex = node["zIndex"].as<int>(0);


            return true;
        }
    };
}

/**
 * @brief 注册 ECS::SpriteComponent 及其属性到组件注册系统。
 *
 * 这允许在编辑器和序列化过程中识别和操作 SpriteComponent 的属性。
 */
REGISTRY
{
    Registry_<ECS::SpriteComponent>("SpriteComponent")
        .property("textureHandle", &ECS::SpriteComponent::textureHandle)
        .property("materialHandle", &ECS::SpriteComponent::materialHandle)

        .property("sourceRect", &ECS::SpriteComponent::sourceRect, false)
        .property("color", &ECS::SpriteComponent::color)
        .property("zIndex", &ECS::SpriteComponent::zIndex);
}
#endif
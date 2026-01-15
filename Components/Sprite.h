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
#include "../Utils/LayerMask.h"

namespace ECS
{
    /**
     * @brief 精灵组件，用于表示游戏中的可渲染精灵。
     *
     * 包含精灵的纹理、材质、裁剪区域、颜色和渲染层级等信息。
     * 支持自发光贴图，用于创建霓虹灯、发光宝石等效果。
     * 
     * Feature: 2d-lighting-enhancement
     * Requirements: 4.1
     */
    struct SpriteComponent : public IComponent
    {
        AssetHandle textureHandle = AssetHandle(AssetType::Texture); ///< 纹理资源的句柄。
        AssetHandle materialHandle = AssetHandle(AssetType::Material); ///< 材质资源的句柄。
        AssetHandle normalMapHandle = AssetHandle(AssetType::Texture); ///< 法线贴图资源的句柄（可选）。
        AssetHandle emissionMapHandle = AssetHandle(AssetType::Texture); ///< 自发光贴图资源的句柄（可选）。Requirements: 4.1
        RectF sourceRect = {0.0f, 0.0f, 0.0f, 0.0f}; ///< 纹理中用于渲染的源矩形区域。
        Color color = Colors::White; ///< 精灵的颜色。
        Color emissionColor = Colors::White; ///< 自发光颜色。Requirements: 4.1
        float emissionIntensity = 0.0f; ///< 自发光强度，支持 HDR 值（>1.0）。Requirements: 4.1, 4.5
        int zIndex = 0; ///< 精灵的Z轴渲染顺序，值越大越靠前。
        LayerMask lightLayer; ///< 光照层掩码，用于控制哪些光源影响此精灵。

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

        /**
         * @brief 检查是否启用了自发光
         * @return 如果自发光强度大于0或有自发光贴图，返回true
         */
        bool HasEmission() const
        {
            return emissionIntensity > 0.0f || emissionMapHandle.Valid();
        }

        sk_sp<RuntimeTexture> image = nullptr; ///< 运行时纹理对象。
        sk_sp<RuntimeTexture> normalMapImage = nullptr; ///< 运行时法线贴图纹理对象。
        sk_sp<RuntimeTexture> emissionMapImage = nullptr; ///< 运行时自发光贴图纹理对象。Requirements: 4.1
        sk_sp<Material> material = nullptr; ///< 运行时材质对象（SkSL - 已弃用）。
        sk_sp<RuntimeWGSLMaterial> wgslMaterial = nullptr; ///< 运行时WGSL材质对象（推荐）。
        AssetHandle lastSpriteHandle = AssetHandle(AssetType::Texture); ///< 上次使用的精灵纹理句柄，用于检测变化。
        AssetHandle lastMaterialHandle = AssetHandle(AssetType::Material); ///< 上次使用的材质句柄，用于检测变化。
        AssetHandle lastNormalMapHandle = AssetHandle(AssetType::Texture); ///< 上次使用的法线贴图句柄，用于检测变化。
        AssetHandle lastEmissionMapHandle = AssetHandle(AssetType::Texture); ///< 上次使用的自发光贴图句柄，用于检测变化。
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于序列化和反序列化 ECS::SpriteComponent。
     * 
     * Feature: 2d-lighting-enhancement
     * Requirements: 4.1
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
            if (sprite.normalMapHandle.Valid())
                node["normalMapHandle"] = sprite.normalMapHandle;
            if (sprite.emissionMapHandle.Valid())
                node["emissionMapHandle"] = sprite.emissionMapHandle;
            node["sourceRect"] = sprite.sourceRect;
            node["color"] = sprite.color;
            // 只有当自发光颜色不是默认白色或强度不为0时才序列化
            if (sprite.emissionIntensity > 0.0f || sprite.emissionColor != ECS::Colors::White)
            {
                node["emissionColor"] = sprite.emissionColor;
                node["emissionIntensity"] = sprite.emissionIntensity;
            }
            node["zIndex"] = sprite.zIndex;
            node["lightLayer"] = sprite.lightLayer.value;
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
            if (node["normalMapHandle"].IsDefined())
            {
                sprite.normalMapHandle = node["normalMapHandle"].as<AssetHandle>();
            }
            else
            {
                sprite.normalMapHandle = AssetHandle(AssetType::Texture);
            }
            // 自发光贴图句柄
            if (node["emissionMapHandle"].IsDefined())
            {
                sprite.emissionMapHandle = node["emissionMapHandle"].as<AssetHandle>();
            }
            else
            {
                sprite.emissionMapHandle = AssetHandle(AssetType::Texture);
            }
            sprite.sourceRect = node["sourceRect"].as<ECS::RectF>();
            sprite.color = node["color"].as<ECS::Color>();
            // 自发光颜色和强度
            if (node["emissionColor"].IsDefined())
            {
                sprite.emissionColor = node["emissionColor"].as<ECS::Color>();
            }
            else
            {
                sprite.emissionColor = ECS::Colors::White;
            }
            if (node["emissionIntensity"].IsDefined())
            {
                sprite.emissionIntensity = node["emissionIntensity"].as<float>();
            }
            else
            {
                sprite.emissionIntensity = 0.0f;
            }
            sprite.zIndex = node["zIndex"].as<int>(0);
            sprite.lightLayer.value = node["lightLayer"].as<uint32_t>(0xFFFFFFFF);


            return true;
        }
    };
}

/**
 * @brief 注册 ECS::SpriteComponent 及其属性到组件注册系统。
 *
 * 这允许在编辑器和序列化过程中识别和操作 SpriteComponent 的属性。
 * 注意：lightLayer 已移至 GameObject 的 LayerComponent，此处保留用于向后兼容
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 4.1
 */
REGISTRY
{
    Registry_<ECS::SpriteComponent>("SpriteComponent")
        .property("textureHandle", &ECS::SpriteComponent::textureHandle)
        .property("materialHandle", &ECS::SpriteComponent::materialHandle)
        .property("normalMapHandle", &ECS::SpriteComponent::normalMapHandle)
        .property("emissionMapHandle", &ECS::SpriteComponent::emissionMapHandle)
        .property("sourceRect", &ECS::SpriteComponent::sourceRect, false)
        .property("color", &ECS::SpriteComponent::color)
        .property("emissionColor", &ECS::SpriteComponent::emissionColor)
        .property("emissionIntensity", &ECS::SpriteComponent::emissionIntensity)
        .property("zIndex", &ECS::SpriteComponent::zIndex);
    // lightLayer 现在由 GameObject 的 LayerComponent 控制
}
#endif
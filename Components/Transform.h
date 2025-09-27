#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "Core.h"
#include "IComponent.h"
#include "./../Renderer/RenderComponent.h"
#include "ComponentRegistry.h"

namespace ECS
{
    /**
     * @brief 表示实体在世界空间中的变换信息（位置、旋转、缩放、锚点）。
     * 继承自 IComponent，使其可以作为实体组件。
     */
    struct TransformComponent : IComponent
    {
        Vector2f position = {0.0f, 0.0f}; ///< 世界空间中锚点的位置。
        float rotation = 0.0f; ///< 世界空间中的旋转角度（弧度）。
        Vector2f scale = 1.0f; ///< 世界空间中的缩放。

        /// <summary>
        /// 锚点，决定了变换作用的基准点。
        /// 使用归一化坐标，(0,0) 代表左上角，(0.5,0.5) 代表中心，(1,1) 代表右下角。
        /// </summary>
        Vector2f anchor = {0.0f, 0.0f};

        Vector2f localPosition = {0.0f, 0.0f}; ///< 相对于父级的局部位置。
        float localRotation = 0.0f; ///< 相对于父级的局部旋转角度。
        Vector2f localScale = 1.0f; ///< 相对于父级的局部缩放。

        /**
         * @brief 默认构造函数，初始化为默认变换值。
         */
        TransformComponent() = default;

        /**
         * @brief 构造函数，使用指定的位置、旋转和缩放初始化变换。
         * @param pos 初始位置。
         * @param rot 初始旋转角度。
         * @param scl 初始缩放。
         */
        TransformComponent(const Vector2f& pos, float rot = 0.0f, float scl = 1.0f)
            : position(pos), rotation(rot), scale(scl)
        {
        }

        /**
         * @brief 构造函数，使用指定的X、Y坐标、旋转和缩放初始化变换。
         * @param x 初始X坐标。
         * @param y 初始Y坐标。
         * @param rot 初始旋转角度。
         * @param scl 初始缩放。
         */
        TransformComponent(float x, float y, float rot = 0.0f, Vector2f scl = 1.0f)
            : position(x, y), rotation(rot), scale(scl)
        {
        }
    };
}

namespace YAML
{
    /**
     * @brief YAML 序列化/反序列化 ECS::Transform 结构的特化转换器。
     */
    template <>
    struct convert<ECS::TransformComponent>
    {
        /**
         * @brief 将 ECS::Transform 对象编码为 YAML 节点。
         * @param transform 要编码的 Transform 对象。
         * @return 表示 Transform 对象的 YAML 节点。
         */
        static Node encode(const ECS::TransformComponent& transform)
        {
            Node node;
            node["position"] = transform.position;
            node["rotation"] = transform.rotation;
            node["scale"] = transform.scale;
            node["anchor"] = transform.anchor; // 新增锚点序列化
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ECS::Transform 对象。
         * @param node 包含 Transform 数据的 YAML 节点。
         * @param transform 用于存储解码结果的 Transform 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::TransformComponent& transform)
        {
            if (!node.IsMap())
                return false;

            transform.position = node["position"].as<ECS::Vector2f>();
            transform.rotation = node["rotation"].as<float>();
            transform.scale = node["scale"].as<ECS::Vector2f>();
            if (node["anchor"]) // 兼容旧格式
            {
                transform.anchor = node["anchor"].as<ECS::Vector2f>();
            }
            return true;
        }
    };
}

/**
 * @brief 注册 ECS::Transform 组件及其属性到组件注册系统。
 * 允许在编辑器或序列化过程中访问和修改这些属性。
 */
REGISTRY
{
    Registry_<ECS::TransformComponent>("TransformComponent")
        .SetNonRemovable() ///< 设置该组件不可移除。
        .property("position", &ECS::TransformComponent::position) ///< 注册位置属性。
        .property("rotation", &ECS::TransformComponent::rotation) ///< 注册旋转属性。
        .property("scale", &ECS::TransformComponent::scale) ///< 注册缩放属性。
        .property("anchor", &ECS::TransformComponent::anchor) ///< 注册锚点属性。
        .property("localPosition", &ECS::TransformComponent::localPosition, false) ///< 注册局部位置属性，不可序列化。
        .property("localRotation", &ECS::TransformComponent::localRotation, false) ///< 注册局部旋转属性，不可序列化。
        .property("localScale", &ECS::TransformComponent::localScale, false); ///< 注册局部缩放属性，不可序列化。
}
#endif
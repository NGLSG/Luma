/**
 * @file ColliderComponent.h
 * @brief 定义了各种碰撞体组件及其相关的YAML序列化和UI绘制功能。
 * 这些组件用于描述实体在物理世界中的碰撞形状。
 */
#ifndef COLLIDERCOMPONENT_H
#define COLLIDERCOMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include <yaml-cpp/yaml.h>
#include <vector>

#include "ComponentRegistry.h"
#include "box2d/id.h"

/**
 * @brief Box2D形状的前向声明。
 */
class b2Shape;

namespace ECS
{
    /**
     * @brief 碰撞体组件的基类。
     * 包含所有碰撞体类型共有的基本属性。
     */
    struct ColliderComponent : public IComponent
    {
        Vector2f offset = {0.0f, 0.0f}; ///< 碰撞体相对于实体位置的偏移量。

        bool isTrigger = false; ///< 指示碰撞体是否为触发器（不产生物理响应，只检测碰撞）。

        bool isDirty = true; ///< 指示碰撞体是否需要更新（例如，属性已更改）。

        b2ShapeId runtimeShape = b2_nullShapeId; ///< 运行时Box2D形状的ID。

        /**
         * @brief 默认析构函数。
         */
        ~ColliderComponent() = default;
    };


    /**
     * @brief 盒形碰撞体组件。
     * 表示一个矩形碰撞体。
     */
    struct BoxColliderComponent : public ColliderComponent
    {
        Vector2f size = {100.0f, 100.0f}; ///< 盒形碰撞体的尺寸。
    };


    /**
     * @brief 圆形碰撞体组件。
     * 表示一个圆形碰撞体。
     */
    struct CircleColliderComponent : public ColliderComponent
    {
        float radius = 100.f; ///< 圆形碰撞体的半径。
    };


    /**
     * @brief 多边形碰撞体组件。
     * 表示一个由顶点定义的多边形碰撞体。
     */
    struct PolygonColliderComponent : public ColliderComponent
    {
        std::vector<Vector2f> vertices; ///< 多边形碰撞体的顶点列表。
    };


    /**
     * @brief 边缘碰撞体组件。
     * 表示一个由一系列顶点定义的边缘链碰撞体。
     */
    struct EdgeColliderComponent : public ColliderComponent
    {
        std::vector<Vector2f> vertices; ///< 边缘碰撞体的顶点列表。

        bool loop = false; ///< 指示边缘链是否闭合形成一个循环。
        b2ChainId runtimeChain = b2_nullChainId; ///< 运行时Box2D链形状的ID。
    };


    /**
     * @brief 胶囊体碰撞体的方向。
     */
    enum class CapsuleDirection
    {
        Vertical,   ///< 垂直方向。
        Horizontal  ///< 水平方向。
    };


    /**
     * @brief 胶囊体碰撞体组件。
     * 表示一个胶囊形状的碰撞体。
     */
    struct CapsuleColliderComponent : public ColliderComponent
    {
        Vector2f size = {1.0f, 2.0f}; ///< 胶囊体碰撞体的尺寸。

        CapsuleDirection direction = CapsuleDirection::Vertical; ///< 胶囊体碰撞体的方向。
    };

    /**
     * @brief 瓦片地图碰撞体组件。
     * 用于表示基于瓦片地图生成的碰撞体。
     */
    struct TilemapColliderComponent : public ColliderComponent
    {
        std::vector<std::vector<Vector2f>> generatedChains; ///< 生成的瓦片地图碰撞链的顶点列表。

        std::vector<b2ChainId> runtimeChains; ///< 运行时Box2D链形状的ID列表（兼容旧实现）。
        std::vector<b2ShapeId> runtimeShapes; ///< 运行时Box2D多边形形状的ID列表（薄条多边形）。
    };
}


namespace YAML
{
    /**
     * @brief YAML编码基础碰撞体属性的宏。
     * @param node YAML节点。
     * @param rhs 碰撞体组件实例。
     */
#define YAML_ENCODE_BASE_COLLIDER(node, rhs) \
        node["offset"] = rhs.offset; \
        node["isTrigger"] = rhs.isTrigger;\
        node["Enable"] = rhs.Enable;
    /**
     * @brief YAML解码基础碰撞体属性的宏。
     * @param node YAML节点。
     * @param rhs 碰撞体组件实例。
     */
#define YAML_DECODE_BASE_COLLIDER(node, rhs) \
        rhs.offset = node["offset"].as<ECS::Vector2f>(ECS::Vector2f(0.0f, 0.0f)); \
        rhs.isTrigger = node["isTrigger"].as<bool>(false);\
        rhs.Enable = node["Enable"].as<bool>(true);\
        rhs.isDirty = true;

    /**
     * @brief 用于ECS::BoxColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::BoxColliderComponent>
    {
        /**
         * @brief 将BoxColliderComponent编码为YAML节点。
         * @param rhs 要编码的BoxColliderComponent实例。
         * @return 表示BoxColliderComponent的YAML节点。
         */
        static Node encode(const ECS::BoxColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);
            node["size"] = rhs.size;
            return node;
        }

        /**
         * @brief 从YAML节点解码BoxColliderComponent。
         * @param node 包含BoxColliderComponent数据的YAML节点。
         * @param rhs 要填充的BoxColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::BoxColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            rhs.size = node["size"].as<ECS::Vector2f>(ECS::Vector2f(1.0f, 1.0f));
            return true;
        }
    };

    /**
     * @brief 用于ECS::CircleColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::CircleColliderComponent>
    {
        /**
         * @brief 将CircleColliderComponent编码为YAML节点。
         * @param rhs 要编码的CircleColliderComponent实例。
         * @return 表示CircleColliderComponent的YAML节点。
         */
        static Node encode(const ECS::CircleColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);
            node["radius"] = rhs.radius;
            return node;
        }

        /**
         * @brief 从YAML节点解码CircleColliderComponent。
         * @param node 包含CircleColliderComponent数据的YAML节点。
         * @param rhs 要填充的CircleColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::CircleColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            rhs.radius = node["radius"].as<float>(0.5f);
            return true;
        }
    };

    /**
     * @brief 用于ECS::PolygonColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::PolygonColliderComponent>
    {
        /**
         * @brief 将PolygonColliderComponent编码为YAML节点。
         * @param rhs 要编码的PolygonColliderComponent实例。
         * @return 表示PolygonColliderComponent的YAML节点。
         */
        static Node encode(const ECS::PolygonColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);
            node["vertices"] = rhs.vertices;
            return node;
        }

        /**
         * @brief 从YAML节点解码PolygonColliderComponent。
         * @param node 包含PolygonColliderComponent数据的YAML节点。
         * @param rhs 要填充的PolygonColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::PolygonColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            rhs.vertices = node["vertices"].as<std::vector<ECS::Vector2f>>();
            return true;
        }
    };

    /**
     * @brief 用于ECS::EdgeColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::EdgeColliderComponent>
    {
        /**
         * @brief 将EdgeColliderComponent编码为YAML节点。
         * @param rhs 要编码的EdgeColliderComponent实例。
         * @return 表示EdgeColliderComponent的YAML节点。
         */
        static Node encode(const ECS::EdgeColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);
            node["vertices"] = rhs.vertices;
            node["loop"] = rhs.loop;
            return node;
        }

        /**
         * @brief 从YAML节点解码EdgeColliderComponent。
         * @param node 包含EdgeColliderComponent数据的YAML节点。
         * @param rhs 要填充的EdgeColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::EdgeColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            rhs.vertices = node["vertices"].as<std::vector<ECS::Vector2f>>();
            rhs.loop = node["loop"].as<bool>(false);
            return true;
        }
    };

    /**
     * @brief 用于ECS::CapsuleColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::CapsuleColliderComponent>
    {
        /**
         * @brief 将CapsuleColliderComponent编码为YAML节点。
         * @param rhs 要编码的CapsuleColliderComponent实例。
         * @return 表示CapsuleColliderComponent的YAML节点。
         */
        static Node encode(const ECS::CapsuleColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);
            node["size"] = rhs.size;
            node["direction"] = static_cast<int>(rhs.direction);
            return node;
        }

        /**
         * @brief 从YAML节点解码CapsuleColliderComponent。
         * @param node 包含CapsuleColliderComponent数据的YAML节点。
         * @param rhs 要填充的CapsuleColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::CapsuleColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            rhs.size = node["size"].as<ECS::Vector2f>(ECS::Vector2f(1.0f, 2.0f));
            rhs.direction = static_cast<ECS::CapsuleDirection>(node["direction"].as<int>(
                static_cast<int>(ECS::CapsuleDirection::Vertical)));
            return true;
        }
    };

    /**
     * @brief 用于ECS::TilemapColliderComponent的YAML转换器特化。
     */
    template <>
    struct convert<ECS::TilemapColliderComponent>
    {
        /**
         * @brief 将TilemapColliderComponent编码为YAML节点。
         * @param rhs 要编码的TilemapColliderComponent实例。
         * @return 表示TilemapColliderComponent的YAML节点。
         */
        static Node encode(const ECS::TilemapColliderComponent& rhs)
        {
            Node node;
            YAML_ENCODE_BASE_COLLIDER(node, rhs);

            return node;
        }

        /**
         * @brief 从YAML节点解码TilemapColliderComponent。
         * @param node 包含TilemapColliderComponent数据的YAML节点。
         * @param rhs 要填充的TilemapColliderComponent实例。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, ECS::TilemapColliderComponent& rhs)
        {
            if (!node.IsMap()) return false;
            YAML_DECODE_BASE_COLLIDER(node, rhs);
            return true;
        }
    };
}


namespace CustomDrawing
{
    /**
     * @brief 用于ECS::CapsuleDirection的UI组件绘制器特化。
     */
    template <>
    struct WidgetDrawer<ECS::CapsuleDirection>
    {
        /**
         * @brief 绘制CapsuleDirection枚举值的UI控件。
         * @param label UI控件的标签。
         * @param value 要绘制和修改的CapsuleDirection值。
         * @param callbacks UI绘制回调数据。
         * @return 如果值被用户修改则返回true，否则返回false。
         */
        static bool Draw(const std::string& label, ECS::CapsuleDirection& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"垂直", "水平"};
            int currentIndex = static_cast<int>(value);
            bool changed = ImGui::Combo(label.c_str(), &currentIndex, items, IM_ARRAYSIZE(items));
            return changed;
        }
    };
}


/**
 * @brief 注册ECS组件及其属性到组件注册表。
 */
REGISTRY
{
    /**
     * @brief 注册BoxColliderComponent组件及其属性。
     */
    Registry_<ECS::BoxColliderComponent>("BoxColliderComponent")
        .property("offset", &ECS::BoxColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::BoxColliderComponent::isTrigger) ///< 注册isTrigger属性。
        .property("size", &ECS::BoxColliderComponent::size); ///< 注册size属性。

    /**
     * @brief 注册CircleColliderComponent组件及其属性。
     */
    Registry_<ECS::CircleColliderComponent>("CircleColliderComponent")
        .property("offset", &ECS::CircleColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::CircleColliderComponent::isTrigger) ///< 注册isTrigger属性。
        .property("radius", &ECS::CircleColliderComponent::radius); ///< 注册radius属性。

    /**
     * @brief 注册PolygonColliderComponent组件及其属性。
     */
    Registry_<ECS::PolygonColliderComponent>("PolygonColliderComponent")
        .property("offset", &ECS::PolygonColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::PolygonColliderComponent::isTrigger) ///< 注册isTrigger属性。
        .property("vertices", &ECS::PolygonColliderComponent::vertices); ///< 注册vertices属性。

    /**
     * @brief 注册EdgeColliderComponent组件及其属性。
     */
    Registry_<ECS::EdgeColliderComponent>("EdgeColliderComponent")
        .property("offset", &ECS::EdgeColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::EdgeColliderComponent::isTrigger) ///< 注册isTrigger属性。
        .property("vertices", &ECS::EdgeColliderComponent::vertices) ///< 注册vertices属性。
        .property("loop", &ECS::EdgeColliderComponent::loop); ///< 注册loop属性。

    /**
     * @brief 注册CapsuleColliderComponent组件及其属性。
     */
    Registry_<ECS::CapsuleColliderComponent>("CapsuleColliderComponent")
        .property("offset", &ECS::CapsuleColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::CapsuleColliderComponent::isTrigger) ///< 注册isTrigger属性。
        .property("size", &ECS::CapsuleColliderComponent::size) ///< 注册size属性。
        .property("direction", &ECS::CapsuleColliderComponent::direction); ///< 注册direction属性。

    /**
     * @brief 注册TilemapColliderComponent组件及其属性。
     */
    Registry_<ECS::TilemapColliderComponent>("TilemapColliderComponent")
        .property("offset", &ECS::TilemapColliderComponent::offset) ///< 注册offset属性。
        .property("isTrigger", &ECS::TilemapColliderComponent::isTrigger); ///< 注册isTrigger属性。
}
#endif

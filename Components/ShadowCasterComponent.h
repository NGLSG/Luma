#ifndef SHADOW_CASTER_COMPONENT_H
#define SHADOW_CASTER_COMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "ComponentRegistry.h"
#include "LightingTypes.h"
#include <vector>
#include <cmath>

namespace ECS
{
    /**
     * @brief 阴影形状枚举
     * 
     * 定义了阴影投射器支持的形状类型。
     */
    enum class ShadowShape : uint8_t
    {
        Auto,       ///< 自动从精灵轮廓生成
        Rectangle,  ///< 矩形
        Circle,     ///< 圆形
        Polygon     ///< 自定义多边形
    };

    /**
     * @brief SDF 数据结构
     * 
     * 存储有符号距离场数据，用于高质量软阴影计算。
     * Requirements: 7.3
     */
    struct SDFData
    {
        std::vector<float> distanceField;    ///< 距离场数据（2D 网格展平为 1D）
        int width = 0;                        ///< 距离场宽度
        int height = 0;                       ///< 距离场高度
        float cellSize = 1.0f;                ///< 每个单元格的世界空间大小
        Vector2f origin = Vector2f(0.0f, 0.0f); ///< 距离场原点（世界坐标）
        bool isValid = false;                 ///< 数据是否有效

        /**
         * @brief 默认构造函数
         */
        SDFData() = default;

        /**
         * @brief 获取指定位置的距离值
         * @param x 网格 X 坐标
         * @param y 网格 Y 坐标
         * @return 距离值，如果越界返回最大距离
         */
        float GetDistance(int x, int y) const
        {
            if (x < 0 || x >= width || y < 0 || y >= height || !isValid)
                return 1e10f;
            return distanceField[y * width + x];
        }

        /**
         * @brief 设置指定位置的距离值
         * @param x 网格 X 坐标
         * @param y 网格 Y 坐标
         * @param distance 距离值
         */
        void SetDistance(int x, int y, float distance)
        {
            if (x >= 0 && x < width && y >= 0 && y < height && isValid)
                distanceField[y * width + x] = distance;
        }

        /**
         * @brief 在世界坐标采样距离值（双线性插值）
         * @param worldPos 世界坐标位置
         * @return 插值后的距离值
         */
        float SampleWorld(const Vector2f& worldPos) const
        {
            if (!isValid || width <= 0 || height <= 0)
                return 1e10f;

            // 转换到网格坐标
            float gridX = (worldPos.x - origin.x) / cellSize;
            float gridY = (worldPos.y - origin.y) / cellSize;

            // 双线性插值
            int x0 = static_cast<int>(std::floor(gridX));
            int y0 = static_cast<int>(std::floor(gridY));
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            float fx = gridX - x0;
            float fy = gridY - y0;

            float d00 = GetDistance(x0, y0);
            float d10 = GetDistance(x1, y0);
            float d01 = GetDistance(x0, y1);
            float d11 = GetDistance(x1, y1);

            float d0 = d00 * (1.0f - fx) + d10 * fx;
            float d1 = d01 * (1.0f - fx) + d11 * fx;

            return d0 * (1.0f - fy) + d1 * fy;
        }

        /**
         * @brief 初始化距离场
         * @param w 宽度
         * @param h 高度
         * @param cell 单元格大小
         * @param orig 原点
         */
        void Initialize(int w, int h, float cell, const Vector2f& orig)
        {
            width = w;
            height = h;
            cellSize = cell;
            origin = orig;
            distanceField.resize(w * h, 1e10f);
            isValid = true;
        }

        /**
         * @brief 清除距离场数据
         */
        void Clear()
        {
            distanceField.clear();
            width = 0;
            height = 0;
            isValid = false;
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const SDFData& other) const
        {
            return width == other.width &&
                   height == other.height &&
                   cellSize == other.cellSize &&
                   origin == other.origin &&
                   isValid == other.isValid &&
                   distanceField == other.distanceField;
        }

        bool operator!=(const SDFData& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 阴影缓存数据结构
     * 
     * 存储静态阴影的缓存数据，避免重复计算。
     * Requirements: 7.6
     */
    struct ShadowCacheData
    {
        bool isCached = false;                ///< 是否已缓存
        bool isDirty = true;                  ///< 是否需要更新
        uint64_t lastUpdateFrame = 0;         ///< 上次更新的帧号
        Vector2f cachedPosition;              ///< 缓存时的位置
        float cachedRotation = 0.0f;          ///< 缓存时的旋转
        Vector2f cachedScale = Vector2f(1.0f, 1.0f); ///< 缓存时的缩放

        /**
         * @brief 检查变换是否改变
         * @param position 当前位置
         * @param rotation 当前旋转
         * @param scale 当前缩放
         * @param tolerance 容差
         * @return 如果变换改变返回 true
         */
        bool HasTransformChanged(const Vector2f& position, float rotation, 
                                  const Vector2f& scale, float tolerance = 0.001f) const
        {
            if (!isCached)
                return true;

            float posDiff = std::sqrt(
                (position.x - cachedPosition.x) * (position.x - cachedPosition.x) +
                (position.y - cachedPosition.y) * (position.y - cachedPosition.y)
            );
            float rotDiff = std::abs(rotation - cachedRotation);
            float scaleDiff = std::sqrt(
                (scale.x - cachedScale.x) * (scale.x - cachedScale.x) +
                (scale.y - cachedScale.y) * (scale.y - cachedScale.y)
            );

            return posDiff > tolerance || rotDiff > tolerance || scaleDiff > tolerance;
        }

        /**
         * @brief 更新缓存的变换数据
         * @param position 当前位置
         * @param rotation 当前旋转
         * @param scale 当前缩放
         * @param frameNumber 当前帧号
         */
        void UpdateCache(const Vector2f& position, float rotation, 
                         const Vector2f& scale, uint64_t frameNumber)
        {
            cachedPosition = position;
            cachedRotation = rotation;
            cachedScale = scale;
            lastUpdateFrame = frameNumber;
            isCached = true;
            isDirty = false;
        }

        /**
         * @brief 标记缓存为脏
         */
        void MarkDirty()
        {
            isDirty = true;
        }

        /**
         * @brief 使缓存失效
         */
        void Invalidate()
        {
            isCached = false;
            isDirty = true;
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const ShadowCacheData& other) const
        {
            return isCached == other.isCached &&
                   isDirty == other.isDirty &&
                   lastUpdateFrame == other.lastUpdateFrame &&
                   cachedPosition == other.cachedPosition &&
                   cachedRotation == other.cachedRotation &&
                   cachedScale == other.cachedScale;
        }

        bool operator!=(const ShadowCacheData& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 阴影投射器组件
     * 
     * 标记实体可以投射阴影，支持多种阴影形状。
     * 可用于创建具有真实深度感的场景效果。
     * 支持 SDF 阴影数据和阴影缓存。
     * 
     * Requirements: 7.3, 7.6
     */
    struct ShadowCasterComponent : public IComponent
    {
        ShadowShape shape = ShadowShape::Auto;           ///< 阴影形状类型
        std::vector<Vector2f> vertices;                  ///< 自定义多边形顶点（仅 Polygon 形状使用）
        float opacity = 1.0f;                            ///< 阴影不透明度 [0, 1]
        bool selfShadow = false;                         ///< 是否自阴影
        float circleRadius = 1.0f;                       ///< 圆形阴影半径（仅 Circle 形状使用）
        Vector2f rectangleSize = Vector2f(1.0f, 1.0f);   ///< 矩形阴影尺寸（仅 Rectangle 形状使用）
        Vector2f offset = Vector2f(0.0f, 0.0f);          ///< 阴影形状偏移

        // ==================== SDF 阴影支持 ====================
        bool enableSDF = false;                          ///< 是否启用 SDF 阴影
        SDFData sdfData;                                 ///< SDF 距离场数据
        int sdfResolution = 64;                          ///< SDF 分辨率（每边像素数）
        float sdfPadding = 2.0f;                         ///< SDF 边界填充（世界单位）

        // ==================== 阴影缓存支持 ====================
        bool enableCache = true;                         ///< 是否启用阴影缓存
        bool isStatic = false;                           ///< 是否为静态物体（不会移动）
        ShadowCacheData cacheData;                       ///< 阴影缓存数据

        /**
         * @brief 默认构造函数
         */
        ShadowCasterComponent() = default;

        /**
         * @brief 带参数的构造函数
         * @param shadowShape 阴影形状类型
         * @param shadowOpacity 阴影不透明度
         */
        ShadowCasterComponent(ShadowShape shadowShape, float shadowOpacity = 1.0f)
            : shape(shadowShape)
            , opacity(shadowOpacity)
        {
        }

        /**
         * @brief 检查是否需要重新生成 SDF
         * @return 如果需要重新生成返回 true
         */
        bool NeedsSdfRegeneration() const
        {
            return enableSDF && (!sdfData.isValid || cacheData.isDirty);
        }

        /**
         * @brief 检查是否需要更新阴影缓存
         * @param position 当前位置
         * @param rotation 当前旋转
         * @param scale 当前缩放
         * @return 如果需要更新返回 true
         */
        bool NeedsCacheUpdate(const Vector2f& position, float rotation, 
                              const Vector2f& scale) const
        {
            if (!enableCache)
                return true;
            if (isStatic && cacheData.isCached && !cacheData.isDirty)
                return false;
            return cacheData.HasTransformChanged(position, rotation, scale);
        }

        /**
         * @brief 使 SDF 数据失效
         */
        void InvalidateSdf()
        {
            sdfData.isValid = false;
            cacheData.MarkDirty();
        }

        /**
         * @brief 比较运算符，用于测试
         */
        bool operator==(const ShadowCasterComponent& other) const
        {
            return shape == other.shape &&
                   vertices == other.vertices &&
                   opacity == other.opacity &&
                   selfShadow == other.selfShadow &&
                   circleRadius == other.circleRadius &&
                   rectangleSize == other.rectangleSize &&
                   offset == other.offset &&
                   enableSDF == other.enableSDF &&
                   sdfResolution == other.sdfResolution &&
                   sdfPadding == other.sdfPadding &&
                   enableCache == other.enableCache &&
                   isStatic == other.isStatic &&
                   Enable == other.Enable;
        }

        bool operator!=(const ShadowCasterComponent& other) const
        {
            return !(*this == other);
        }
    };
}

namespace YAML
{
    /**
     * @brief ShadowShape 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::ShadowShape>
    {
        static Node encode(const ECS::ShadowShape& shape)
        {
            switch (shape)
            {
                case ECS::ShadowShape::Auto: return Node("Auto");
                case ECS::ShadowShape::Rectangle: return Node("Rectangle");
                case ECS::ShadowShape::Circle: return Node("Circle");
                case ECS::ShadowShape::Polygon: return Node("Polygon");
                default: return Node("Auto");
            }
        }

        static bool decode(const Node& node, ECS::ShadowShape& shape)
        {
            if (!node.IsScalar())
                return false;

            std::string value = node.as<std::string>();
            if (value == "Auto")
                shape = ECS::ShadowShape::Auto;
            else if (value == "Rectangle")
                shape = ECS::ShadowShape::Rectangle;
            else if (value == "Circle")
                shape = ECS::ShadowShape::Circle;
            else if (value == "Polygon")
                shape = ECS::ShadowShape::Polygon;
            else
                shape = ECS::ShadowShape::Auto; // 默认值

            return true;
        }
    };

    /**
     * @brief ShadowCasterComponent 的 YAML 转换器特化
     */
    template <>
    struct convert<ECS::ShadowCasterComponent>
    {
        /**
         * @brief 将 ShadowCasterComponent 编码为 YAML 节点
         * @param caster 阴影投射器组件
         * @return YAML 节点
         */
        static Node encode(const ECS::ShadowCasterComponent& caster)
        {
            Node node;
            node["Enable"] = caster.Enable;
            node["shape"] = caster.shape;
            node["opacity"] = caster.opacity;
            node["selfShadow"] = caster.selfShadow;
            node["circleRadius"] = caster.circleRadius;
            node["rectangleSize"] = caster.rectangleSize;
            node["offset"] = caster.offset;

            // SDF 阴影设置
            node["enableSDF"] = caster.enableSDF;
            node["sdfResolution"] = caster.sdfResolution;
            node["sdfPadding"] = caster.sdfPadding;

            // 阴影缓存设置
            node["enableCache"] = caster.enableCache;
            node["isStatic"] = caster.isStatic;

            // 序列化顶点数组
            if (!caster.vertices.empty())
            {
                Node verticesNode;
                for (const auto& vertex : caster.vertices)
                {
                    verticesNode.push_back(vertex);
                }
                node["vertices"] = verticesNode;
            }

            return node;
        }

        /**
         * @brief 从 YAML 节点解码 ShadowCasterComponent
         * @param node YAML 节点
         * @param caster 阴影投射器组件
         * @return 解码是否成功
         */
        static bool decode(const Node& node, ECS::ShadowCasterComponent& caster)
        {
            if (!node.IsMap())
                return false;

            // Enable 字段（可选，默认为 true）
            if (node["Enable"])
                caster.Enable = node["Enable"].as<bool>();

            if (node["shape"])
                caster.shape = node["shape"].as<ECS::ShadowShape>();

            if (node["opacity"])
                caster.opacity = node["opacity"].as<float>();

            if (node["selfShadow"])
                caster.selfShadow = node["selfShadow"].as<bool>();

            if (node["circleRadius"])
                caster.circleRadius = node["circleRadius"].as<float>();

            if (node["rectangleSize"])
                caster.rectangleSize = node["rectangleSize"].as<ECS::Vector2f>();

            if (node["offset"])
                caster.offset = node["offset"].as<ECS::Vector2f>();

            // SDF 阴影设置
            if (node["enableSDF"])
                caster.enableSDF = node["enableSDF"].as<bool>();

            if (node["sdfResolution"])
                caster.sdfResolution = node["sdfResolution"].as<int>();

            if (node["sdfPadding"])
                caster.sdfPadding = node["sdfPadding"].as<float>();

            // 阴影缓存设置
            if (node["enableCache"])
                caster.enableCache = node["enableCache"].as<bool>();

            if (node["isStatic"])
                caster.isStatic = node["isStatic"].as<bool>();

            // 反序列化顶点数组
            if (node["vertices"] && node["vertices"].IsSequence())
            {
                caster.vertices.clear();
                for (const auto& vertexNode : node["vertices"])
                {
                    caster.vertices.push_back(vertexNode.as<ECS::Vector2f>());
                }
            }

            return true;
        }
    };
}

namespace CustomDrawing
{
    template <>
    struct WidgetDrawer<ECS::ShadowShape>
    {
        static bool Draw(const std::string& label, ECS::ShadowShape& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Auto", "Rectangle", "Circle", "Polygon"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<ECS::ShadowShape>(current);
                if (callbacks.onValueChanged) callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
}

/**
 * @brief 注册 ShadowCasterComponent 到组件注册系统
 */
REGISTRY
{
    Registry_<ECS::ShadowCasterComponent>("ShadowCasterComponent")
        .property("shape", &ECS::ShadowCasterComponent::shape)
        .property("opacity", &ECS::ShadowCasterComponent::opacity)
        .property("selfShadow", &ECS::ShadowCasterComponent::selfShadow)
        .property("circleRadius", &ECS::ShadowCasterComponent::circleRadius)
        .property("rectangleSize", &ECS::ShadowCasterComponent::rectangleSize)
        .property("offset", &ECS::ShadowCasterComponent::offset)
        .property("vertices", &ECS::ShadowCasterComponent::vertices)
        // SDF 阴影设置
        .property("enableSDF", &ECS::ShadowCasterComponent::enableSDF)
        .property("sdfResolution", &ECS::ShadowCasterComponent::sdfResolution)
        .property("sdfPadding", &ECS::ShadowCasterComponent::sdfPadding)
        // 阴影缓存设置
        .property("enableCache", &ECS::ShadowCasterComponent::enableCache)
        .property("isStatic", &ECS::ShadowCasterComponent::isStatic);
}

#endif // SHADOW_CASTER_COMPONENT_H

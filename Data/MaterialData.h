#pragma once

#include <string>
#include <vector>
#include <variant>
#include <yaml-cpp/yaml.h>

#include <include/core/SkPoint.h>
#include <include/core/SkColor.h>

#include "IData.h"
#include "SceneData.h"  // 需要SkPoint的YAML转换器定义

namespace Data
{

    /**
     * @brief 统一变量类型枚举。
     * 定义了材质中可能使用的各种统一变量的数据类型。
     */
    enum class UniformType
    {
        Float, ///< 浮点数类型。
        Color4f, ///< 4分量颜色类型（vec4）。
        Int, ///< 整型类型。
        Point, ///< 点坐标类型（vec2）。
        Shader, ///< 着色器类型（已弃用）。
        Vec2, ///< 2D向量。
        Vec3, ///< 3D向量。
        Vec4, ///< 4D向量。
        Mat4 ///< 4x4矩阵。
    };

    /**
     * @brief 材质统一变量结构体。
     * 描述了材质中一个统一变量的名称、类型和值。
     */
    struct MaterialUniform
    {
        std::string name; ///< 统一变量的名称。
        UniformType type; ///< 统一变量的类型。
        YAML::Node valueNode; ///< 统一变量的值，以YAML节点形式存储。
    };

    /**
     * @brief 材质定义结构体。
     * 继承自IData，用于定义一个材质的完整信息，包括着色器代码和统一变量。
     */
    struct MaterialDefinition : IData<MaterialDefinition>
    {
    public:
        friend class IData<MaterialDefinition>;
        AssetHandle shaderHandle; ///< 着色器资源的句柄。
        std::vector<MaterialUniform> uniforms; ///< 材质中包含的统一变量列表。

    private:
        static constexpr const char* TypeName = "mat"; ///< 材质类型名称常量。
    };
}

namespace YAML
{
    /**
     * @brief Data::MaterialUniform类型的YAML转换器特化。
     * 允许Data::MaterialUniform对象在YAML节点和C++对象之间进行编码和解码。
     * 
     * 注意：SkPoint的YAML转换器已在SceneData.h中定义
     */
    template <>
    struct convert<Data::MaterialUniform>
    {
        /**
         * @brief 将Data::MaterialUniform对象编码为YAML节点。
         * @param rhs 要编码的Data::MaterialUniform对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const Data::MaterialUniform& rhs)
        {
            Node node;
            std::string typeStr;
            switch (rhs.type)
            {
            case Data::UniformType::Float: typeStr = "float";
                break;
            case Data::UniformType::Color4f: typeStr = "color4f";
                break;
            case Data::UniformType::Int: typeStr = "int";
                break;
            case Data::UniformType::Point: typeStr = "point";
                break;
            case Data::UniformType::Shader: typeStr = "shader";
                break;
            case Data::UniformType::Vec2: typeStr = "vec2";
                break;
            case Data::UniformType::Vec3: typeStr = "vec3";
                break;
            case Data::UniformType::Vec4: typeStr = "vec4";
                break;
            case Data::UniformType::Mat4: typeStr = "mat4";
                break;
            default:
                typeStr = "float";
                break;
            }
            node["type"] = typeStr;
            if (rhs.valueNode.IsDefined())
            {
                node["value"] = rhs.valueNode;
            }
            return node;
        }

        /**
         * @brief 将YAML节点解码为Data::MaterialUniform对象。
         * @param node 要解码的YAML节点。
         * @param rhs 解码后的Data::MaterialUniform对象。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, Data::MaterialUniform& rhs)
        {
            if (!node.IsMap() || !node["type"]) return false;

            rhs.name = "";
            std::string typeStr = node["type"].as<std::string>();

            if (typeStr == "float") rhs.type = Data::UniformType::Float;
            else if (typeStr == "color4f") rhs.type = Data::UniformType::Color4f;
            else if (typeStr == "int") rhs.type = Data::UniformType::Int;
            else if (typeStr == "point") rhs.type = Data::UniformType::Point;
            else if (typeStr == "shader") rhs.type = Data::UniformType::Shader;
            else if (typeStr == "vec2") rhs.type = Data::UniformType::Vec2;
            else if (typeStr == "vec3") rhs.type = Data::UniformType::Vec3;
            else if (typeStr == "vec4") rhs.type = Data::UniformType::Vec4;
            else if (typeStr == "mat4") rhs.type = Data::UniformType::Mat4;
            else return false;

            if (node["value"])
            {
                rhs.valueNode = node["value"];
            }
            return true;
        }
    };

    /**
     * @brief Data::MaterialDefinition类型的YAML转换器特化。
     * 允许Data::MaterialDefinition对象在YAML节点和C++对象之间进行编码和解码。
     */
    template <>
    struct convert<Data::MaterialDefinition>
    {
        /**
         * @brief 将Data::MaterialDefinition对象编码为YAML节点。
         * @param rhs 要编码的Data::MaterialDefinition对象。
         * @return 编码后的YAML节点。
         */
        static Node encode(const Data::MaterialDefinition& rhs)
        {
            Node node;
            node["shaderHandle"] = rhs.shaderHandle;
            Node uniformsNode = node["uniforms"];
            for (const auto& uniform : rhs.uniforms)
            {
                uniformsNode[uniform.name] = uniform;
            }
            return node;
        }

        /**
         * @brief 将YAML节点解码为Data::MaterialDefinition对象。
         * @param node 要解码的YAML节点。
         * @param rhs 解码后的Data::MaterialDefinition对象。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, Data::MaterialDefinition& rhs)
        {
            if (!node.IsMap()) return false;

            if (node["shaderHandle"])
            {
                rhs.shaderHandle = node["shaderHandle"].as<AssetHandle>();
            }

            if (node["uniforms"] && node["uniforms"].IsMap())
            {
                for (YAML::const_iterator it = node["uniforms"].begin(); it != node["uniforms"].end(); ++it)
                {
                    Data::MaterialUniform uniform = it->second.as<Data::MaterialUniform>();
                    uniform.name = it->first.as<std::string>();
                    rhs.uniforms.push_back(uniform);
                }
            }
            return true;
        }
    };
}

namespace CustomDrawing
{
    /**
     * @brief Data::UniformType类型的UI组件绘制器特化。
     * 提供了在UI中绘制和编辑Data::UniformType枚举值的方法。
     */
    template <>
    struct WidgetDrawer<Data::UniformType>
    {
        /**
         * @brief 绘制Data::UniformType的UI组件。
         * @param label UI组件的标签。
         * @param value 要绘制和编辑的Data::UniformType值。
         * @param callbacks UI绘制回调数据。
         * @return 如果值被修改则返回true，否则返回false。
         */
        static bool Draw(const std::string& label, Data::UniformType& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"Float", "Color4f", "Int", "Point", "Shader"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Data::UniformType>(current);
                return true;
            }
            return false;
        }
    };

    /**
     * @brief Data::MaterialUniform类型的UI组件绘制器特化。
     * 提供了在UI中绘制和编辑Data::MaterialUniform结构体的方法。
     */
    template <>
    struct WidgetDrawer<Data::MaterialUniform>
    {
        /**
         * @brief 绘制Data::MaterialUniform的UI组件。
         * @param label UI组件的标签。
         * @param uniform 要绘制和编辑的Data::MaterialUniform对象。
         * @param callbacks UI绘制回调数据。
         * @return 如果uniform的任何部分被修改则返回true，否则返回false。
         */
        static bool Draw(const std::string& label, Data::MaterialUniform& uniform, const UIDrawData& callbacks)
        {
            bool changed = false;
            ImGui::Text("%s", label.c_str());
            ImGui::Indent();
            changed |= WidgetDrawer<std::string>::Draw("Name", uniform.name, callbacks);
            changed |= WidgetDrawer<Data::UniformType>::Draw("Type", uniform.type, callbacks);

            switch (uniform.type)
            {
            case Data::UniformType::Float:
                {
                    float val = uniform.valueNode.IsDefined() ? uniform.valueNode.as<float>() : 0.0f;
                    if (WidgetDrawer<float>::Draw("Value", val, {}))
                    {
                        uniform.valueNode = val;
                        changed = true;
                    }
                    break;
                }
            case Data::UniformType::Color4f:
                {
                    ECS::Color color = uniform.valueNode.IsDefined()
                                           ? uniform.valueNode.as<ECS::Color>()
                                           : ECS::Color(255, 255, 255, 255);
                    float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
                    if (WidgetDrawer<ECS::Color>::Draw("Value", color, {}))
                    {
                        uniform.valueNode = color;
                        changed = true;
                    }
                    break;
                }
            case Data::UniformType::Int:
                {
                    int val = uniform.valueNode.IsDefined() ? uniform.valueNode.as<int>() : 0;
                    if (ImGui::InputInt("Value", &val))
                    {
                        uniform.valueNode = val;
                        changed = true;
                    }
                    break;
                }
            case Data::UniformType::Point:
                {
                    SkPoint point = uniform.valueNode.IsDefined() ? uniform.valueNode.as<SkPoint>() : SkPoint{0, 0};
                    if (WidgetDrawer<SkPoint>::Draw("Value", point, {}))
                    {
                        uniform.valueNode = point;
                        changed = true;
                    }
                    break;
                }
            case Data::UniformType::Shader:
                {
                    std::string shaderHandle = uniform.valueNode.IsDefined() ? uniform.valueNode.as<std::string>() : "";
                    if (WidgetDrawer<std::string>::Draw("Value", shaderHandle, {}))
                    {
                        uniform.valueNode = shaderHandle;
                        changed = true;
                    }
                    break;
                }
            default:
                break;
            }
            return changed;
        }
    };
}

REGISTRY
{
    AssetRegistry_<Data::MaterialDefinition>(AssetType::Material)
        .property("shaderHandle", &Data::MaterialDefinition::shaderHandle)
        .property("uniforms", &Data::MaterialDefinition::uniforms);
}

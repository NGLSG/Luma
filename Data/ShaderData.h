#ifndef LUMAENGINE_SHADERDATA_H
#define LUMAENGINE_SHADERDATA_H
#include <string>
#include <yaml-cpp/yaml.h>

#include "IData.h"

namespace Data
{
    enum class ShaderType
    {
        VertFrag,
        Compute
    };

    enum class ShaderLanguage
    {
        SKSL,
        WGSL
    };

    struct ShaderData : IData<ShaderData>
    {
        ShaderType type;
        ShaderLanguage language;
        std::string source;

    private:
        static const char* GetStaticType()
        {
            return "shader";
        }
    };
}

namespace YAML
{
    template <>
    struct convert<Data::ShaderType>
    {
        static Node encode(const Data::ShaderType& type)
        {
            switch (type)
            {
            case Data::ShaderType::VertFrag: return Node("VertFrag");
            case Data::ShaderType::Compute: return Node("Compute");
            default: return Node("VertFrag");
            }
        }

        static bool decode(const Node& node, Data::ShaderType& type)
        {
            if (!node.IsScalar()) return false;
            std::string s = node.as<std::string>();
            if (s == "VertFrag") type = Data::ShaderType::VertFrag;
            else if (s == "Compute") type = Data::ShaderType::Compute;
            else return false;
            return true;
        }
    };

    template <>
    struct convert<Data::ShaderLanguage>
    {
        static Node encode(const Data::ShaderLanguage& language)
        {
            switch (language)
            {
            case Data::ShaderLanguage::SKSL: return Node("SKSL");
            case Data::ShaderLanguage::WGSL: return Node("WGSL");
            default: return Node("WGSL");
            };
        }

        static bool decode(const Node& node, Data::ShaderLanguage& language)
        {
            if (!node.IsScalar()) return false;
            std::string s = node.as<std::string>();
            if (s == "SKSL") language = Data::ShaderLanguage::SKSL;
            else if (s == "WGSL") language = Data::ShaderLanguage::WGSL;
            else return false;
            return true;
        }
    };

    template <>
    struct convert<Data::ShaderData>
    {
        static Node encode(const Data::ShaderData& shaderData)
        {
            Node node;
            node["type"] = shaderData.type;
            node["language"] = shaderData.language;
            node["source"] = shaderData.source;
            return node;
        }

        static bool decode(const Node& node, Data::ShaderData& shaderData)
        {
            if (!node.IsMap()) return false;
            shaderData.type = node["type"].as<Data::ShaderType>();
            shaderData.language = node["language"].as<Data::ShaderLanguage>();
            shaderData.source = node["source"].as<std::string>();
            return true;
        }
    };
}

namespace CustomDrawing
{
    template <>
    struct WidgetDrawer<Data::ShaderType>
    {
        static bool Draw(const std::string& label, Data::ShaderType& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"VertFrag", "Compute"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Data::ShaderType>(current);
                return true;
            }
            return false;
        }
    };

    template <>
    struct WidgetDrawer<Data::ShaderLanguage>
    {
        static bool Draw(const std::string& label, Data::ShaderLanguage& value, const UIDrawData& callbacks)
        {
            const char* items[] = {"SKSL (已抛弃)", "WGSL"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, items, IM_ARRAYSIZE(items)))
            {
                value = static_cast<Data::ShaderLanguage>(current);
                return true;
            }
            return false;
        }
    };
}

REGISTRY
{
    AssetRegistry_<Data::ShaderData>(AssetType::Shader).property("Type", &Data::ShaderData::type)
                                                       .property("Language", &Data::ShaderData::language)
                                                       .property("Source", &Data::ShaderData::source);
}
#endif //LUMAENGINE_SHADERDATA_H

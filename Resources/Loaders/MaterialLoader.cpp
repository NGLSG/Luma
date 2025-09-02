#include "MaterialLoader.h"
#include "../../Data/MaterialData.h"
#include "../AssetManager.h"
#include "../Managers/RuntimeMaterialManager.h"

sk_sp<Material> MaterialLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Material || !metadata.importerSettings.IsDefined())
    {
        return nullptr;
    }


    Data::MaterialDefinition definition = metadata.importerSettings.as<Data::MaterialDefinition>();


    if (definition.shaderCode.empty()) return nullptr;

    auto [effect, errorText] = SkRuntimeEffect::MakeForShader(SkString(definition.shaderCode.c_str()));
    if (!effect)
    {
        return nullptr;
    }

    auto material = sk_make_sp<Material>();
    material->effect = std::move(effect);


    for (const auto& uniformDef : definition.uniforms)
    {
        const std::string& name = uniformDef.name;
        const YAML::Node& valueNode = uniformDef.valueNode;

        switch (uniformDef.type)
        {
        case Data::UniformType::Float:
            material->uniforms[name] = valueNode.as<float>();
            break;
        case Data::UniformType::Color4f:
            {
                auto vec = valueNode.as<std::vector<float>>();
                if (vec.size() == 4) material->uniforms[name] = SkColor4f{vec[0], vec[1], vec[2], vec[3]};
            }
            break;
        case Data::UniformType::Int:
            material->uniforms[name] = valueNode.as<int>();
            break;
        case Data::UniformType::Point:
            {
                auto vec = valueNode.as<std::vector<float>>();
                if (vec.size() == 2) material->uniforms[name] = SkPoint{vec[0], vec[1]};
            }
            break;
        case Data::UniformType::Shader:
            material->uniforms[name] = sk_sp<SkShader>(nullptr);
            break;
        }
    }

    return material;
}

sk_sp<Material> MaterialLoader::LoadAsset(const Guid& Guid)
{
    sk_sp<Material> _material;
    if (RuntimeMaterialManager::GetInstance().TryGetAsset(Guid, _material))
    {
        return _material;
    }
    auto metadata = AssetManager::GetInstance().GetMetadata(Guid);
    if (!metadata || metadata->type != AssetType::Material)
    {
        return nullptr;
    }
    auto material = LoadAsset(*metadata);
    if (material)
    {
        RuntimeMaterialManager::GetInstance().TryAddOrUpdateAsset(Guid, material);
    }
    return material;
}

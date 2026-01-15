#include "ShaderLoader.h"

#include "AssetManager.h"
#include "GraphicsBackend.h"
#include "Managers/RuntimeShaderManager.h"
#include "../../Utils/BuiltinShaders.h"

Data::ShaderData ShaderLoader::LoadShaderDataFromGuid(const Guid& guid)
{
    // 首先检查是否为内建shader
    if (BuiltinShaders::IsBuiltinShaderGuid(guid))
    {
        return BuiltinShaders::GetBuiltinShaderData(guid);
    }

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata || metadata->type != AssetType::Shader || !metadata->importerSettings.IsDefined())
    {
        LogError("ShaderLoader::LoadShaderDataFromFile - Invalid shader GUID: {}", guid.ToString());
        return {};
    }

    Data::ShaderData shaderData = metadata->importerSettings.as<Data::ShaderData>();
    return shaderData;
}

sk_sp<RuntimeShader> ShaderLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Shader || !metadata.importerSettings.IsDefined())
    {
        LogError("ShaderLoader::LoadAsset - Invalid metadata");
        return nullptr;
    }

    Data::ShaderData shaderData = metadata.importerSettings.as<Data::ShaderData>();

    if (!context)
    {
        LogError("ShaderLoader::LoadAsset - NutContext is null");
        return nullptr;
    }

    auto runtimeShader = sk_make_sp<RuntimeShader>(shaderData, context, metadata.guid);

    return runtimeShader;
}

sk_sp<RuntimeShader> ShaderLoader::LoadAsset(const Guid& guid)
{
    // 首先检查是否为内建shader
    if (BuiltinShaders::IsBuiltinShaderGuid(guid))
    {
        sk_sp<RuntimeShader> runtimeShader;
        if (RuntimeShaderManager::GetInstance().TryGetAsset(guid, runtimeShader))
        {
            return runtimeShader;
        }

        Data::ShaderData shaderData = BuiltinShaders::GetBuiltinShaderData(guid);
        if (shaderData.source.empty())
        {
            LogError("ShaderLoader::LoadAsset - Failed to load builtin shader: {}", guid.ToString());
            return nullptr;
        }

        runtimeShader = sk_make_sp<RuntimeShader>(shaderData, context, guid);
        RuntimeShaderManager::GetInstance().TryAddOrUpdateAsset(guid, runtimeShader);
        return runtimeShader;
    }

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata || metadata->type != AssetType::Shader)
    {
        LogError("ShaderLoader::LoadAsset - Invalid shader GUID: {}", guid.ToString());
        return nullptr;
    }

    sk_sp<RuntimeShader> runtimeShader;
    if (RuntimeShaderManager::GetInstance().TryGetAsset(guid, runtimeShader))
    {
        return runtimeShader;
    }

    runtimeShader = LoadAsset(*metadata);
    if (runtimeShader == nullptr)
    {
        RuntimeShaderManager::GetInstance().TryAddOrUpdateAsset(guid, runtimeShader);
    }
    return runtimeShader;
}

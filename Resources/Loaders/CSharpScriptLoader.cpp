#include "CSharpScriptLoader.h"
#include "../AssetManager.h"
#include "../Managers/RuntimeCSharpScriptManager.h"
#include "../../Scripting/ScriptMetadataRegistry.h"

sk_sp<RuntimeCSharpScript> CSharpScriptLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::CSharpScript)
    {
        return nullptr;
    }
    std::string className = metadata.importerSettings["className"].as<std::string>("");
    std::string assemblyName = metadata.importerSettings["assemblyName"].as<std::string>("");
    if (className.empty() || assemblyName.empty())
    {
        LogError("CSharpScriptLoader: className or assemblyName is missing in asset metadata for GUID: {}",
                 metadata.guid.ToString());
        return nullptr;
    }
    ScriptClassMetadata classMetadata = ScriptMetadataRegistry::GetInstance().GetMetadata(className);

    return sk_make_sp<RuntimeCSharpScript>(metadata.guid, std::move(className), std::move(assemblyName), classMetadata);
}

sk_sp<RuntimeCSharpScript> CSharpScriptLoader::LoadAsset(const Guid& guid)
{
    auto& manager = RuntimeCSharpScriptManager::GetInstance();
    sk_sp<RuntimeCSharpScript> script;
    if (manager.TryGetAsset(guid, script))
    {
        return script;
    }

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata) return nullptr;

    script = LoadAsset(*metadata);

    if (script)
    {
        manager.TryAddOrUpdateAsset(guid, script);
    }
    return script;
}

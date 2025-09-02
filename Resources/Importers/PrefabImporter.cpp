#include "PrefabImporter.h"
#include "../../Data/PrefabData.h"
#include "../../Utils/Utils.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "AssetManager.h"
#include "Path.h"

const std::vector<std::string>& PrefabImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".prefab"};
    return extensions;
}

AssetMetadata PrefabImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.type = AssetType::Prefab;
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());


    Data::PrefabData prefabData = YAML::LoadFile(assetPath.string()).as<Data::PrefabData>();


    prefabData.root.prefabSource = AssetHandle(metadata.guid, AssetType::Prefab);


    YAML::Emitter emitter;
    emitter.SetIndent(2);
    emitter << YAML::convert<Data::PrefabData>::encode(prefabData);
    std::ofstream fout(assetPath);
    fout << emitter.c_str();
    fout.close();


    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.importerSettings = YAML::convert<Data::PrefabData>::encode(prefabData);

    return metadata;
}

AssetMetadata PrefabImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;


    const auto& assetPath = (AssetManager::GetInstance().GetAssetsRootPath() / updatedMeta.assetPath);

    Data::PrefabData prefabData = YAML::LoadFile(assetPath.string()).as<Data::PrefabData>();


    prefabData.root.prefabSource = AssetHandle(updatedMeta.guid, AssetType::Prefab);


    YAML::Emitter emitter;
    emitter.SetIndent(2);
    emitter << YAML::convert<Data::PrefabData>::encode(prefabData);
    std::ofstream fout(assetPath);
    fout << emitter.c_str();
    fout.close();


    updatedMeta.fileHash = Utils::GetHashFromFile(assetPath.string());
    updatedMeta.importerSettings = YAML::convert<Data::PrefabData>::encode(prefabData);

    return updatedMeta;
}

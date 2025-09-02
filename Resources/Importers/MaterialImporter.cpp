#include "MaterialImporter.h"
#include "../AssetManager.h"
#include "../../Utils/Utils.h"
#include <fstream>

#include <include/effects/SkRuntimeEffect.h>
#include <include/core/SkImage.h>
#include <include/core/SkData.h>
#include <include/core/SkSamplingOptions.h>
#include <include/core/SkString.h>

#include "Path.h"


const std::vector<std::string>& MaterialImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".mat"};
    return extensions;
}


AssetMetadata MaterialImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Material;


    metadata.importerSettings = YAML::LoadFile(assetPath.string());

    return metadata;
}


AssetMetadata MaterialImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());

    YAML::Node matData =
        YAML::LoadFile((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    Data::MaterialDefinition definition = matData.as<Data::MaterialDefinition>();
    updatedMeta.importerSettings = definition;

    return updatedMeta;
}

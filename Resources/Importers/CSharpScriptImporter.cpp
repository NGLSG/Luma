#include "CSharpScriptImporter.h"
#include "../../Utils/Utils.h"
#include <fstream>
#include <string>
#include <regex>

#include "AssetManager.h"
#include "Path.h"
#include "Event/EventBus.h"
#include "Event/Events.h"

const std::vector<std::string>& CSharpScriptImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".cs"};
    return extensions;
}

void CSharpScriptImporter::ExtractScriptInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode)
{
    std::ifstream file(assetPath);
    if (!file.is_open()) return;

    std::string line;
    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    std::string nspace;
    std::string className;


    std::smatch namespace_match;
    std::regex namespace_regex(R"(namespace\s+([a-zA-Z0-9\._]+))");
    if (std::regex_search(fileContent, namespace_match, namespace_regex) && namespace_match.size() > 1)
    {
        nspace = namespace_match[1].str();
    }


    std::smatch class_match;
    std::regex class_regex(R"(class\s+([a-zA-Z0-9_]+)\s*:\s*Script)");
    if (std::regex_search(fileContent, class_match, class_regex) && class_match.size() > 1)
    {
        className = class_match[1].str();
    }

    if (!className.empty())
    {
        settingsNode["className"] = nspace.empty() ? className : nspace + "." + className;
        settingsNode["assemblyName"] = "GameScripts";
    }
}

AssetMetadata CSharpScriptImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());

    metadata.fileHash = Utils::GetHashFromFile(assetPath.generic_string());
    metadata.type = AssetType::CSharpScript;
    ExtractScriptInfo(assetPath, metadata.importerSettings);
    return metadata;
}

AssetMetadata CSharpScriptImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    updatedMeta.fileHash = Utils::GetHashFromFile(
        (AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string());
    ExtractScriptInfo((AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath).string(),
                      updatedMeta.importerSettings);
    EventBus::GetInstance().Publish(CSharpScriptUpdate());
    return updatedMeta;
}

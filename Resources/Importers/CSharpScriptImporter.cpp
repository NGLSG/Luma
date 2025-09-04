#include "CSharpScriptImporter.h"
#include "../../Utils/Utils.h"
#include <fstream>
#include <string>
#include <regex>

#include "AssetManager.h"
#include "Path.h"
#include "Event/EventBus.h"
#include "Event/Events.h"

#if defined(LUMA_PLATFORM_WINDOWS)
#define LUMA_PLATFORM_NAME "Windows"
#define LUMA_PARSER_EXECUTABLE "Luma.Parser.exe"
#elif defined(LUMA_PLATFORM_LINUX)
#define LUMA_PLATFORM_NAME "Linux"
#define LUMA_PARSER_EXECUTABLE "Luma.Parser"
#elif defined(LUMA_PLATFORM_ANDROID)
#define LUMA_PLATFORM_NAME "Android"
#define LUMA_PARSER_EXECUTABLE "Luma.Parser"
#else
#error "Unsupported platform!"
#endif

const std::vector<std::string>& CSharpScriptImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".cs"};
    return extensions;
}

void CSharpScriptImporter::ExtractScriptInfo(const std::filesystem::path& assetPath, YAML::Node& settingsNode)
{
    std::filesystem::path executablePath = Directory::GetCurrentExecutablePath();
    std::filesystem::path executableDir = executablePath.parent_path();

    std::filesystem::path parserPath = executableDir / "Tools" / LUMA_PLATFORM_NAME / LUMA_PARSER_EXECUTABLE;

    if (!std::filesystem::exists(parserPath))
    {
        LogError("Script parser tool not found at: {0}", parserPath.string());
        settingsNode.remove("className");
        settingsNode.remove("assemblyName");
        return;
    }

    std::string command = parserPath.string() + " " + assetPath.string();
    std::string fullClassName = Utils::ExecuteCommandAndGetOutput(command);

    if (!fullClassName.empty())
    {
        settingsNode["className"] = fullClassName;
        settingsNode["assemblyName"] = "GameScripts";
    }
    else
    {
        
        settingsNode.remove("className");
        settingsNode.remove("assemblyName");
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
    auto absolutePath = AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath;
    updatedMeta.fileHash = Utils::GetHashFromFile(absolutePath.string());
    ExtractScriptInfo(absolutePath, updatedMeta.importerSettings);

    EventBus::GetInstance().Publish(CSharpScriptUpdateEvent());
    return updatedMeta;
}

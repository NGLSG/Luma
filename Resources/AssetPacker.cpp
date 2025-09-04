#include "AssetPacker.h"
#include "AssetManager.h"
#include "../Utils/EngineCrypto.h"
#include "../Utils/Logger.h"
#include "../Utils/Path.h"
#include <fstream>
#include <random>
#include <yaml-cpp/yaml.h>

bool AssetPacker::Pack(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                       const std::filesystem::path& outputPath,
                       int maxChunks)
{
    LogInfo("AssetPacker: Starting asset packing process...");
    try
    {
        const auto& metadataMap = assetDatabase;
        if (metadataMap.empty())
        {
            LogWarn("AssetPacker: Asset database is empty. Nothing to pack.");
            return true;
        }


        YAML::Node rootNode;
        for (const auto& [guid, metadata] : metadataMap)
        {
            rootNode[guid] = metadata;
        }
        YAML::Emitter emitter;
        emitter << rootNode;
        std::string yamlString = emitter.c_str();


        std::vector<unsigned char> packageData(yamlString.begin(), yamlString.end());
        auto encryptedPackage = EngineCrypto::GetInstance().Encrypt(packageData);
        LogInfo("AssetPacker: Serialized database size: {} bytes, Encrypted size: {} bytes", packageData.size(),
                encryptedPackage.size());

        if (encryptedPackage.empty())
        {
            throw std::runtime_error("Encryption resulted in an empty package.");
        }


        std::vector<std::string> chunkFileNames;
        std::mt19937 rng(std::random_device{}());
        int numChunks = std::uniform_int_distribution<>(1, std::min((int)encryptedPackage.size(), maxChunks))(rng);
        size_t remainingSize = encryptedPackage.size();
        size_t offset = 0;

        for (int i = 0; i < numChunks; ++i)
        {
            size_t chunkSize;
            if (i == numChunks - 1)
            {
                chunkSize = remainingSize;
            }
            else
            {
                size_t minChunkSize = 1;
                size_t maxChunkSize = remainingSize - (numChunks - 1 - i);
                chunkSize = std::uniform_int_distribution<size_t>(minChunkSize, maxChunkSize)(rng);
            }

            std::string chunkFileName = Guid::NewGuid().ToString() + ".luma_pack";
            chunkFileNames.push_back(chunkFileName);

            Path::WriteAllBytes((outputPath / chunkFileName).string(),
                                {encryptedPackage.data() + offset, encryptedPackage.data() + offset + chunkSize});

            offset += chunkSize;
            remainingSize -= chunkSize;
        }


        std::ofstream manifestFile(outputPath / "package.manifest");
        for (const auto& name : chunkFileNames)
        {
            manifestFile << name << std::endl;
        }

        LogInfo("AssetPacker: Packing completed successfully. {} chunks created.", chunkFileNames.size());
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: Packing failed. Reason: {}", e.what());
        return false;
    }
}

std::unordered_map<std::string, AssetMetadata> AssetPacker::Unpack(const std::filesystem::path& packageManifestPath)
{
    std::vector<unsigned char> encryptedPackage;
    std::ifstream manifestFile(packageManifestPath);
    if (!manifestFile) throw std::runtime_error("Cannot open package manifest: " + packageManifestPath.string());

    std::string chunkFileName;
    while (std::getline(manifestFile, chunkFileName))
    {
        
        if (!chunkFileName.empty() && chunkFileName.back() == '\r')
        {
            chunkFileName.pop_back();
        }

        auto chunkData = Path::ReadAllBytes(
            Path::GetFullPath((packageManifestPath.parent_path() / chunkFileName).string()));
        encryptedPackage.insert(encryptedPackage.end(), chunkData.begin(), chunkData.end());
    }


    auto decryptedData = EngineCrypto::GetInstance().Decrypt(encryptedPackage);
    std::string yamlString(decryptedData.begin(), decryptedData.end());


    YAML::Node rootNode = YAML::Load(yamlString);
    if (!rootNode.IsMap())
    {
        throw std::runtime_error("Unpacked data is not a valid asset database.");
    }


    std::unordered_map<std::string, AssetMetadata> result;
    for (auto it = rootNode.begin(); it != rootNode.end(); ++it)
    {
        AssetMetadata metadata = it->second.as<AssetMetadata>();
        result[it->first.as<std::string>()] = std::move(metadata);
    }

    return result;
}

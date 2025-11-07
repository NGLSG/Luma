#include "AssetPacker.h"
#include "AssetManager.h"
#include "../Utils/EngineCrypto.h"
#include "../Utils/Logger.h"
#include "../Utils/Path.h"
#include <fstream>
#include <random>
#include <yaml-cpp/yaml.h>
#include <thread>
#include <future>
#include <vector>
#include <mutex>

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

std::vector<unsigned char> AssetPacker::ReadChunkFile(const std::filesystem::path& chunkPath)
{
    return Path::ReadAllBytes(Path::GetFullPath(chunkPath.string()));
}

std::unordered_map<std::string, AssetMetadata> AssetPacker::Unpack(const std::filesystem::path& packageManifestPath)
{
    LogInfo("AssetPacker: Starting multi-threaded unpacking process...");

    try
    {
        
        std::ifstream manifestFile(packageManifestPath);
        if (!manifestFile)
        {
            throw std::runtime_error("Cannot open package manifest: " + packageManifestPath.string());
        }

        std::vector<std::string> chunkFileNames;
        std::string chunkFileName;
        while (std::getline(manifestFile, chunkFileName))
        {
            
            if (!chunkFileName.empty() && chunkFileName.back() == '\r')
            {
                chunkFileName.pop_back();
            }
            if (!chunkFileName.empty())
            {
                chunkFileNames.push_back(chunkFileName);
            }
        }
        manifestFile.close();

        if (chunkFileNames.empty())
        {
            throw std::runtime_error("Package manifest is empty.");
        }

        LogInfo("AssetPacker: Found {} chunk files to unpack.", chunkFileNames.size());

        
        const size_t numChunks = chunkFileNames.size();
        std::vector<std::future<std::vector<unsigned char>>> futures;
        futures.reserve(numChunks);

        const std::filesystem::path parentPath = packageManifestPath.parent_path();

        
        for (const auto& fileName : chunkFileNames)
        {
            std::filesystem::path chunkPath = parentPath / fileName;
            futures.push_back(std::async(std::launch::async, ReadChunkFile, chunkPath));
        }

        
        std::vector<unsigned char> encryptedPackage;
        size_t totalSize = 0;

        
        std::vector<std::vector<unsigned char>> chunkDataList;
        chunkDataList.reserve(numChunks);

        for (size_t i = 0; i < numChunks; ++i)
        {
            auto chunkData = futures[i].get();
            totalSize += chunkData.size();
            chunkDataList.push_back(std::move(chunkData));
        }

        
        encryptedPackage.reserve(totalSize);
        for (auto& chunkData : chunkDataList)
        {
            encryptedPackage.insert(encryptedPackage.end(),
                                   std::make_move_iterator(chunkData.begin()),
                                   std::make_move_iterator(chunkData.end()));
        }

        LogInfo("AssetPacker: All chunks loaded. Total encrypted size: {} bytes", encryptedPackage.size());

        
        auto decryptedData = EngineCrypto::GetInstance().Decrypt(encryptedPackage);
        std::string yamlString(decryptedData.begin(), decryptedData.end());

        LogInfo("AssetPacker: Decrypted data size: {} bytes", decryptedData.size());

        
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

        LogInfo("AssetPacker: Unpacking completed successfully. {} assets loaded.", result.size());
        return result;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: Unpacking failed. Reason: {}", e.what());
        throw;
    }
}
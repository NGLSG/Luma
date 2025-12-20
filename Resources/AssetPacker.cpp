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
#include <nlohmann/json.hpp>
#include <mutex>
#include <algorithm>
#include <unordered_set>

namespace
{
    constexpr const char* kAddressablesIndexFileName = "package.addressables";

    std::string NormalizeAddress(const std::string& address)
    {
        std::string normalized = address;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    }

    AddressablesIndex BuildAddressablesIndex(const std::unordered_map<std::string, AssetMetadata>& assetDatabase)
    {
        AddressablesIndex index;
        for (const auto& [guidStr, metadata] : assetDatabase)
        {
            std::string addressKey = NormalizeAddress(GetAssetAddressKey(metadata));
            if (addressKey.empty())
            {
                continue;
            }

            Guid guid = metadata.guid.Valid() ? metadata.guid : Guid::FromString(guidStr);
            auto [it, inserted] = index.addressToGuid.emplace(addressKey, guid);
            if (!inserted && it->second != guid)
            {
                LogWarn("AssetPacker: Addressable冲突 '{}' => {} (已存在: {})",
                        addressKey, guid.ToString(), it->second.ToString());
            }

            if (!metadata.groupNames.empty())
            {
                std::unordered_set<std::string> uniqueGroups(metadata.groupNames.begin(),
                                                             metadata.groupNames.end());
                for (const auto& groupName : uniqueGroups)
                {
                    if (groupName.empty())
                    {
                        continue;
                    }

                    auto& list = index.groupToGuids[groupName];
                    if (std::find(list.begin(), list.end(), guid) == list.end())
                    {
                        list.push_back(guid);
                    }
                }
            }
        }
        return index;
    }
}

void to_json(nlohmann::json& j, const AssetMetadata& meta)
{
    j = nlohmann::json{
        {"guid", meta.guid.ToString()},
        {"fileHash", meta.fileHash},
        {"assetPath", meta.assetPath.string()},
        {"type", static_cast<int>(meta.type)},
        {"addressName", meta.addressName},
        {"groupNames", meta.groupNames}
    };

    
    if (meta.importerSettings.IsDefined() && !meta.importerSettings.IsNull())
    {
        try
        {
            YAML::Emitter emitter;
            emitter << meta.importerSettings;
            if (emitter.good())
            {
                j["importerSettings"] = std::string(emitter.c_str());
            }
            else
            {
                j["importerSettings"] = "";
            }
        }
        catch (...)
        {
            j["importerSettings"] = "";
        }
    }
    else
    {
        j["importerSettings"] = "";
    }
}


void from_json(const nlohmann::json& j, AssetMetadata& meta)
{
    meta.guid = Guid::FromString(j.at("guid").get<std::string>());
    meta.fileHash = j.at("fileHash").get<std::string>();
    meta.assetPath = j.at("assetPath").get<std::string>();
    meta.type = static_cast<AssetType>(j.at("type").get<int>());
    if (j.contains("addressName") && j.at("addressName").is_string())
    {
        meta.addressName = j.at("addressName").get<std::string>();
    }
    if (j.contains("groupNames") && j.at("groupNames").is_array())
    {
        meta.groupNames = j.at("groupNames").get<std::vector<std::string>>();
    }

    
    if (j.contains("importerSettings"))
    {
        std::string yamlStr = j.at("importerSettings").get<std::string>();
        if (!yamlStr.empty())
        {
            try
            {
                meta.importerSettings = YAML::Load(yamlStr);
            }
            catch (...)
            {
                meta.importerSettings = YAML::Node();
            }
        }
        else
        {
            meta.importerSettings = YAML::Node();
        }
    }
    else
    {
        meta.importerSettings = YAML::Node();
    }
}

bool AssetPacker::Pack(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                       const std::filesystem::path& outputPath,
                       int maxChunks)
{
    LogInfo("AssetPacker: 开始资产打包流程...");
    try
    {
        auto db = assetDatabase;
        if (db.empty())
        {
            LogWarn("AssetPacker: 资产数据库为空,无需打包");
            return true;
        }

        
        nlohmann::json rootJson = nlohmann::json::object();
        nlohmann::json indexJson = nlohmann::json::array();

        try
        {
            size_t currentOffset = 0;
            for (const auto& [guid, metadata] : db)
            {
                try
                {
                    nlohmann::json metaJson;
                    to_json(metaJson, metadata);

                    
                    std::vector<uint8_t> singleAssetData = nlohmann::json::to_msgpack(metaJson);

                    
                    nlohmann::json indexEntry;
                    indexEntry["guid"] = guid;
                    indexEntry["offset"] = currentOffset;
                    indexEntry["size"] = singleAssetData.size();
                    indexJson.push_back(indexEntry);

                    rootJson[guid] = std::move(metaJson);
                    currentOffset += singleAssetData.size();
                }
                catch (const std::exception& e)
                {
                    LogError("AssetPacker: 序列化资产失败 {}: {}", guid, e.what());
                    throw;
                }
            }
        }
        catch (const std::exception& e)
        {
            LogError("AssetPacker: 序列化失败: {}", e.what());
            throw;
        }

        LogInfo("AssetPacker: 已序列化 {} 个资产到JSON", rootJson.size());

        
        std::vector<uint8_t> binaryData;
        std::vector<uint8_t> indexData;
        try
        {
            binaryData = nlohmann::json::to_msgpack(rootJson);
            indexData = nlohmann::json::to_msgpack(indexJson);
        }
        catch (const std::exception& e)
        {
            LogError("AssetPacker: MessagePack转换失败: {}", e.what());
            throw;
        }

        
        std::vector<unsigned char> packageData(binaryData.begin(), binaryData.end());
        auto encryptedPackage = EngineCrypto::GetInstance().Encrypt(packageData);

        
        std::vector<unsigned char> indexPackageData(indexData.begin(), indexData.end());
        auto encryptedIndex = EngineCrypto::GetInstance().Encrypt(indexPackageData);

        LogInfo("AssetPacker: 序列化数据库大小: {} 字节, 加密大小: {} 字节, 索引大小: {} 字节",
                packageData.size(), encryptedPackage.size(), encryptedIndex.size());

        if (encryptedPackage.empty())
        {
            throw std::runtime_error("Encryption resulted in an empty package.");
        }

        
        Path::WriteAllBytes((outputPath / "package.index").string(), encryptedIndex);

        
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

        if (!SaveAddressablesIndex(assetDatabase, outputPath))
        {
            LogWarn("AssetPacker: Addressables 索引写入失败");
        }

        LogInfo("AssetPacker: 打包完成,已创建 {} 个数据块", chunkFileNames.size());
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: 打包失败,原因: {}", e.what());
        return false;
    }
}

bool AssetPacker::SaveAddressablesIndex(const std::unordered_map<std::string, AssetMetadata>& assetDatabase,
                                        const std::filesystem::path& outputPath)
{
    try
    {
        auto index = BuildAddressablesIndex(assetDatabase);

        nlohmann::json rootJson = nlohmann::json::object();
        rootJson["addresses"] = nlohmann::json::object();
        rootJson["groups"] = nlohmann::json::object();

        for (const auto& [address, guid] : index.addressToGuid)
        {
            rootJson["addresses"][address] = guid.ToString();
        }

        for (const auto& [group, guidList] : index.groupToGuids)
        {
            std::vector<std::string> guidStrings;
            guidStrings.reserve(guidList.size());
            for (const auto& guid : guidList)
            {
                guidStrings.push_back(guid.ToString());
            }
            rootJson["groups"][group] = std::move(guidStrings);
        }

        std::vector<uint8_t> indexData = nlohmann::json::to_msgpack(rootJson);
        std::vector<unsigned char> packageData(indexData.begin(), indexData.end());
        auto encryptedIndex = EngineCrypto::GetInstance().Encrypt(packageData);

        Path::WriteAllBytes((outputPath / kAddressablesIndexFileName).string(), encryptedIndex);

        LogInfo("AssetPacker: Addressables 索引写入完成, 地址数量: {}, 分组数量: {}",
                index.addressToGuid.size(), index.groupToGuids.size());
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: 写入 Addressables 索引失败: {}", e.what());
        return false;
    }
}

std::vector<unsigned char> AssetPacker::ReadChunkFile(const std::filesystem::path& chunkPath)
{
    return Path::ReadAllBytes(Path::GetFullPath(chunkPath.string()));
}

std::vector<unsigned char> AssetPacker::LoadPackageData(const std::filesystem::path& packageManifestPath)
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

    return encryptedPackage;
}

std::unordered_map<std::string, AssetIndexEntry> AssetPacker::LoadIndex(const std::filesystem::path& packageManifestPath)
{
    LogInfo("AssetPacker: 开始加载资产索引...");

    try
    {
        const std::filesystem::path parentPath = packageManifestPath.parent_path();
        const std::filesystem::path indexPath = parentPath / "package.index";

        
        auto encryptedIndex = Path::ReadAllBytes(Path::GetFullPath(indexPath.string()));

        
        auto decryptedIndex = EngineCrypto::GetInstance().Decrypt(encryptedIndex);

        
        std::vector<uint8_t> indexData(decryptedIndex.begin(), decryptedIndex.end());
        nlohmann::json indexJson = nlohmann::json::from_msgpack(indexData);

        std::unordered_map<std::string, AssetIndexEntry> indexMap;
        indexMap.reserve(indexJson.size());

        for (const auto& entry : indexJson)
        {
            AssetIndexEntry indexEntry;
            indexEntry.guid = entry.at("guid").get<std::string>();
            indexEntry.offset = entry.at("offset").get<size_t>();
            indexEntry.size = entry.at("size").get<size_t>();

            indexMap[indexEntry.guid] = indexEntry;
        }

        LogInfo("AssetPacker: 索引加载完成,共 {} 个资产", indexMap.size());
        return indexMap;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: 索引加载失败: {}", e.what());
        throw;
    }
}

bool AssetPacker::TryLoadAddressablesIndex(const std::filesystem::path& packageManifestPath, AddressablesIndex& outIndex)
{
    try
    {
        const std::filesystem::path parentPath = packageManifestPath.parent_path();
        const std::filesystem::path indexPath = parentPath / kAddressablesIndexFileName;
        if (!std::filesystem::exists(indexPath))
        {
            return false;
        }

        auto encryptedIndex = Path::ReadAllBytes(Path::GetFullPath(indexPath.string()));
        auto decryptedIndex = EngineCrypto::GetInstance().Decrypt(encryptedIndex);
        std::vector<uint8_t> indexData(decryptedIndex.begin(), decryptedIndex.end());
        nlohmann::json rootJson = nlohmann::json::from_msgpack(indexData);

        if (!rootJson.is_object())
        {
            LogWarn("AssetPacker: Addressables 索引格式无效");
            return false;
        }

        outIndex.addressToGuid.clear();
        outIndex.groupToGuids.clear();

        if (rootJson.contains("addresses") && rootJson["addresses"].is_object())
        {
            for (auto it = rootJson["addresses"].begin(); it != rootJson["addresses"].end(); ++it)
            {
                if (!it.value().is_string())
                {
                    continue;
                }
                std::string guidStr = it.value().get<std::string>();
                if (guidStr.empty())
                {
                    continue;
                }
                outIndex.addressToGuid[NormalizeAddress(it.key())] = Guid::FromString(guidStr);
            }
        }

        if (rootJson.contains("groups") && rootJson["groups"].is_object())
        {
            for (auto it = rootJson["groups"].begin(); it != rootJson["groups"].end(); ++it)
            {
                if (!it.value().is_array())
                {
                    continue;
                }
                std::vector<Guid> guids;
                for (const auto& entry : it.value())
                {
                    if (!entry.is_string())
                    {
                        continue;
                    }
                    std::string guidStr = entry.get<std::string>();
                    if (guidStr.empty())
                    {
                        continue;
                    }
                    guids.push_back(Guid::FromString(guidStr));
                }
                if (!guids.empty())
                {
                    outIndex.groupToGuids[it.key()] = std::move(guids);
                }
            }
        }

        LogInfo("AssetPacker: Addressables 索引加载完成, 地址数量: {}, 分组数量: {}",
                outIndex.addressToGuid.size(), outIndex.groupToGuids.size());
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: Addressables 索引加载失败: {}", e.what());
        return false;
    }
}

AssetMetadata AssetPacker::LoadSingleAsset(const std::filesystem::path& packageManifestPath,
                                          const AssetIndexEntry& indexEntry)
{
    try
    {
        
        static std::vector<unsigned char> cachedPackageData;
        static std::filesystem::path cachedPath;
        static std::mutex cacheMutex;

        std::lock_guard<std::mutex> lock(cacheMutex);

        
        if (cachedPath != packageManifestPath)
        {
            auto encryptedPackage = LoadPackageData(packageManifestPath);
            cachedPackageData = EngineCrypto::GetInstance().Decrypt(encryptedPackage);
            cachedPath = packageManifestPath;
        }

        
        std::vector<uint8_t> binaryData(cachedPackageData.begin(), cachedPackageData.end());
        nlohmann::json rootJson = nlohmann::json::from_msgpack(binaryData);

        if (!rootJson.contains(indexEntry.guid))
        {
            throw std::runtime_error("Asset not found in package: " + indexEntry.guid);
        }

        AssetMetadata metadata;
        from_json(rootJson[indexEntry.guid], metadata);

        return metadata;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: 加载单个资产失败 {}: {}", indexEntry.guid, e.what());
        throw;
    }
}

std::unordered_map<std::string, AssetMetadata> AssetPacker::Unpack(const std::filesystem::path& packageManifestPath)
{
    LogInfo("AssetPacker: 开始多线程解包流程...");

    try
    {
        auto encryptedPackage = LoadPackageData(packageManifestPath);

        LogInfo("AssetPacker: 所有数据块已加载,总加密大小: {} 字节", encryptedPackage.size());

        
        auto decryptedData = EngineCrypto::GetInstance().Decrypt(encryptedPackage);

        LogInfo("AssetPacker: 解密数据大小: {} 字节", decryptedData.size());

        
        std::vector<uint8_t> binaryData(decryptedData.begin(), decryptedData.end());
        nlohmann::json rootJson;

        try
        {
            rootJson = nlohmann::json::from_msgpack(binaryData);
        }
        catch (const std::exception& e)
        {
            LogError("AssetPacker: MessagePack解析失败: {}", e.what());
            throw;
        }

        if (!rootJson.is_object())
        {
            throw std::runtime_error("Unpacked data is not a valid asset database.");
        }

        LogInfo("AssetPacker: 开始多线程JSON解析,共 {} 个资产", rootJson.size());

        
        std::vector<std::pair<std::string, nlohmann::json>> jsonEntries;
        jsonEntries.reserve(rootJson.size());

        for (auto it = rootJson.begin(); it != rootJson.end(); ++it)
        {
            jsonEntries.emplace_back(it.key(), it.value());
        }

        
        const size_t hardwareConcurrency = std::thread::hardware_concurrency();
        const size_t numThreads = std::min(hardwareConcurrency > 0 ? hardwareConcurrency : 4, jsonEntries.size());
        const size_t chunkSize = (jsonEntries.size() + numThreads - 1) / numThreads;

        
        std::vector<std::future<std::unordered_map<std::string, AssetMetadata>>> parseFutures;
        parseFutures.reserve(numThreads);

        for (size_t i = 0; i < numThreads; ++i)
        {
            size_t startIdx = i * chunkSize;
            size_t endIdx = std::min(startIdx + chunkSize, jsonEntries.size());

            if (startIdx >= jsonEntries.size())
                break;

            parseFutures.push_back(std::async(std::launch::async,
                                              [&jsonEntries, startIdx, endIdx
                                              ]() -> std::unordered_map<std::string, AssetMetadata>
                                              {
                                                  std::unordered_map<std::string, AssetMetadata> localResult;
                                                  localResult.reserve(endIdx - startIdx);

                                                  for (size_t idx = startIdx; idx < endIdx; ++idx)
                                                  {
                                                      const auto& entry = jsonEntries[idx];
                                                      try
                                                      {
                                                          AssetMetadata metadata;
                                                          from_json(entry.second, metadata);
                                                          localResult[entry.first] = std::move(metadata);
                                                      }
                                                      catch (const std::exception& e)
                                                      {
                                                          LogError("AssetPacker: 反序列化资产失败 {}: {}",
                                                                   entry.first, e.what());
                                                          throw;
                                                      }
                                                  }

                                                  return localResult;
                                              }
            ));
        }

        
        std::unordered_map<std::string, AssetMetadata> result;
        result.reserve(jsonEntries.size());

        for (auto& future : parseFutures)
        {
            auto partialResult = future.get();
            for (auto& pair : partialResult)
            {
                result[pair.first] = std::move(pair.second);
            }
        }

        LogInfo("AssetPacker: 解包完成,已加载 {} 个资产", result.size());
        return result;
    }
    catch (const std::exception& e)
    {
        LogError("AssetPacker: 解包失败: {}", e.what());
        throw;
    }
}

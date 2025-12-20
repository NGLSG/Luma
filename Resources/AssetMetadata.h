#ifndef ASSETSMANAGER_H
#define ASSETSMANAGER_H


#pragma once

#include "../Utils/Guid.h"
#include <string>
#include <filesystem>
#include <vector>
#include <yaml-cpp/yaml.h>

/**
 * @brief 资产类型枚举。
 */
enum class AssetType
{
    Unknown = 0, ///< 未知类型。
    Texture, ///< 纹理资产。
    Material, ///< 材质资产。
    CSharpScript, ///< C# 脚本资产。
    Scene, ///< 场景资产。
    Prefab, ///< 预制体资产。
    Audio, ///< 音频资产。
    Video, ///< 视频资产。
    AnimationClip, ///< 动画剪辑资产。
    AnimationController, ///< 动画控制器资产。
    PhysicsMaterial, ///< 物理材质资产。
    LocalGameObject, ///< 本地游戏对象资产。
    Blueprint, ///< 蓝图资产。
    Tile, ///< 瓦片资产。
    Tileset, ///< 瓦片集资产。
    RuleTile, ///< 规则瓦片资产。
    Font, ///< 字体资产。
    Shader, ///< 着色器资产。
};


/**
 * @brief 将资产类型枚举转换为对应的字符串。
 * @param type 要转换的资产类型。
 * @return 资产类型的字符串表示。
 */
inline const char* AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::Texture: return "Texture";
    case AssetType::Shader: return "Shader";
    case AssetType::Material: return "Material";
    case AssetType::Prefab: return "Prefab";
    case AssetType::Scene: return "Scene";
    case AssetType::CSharpScript: return "CSharpScript";
    case AssetType::Font: return "Font";
    case AssetType::PhysicsMaterial: return "PhysicsMaterial";
    case AssetType::Audio: return "Audio";
    case AssetType::Video: return "Video";
    case AssetType::AnimationClip: return "AnimationClip";
    case AssetType::AnimationController: return "AnimationController";
    case AssetType::Blueprint: return "Blueprint";
    case AssetType::Tile: return "Tile";
    case AssetType::Tileset: return "Tileset";
    case AssetType::RuleTile: return "RuleTile";


    default: return "Unknown";
    }
}

/**
 * @brief 将字符串转换为对应的资产类型枚举。
 * @param str 要转换的字符串。
 * @return 对应的资产类型枚举，如果无法匹配则返回 AssetType::Unknown。
 */
inline AssetType StringToAssetType(const std::string& str)
{
    if (str == "Texture") return AssetType::Texture;
    if (str == "Shader") return AssetType::Shader;
    if (str == "Material") return AssetType::Material;
    if (str == "Prefab") return AssetType::Prefab;
    if (str == "Scene") return AssetType::Scene;
    if (str == "CSharpScript") return AssetType::CSharpScript;
    if (str == "Font") return AssetType::Font;
    if (str == "PhysicsMaterial") return AssetType::PhysicsMaterial;
    if (str == "Audio") return AssetType::Audio;
    if (str == "Video") return AssetType::Video;
    if (str == "AnimationClip") return AssetType::AnimationClip;
    if (str == "AnimationController") return AssetType::AnimationController;
    if (str == "Blueprint") return AssetType::Blueprint;
    if (str == "Tile") return AssetType::Tile;
    if (str == "Tileset") return AssetType::Tileset;
    if (str == "RuleTile") return AssetType::RuleTile;

    return AssetType::Unknown;
}


/**
 * @brief 资产元数据结构体，包含资产的唯一标识、文件哈希、路径和类型等信息。
 */
struct AssetMetadata
{
    Guid guid; ///< 资产的全局唯一标识符。
    std::string fileHash; ///< 资产文件的哈希值。
    std::filesystem::path assetPath; ///< 资产在文件系统中的路径。
    AssetType type = AssetType::Unknown; ///< 资产的类型。
    std::string addressName; ///< 资产的地址名称。
    std::vector<std::string> groupNames; ///< 资产所属的组名称列表。


    YAML::Node importerSettings; ///< 资产导入器设置。
};

inline std::string GetAssetAddressKey(const AssetMetadata& metadata)
{
    if (!metadata.addressName.empty())
    {
        return metadata.addressName;
    }
    return metadata.assetPath.generic_string();
}

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于 AssetType 枚举的序列化和反序列化。
     */
    template <>
    struct convert<AssetType>
    {
        /**
         * @brief 将 AssetType 枚举编码为 YAML 节点。
         * @param type 要编码的 AssetType。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const AssetType& type)
        {
            return Node(AssetTypeToString(type));
        }

        /**
         * @brief 从 YAML 节点解码 AssetType 枚举。
         * @param node 包含 AssetType 数据的 YAML 节点。
         * @param type 解码后的 AssetType。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, AssetType& type)
        {
            if (!node.IsScalar())
                return false;

            type = StringToAssetType(node.as<std::string>());
            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 AssetMetadata 结构体的序列化和反序列化。
     */
    template <>
    struct convert<AssetMetadata>
    {
        /**
         * @brief 将 AssetMetadata 结构体编码为 YAML 节点。
         * @param metadata 要编码的 AssetMetadata。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const AssetMetadata& metadata)
        {
            Node node;
            node["guid"] = metadata.guid.ToString();
            node["fileHash"] = metadata.fileHash;


            node["assetPath"] = metadata.assetPath.generic_string();

            node["assetType"] = AssetTypeToString(metadata.type);

            node["addressName"] = metadata.addressName;
            node["groupNames"] = metadata.groupNames;

            if (metadata.importerSettings.IsDefined() && !metadata.importerSettings.IsNull())
            {
                node["importerSettings"] = metadata.importerSettings;
            }
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 AssetMetadata 结构体。
         * @param node 包含 AssetMetadata 数据的 YAML 节点。
         * @param metadata 解码后的 AssetMetadata。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, AssetMetadata& metadata)
        {
            if (!node.IsMap() || !node["guid"])
                return false;

            metadata.guid = Guid::FromString(node["guid"].as<std::string>());
            metadata.fileHash = node["fileHash"].as<std::string>("");
            metadata.assetPath = std::filesystem::path(node["assetPath"].as<std::string>(""));
            metadata.type = StringToAssetType(node["assetType"].as<std::string>("Unknown"));
            metadata.addressName = node["addressName"].as<std::string>("");
            if (node["groupNames"] && node["groupNames"].IsSequence())
            {
                metadata.groupNames = node["groupNames"].as<std::vector<std::string>>();
            }
            if (node["importerSettings"])
            {
                metadata.importerSettings = node["importerSettings"];
            }

            return true;
        }
    };
}

#endif

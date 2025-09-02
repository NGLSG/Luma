/**
 * @file AssetHandle.h
 * @brief 定义了资源句柄结构体，用于唯一标识和管理游戏资源。
 */
#ifndef ASSETHANDLE_H
#define ASSETHANDLE_H


#include "../Utils/Guid.h"
#include <yaml-cpp/yaml.h>
#include "../Resources/AssetMetadata.h"


/**
 * @brief 资源句柄结构体，包含资源的全局唯一标识符（GUID）和资源类型。
 * 用于在运行时引用和管理特定资源。
 */
struct AssetHandle
{
    Guid assetGuid = Guid::Invalid(); ///< 资源的全局唯一标识符。
    AssetType assetType = AssetType::Unknown; ///< 资源的类型。

    /**
     * @brief 默认构造函数，创建一个无效的资源句柄。
     */
    AssetHandle() = default;

    /**
     * @brief 使用指定的GUID构造资源句柄。
     * @param guid 资源的全局唯一标识符。
     */
    AssetHandle(const Guid& guid) : assetGuid(guid)
    {
    }

    /**
     * @brief 使用指定的资源类型构造资源句柄。
     * @param type 资源的类型。
     */
    AssetHandle(AssetType type) : assetType(type)
    {
    }

    /**
     * @brief 使用指定的GUID和资源类型构造资源句柄。
     * @param guid 资源的全局唯一标识符。
     * @param type 资源的类型。
     */
    AssetHandle(const Guid& guid, AssetType type)
        : assetGuid(guid), assetType(type)
    {
    }


    /**
     * @brief 检查资源句柄是否有效。
     * @return 如果GUID有效，则返回true；否则返回false。
     */
    bool Valid() const
    {
        return assetGuid.Valid();
    }


    /**
     * @brief 默认的比较运算符，用于比较两个AssetHandle对象。
     * @param other 另一个AssetHandle对象。
     * @return 比较结果。
     */
    auto operator<=>(const AssetHandle& other) const = default;

    /**
     * @brief 隐式转换为Guid类型。
     * @return 资源句柄的GUID。
     */
    operator Guid() const
    {
        return assetGuid;
    }

    /**
     * @brief 从GUID创建一个AssetHandle。
     * @param guid 用于创建AssetHandle的GUID。
     * @return 新创建的AssetHandle对象。
     */
    static AssetHandle FromGuid(const Guid& guid)
    {
        return AssetHandle(guid);
    }
};

namespace YAML
{
    /**
     * @brief YAML转换器特化，用于AssetHandle类型。
     * 允许AssetHandle对象在YAML节点之间进行编码和解码。
     */
    template <>
    struct convert<AssetHandle>
    {
        /**
         * @brief 将AssetHandle对象编码为YAML节点。
         * @param handle 要编码的AssetHandle对象。
         * @return 表示AssetHandle的YAML节点。
         */
        static Node encode(const AssetHandle& handle)
        {
            Node node;
            node["guid"] = handle.assetGuid;
            node["type"] = handle.assetType;
            return node;
        }

        /**
         * @brief 从YAML节点解码AssetHandle对象。
         * @param node 包含AssetHandle数据的YAML节点。
         * @param handle 用于存储解码结果的AssetHandle对象。
         * @return 如果解码成功则返回true，否则返回false。
         */
        static bool decode(const Node& node, AssetHandle& handle)
        {
            if (!node.IsMap() || !node["guid"])
                return false;

            handle.assetGuid = node["guid"].as<Guid>();
            if (node["type"])
            {
                handle.assetType = node["type"].as<AssetType>();
            }
            else
            {
                handle.assetType = AssetType::Unknown;
            }
            return true;
        }
    };
}

#endif
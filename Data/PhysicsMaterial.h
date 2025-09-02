#ifndef PHYSICSMATERIALDATA_H
#define PHYSICSMATERIALDATA_H

#include "IData.h"
#include <yaml-cpp/yaml.h>

namespace Data
{
    /**
     * @brief 物理材质数据结构体。
     *
     * 此结构体继承自 IData，用于存储和管理物理材质相关的属性，
     * 如摩擦力、恢复系数、滚动阻力等。
     */
    struct PhysicsMaterialData : public IData<PhysicsMaterialData>
    {
    public:
        friend class IData<PhysicsMaterialData>;

        float friction = 0.4f; ///< 摩擦力。
        float restitution = 0.0f; ///< 恢复系数（弹性）。
        float rollingResistance = 0.0f; ///< 滚动阻力。
        float tangentSpeed = 0.0f; ///< 切向速度。

    private:
        static constexpr const char* TypeName = "physmat";
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器，用于序列化和反序列化 PhysicsMaterialData。
     *
     * 此特化模板提供了将 Data::PhysicsMaterialData 对象与 YAML 节点之间进行转换的方法。
     */
    template <>
    struct convert<Data::PhysicsMaterialData>
    {
        /**
         * @brief 将 PhysicsMaterialData 对象编码为 YAML 节点。
         * @param rhs 要编码的 PhysicsMaterialData 对象。
         * @return 表示 PhysicsMaterialData 对象的 YAML 节点。
         */
        static Node encode(const Data::PhysicsMaterialData& rhs)
        {
            Node node;
            node["friction"] = rhs.friction;
            node["restitution"] = rhs.restitution;
            node["rollingResistance"] = rhs.rollingResistance;
            node["tangentSpeed"] = rhs.tangentSpeed;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 PhysicsMaterialData 对象。
         * @param node 包含 PhysicsMaterialData 数据的 YAML 节点。
         * @param rhs 用于存储解码结果的 PhysicsMaterialData 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Data::PhysicsMaterialData& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.friction = node["friction"].as<float>(0.4f);
            rhs.restitution = node["restitution"].as<float>(0.0f);
            rhs.rollingResistance = node["rollingResistance"].as<float>(0.0f);
            rhs.tangentSpeed = node["tangentSpeed"].as<float>(0.0f);
            return true;
        }
    };
}

/**
 * @brief 注册 PhysicsMaterialData 的资产属性。
 *
 * 此宏块用于将 PhysicsMaterialData 类型注册为资产，并定义其可序列化的属性。
 */
REGISTRY
{
    AssetRegistry_<Data::PhysicsMaterialData>(AssetType::PhysicsMaterial)
        .property("friction", &Data::PhysicsMaterialData::friction)
        .property("restitution", &Data::PhysicsMaterialData::restitution)
        .property("rollingResistance", &Data::PhysicsMaterialData::rollingResistance)
        .property("tangentSpeed", &Data::PhysicsMaterialData::tangentSpeed);
}

#endif
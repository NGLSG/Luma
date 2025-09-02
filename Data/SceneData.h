#ifndef SCENEDATA_H
#define SCENEDATA_H

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "IData.h"
#include "PrefabData.h"
#include "../Utils/Guid.h"
#include "../Components/AssetHandle.h"
#include "../Components/Core.h"
#include "../Renderer/Camera.h"
#include "include/core/SkRefCnt.h"

namespace Data
{
    /**
     * @brief 表示场景数据。
     *
     * 场景数据包含场景的名称、摄像机属性以及场景中的实体列表。
     */
    struct SceneData : public IData<SceneData>
    {
        friend class IData<SceneData>;
        std::string name; ///< 场景的名称。
        Camera::CamProperties cameraProperties; ///< 场景的摄像机属性。
        std::vector<PrefabNode> entities; ///< 场景中包含的实体列表。

    private:
        static constexpr const char* TypeName = "scene"; ///< 场景数据类型的名称。
    };
}


namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于 SkPoint 类型。
     *
     * 提供了 SkPoint 类型与 YAML 节点之间的编码和解码功能。
     */
    template <>
    struct convert<SkPoint>
    {
        /**
         * @brief 将 SkPoint 对象编码为 YAML 节点。
         * @param rhs 要编码的 SkPoint 对象。
         * @return 表示 SkPoint 对象的 YAML 节点。
         */
        static Node encode(const SkPoint& rhs)
        {
            Node n;
            n.push_back(rhs.fX);
            n.push_back(rhs.fY);
            return n;
        }

        /**
         * @brief 将 YAML 节点解码为 SkPoint 对象。
         * @param n 要解码的 YAML 节点。
         * @param rhs 解码后的 SkPoint 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& n, SkPoint& rhs)
        {
            if (!n.IsSequence() || n.size() != 2) return false;
            rhs.fX = n[0].as<float>();
            rhs.fY = n[1].as<float>();
            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 SkColor4f 类型。
     *
     * 提供了 SkColor4f 类型与 YAML 节点之间的编码和解码功能。
     */
    template <>
    struct convert<SkColor4f>
    {
        /**
         * @brief 将 SkColor4f 对象编码为 YAML 节点。
         * @param rhs 要编码的 SkColor4f 对象。
         * @return 表示 SkColor4f 对象的 YAML 节点。
         */
        static Node encode(const SkColor4f& rhs)
        {
            Node n;
            n.push_back(rhs.fR);
            n.push_back(rhs.fG);
            n.push_back(rhs.fB);
            n.push_back(rhs.fA);
            return n;
        }

        /**
         * @brief 将 YAML 节点解码为 SkColor4f 对象。
         * @param n 要解码的 YAML 节点。
         * @param rhs 解码后的 SkColor4f 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& n, SkColor4f& rhs)
        {
            if (!n.IsSequence() || n.size() != 4) return false;
            rhs.fR = n[0].as<float>();
            rhs.fG = n[1].as<float>();
            rhs.fB = n[2].as<float>();
            rhs.fA = n[3].as<float>();
            return true;
        }
    };

    /**
     * @brief YAML 转换器特化，用于 Camera::CamProperties 类型。
     *
     * 提供了 Camera::CamProperties 类型与 YAML 节点之间的编码和解码功能。
     */
    template <>
    struct convert<Camera::CamProperties>
    {
        /**
         * @brief 将 Camera::CamProperties 对象编码为 YAML 节点。
         * @param rhs 要编码的 Camera::CamProperties 对象。
         * @return 表示 Camera::CamProperties 对象的 YAML 节点。
         */
        static Node encode(const Camera::CamProperties& rhs)
        {
            Node node;
            node["position"] = rhs.position;
            node["zoom"] = rhs.zoom;
            node["rotation"] = rhs.rotation;
            node["clearColor"] = rhs.clearColor;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Camera::CamProperties 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 Camera::CamProperties 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Camera::CamProperties& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.position = node["position"].as<SkPoint>(SkPoint{0, 0});
            rhs.zoom = node["zoom"].as<float>(1.0f);
            rhs.rotation = node["rotation"].as<float>(0.0f);
            rhs.clearColor = node["clearColor"].as<SkColor4f>(SkColor4f{0.1f, 0.1f, 0.1f, 1.0f});
            return true;
        }
    };


    /**
     * @brief YAML 转换器特化，用于 Data::SceneData 类型。
     *
     * 提供了 Data::SceneData 类型与 YAML 节点之间的编码和解码功能。
     */
    template <>
    struct convert<Data::SceneData>
    {
        /**
         * @brief 将 Data::SceneData 对象编码为 YAML 节点。
         * @param rhs 要编码的 Data::SceneData 对象。
         * @return 表示 Data::SceneData 对象的 YAML 节点。
         */
        static Node encode(const Data::SceneData& rhs)
        {
            Node node;
            node["name"] = rhs.name;
            node["camp"] = rhs.cameraProperties;
            node["entities"] = rhs.entities;
            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 Data::SceneData 对象。
         * @param node 要解码的 YAML 节点。
         * @param rhs 解码后的 Data::SceneData 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Data::SceneData& rhs)
        {
            if (!node.IsMap() || !node["name"]) return false;
            rhs.name = node["name"].as<std::string>();
            if (node["camp"])
            {
                rhs.cameraProperties = node["camp"].as<Camera::CamProperties>();
            }
            if (node["entities"])
            {
                rhs.entities = node["entities"].as<std::vector<Data::PrefabNode>>();
            }
            return true;
        }
    };
}

#endif
#ifndef LUMAENGINE_ANIMATIONCLIP_H
#define LUMAENGINE_ANIMATIONCLIP_H
#include "Guid.h"
#include "IComponent.h"
#include "IData.h"

/**
 * @brief 表示动画中的一个关键帧数据。
 */
struct AnimFrame
{
    /// 存储动画组件的数据，键为组件名称，值为YAML节点。
    std::unordered_map<std::string, YAML::Node> animationData;
    /// 存储与此帧相关的可序列化事件目标。
    std::vector<ECS::SerializableEventTarget> eventTargets;
};

/**
 * @brief 表示一个动画剪辑，包含名称、目标实体GUID和一系列动画帧。
 */
struct AnimationClip : Data::IData<AnimationClip>

{
    /// 动画剪辑的名称。
    std::string Name;
    /// 动画剪辑所针对的目标实体的全局唯一标识符。
    Guid TargetEntityGuid;
    /// 存储动画的所有帧，键为帧索引，值为对应的动画帧数据。
    std::unordered_map<int, AnimFrame> Frames;
};


namespace YAML
{
    /**
     * @brief 为 AnimationClip 类型提供 YAML 序列化和反序列化转换。
     */
    template <>
    struct convert<AnimationClip>
    {
        /**
         * @brief 将 AnimationClip 对象编码为 YAML 节点。
         * @param clip 要编码的 AnimationClip 对象。
         * @return 表示 AnimationClip 的 YAML 节点。
         */
        static Node encode(const AnimationClip& clip)
        {
            Node node;
            node["Name"] = clip.Name;
            node["TargetEntityGuid"] = clip.TargetEntityGuid.ToString();


            Node framesNode(YAML::NodeType::Map);
            for (const auto& [index, frame] : clip.Frames)
            {
                Node frameNode;
                for (const auto& [compName, compData] : frame.animationData)
                {
                    frameNode[compName] = compData;
                }
                frameNode["EventTargets"] = frame.eventTargets;
                framesNode[index] = frameNode;
            }
            node["Frames"] = framesNode;

            return node;
        }

        /**
         * @brief 将 YAML 节点解码为 AnimationClip 对象。
         * @param node 要解码的 YAML 节点。
         * @param clip 接收解码结果的 AnimationClip 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, AnimationClip& clip)
        {
            if (!node.IsMap() || !node["Name"] || !node["TargetEntityGuid"])
            {
                return false;
            }

            clip.Name = node["Name"].as<std::string>();
            clip.TargetEntityGuid = Guid::FromString(node["TargetEntityGuid"].as<std::string>());


            clip.Frames.clear();

            const Node& framesNode = node["Frames"];
            if (!framesNode)
            {
                return true;
            }


            if (framesNode.IsMap())
            {
                for (const auto& it : framesNode)
                {
                    int index = it.first.as<int>();
                    const Node& frameValueNode = it.second;

                    AnimFrame frame;
                    for (const auto& compNode : frameValueNode)
                    {
                        std::string compName = compNode.first.as<std::string>();

                        if (compName == "EventTargets")
                        {
                            continue;
                        }
                        frame.animationData[compName] = compNode.second;
                    }

                    if (frameValueNode["EventTargets"])
                    {
                        frame.eventTargets = frameValueNode["EventTargets"].as<std::vector<
                            ECS::SerializableEventTarget>>();
                    }
                    clip.Frames[index] = std::move(frame);
                }
            }

            else if (framesNode.IsSequence())
            {
                int index = 0;
                for (const auto& frameValueNode : framesNode)
                {
                    AnimFrame frame;
                    for (const auto& compNode : frameValueNode)
                    {
                        std::string compName = compNode.first.as<std::string>();
                        if (compName == "EventTargets")
                        {
                            continue;
                        }
                        frame.animationData[compName] = compNode.second;
                    }

                    if (frameValueNode["EventTargets"])
                    {
                        frame.eventTargets = frameValueNode["EventTargets"].as<std::vector<
                            ECS::SerializableEventTarget>>();
                    }

                    clip.Frames[index++] = std::move(frame);
                }
            }

            return true;
        }
    };
}
#endif
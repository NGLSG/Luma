#ifndef ANIMATIONCONTROLLERCOMPONENT_H
#define ANIMATIONCONTROLLERCOMPONENT_H
#include "ComponentRegistry.h"
#include "IComponent.h"
#include "RuntimeAsset/RuntimeAnimationController.h"

namespace ECS
{
    /**
     * @brief 动画控制器组件。
     * 存储用于控制实体动画的数据。
     */
    struct AnimationControllerComponent : IComponent
    {
        AssetHandle animationController = AssetHandle(AssetType::AnimationController); ///< 动画控制器资源的句柄。
        int targetFrame = 12; ///< 目标帧数。

        sk_sp<RuntimeAnimationController> runtimeController; ///< 运行时动画控制器实例。
    };
}

namespace YAML
{
    /**
     * @brief YAML 转换器，用于序列化和反序列化 ECS::AnimationControllerComponent。
     */
    template <>
    struct convert<ECS::AnimationControllerComponent>
    {
        /**
         * @brief 将 AnimationControllerComponent 编码为 YAML 节点。
         * @param acc 要编码的 AnimationControllerComponent 实例。
         * @return 表示 AnimationControllerComponent 的 YAML 节点。
         */
        static Node encode(const ECS::AnimationControllerComponent& acc)
        {
            Node node;
            node["Enable"] = acc.Enable;
            node["animationController"] = acc.animationController;
            node["targetFrame"] = acc.targetFrame;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码 AnimationControllerComponent。
         * @param node 包含 AnimationControllerComponent 数据的 YAML 节点。
         * @param acc 要填充的 AnimationControllerComponent 实例。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ECS::AnimationControllerComponent& acc)
        {
            if (!node.IsMap())
                return false;

            if (node["animationController"])
                acc.animationController = node["animationController"].as<AssetHandle>();
            if (node["targetFrame"])
                acc.targetFrame = node["targetFrame"].as<int>();
            if (node["Enable"])
                acc.Enable = node["Enable"].as<bool>();
            else
                acc.Enable = true;
            return true;
        }
    };
}

REGISTRY
{
    Registry_<ECS::AnimationControllerComponent>("AnimationControllerComponent")
        .property("animationController",
                  &ECS::AnimationControllerComponent::animationController)
        .property("targetFrame", &ECS::AnimationControllerComponent::targetFrame);
}
#endif
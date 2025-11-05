#ifndef RIGIDBODYCOMPONENT_H
#define RIGIDBODYCOMPONENT_H

#include "Core.h"
#include "IComponent.h"
#include "AssetHandle.h"
#include <yaml-cpp/yaml.h>

#include "ComponentRegistry.h"
#include "box2d/id.h"


namespace ECS
{
    /// @brief 物理刚体类型枚举
    enum class BodyType { Static, Kinematic, Dynamic };

    /// @brief 碰撞检测类型枚举
    enum class CollisionDetectionType { Discrete, Continuous };

    /// @brief 休眠模式枚举
    enum class SleepingMode { NeverSleep, StartAwake, StartAsleep };


    /// @brief 将BodyType转换为字符串
    /// @param type 刚体类型
    /// @return 刚体类型的字符串表示
    inline const char* BodyTypeToString(BodyType type)
    {
        switch (type)
        {
        case BodyType::Static: return "Static";
        case BodyType::Kinematic: return "Kinematic";
        case BodyType::Dynamic: return "Dynamic";
        }
        return "Static";
    }

    /// @brief 将字符串转换为BodyType
    /// @param str 刚体类型的字符串表示
    /// @return 刚体类型
    inline BodyType StringToBodyType(const std::string& str)
    {
        if (str == "Kinematic") return BodyType::Kinematic;
        if (str == "Dynamic") return BodyType::Dynamic;
        return BodyType::Static;
    }

    /// @brief 将CollisionDetectionType转换为字符串
    /// @param type 碰撞检测类型
    /// @return 碰撞检测类型的字符串表示
    inline const char* CollisionDetectionTypeToString(CollisionDetectionType type)
    {
        switch (type)
        {
        case CollisionDetectionType::Continuous: return "Continuous";
        case CollisionDetectionType::Discrete: return "Discrete";
        }
        return "Discrete";
    }

    /// @brief 将字符串转换为CollisionDetectionType
    /// @param str 碰撞检测类型的字符串表示
    /// @return 碰撞检测类型
    inline CollisionDetectionType StringToCollisionDetectionType(const std::string& str)
    {
        if (str == "Continuous") return CollisionDetectionType::Continuous;
        return CollisionDetectionType::Discrete;
    }

    /// @brief 将SleepingMode转换为字符串
    /// @param mode 休眠模式
    /// @return 休眠模式的字符串表示
    inline const char* SleepingModeToString(SleepingMode mode)
    {
        switch (mode)
        {
        case SleepingMode::NeverSleep: return "NeverSleep";
        case SleepingMode::StartAsleep: return "StartAsleep";
        case SleepingMode::StartAwake: return "StartAwake";
        }
        return "StartAwake";
    }

    /// @brief 将字符串转换为SleepingMode
    /// @param str 休眠模式的字符串表示
    /// @return 休眠模式
    inline SleepingMode StringToSleepingMode(const std::string& str)
    {
        if (str == "NeverSleep") return SleepingMode::NeverSleep;
        if (str == "StartAsleep") return SleepingMode::StartAsleep;
        return SleepingMode::StartAwake;
    }


    /// @brief 刚体约束结构体
    struct BodyConstraints
    {
        bool freezePositionX = false; ///< 是否冻结X轴位置
        bool freezePositionY = false; ///< 是否冻结Y轴位置
        bool freezeRotation = false; ///< 是否冻结旋转
    };


    /// @brief 刚体组件
    struct RigidBodyComponent : public IComponent
    {
        BodyType bodyType = BodyType::Dynamic; ///< 刚体类型

        AssetHandle physicsMaterial = AssetHandle(AssetType::PhysicsMaterial); ///< 物理材质

        Vector2f linearVelocity = Vector2f(0.0f); /// < 线性速度
        float angularVelocity = 0.0f; /// < 角速度

        bool simulated = true; ///< 是否模拟

        float mass = 1.0f; ///< 质量

        float linearDamping = 0.0f; ///< 线性阻尼

        float angularDamping = 0.05f; ///< 角阻尼

        float gravityScale = 1.0f; ///< 重力缩放

        CollisionDetectionType collisionDetection = CollisionDetectionType::Discrete; ///< 碰撞检测类型

        SleepingMode sleepingMode = SleepingMode::StartAwake; ///< 休眠模式

        BodyConstraints constraints; ///< 刚体约束

        b2BodyId runtimeBody = b2_nullBodyId; ///< 运行时刚体ID

        RigidBodyComponent() = default;
    };
}


namespace YAML
{
    /// @brief YAML转换器，用于BodyConstraints结构体
    template <>
    struct convert<ECS::BodyConstraints>
    {
        /// @brief 将BodyConstraints编码为YAML节点
        /// @param constraints 约束
        /// @return YAML节点
        static Node encode(const ECS::BodyConstraints& constraints)
        {
            Node node;
            node["freezePositionX"] = constraints.freezePositionX;
            node["freezePositionY"] = constraints.freezePositionY;
            node["freezeRotation"] = constraints.freezeRotation;
            return node;
        }

        /// @brief 将YAML节点解码为BodyConstraints
        /// @param node YAML节点
        /// @param constraints 约束
        /// @return 是否解码成功
        static bool decode(const Node& node, ECS::BodyConstraints& constraints)
        {
            if (!node.IsMap()) return false;

            constraints.freezePositionX = node["freezePositionX"].as<bool>(false);
            constraints.freezePositionY = node["freezePositionY"].as<bool>(false);
            constraints.freezeRotation = node["freezeRotation"].as<bool>(false);
            return true;
        }
    };

    /// @brief YAML转换器，用于RigidBodyComponent结构体
    template <>
    struct convert<ECS::RigidBodyComponent>
    {
        /// @brief 将RigidBodyComponent编码为YAML节点
        /// @param rhs 刚体组件
        /// @return YAML节点
        static Node encode(const ECS::RigidBodyComponent& rhs)
        {
            Node node;
            node["Enable"] = rhs.Enable;

            node["bodyType"] = ECS::BodyTypeToString(rhs.bodyType);
            node["physicsMaterial"] = rhs.physicsMaterial;
            node["simulated"] = rhs.simulated;
            node["mass"] = rhs.mass;
            node["linearDamping"] = rhs.linearDamping;
            node["angularDamping"] = rhs.angularDamping;
            node["gravityScale"] = rhs.gravityScale;
            node["collisionDetection"] = ECS::CollisionDetectionTypeToString(rhs.collisionDetection);
            node["sleepingMode"] = ECS::SleepingModeToString(rhs.sleepingMode);
            node["constraints"] = rhs.constraints;
            node["linearVelocity"] = rhs.linearVelocity;
            node["angularVelocity"] = rhs.angularVelocity;

            return node;
        }

        /// @brief 将YAML节点解码为RigidBodyComponent
        /// @param node YAML节点
        /// @param rhs 刚体组件
        /// @return 是否解码成功
        static bool decode(const Node& node, ECS::RigidBodyComponent& rhs)
        {
            if (!node.IsMap()) return false;
            rhs.Enable = node["Enable"].as<bool>(true);
            rhs.bodyType = ECS::StringToBodyType(node["bodyType"].as<std::string>("Dynamic"));
            rhs.physicsMaterial = node["physicsMaterial"] ? node["physicsMaterial"].as<AssetHandle>() : AssetHandle();
            rhs.simulated = node["simulated"].as<bool>(true);
            rhs.mass = node["mass"].as<float>(1.0f);
            rhs.linearDamping = node["linearDamping"].as<float>(0.0f);
            rhs.angularDamping = node["angularDamping"].as<float>(0.05f);
            rhs.gravityScale = node["gravityScale"].as<float>(1.0f);
            rhs.collisionDetection = ECS::StringToCollisionDetectionType(
                node["collisionDetection"].as<std::string>("Discrete"));
            rhs.sleepingMode = ECS::StringToSleepingMode(node["sleepingMode"].as<std::string>("StartAwake"));

            if (node["constraints"])
            {
                rhs.constraints = node["constraints"].as<ECS::BodyConstraints>();
            }

            if (node["linearVelocity"])
            {
                rhs.linearVelocity = node["linearVelocity"].as<ECS::Vector2f>();
            }
            if (node["angularVelocity"])
            {
                rhs.angularVelocity = node["angularVelocity"].as<float>();
            }
            return true;
        }
    };
}

namespace CustomDrawing
{
    /// @brief ImGui小部件绘制器，用于BodyType枚举
    template <>
    struct WidgetDrawer<ECS::BodyType>
    {
        /// @brief 绘制BodyType小部件
        /// @param label 标签
        /// @param value 值
        /// @param callbacks 回调函数
        /// @return 是否绘制成功
        static bool Draw(const std::string& label, ECS::BodyType& value, const UIDrawData& callbacks)
        {
            const char* options[] = {"Static", "Kinematic", "Dynamic"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, options, IM_ARRAYSIZE(options)))
            {
                value = static_cast<ECS::BodyType>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };

    /// @brief ImGui小部件绘制器，用于CollisionDetectionType枚举
    template <>
    struct WidgetDrawer<ECS::CollisionDetectionType>
    {
        /// @brief 绘制CollisionDetectionType小部件
        /// @param label 标签
        /// @param value 值
        /// @param callbacks 回调函数
        /// @return 是否绘制成功
        static bool Draw(const std::string& label, ECS::CollisionDetectionType& value, const UIDrawData& callbacks)
        {
            const char* options[] = {"Discrete", "Continuous"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, options, IM_ARRAYSIZE(options)))
            {
                value = static_cast<ECS::CollisionDetectionType>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };

    /// @brief ImGui小部件绘制器，用于SleepingMode枚举
    template <>
    struct WidgetDrawer<ECS::SleepingMode>
    {
        /// @brief 绘制SleepingMode小部件
        /// @param label 标签
        /// @param value 值
        /// @param callbacks 回调函数
        /// @return 是否绘制成功
        static bool Draw(const std::string& label, ECS::SleepingMode& value, const UIDrawData& callbacks)
        {
            const char* options[] = {"NeverSleep", "StartAwake", "StartAsleep"};
            int current = static_cast<int>(value);
            if (ImGui::Combo(label.c_str(), &current, options, IM_ARRAYSIZE(options)))
            {
                value = static_cast<ECS::SleepingMode>(current);
                callbacks.onValueChanged();
                return true;
            }
            return false;
        }
    };
}

/// @brief 组件注册宏，用于注册ECS::RigidBodyComponent及其属性
REGISTRY
{
    Registry_<ECS::RigidBodyComponent>("RigidBodyComponent")

        .property("Body Type", &ECS::RigidBodyComponent::bodyType)
        .property("Physics Material", &ECS::RigidBodyComponent::physicsMaterial)
        .property("Simulated", &ECS::RigidBodyComponent::simulated)
        .property("Linear Velocity", &ECS::RigidBodyComponent::linearVelocity)
        .property("Angular Velocity", &ECS::RigidBodyComponent::angularVelocity)
        .property("Mass", &ECS::RigidBodyComponent::mass)
        .property("Linear Damping", &ECS::RigidBodyComponent::linearDamping)
        .property("Angular Damping", &ECS::RigidBodyComponent::angularDamping)
        .property("Gravity Scale", &ECS::RigidBodyComponent::gravityScale)
        .property("Collision Detection", &ECS::RigidBodyComponent::collisionDetection)
        .property("Sleeping Mode", &ECS::RigidBodyComponent::sleepingMode)


        .property("Freeze Position X",
                  [](const ECS::RigidBodyComponent& c) { return c.constraints.freezePositionX; },
                  [](ECS::RigidBodyComponent& c, bool v) { c.constraints.freezePositionX = v; }
        )
        .property("Freeze Position Y",
                  [](const ECS::RigidBodyComponent& c) { return c.constraints.freezePositionY; },
                  [](ECS::RigidBodyComponent& c, bool v) { c.constraints.freezePositionY = v; }
        )
        .property("Freeze Rotation",
                  [](const ECS::RigidBodyComponent& c) { return c.constraints.freezeRotation; },
                  [](ECS::RigidBodyComponent& c, bool v) { c.constraints.freezeRotation = v; }
        );
}
#endif

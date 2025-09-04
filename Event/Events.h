#ifndef GAMEOBJECTEVENTS_H
#define GAMEOBJECTEVENTS_H

#include <entt/entt.hpp>
#include "AssetMetadata.h"


/**
 * @brief 表示游戏对象被创建的事件。
 */
struct GameObjectCreatedEvent
{
    entt::registry& registry; ///< 实体注册表引用。
    entt::entity entity; ///< 被创建的游戏对象实体。

    /**
     * @brief 构造一个 GameObjectCreatedEvent 事件。
     * @param reg 实体注册表。
     * @param e 被创建的游戏对象实体。
     */
    GameObjectCreatedEvent(entt::registry& reg, entt::entity e)
        : registry(reg), entity(e)
    {
    }
};


/**
 * @brief 表示游戏对象被销毁的事件。
 */
struct GameObjectDestroyedEvent
{
    entt::registry& registry; ///< 实体注册表引用。
    entt::entity entity; ///< 被销毁的游戏对象实体。

    /**
     * @brief 构造一个 GameObjectDestroyedEvent 事件。
     * @param reg 实体注册表。
     * @param e 被销毁的游戏对象实体。
     */
    GameObjectDestroyedEvent(entt::registry& reg, entt::entity e)
        : registry(reg), entity(e)
    {
    }
};


/**
 * @brief 表示组件被添加到游戏对象的事件。
 */
struct ComponentAddedEvent
{
    entt::registry& registry; ///< 实体注册表引用。
    entt::entity entity; ///< 被添加组件的游戏对象实体。
    std::string componentName; ///< 被添加组件的名称。

    /**
     * @brief 构造一个 ComponentAddedEvent 事件。
     * @param reg 实体注册表。
     * @param e 被添加组件的游戏对象实体。
     * @param name 被添加组件的名称。
     */
    ComponentAddedEvent(entt::registry& reg, entt::entity e, const std::string& name)
        : registry(reg), entity(e), componentName(name)
    {
    }
};


/**
 * @brief 表示组件从游戏对象中被移除的事件。
 */
struct ComponentRemovedEvent
{
    entt::registry& registry; ///< 实体注册表引用。
    entt::entity entity; ///< 被移除组件的游戏对象实体。
    std::string componentName; ///< 被移除组件的名称。

    /**
     * @brief 构造一个 ComponentRemovedEvent 事件。
     * @param reg 实体注册表。
     * @param e 被移除组件的游戏对象实体。
     * @param name 被移除组件的名称。
     */
    ComponentRemovedEvent(entt::registry& reg, entt::entity e, const std::string& name)
        : registry(reg), entity(e), componentName(name)
    {
    }
};

/**
 * @brief 表示组件数据被更新的事件。
 */
struct ComponentUpdatedEvent
{
    entt::registry& registry; ///< 实体注册表引用。
    entt::entity entity; ///< 组件被更新的游戏对象实体。
};

/**
 * @brief 表示 C# 脚本需要更新的事件。
 */
struct CSharpScriptUpdateEvent
{
};

/**
 * @brief 表示 C# 脚本被重新构建的事件。
 */
struct CSharpScriptRebuiltEvent
{
};

/**
 * @brief 表示 C# 脚本编译完成的事件。
 */
struct CSharpScriptCompiledEvent
{
};

/**
 * @brief 表示文件被拖放到应用程序中的事件。
 */
struct DragDorpFileEvent
{
    std::vector<std::string> filePaths; ///< 被拖放的文件的路径列表。
};

/**
 * @brief 表示与脚本交互的事件。
 */
struct InteractScriptEvent
{
    /**
     * @brief 脚本交互命令类型。
     */
    enum class CommandType
    {
        ActivityChange, ///< 活动状态改变。
        CreateInstance, ///< 创建脚本实例。
        OnCreate, ///< 调用脚本的 OnCreate 方法。
        DestroyInstance, ///< 销毁脚本实例。
        UpdateInstance, ///< 更新脚本实例。
        SetProperty, ///< 设置脚本属性。
        InvokeMethod, ///< 调用脚本方法。
    };

    CommandType type; ///< 命令类型。
    uint32_t entityId; ///< 目标实体ID。
    intptr_t gch; ///< 托管句柄（用于标识脚本实例）。
    std::string typeName; ///< 脚本类型名称。
    std::string assemblyName; ///< 脚本所在的程序集名称。
    std::string propertyName; ///< 属性名称（用于 SetProperty）。
    std::string propertyValue; ///< 属性值（用于 SetProperty）。
    std::string methodName; ///< 方法名称（用于 InvokeMethod）。
    std::string methodArgs; ///< 方法参数（用于 InvokeMethod）。
    float deltaTime; ///< 增量时间（用于 UpdateInstance）。
    bool isActive; ///< 活动状态（用于 ActivityChange）。
};

/**
 * @brief 表示物理接触事件。
 */
struct PhysicsContactEvent
{
    /**
     * @brief 接触类型。
     */
    enum class ContactType
    {
        CollisionEnter, ///< 碰撞开始。
        CollisionStay, ///< 碰撞持续。
        CollisionExit, ///< 碰撞结束。
        TriggerEnter, ///< 触发器进入。
        TriggerStay, ///< 触发器持续。
        TriggerExit ///< 触发器退出。
    };

    ContactType type; ///< 接触类型。
    entt::entity entityA; ///< 参与接触的第一个实体。
    entt::entity entityB; ///< 参与接触的第二个实体。
};

/**
 * @brief 表示资产被更新的事件。
 */
struct AssetUpdatedEvent
{
    AssetType assetType; ///< 被更新资产的类型。
    Guid guid; ///< 被更新资产的全局唯一标识符。
};

#endif

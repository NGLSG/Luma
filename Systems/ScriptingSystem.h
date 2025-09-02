/**
 * @brief 脚本系统，负责管理和执行游戏中的脚本逻辑。
 *        它继承自ISystem，提供了生命周期管理和事件处理功能。
 */
#ifndef SCRIPTINGSYSTEM_H
#define SCRIPTINGSYSTEM_H

#include "ISystem.h"
#include <string>
#include <unordered_map>
#include "../Scripting/CoreCLRHost.h"
#include "Event/Events.h"

namespace Systems
{
    class ScriptingSystem final : public ISystem
    {
    public:
        /**
         * @brief 在系统创建时调用，用于初始化脚本系统。
         * @param scene 运行时场景指针。
         * @param context 引擎上下文引用。
         * @return 无。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 在系统每帧更新时调用，用于执行脚本的更新逻辑。
         * @param scene 运行时场景指针。
         * @param deltaTime 帧之间的时间间隔。
         * @param context 引擎上下文引用。
         * @return 无。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

        /**
         * @brief 在系统销毁时调用，用于清理脚本系统资源。
         * @param scene 运行时场景指针。
         * @return 无。
         */
        void OnDestroy(RuntimeScene* scene) override;

    private:
        /**
         * @brief 处理脚本交互事件。
         * @param event 脚本交互事件。
         * @return 无。
         */
        void handleScriptInteractEvent(const InteractScriptEvent& event);

        /**
         * @brief 处理物理接触事件。
         * @param event 物理接触事件。
         * @return 无。
         */
        void handlePhysicsContactEvent(const PhysicsContactEvent& event);

        /**
         * @brief 创建脚本实例的命令。
         * @param entityId 实体ID。
         * @param typeName 类型名称。
         * @param assemblyName 程序集名称。
         * @return 无。
         */
        void createInstanceCommand(uint32_t entityId, const std::string& typeName, const std::string& assemblyName);

        /**
         * @brief 改变脚本实例活跃状态的命令。
         * @param entityId 实体ID。
         * @param isActive 是否活跃。
         * @return 无。
         */
        void activityChangeCommand(uint32_t entityId, bool isActive);

        /**
         * @brief 调用脚本实例的OnCreate方法的命令。
         * @param entityId 实体ID。
         * @return 无。
         */
        void onCreateCommand(uint32_t entityId);

        /**
         * @brief 销毁脚本实例的命令。
         * @param entityId 实体ID。
         * @return 无。
         */
        void destroyInstanceCommand(uint32_t entityId);

        /**
         * @brief 更新脚本实例的命令。
         * @param entityId 实体ID。
         * @param deltaTime 帧之间的时间间隔。
         * @return 无。
         */
        void updateInstanceCommand(uint32_t entityId, float deltaTime);

        /**
         * @brief 设置脚本实例属性的命令。
         * @param entityId 实体ID。
         * @param propertyName 属性名称。
         * @param value 属性值。
         * @return 无。
         */
        void setPropertyCommand(uint32_t entityId, const std::string& propertyName, const std::string& value);

        /**
         * @brief 调用脚本实例方法的命令。
         * @param entityId 实体ID。
         * @param methodName 方法名称。
         * @param args 方法参数。
         * @return 无。
         */
        void invokeMethodCommand(uint32_t entityId, const std::string& methodName, const std::string& args);

        /**
         * @brief 销毁所有脚本实例。
         * @return 无。
         */
        void destroyAllInstances();

        /**
         * @brief 获取CoreCLR宿主实例。
         * @return CoreCLR宿主指针。
         */
        CoreCLRHost* getHost() const { return CoreCLRHost::GetInstance(); }

        std::unordered_map<uint32_t, ManagedGCHandle> m_entityHandles; ///< 实体ID到托管GC句柄的映射。

        ListenerHandle m_scriptEventHandle; ///< 脚本事件监听器句柄。
        ListenerHandle m_physicsContactEventHandle; ///< 物理接触事件监听器句柄。

        RuntimeScene* m_currentScene = nullptr; ///< 当前运行时场景指针。

        std::filesystem::path m_loadedAssemblyPath; ///< 已加载程序集的路径。
        bool m_isEditorMode = false; ///< 是否处于编辑器模式。
    };
}

#endif
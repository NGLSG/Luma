#ifndef RUNTIMESCENE_H
#define RUNTIMESCENE_H
#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

#include "IRuntimeAsset.h"
#include "RuntimePrefab.h"
#include "SystemsManager.h"
#include "../../Data/SceneData.h"
#include "../../Systems/ISystem.h"
#include "../../Data/EngineContext.h"

struct ComponentRegistration;

namespace ECS
{
    struct IDComponent;
}

class RuntimeGameObject;

struct SceneUpdateEvent
{
};

/**
 * @brief 表示一个运行时场景，管理游戏对象、组件和系统。
 *
 * 运行时场景是游戏世界的核心容器，负责加载、保存、更新和销毁场景中的所有实体。
 */
class RuntimeScene : public IRuntimeAsset
{
public:
    /**
     * @brief 使用指定的全局唯一标识符(GUID)构造一个运行时场景。
     * @param guid 场景的全局唯一标识符。
     */
    RuntimeScene(const Guid& guid);

    /**
     * @brief 构造一个默认的运行时场景。
     */
    RuntimeScene();

    /**
     * @brief 析构函数，清理场景资源。
     */
    ~RuntimeScene() override;

    /**
     * @brief 激活场景，准备其运行。
     * @param engineCtx 引擎上下文，提供引擎核心服务。
     */
    void Activate(EngineContext& engineCtx);

    /**
     * @brief 创建当前场景的一个播放模式副本。
     * @return 当前场景的智能指针副本，用于播放模式。
     */
    sk_sp<RuntimeScene> CreatePlayModeCopy();

    /**
     * @brief 从另一个源场景克隆数据到当前场景。
     * @param sourceScene 源场景，其数据将被克隆。
     */
    void CloneFromScene(const RuntimeScene& sourceScene);

    /**
     * @brief 向场景的模拟线程添加一个系统（默认）。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddSystem(Args&&... args);

    /**
     * @brief 向场景添加一个核心模拟线程系统（默认）。
     * @tparam T 核心系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的核心系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddEssentialSystem(Args&&... args);

    /**
     * @brief 向场景的模拟线程添加一个系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddSystemToSimulationThread(Args&&... args);

    /**
     * @brief 向场景的主线程添加一个系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddSystemToMainThread(Args&&... args);

    /**
     * @brief 向场景添加一个核心模拟线程系统。
     * @tparam T 核心系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的核心系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddEssentialSystemToSimulationThread(Args&&... args);

    /**
     * @brief 向场景添加一个核心主线程系统。
     * @tparam T 核心系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的核心系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddEssentialSystemToMainThread(Args&&... args);

    /**
     * @brief 获取场景中指定类型的系统。
     * @tparam T 要获取的系统类型。
     * @return 指定类型的系统实例的原始指针，如果不存在则返回 nullptr。
     */
    template <typename T>
    T* GetSystem();

    /**
     * @brief 从场景数据加载场景内容。
     * @param sceneData 包含场景数据的结构体。
     */
    void LoadFromData(const Data::SceneData& sceneData);

    /**
     * @brief 实例化一个预制体，创建新的游戏对象。
     * @param prefab 要实例化的预制体。
     * @param parent 新创建游戏对象的父级，如果为 nullptr 则为根对象。
     * @return 新创建的游戏对象。
     */
    RuntimeGameObject Instantiate(RuntimePrefab& prefab, RuntimeGameObject* parent = nullptr);

    /**
     * @brief 创建一个新的游戏对象。
     * @param name 游戏对象的名称，默认为 "GameObject"。
     * @return 新创建的游戏对象。
     */
    RuntimeGameObject CreateGameObject(const std::string& name = "GameObject");

    /**
     * @brief 获取指定实体上所有已注册的组件信息。
     * @param entity 要查询的实体句柄。
     * @return 包含组件名称到注册信息的映射。
     */
    std::unordered_map<std::string, const ComponentRegistration*> GetAllComponents(entt::entity entity);

    /**
     * @brief 获取场景的实体注册表。
     * @return 实体注册表的引用。
     */
    entt::registry& GetRegistry();

    /**
     * @brief 获取场景的常量实体注册表。
     * @return 实体注册表的常量引用。
     */
    const entt::registry& GetRegistry() const;

    /**
     * @brief 根据实体句柄查找游戏对象。
     * @param handle 要查找的实体句柄。
     * @return 对应的运行时游戏对象。
     */
    RuntimeGameObject FindGameObjectByEntity(entt::entity handle);

    /**
     * @brief 获取场景中的所有根游戏对象。
     * @return 根游戏对象的向量引用。
     */
    std::vector<RuntimeGameObject>& GetRootGameObjects();

    /**
     * @brief 根据全局唯一标识符(GUID)查找游戏对象。
     * @param guid 要查找的游戏对象的GUID。
     * @return 对应的运行时游戏对象。
     */
    RuntimeGameObject FindGameObjectByGuid(const Guid& guid);

    /**
     * @brief 对指定实体上的脚本组件调用一个事件。
     * @tparam Args 事件参数的类型。
     * @param entity 目标实体句柄。
     * @param eventName 事件的名称。
     * @param args 事件的参数。
     */
    template <typename... Args>
    void InvokeEvent(entt::entity entity, const std::string& eventName, Args&&... args);

    /**
     * @brief 从序列化的YAML字符串参数调用指定实体上的脚本事件。
     * @param entity 目标实体句柄。
     * @param eventName 事件的名称。
     * @param argsAsYaml 序列化为YAML字符串的事件参数。
     */
    void InvokeEventFromSerializedArgs(entt::entity entity, const std::string& eventName,
                                       const std::string& argsAsYaml);

    /**
     * @brief 创建一个带有指定组件的新实体（游戏对象）。
     * @tparam T 要添加的组件类型。
     * @tparam Args 构造组件所需的参数类型。
     * @param name 游戏对象的名称。
     * @param args 构造组件所需的参数。
     * @return 新创建的运行时游戏对象。
     */
    template <typename T, typename... Args>
    RuntimeGameObject CreateEntityWith(const std::string& name, Args&&... args);

    /**
     * @brief 清空场景中的所有游戏对象和系统。
     */
    void Clear();

    /**
     * @brief 序列化一个实体到预制体节点数据。
     * @param entity 要序列化的实体句柄。
     * @param registry 实体所在的注册表。
     * @return 包含实体数据的预制体节点。
     */
    Data::PrefabNode SerializeEntity(entt::entity entity, entt::registry& registry);

    /**
     * @brief 将整个场景序列化为场景数据。
     * @return 包含场景所有数据的结构体。
     */
    Data::SceneData SerializeToData();

    /**
     * @brief 获取场景的名称。
     * @return 场景的名称。
     */
    const std::string& GetName() const { return m_name; }

    /**
     * @brief 设置场景的名称。
     * @param name 要设置的场景名称。
     */
    void SetName(const std::string& name) { m_name = name; }

    /**
     * @brief 获取场景的源GUID。
     * @return 场景的源GUID。
     */
    Guid GetGuid() { return m_sourceGuid; }

    /**
     * @brief 将一个游戏对象添加到场景的根对象列表。
     * @param go 要添加的游戏对象。
     */
    void AddToRoot(RuntimeGameObject go);

    /**
     * @brief 从场景的根对象列表中移除一个游戏对象。
     * @param go 要移除的游戏对象。
     */
    void RemoveFromRoot(RuntimeGameObject go);

    /**
     * @brief 设置场景的摄像机属性。
     * @param properties 要设置的摄像机属性。
     */
    void SetCameraProperties(const Camera::CamProperties& properties);

    /**
     * @brief 获取场景的摄像机属性。
     * @return 摄像机属性的引用。
     */
    Camera::CamProperties& GetCameraProperties();

    /**
     * @brief 销毁一个游戏对象及其所有子对象。
     * @param gameObject 要销毁的游戏对象。
     */
    void DestroyGameObject(RuntimeGameObject& gameObject);

    /**
     * @brief 从预制体节点创建游戏对象层级结构。
     * @param node 包含层级结构数据的预制体节点。
     * @param parent 新创建游戏对象的父级，如果为 nullptr 则为根对象。
     * @param newGuid 是否为新创建的对象生成新的GUID。
     * @return 新创建的根游戏对象。
     */
    RuntimeGameObject CreateHierarchyFromNode(const Data::PrefabNode& node, RuntimeGameObject* parent,
                                              bool newGuid = true);

    /**
     * @brief 设置根游戏对象的同级索引。
     * @param object 要设置索引的根游戏对象。
     * @param newIndex 新的同级索引。
     */
    void SetRootSiblingIndex(RuntimeGameObject& object, int newIndex);

    /**
     * @brief 获取根游戏对象的同级索引。
     * @param object 要查询索引的根游戏对象。
     * @return 根游戏对象的同级索引。
     */
    int GetRootSiblingIndex(const RuntimeGameObject& object) const;

private:
    friend class ApplicationBase; ///< 允许 ApplicationBase 访问私有成员。
    friend class Editor; ///< 允许 Editor 访问私有成员。
    friend class Game; ///< 允许 Game 访问私有成员。
    friend class ISystem; ///< 允许 ISystem 访问私有成员。

    /**
     * @brief 更新场景中模拟线程的所有系统。
     * @param deltaTime 帧时间间隔。
     * @param engineCtx 引擎上下文。
     * @param pauseNormalSystem 是否暂停普通系统的更新。
     */
    void UpdateSimulation(float deltaTime, EngineContext& engineCtx, bool pauseNormalSystem);

    /**
     * @brief 更新场景中主线程的所有系统。
     * @param deltaTime 帧时间间隔。
     * @param engineCtx 引擎上下文。
     * @param pauseNormalSystem 是否暂停普通系统的更新。
     */
    void UpdateMainThread(float deltaTime, EngineContext& engineCtx, bool pauseNormalSystem);

    /**
     * @brief 当实体被销毁时调用的回调函数。
     * @param registry 实体所在的注册表。
     * @param entity 被销毁的实体句柄。
     */
    void OnEntityDestroyed(entt::registry& registry, entt::entity entity);

    /**
     * @brief 克隆一个实体及其组件到目标注册表。
     * @param sourceRegistry 源实体所在的注册表。
     * @param sourceEntity 要克隆的源实体句柄。
     * @param targetRegistry 目标实体将被创建的注册表。
     * @param entityMapping 实体句柄从源到目标的映射。
     * @return 在目标注册表中创建的新实体句柄。
     */
    entt::entity cloneEntity(const entt::registry& sourceRegistry, entt::entity sourceEntity,
                             entt::registry& targetRegistry,
                             std::unordered_map<entt::entity, entt::entity>& entityMapping);

    /**
     * @brief 根据实体映射重建实体之间的父子关系。
     * @param entityMapping 实体句柄从源到目标的映射。
     */
    void rebuildRelationships(const std::unordered_map<entt::entity, entt::entity>& entityMapping);

    bool m_isPlayModeCopy = false; ///< 指示当前场景是否为播放模式的副本。
    std::unique_ptr<std::vector<uint8_t>> m_snapshotData = nullptr; ///< 场景快照数据，用于播放模式恢复。
    entt::registry m_registry; ///< 场景的实体组件系统(ECS)注册表。
    std::vector<RuntimeGameObject> m_rootGameObjects; ///< 场景中的所有根游戏对象。
    SystemsManager m_systemsManager; ///< 系统管理器，负责管理所有系统。
    std::unordered_map<Guid, entt::entity> m_guidToEntityMap; ///< GUID到实体句柄的映射。
    std::string m_name = "Untitled Scene"; ///< 场景的名称。
    Camera::CamProperties m_cameraProperties; ///< 场景的摄像机属性。
    mutable bool m_isDirty = false; ///< 指示场景数据是否已被修改。
};

#include <yaml-cpp/yaml.h>

#include "RuntimeGameObject.h"
#include "../../Components/ScriptComponent.h"
#if !defined(LUMA_DISABLE_SCRIPTING)
#include "../../Scripting/ManagedHost.h"
#endif

/**
 * @brief 向场景的模拟线程添加一个系统（默认）。
 * @tparam T 系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddSystem(Args&&... args)
{
    return m_systemsManager.AddSimulationSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 向场景添加一个核心模拟线程系统（默认）。
 * @tparam T 核心系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的核心系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddEssentialSystem(Args&&... args)
{
    return m_systemsManager.AddEssentialSimulationSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 向场景的模拟线程添加一个系统。
 * @tparam T 系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddSystemToSimulationThread(Args&&... args)
{
    return m_systemsManager.AddSimulationSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 向场景的主线程添加一个系统。
 * @tparam T 系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddSystemToMainThread(Args&&... args)
{
    return m_systemsManager.AddMainThreadSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 向场景添加一个核心模拟线程系统。
 * @tparam T 核心系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的核心系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddEssentialSystemToSimulationThread(Args&&... args)
{
    return m_systemsManager.AddEssentialSimulationSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 向场景添加一个核心主线程系统。
 * @tparam T 核心系统的类型，必须继承自 ISystem。
 * @tparam Args 构造系统所需的参数类型。
 * @param args 构造系统所需的参数。
 * @return 添加的核心系统实例的原始指针。
 */
template <typename T, typename... Args>
T* RuntimeScene::AddEssentialSystemToMainThread(Args&&... args)
{
    return m_systemsManager.AddEssentialMainThreadSystem<T>(std::forward<Args>(args)...);
}

/**
 * @brief 获取场景中指定类型的系统。
 * @tparam T 要获取的系统类型。
 * @return 指定类型的系统实例的原始指针，如果不存在则返回 nullptr。
 */
template <typename T>
T* RuntimeScene::GetSystem()
{
    return m_systemsManager.GetSystem<T>();
}

/**
 * @brief 对指定实体上的脚本组件调用一个事件。
 * @tparam Args 事件参数的类型。
 * @param entity 目标实体句柄。
 * @param eventName 事件的名称。
 * @param args 事件的参数。
 */
template <typename... Args>
void RuntimeScene::InvokeEvent(entt::entity entity, const std::string& eventName, Args&&... args)
{
    if (!m_registry.all_of<ECS::ScriptsComponent>(entity)) return;

    auto& sourceScriptsComponent = m_registry.get<ECS::ScriptsComponent>(entity);
    YAML::Node argsNode;
    (argsNode.push_back(std::forward<Args>(args)), ...);
    YAML::Emitter emitter;
    emitter << argsNode;
    std::string argsAsYaml = emitter.c_str();

    for (const auto& sourceScript : sourceScriptsComponent.scripts)
    {
        if (!sourceScript.eventLinks.contains(eventName))
        {
            continue;
        }

        const auto& targets = sourceScript.eventLinks.at(eventName);
        for (const auto& target : targets)
        {
            RuntimeGameObject targetGO = FindGameObjectByGuid(target.targetEntityGuid);

            if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptsComponent>())
            {
                auto& targetScriptsComponent = targetGO.GetComponent<ECS::ScriptsComponent>();
                for (const auto& targetScript : targetScriptsComponent.scripts)
                {
                    if (targetScript.metadata && targetScript.metadata->name == target.targetComponentName)
                    {
                        InteractScriptEvent scriptEvent;
                        scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                        scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                        scriptEvent.methodName = target.targetMethodName;
                        scriptEvent.methodArgs = argsAsYaml;

                        EventBus::GetInstance().Publish(scriptEvent);

                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief 创建一个带有指定组件的新实体（游戏对象）。
 * @tparam T 要添加的组件类型。
 * @tparam Args 构造组件所需的参数类型。
 * @param name 游戏对象的名称。
 * @param args 构造组件所需的参数。
 * @return 新创建的运行时游戏对象。
 */
template <typename T, typename... Args>
RuntimeGameObject RuntimeScene::CreateEntityWith(const std::string& name, Args&&... args)
{
    RuntimeGameObject go = CreateGameObject(name);
    go.AddComponent<T>(std::forward<Args>(args)...);
    return go;
}

#include "RuntimeObject.inl"

#endif

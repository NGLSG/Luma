#include "ScriptingSystem.h"
#include "ProjectSettings.h"
#include "ScriptMetadataRegistry.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Resources/Loaders/CSharpScriptLoader.h"
#include "../Components/ScriptComponent.h"
#include "../Utils/PCH.h"

namespace Systems
{
    void ScriptingSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        m_scriptEventHandle = EventBus::GetInstance().Subscribe<InteractScriptEvent>(
            [this](const InteractScriptEvent& event) { handleScriptInteractEvent(event); }
        );
        m_physicsContactEventHandle = EventBus::GetInstance().Subscribe<PhysicsContactEvent>(
            [this](const PhysicsContactEvent& event) { handlePhysicsContactEvent(event); }
        );
        m_currentScene = scene;
        m_isEditorMode = (context.appMode != ApplicationMode::Runtime);

        m_loadedAssemblyPath = m_isEditorMode
                                   ? (ProjectSettings::GetInstance().GetProjectRoot() / "Library/GameScripts.dll").
                                   string()
                                   : Directory::GetAbsolutePath("./GameData/GameScripts.dll");

        CoreCLRHost::CreateNewInstance();
        CoreCLRHost* host = getHost();
        if (!host)
        {
            LogError("ScriptingSystem: 无法获取 CoreCLRHost 实例");
            return;
        }

        if (!host->Initialize(m_loadedAssemblyPath, m_isEditorMode))
        {
            LogError("ScriptingSystem: 初始化 CLR 宿主失败");
            return;
        }

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::ScriptComponent>();
        CSharpScriptLoader scriptLoader;


        for (auto entity : view)
        {
            auto& scriptComp = view.get<ECS::ScriptComponent>(entity);
            if (!scriptComp.scriptAsset.Valid()) continue;

            sk_sp<RuntimeCSharpScript> scriptAsset = scriptLoader.LoadAsset(scriptComp.scriptAsset.assetGuid);
            if (scriptAsset)
            {
                scriptComp.metadata = &scriptAsset->GetMetadata();
                createInstanceCommand(static_cast<uint32_t>(entity), scriptAsset->GetScriptClassName(),
                                      scriptAsset->GetAssemblyName());
            }
        }


        for (auto entity : view)
        {
            auto& scriptComp = view.get<ECS::ScriptComponent>(entity);
            if (!scriptComp.scriptAsset.Valid()) continue;

            if (scriptComp.propertyOverrides.IsMap())
            {
                for (const auto& it : scriptComp.propertyOverrides)
                {
                    std::string propName = it.first.as<std::string>();
                    YAML::Emitter emitter;
                    emitter << it.second;
                    setPropertyCommand(static_cast<uint32_t>(entity), propName, emitter.c_str());
                }
            }
            onCreateCommand(static_cast<uint32_t>(entity));
        }
    }

    void ScriptingSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        CoreCLRHost* host = getHost();
        if (!host || !host->GetUpdateInstanceFn()) return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<const ECS::ScriptComponent>();
        for (auto entity : view)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            const auto& scriptComp = view.get<const ECS::ScriptComponent>(entity);
            if (!scriptComp.Enable)
                continue;
            if (scriptComp.scriptAsset.Valid())
            {
                updateInstanceCommand(static_cast<uint32_t>(entity), deltaTime);
            }
        }
    }

    void ScriptingSystem::OnDestroy(RuntimeScene* scene)
    {
        if (m_scriptEventHandle.IsValid())
        {
            EventBus::GetInstance().Unsubscribe(m_scriptEventHandle);
        }
        if (m_physicsContactEventHandle.IsValid())
        {
            EventBus::GetInstance().Unsubscribe(m_physicsContactEventHandle);
        }
        destroyAllInstances();
        CoreCLRHost::DestroyInstance();
        m_currentScene = nullptr;
        m_loadedAssemblyPath.clear();
        LogInfo("ScriptingSystem: 脚本系统已关闭。");
    }

    void ScriptingSystem::handleScriptInteractEvent(const InteractScriptEvent& event)
    {
        switch (event.type)
        {
        case InteractScriptEvent::CommandType::CreateInstance:
            createInstanceCommand(event.entityId, event.typeName, event.assemblyName);
            break;
        case InteractScriptEvent::CommandType::OnCreate:
            onCreateCommand(event.entityId);
            break;
        case InteractScriptEvent::CommandType::ActivityChange:
            activityChangeCommand(event.entityId, event.isActive);
            break;
        case InteractScriptEvent::CommandType::DestroyInstance:
            destroyInstanceCommand(event.entityId);
            break;
        case InteractScriptEvent::CommandType::UpdateInstance:
            updateInstanceCommand(event.entityId, event.deltaTime);
            break;
        case InteractScriptEvent::CommandType::SetProperty:
            setPropertyCommand(event.entityId, event.propertyName, event.propertyValue);
            break;
        case InteractScriptEvent::CommandType::InvokeMethod:
            invokeMethodCommand(event.entityId, event.methodName, event.methodArgs);
            break;
        }
    }

    void ScriptingSystem::handlePhysicsContactEvent(const PhysicsContactEvent& event)
    {
        if (!m_currentScene) return;
        auto host = getHost();
        auto m_dispatchCollisionFn = host ? host->GetDispatchCollisionEventFn() : nullptr;
        if (!m_dispatchCollisionFn)
        {
            LogError("ScriptingSystem: DispatchCollisionEventFn 不可用，无法派发碰撞事件。");
            return;
        }
        auto& registry = m_currentScene->GetRegistry();


        if (registry.valid(event.entityA) && registry.all_of<ECS::ScriptComponent>(event.entityA))
        {
            auto& scriptComp = registry.get<ECS::ScriptComponent>(event.entityA);
            if (scriptComp.managedGCHandle)
            {
                m_dispatchCollisionFn(*scriptComp.managedGCHandle, (int)event.type, (uint32_t)event.entityB);
            }
        }


        if (registry.valid(event.entityB) && registry.all_of<ECS::ScriptComponent>(event.entityB))
        {
            auto& scriptComp = registry.get<ECS::ScriptComponent>(event.entityB);
            if (scriptComp.managedGCHandle)
            {
                m_dispatchCollisionFn(*scriptComp.managedGCHandle, (int)event.type, (uint32_t)event.entityA);
            }
        }
    }

    void ScriptingSystem::createInstanceCommand(uint32_t entityId, const std::string& typeName,
                                                const std::string& assemblyName)
    {
        CoreCLRHost* host = getHost();
        if (!host || !host->GetCreateInstanceFn())
        {
            LogError("ScriptingSystem: CreateInstanceFn 不可用，无法创建实例。");
            return;
        }

        ManagedGCHandle handle = host->GetCreateInstanceFn()(m_currentScene, entityId, typeName.c_str(),
                                                             assemblyName.c_str());
        if (handle != 0)
        {
            m_entityHandles[entityId] = handle;
            auto& registry = m_currentScene->GetRegistry();
            if (registry.valid((entt::entity)entityId) && registry.all_of<ECS::ScriptComponent>((entt::entity)entityId))
            {
                auto& scriptComp = registry.get<ECS::ScriptComponent>((entt::entity)entityId);
                scriptComp.managedGCHandle = &m_entityHandles[entityId];
            }
        }
        else
        {
            LogError("ScriptingSystem: 创建实例失败，实体ID: {}, 类型: {}", entityId, typeName);
        }
    }

    void ScriptingSystem::activityChangeCommand(uint32_t entityId, bool isActive)
    {
        CoreCLRHost* host = getHost();
        auto it = m_entityHandles.find(entityId);
        if (!host || it == m_entityHandles.end()) return;
        if (isActive)
        {
            auto callOnEnableFn = host->GetCallOnEnableFn();
            if (callOnEnableFn)
            {
                callOnEnableFn(it->second);
            }
        }
        else
        {
            auto callOnDisableFn = host->GetCallOnDisableFn();
            if (callOnDisableFn)
            {
                callOnDisableFn(it->second);
            }
        }
    }

    void ScriptingSystem::onCreateCommand(uint32_t entityId)
    {
        CoreCLRHost* host = getHost();
        auto it = m_entityHandles.find(entityId);
        if (!host || !host->GetOnCreateFn() || it == m_entityHandles.end()) return;
        host->GetOnCreateFn()(it->second);
    }

    void ScriptingSystem::destroyInstanceCommand(uint32_t entityId)
    {
        auto it = m_entityHandles.find(entityId);
        if (it == m_entityHandles.end()) return;

        CoreCLRHost* host = getHost();
        if (!host || !host->GetDestroyInstanceFn())
        {
            LogWarn("ScriptingSystem: DestroyInstanceFn 不可用，句柄可能已泄露！实体ID: {}", entityId);
            m_entityHandles.erase(it);
            return;
        }

        auto& registry = m_currentScene->GetRegistry();
        if (registry.valid((entt::entity)entityId) && registry.all_of<ECS::ScriptComponent>((entt::entity)entityId))
        {
            registry.get<ECS::ScriptComponent>((entt::entity)entityId).managedGCHandle = nullptr;
        }

        host->GetDestroyInstanceFn()(it->second);
        m_entityHandles.erase(it);
    }

    void ScriptingSystem::updateInstanceCommand(uint32_t entityId, float deltaTime)
    {
        CoreCLRHost* host = getHost();
        auto it = m_entityHandles.find(entityId);
        if (!host || !host->GetUpdateInstanceFn() || it == m_entityHandles.end()) return;
        host->GetUpdateInstanceFn()(it->second, deltaTime);
    }

    void ScriptingSystem::setPropertyCommand(uint32_t entityId, const std::string& propertyName,
                                             const std::string& value)
    {
        CoreCLRHost* host = getHost();
        auto it = m_entityHandles.find(entityId);
        if (!host || !host->GetSetPropertyFn() || it == m_entityHandles.end())
        {
            LogError("ScriptingSystem: SetProperty 失败，实体ID: {}，句柄不存在或委托不可用", entityId);
            return;
        }
        host->GetSetPropertyFn()(it->second, propertyName.c_str(), value.c_str());
    }

    void ScriptingSystem::invokeMethodCommand(uint32_t entityId, const std::string& methodName, const std::string& args)
    {
        CoreCLRHost* host = getHost();
        auto it = m_entityHandles.find(entityId);
        if (!host || !host->GetInvokeMethodFn() || it == m_entityHandles.end())
        {
            LogError("ScriptingSystem: InvokeMethod 失败，实体ID: {}，句柄不存在或委托不可用", entityId);
            return;
        }

        host->GetInvokeMethodFn()(it->second, methodName.c_str(), args.c_str());
        LogInfo("ScriptingSystem: 调用方法成功，实体ID: {}, 方法: {}", entityId, methodName);
    }

    void ScriptingSystem::destroyAllInstances()
    {
        CoreCLRHost* host = getHost();
        auto& registry = m_currentScene->GetRegistry();

        for (auto& [entityId, handle] : m_entityHandles)
        {
            if (registry.valid((entt::entity)entityId) && registry.all_of<ECS::ScriptComponent>((entt::entity)entityId))
            {
                registry.get<ECS::ScriptComponent>((entt::entity)entityId).managedGCHandle = nullptr;
            }
            if (host && host->GetDestroyInstanceFn() && handle != 0)
            {
                host->GetDestroyInstanceFn()(handle);
            }
        }
        m_entityHandles.clear();
    }
}

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

        ManagedHost::CreateNewInstance();
        ManagedHost* host = getHost();
        if (!host)
        {
            LogError("ScriptingSystem: 无法获取 ManagedHost 实例");
            return;
        }

        if (!host->Initialize(m_loadedAssemblyPath, m_isEditorMode))
        {
            LogError("ScriptingSystem: 初始化 CLR 宿主失败");
            return;
        }

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::ScriptsComponent>();
        CSharpScriptLoader scriptLoader;

        
        for (auto entity : view)
        {
            auto& scriptsComp = view.get<ECS::ScriptsComponent>(entity);
            for (auto& scriptComp : scriptsComp.scripts)
            {
                if (!scriptComp.scriptAsset.Valid()) continue;

                sk_sp<RuntimeCSharpScript> scriptAsset = scriptLoader.LoadAsset(scriptComp.scriptAsset.assetGuid);
                if (scriptAsset)
                {
                    scriptComp.metadata = &scriptAsset->GetMetadata();
                }
            }
        }

        
        for (auto entity : view)
        {
            auto& scriptsComp = view.get<ECS::ScriptsComponent>(entity);
            for (auto& scriptComp : scriptsComp.scripts)
            {
                if (!scriptComp.scriptAsset.Valid() || scriptComp.managedGCHandle) continue;

                sk_sp<RuntimeCSharpScript> scriptAsset = scriptLoader.LoadAsset(scriptComp.scriptAsset.assetGuid);
                if (!scriptAsset) continue;

                if (!host->GetCreateInstanceFn())
                {
                    LogError("ScriptingSystem: CreateInstanceFn 不可用，跳过实例创建。");
                    continue;
                }

                ManagedGCHandle handle = host->GetCreateInstanceFn()(
                    m_currentScene,
                    static_cast<uint32_t>(entity),
                    scriptAsset->GetScriptClassName().c_str(),
                    scriptAsset->GetAssemblyName().c_str()
                );

                if (handle != 0)
                {
                    m_managedHandles.push_back(handle);
                    scriptComp.managedGCHandle = &m_managedHandles.back();
                }
                else
                {
                    LogError("ScriptingSystem: 创建实例失败，实体ID: {}, 类型: {}", static_cast<uint32_t>(entity),
                             scriptAsset->GetScriptClassName());
                }
            }
        }

        
        for (auto entity : view)
        {
            auto& scriptsComp = view.get<ECS::ScriptsComponent>(entity);
            for (auto& scriptComp : scriptsComp.scripts)
            {
                if (!scriptComp.managedGCHandle) continue;

                
                if (scriptComp.propertyOverrides.IsMap())
                {
                    if (auto setPropertyFn = host->GetSetPropertyFn())
                    {
                        for (const auto& it : scriptComp.propertyOverrides)
                        {
                            std::string propName = it.first.as<std::string>();
                            YAML::Emitter emitter;
                            emitter << it.second;
                            setPropertyFn(*scriptComp.managedGCHandle, propName.c_str(), emitter.c_str());
                        }
                    }
                }

                
                setupEventLinks(scriptComp, static_cast<uint32_t>(entity));

                
                if (auto onCreateFn = host->GetOnCreateFn())
                {
                    onCreateFn(*scriptComp.managedGCHandle);
                }
            }
        }
    }

    void ScriptingSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        ManagedHost* host = getHost();
        if (!host || !host->GetUpdateInstanceFn()) return;

        auto updateFn = host->GetUpdateInstanceFn();
        auto& registry = scene->GetRegistry();
        auto view = registry.view<const ECS::ScriptsComponent>();

        for (auto entity : view)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;

            const auto& scriptsComp = view.get<const ECS::ScriptsComponent>(entity);
            for (const auto& scriptComp : scriptsComp.scripts)
            {
                if (!scriptComp.Enable || !scriptComp.managedGCHandle)
                    continue;

                updateFn(*scriptComp.managedGCHandle, deltaTime);
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
        ManagedHost::DestroyInstance();
        m_currentScene = nullptr;
        m_loadedAssemblyPath.clear();
        LogInfo("ScriptingSystem: 脚本系统已关闭。");
    }

    void ScriptingSystem::setupEventLinks(const ECS::ScriptComponent& scriptComp, uint32_t entityId)
    {
        ManagedHost* host = getHost();
        if (!host || !host->GetSetPropertyFn() || !scriptComp.managedGCHandle) return;

        auto setPropertyFn = host->GetSetPropertyFn();

        
        for (const auto& [eventName, targets] : scriptComp.eventLinks)
        {
            if (targets.empty()) continue;

            
            YAML::Node eventTargetsNode;
            eventTargetsNode = YAML::Node(YAML::NodeType::Sequence);

            for (const auto& target : targets)
            {
                if (!target.targetEntityGuid.Valid()) continue;

                YAML::Node targetNode;
                targetNode["entityGuid"] = target.targetEntityGuid.ToString();
                targetNode["componentName"] = target.targetComponentName;
                targetNode["methodName"] = target.targetMethodName;

                eventTargetsNode.push_back(targetNode);
            }

            if (eventTargetsNode.size() > 0)
            {
                YAML::Emitter emitter;
                emitter << eventTargetsNode;

                
                std::string eventLinkPropertyName = "__EventLink_" + eventName;
                setPropertyFn(*scriptComp.managedGCHandle, eventLinkPropertyName.c_str(), emitter.c_str());

                LogInfo("ScriptingSystem: 为实体 {} 的事件 '{}' 设置了 {} 个目标",
                       entityId, eventName, targets.size());
            }
        }
    }

    void ScriptingSystem::handleScriptInteractEvent(const InteractScriptEvent& event)
    {
        switch (event.type)
        {
        case InteractScriptEvent::CommandType::CreateInstance:
            createInstanceCommand(event.entityId, event.typeName, event.assemblyName);
            break;
        case InteractScriptEvent::CommandType::OnCreate:
            onCreateCommand(event.entityId, event.typeName);
            break;
        case InteractScriptEvent::CommandType::ActivityChange:
            activityChangeCommand(event.entityId, event.typeName, event.isActive);
            break;
        case InteractScriptEvent::CommandType::DestroyInstance:
            destroyInstanceCommand(event.entityId, event.typeName);
            break;
        case InteractScriptEvent::CommandType::UpdateInstance:
            {
                ManagedHost* host = getHost();
                auto* scriptComp = findScriptByTypeName(event.entityId, event.typeName);
                if (host && host->GetUpdateInstanceFn() && scriptComp && scriptComp->managedGCHandle)
                {
                    host->GetUpdateInstanceFn()(*scriptComp->managedGCHandle, event.deltaTime);
                }
            }
            break;
        case InteractScriptEvent::CommandType::SetProperty:
            setPropertyCommand(event.entityId, event.typeName, event.propertyName, event.propertyValue);
            break;
        case InteractScriptEvent::CommandType::InvokeMethod:
            invokeMethodCommand(event.entityId, event.typeName, event.methodName, event.methodArgs);
            break;
        }
    }

    void ScriptingSystem::handlePhysicsContactEvent(const PhysicsContactEvent& event)
    {
        if (!m_currentScene) return;
        auto host = getHost();
        auto dispatchCollisionFn = host ? host->GetDispatchCollisionEventFn() : nullptr;
        if (!dispatchCollisionFn)
        {
            LogError("ScriptingSystem: DispatchCollisionEventFn 不可用，无法派发碰撞事件。");
            return;
        }

        auto& registry = m_currentScene->GetRegistry();
        auto dispatchToEntity = [&](entt::entity entity, entt::entity otherEntity)
        {
            if (registry.valid(entity) && registry.all_of<ECS::ScriptsComponent>(entity))
            {
                auto& scriptsComp = registry.get<ECS::ScriptsComponent>(entity);
                for (auto& scriptComp : scriptsComp.scripts)
                {
                    if (scriptComp.managedGCHandle)
                    {
                        dispatchCollisionFn(*scriptComp.managedGCHandle, (int)event.type, (uint32_t)otherEntity);
                    }
                }
            }
        };

        dispatchToEntity(event.entityA, event.entityB);
        dispatchToEntity(event.entityB, event.entityA);
    }

    ECS::ScriptComponent* ScriptingSystem::findScriptByTypeName(uint32_t entityId, const std::string& typeName)
    {
        if (!m_currentScene) return nullptr;
        auto& registry = m_currentScene->GetRegistry();
        if (!registry.valid((entt::entity)entityId) || !registry.all_of<ECS::ScriptsComponent>((entt::entity)entityId))
        {
            return nullptr;
        }

        CSharpScriptLoader loader;
        auto& scriptsComp = registry.get<ECS::ScriptsComponent>((entt::entity)entityId);
        for (auto& script : scriptsComp.scripts)
        {
            if (!script.scriptAsset.Valid()) continue;

            sk_sp<RuntimeCSharpScript> scriptAsset = loader.LoadAsset(script.scriptAsset.assetGuid);
            if (scriptAsset && scriptAsset->GetScriptClassName() == typeName)
            {
                return &script;
            }
        }
        LogWarn("ScriptingSystem: 无法在实体 {} 上找到类型为 '{}' 的脚本。", entityId, typeName);
        return nullptr;
    }

    void ScriptingSystem::createInstanceCommand(uint32_t entityId, const std::string& typeName,
                                                const std::string& assemblyName)
    {
        ManagedHost* host = getHost();
        if (!host || !host->GetCreateInstanceFn())
        {
            LogError("ScriptingSystem: CreateInstanceFn 不可用，无法创建实例。");
            return;
        }

        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!scriptComp) return;

        if (scriptComp->managedGCHandle)
        {
            LogWarn("ScriptingSystem: 实体 {} 的脚本 '{}' 实例已存在。", entityId, typeName);
            return;
        }

        ManagedGCHandle handle = host->GetCreateInstanceFn()(m_currentScene, entityId, typeName.c_str(),
                                                             assemblyName.c_str());
        if (handle != 0)
        {
            m_managedHandles.push_back(handle);
            scriptComp->managedGCHandle = &m_managedHandles.back();

            
            setupEventLinks(*scriptComp, entityId);
        }
        else
        {
            LogError("ScriptingSystem: 创建实例失败，实体ID: {}, 类型: {}", entityId, typeName);
        }
    }

    void ScriptingSystem::activityChangeCommand(uint32_t entityId, const std::string& typeName, bool isActive)
    {
        ManagedHost* host = getHost();
        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!host || !scriptComp || !scriptComp->managedGCHandle) return;

        if (isActive)
        {
            if (auto callOnEnableFn = host->GetCallOnEnableFn())
            {
                callOnEnableFn(*scriptComp->managedGCHandle);
            }
        }
        else
        {
            if (auto callOnDisableFn = host->GetCallOnDisableFn())
            {
                callOnDisableFn(*scriptComp->managedGCHandle);
            }
        }
    }

    void ScriptingSystem::onCreateCommand(uint32_t entityId, const std::string& typeName)
    {
        ManagedHost* host = getHost();
        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!host || !host->GetOnCreateFn() || !scriptComp || !scriptComp->managedGCHandle) return;

        host->GetOnCreateFn()(*scriptComp->managedGCHandle);
    }

    void ScriptingSystem::destroyInstanceCommand(uint32_t entityId, const std::string& typeName)
    {
        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!scriptComp || !scriptComp->managedGCHandle) return;

        ManagedGCHandle handle = *scriptComp->managedGCHandle;
        ManagedHost* host = getHost();

        if (host && host->GetDestroyInstanceFn())
        {
            host->GetDestroyInstanceFn()(handle);
        }
        else
        {
            LogWarn("ScriptingSystem: DestroyInstanceFn 不可用，句柄可能已泄露！实体ID: {}，类型: {}", entityId, typeName);
        }

        m_managedHandles.remove(handle);
        scriptComp->managedGCHandle = nullptr;
    }

    void ScriptingSystem::setPropertyCommand(uint32_t entityId, const std::string& typeName,
                                             const std::string& propertyName,
                                             const std::string& value)
    {
        ManagedHost* host = getHost();
        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!host || !host->GetSetPropertyFn() || !scriptComp || !scriptComp->managedGCHandle)
        {
            LogError("ScriptingSystem: SetProperty 失败，实体ID: {}，类型: {}，句柄不存在或委托不可用", entityId, typeName);
            return;
        }
        host->GetSetPropertyFn()(*scriptComp->managedGCHandle, propertyName.c_str(), value.c_str());
    }

    void ScriptingSystem::invokeMethodCommand(uint32_t entityId, const std::string& typeName,
                                              const std::string& methodName,
                                              const std::string& args)
    {
        ManagedHost* host = getHost();
        auto* scriptComp = findScriptByTypeName(entityId, typeName);
        if (!host || !host->GetInvokeMethodFn() || !scriptComp || !scriptComp->managedGCHandle)
        {
            LogError("ScriptingSystem: InvokeMethod 失败，实体ID: {}，类型: {}，句柄不存在或委托不可用", entityId, typeName);
            return;
        }

        host->GetInvokeMethodFn()(*scriptComp->managedGCHandle, methodName.c_str(), args.c_str());
        LogInfo("ScriptingSystem: 调用方法成功，实体ID: {}, 类型: {}, 方法: {}", entityId, typeName, methodName);
    }

    void ScriptingSystem::destroyAllInstances()
    {
        ManagedHost* host = getHost();
        if (host && host->GetDestroyInstanceFn())
        {
            auto destroyFn = host->GetDestroyInstanceFn();
            for (auto handle : m_managedHandles)
            {
                if (handle != 0)
                {
                    destroyFn(handle);
                }
            }
        }

        m_managedHandles.clear();
    }
}

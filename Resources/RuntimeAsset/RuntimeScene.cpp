#include "RuntimeScene.h"
#include "RuntimeGameObject.h"
#include "../../Components/IDComponent.h"
#include "../../Components/Transform.h"
#include "../../Components/RelationshipComponent.h"
#include "../../Components/ComponentRegistry.h"


#include "ActivityComponent.h"
#include "../Loaders/PrefabLoader.h"
#include "Event/Events.h"

RuntimeScene::RuntimeScene(const Guid& guid)
{
    m_sourceGuid = guid;

    m_registry.on_destroy<ECS::IDComponent>().connect<&RuntimeScene::OnEntityDestroyed>(this);
}

RuntimeScene::RuntimeScene() : RuntimeScene(Guid::NewGuid())
{
}

RuntimeScene::~RuntimeScene()
{
    m_registry.on_destroy<ECS::IDComponent>().disconnect(this);


    for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it)
    {
        (*it)->OnDestroy(this);
    }
    m_systems.clear();
    for (auto it = m_essSystems.rbegin(); it != m_essSystems.rend(); ++it)
    {
        (*it)->OnDestroy(this);
    }
    m_essSystems.clear();
}

void RuntimeScene::Activate(EngineContext& engineCtx)
{
    for (auto& essSystem : m_essSystems)
    {
        essSystem->OnCreate(this, engineCtx);
    }

    for (const auto& system : m_systems)
    {
        system->OnCreate(this, engineCtx);
    }
}

sk_sp<RuntimeScene> RuntimeScene::CreatePlayModeCopy()
{
    auto copy = sk_make_sp<RuntimeScene>();
    copy->CloneFromScene(*this);
    return copy;
}

void RuntimeScene::CloneFromScene(const RuntimeScene& sourceScene)
{
    Clear();


    m_name = sourceScene.m_name;
    m_cameraProperties = sourceScene.m_cameraProperties;


    std::unordered_map<entt::entity, entt::entity> entityMapping;


    auto allEntities = sourceScene.m_registry.view<ECS::IDComponent>();
    for (auto sourceEntity : allEntities)
    {
        entt::entity newEntity = cloneEntity(sourceScene.m_registry, sourceEntity,
                                             m_registry, entityMapping);


        if (m_registry.all_of<ECS::IDComponent>(newEntity))
        {
            const auto& idComp = m_registry.get<ECS::IDComponent>(newEntity);
            m_guidToEntityMap[idComp.guid] = newEntity;
        }
    }


    rebuildRelationships(entityMapping);


    m_rootGameObjects.clear();
    auto rootView = m_registry.view<ECS::IDComponent>(entt::exclude<ECS::ParentComponent>);
    for (auto entity : rootView)
    {
        m_rootGameObjects.emplace_back(entity, this);
    }
}

entt::entity RuntimeScene::cloneEntity(const entt::registry& sourceRegistry, entt::entity sourceEntity,
                                       entt::registry& targetRegistry,
                                       std::unordered_map<entt::entity, entt::entity>& entityMapping)
{
    entt::entity newEntity = targetRegistry.create();


    entityMapping[sourceEntity] = newEntity;


    auto& compRegistry = ComponentRegistry::GetInstance();
    for (const auto& compName : compRegistry.GetAllRegisteredNames())
    {
        compRegistry.CloneComponent(compName, sourceRegistry, sourceEntity,
                                    targetRegistry, newEntity);
    }

    return newEntity;
}

void RuntimeScene::rebuildRelationships(const std::unordered_map<entt::entity, entt::entity>& entityMapping)
{
    auto childView = m_registry.view<ECS::ParentComponent>();
    for (auto childEntity : childView)
    {
        auto& parentComp = m_registry.get<ECS::ParentComponent>(childEntity);


        auto parentIt = entityMapping.find(parentComp.parent);
        if (parentIt != entityMapping.end())
        {
            entt::entity newParentEntity = parentIt->second;


            parentComp.parent = newParentEntity;


            if (!m_registry.all_of<ECS::ChildrenComponent>(newParentEntity))
            {
                m_registry.emplace<ECS::ChildrenComponent>(newParentEntity);
            }

            auto& childrenComp = m_registry.get<ECS::ChildrenComponent>(newParentEntity);


            if (std::find(childrenComp.children.begin(), childrenComp.children.end(), childEntity)
                == childrenComp.children.end())
            {
                childrenComp.children.push_back(childEntity);
            }
        }
    }
}

void RuntimeScene::Clear()
{
    m_registry.clear();
    m_rootGameObjects.clear();
    m_guidToEntityMap.clear();
}

Data::PrefabNode RuntimeScene::SerializeEntity(entt::entity entity, entt::registry& registry)
{
    Data::PrefabNode nodeData;
    auto& compRegistry = ComponentRegistry::GetInstance();

    nodeData.name = registry.get<ECS::IDComponent>(entity).name;
    nodeData.localGuid = registry.get<ECS::IDComponent>(entity).guid;

    for (const auto& compName : compRegistry.GetAllRegisteredNames())
    {
        auto* registration = compRegistry.Get(compName);
        if (registration && registration->serialize && registration->has(registry, entity))
        {
            nodeData.components[compName] = registration->serialize(registry, entity);
        }
    }

    if (registry.all_of<ECS::ChildrenComponent>(entity))
    {
        for (auto childEntity : registry.get<ECS::ChildrenComponent>(entity).children)
        {
            nodeData.children.push_back(SerializeEntity(childEntity, registry));
        }
    }
    return nodeData;
}


Data::SceneData RuntimeScene::SerializeToData()
{
    Data::SceneData sceneData;
    sceneData.name = m_name;
    sceneData.cameraProperties = m_cameraProperties;

    auto rootView = m_registry.view<ECS::IDComponent>(entt::exclude<ECS::ParentComponent>);
    for (auto entity : rootView)
    {
        sceneData.entities.push_back(SerializeEntity(entity, m_registry));
    }
    return sceneData;
}


void RuntimeScene::AddToRoot(RuntimeGameObject go)
{
    auto& roots = GetRootGameObjects();
    if (std::find(roots.begin(), roots.end(), go) == roots.end())
    {
        roots.push_back(go);
    }
}

void RuntimeScene::RemoveFromRoot(RuntimeGameObject go)
{
    std::erase(GetRootGameObjects(), go);
}

void RuntimeScene::SetCameraProperties(const Camera::CamProperties& properties)
{
    m_cameraProperties = properties;
}

Camera::CamProperties& RuntimeScene::GetCameraProperties()
{
    return m_cameraProperties;
}

void RuntimeScene::DestroyGameObject(RuntimeGameObject& gameObject)
{
    entt::entity entity = static_cast<entt::entity>(gameObject);


    EventBus::GetInstance().Publish(GameObjectDestroyedEvent{m_registry, entity});


    for (auto& child : gameObject.GetChildren())
    {
        DestroyGameObject(child);
    }


    RemoveFromRoot(gameObject);


    m_registry.destroy(entity);
}

void RuntimeScene::Update(float deltaTime, EngineContext& engineCtx, bool pauseNormalSystem)
{
    for (auto& essSystem : m_essSystems)
    {
        essSystem->OnUpdate(this, deltaTime, engineCtx);
    }
    if (!pauseNormalSystem)
    {
        for (const auto& system : m_systems)
        {
            system->OnUpdate(this, deltaTime, engineCtx);
        }
    }
}

void RuntimeScene::LoadFromData(const Data::SceneData& sceneData)
{
    m_name = sceneData.name;
    m_cameraProperties = sceneData.cameraProperties;
    Camera::GetInstance().SetProperties(sceneData.cameraProperties);
    m_registry.clear();
    m_rootGameObjects.clear();
    m_guidToEntityMap.clear();

    for (const auto& rootNode : sceneData.entities)
    {
        CreateHierarchyFromNode(rootNode, nullptr, false);
    }
}

RuntimeGameObject RuntimeScene::FindGameObjectByEntity(entt::entity handle)
{
    if (m_registry.valid(handle))
    {
        return {handle, this};
    }

    return {entt::null, nullptr};
}

RuntimeGameObject RuntimeScene::FindGameObjectByGuid(const Guid& guid)
{
    auto it = m_guidToEntityMap.find(guid);
    if (it != m_guidToEntityMap.end())
    {
        if (m_registry.valid(it->second))
        {
            return {it->second, this};
        }

        m_guidToEntityMap.erase(it);
    }
    return {entt::null, nullptr};
}


RuntimeGameObject RuntimeScene::CreateGameObject(const std::string& name)
{
    entt::entity newHandle = m_registry.create();
    RuntimeGameObject newGameObject(newHandle, this);


    auto& id = newGameObject.AddComponent<ECS::IDComponent>();
    id.name = name;
    id.guid = Guid::NewGuid();


    newGameObject.AddComponent<ECS::Transform>();
    newGameObject.AddComponent<ECS::ActivityComponent>();


    m_guidToEntityMap[id.guid] = newHandle;


    m_rootGameObjects.push_back(newGameObject);


    EventBus::GetInstance().Publish(GameObjectCreatedEvent{m_registry, newHandle});

    return newGameObject;
}

std::unordered_map<std::string, const ComponentRegistration*> RuntimeScene::GetAllComponents(entt::entity entity)
{
    std::unordered_map<std::string, const ComponentRegistration*> components;
    auto& compRegistry = ComponentRegistry::GetInstance();

    for (const auto& compName : compRegistry.GetAllRegisteredNames())
    {
        const ComponentRegistration* reg = compRegistry.Get(compName);
        if (reg && reg->has(m_registry, entity))
        {
            components[compName] = reg;
        }
    }
    return components;
}

entt::registry& RuntimeScene::GetRegistry()
{
    return m_registry;
}

const entt::registry& RuntimeScene::GetRegistry() const
{
    return m_registry;
}

std::vector<RuntimeGameObject>& RuntimeScene::GetRootGameObjects()
{
    return m_rootGameObjects;
}

RuntimeGameObject RuntimeScene::CreateHierarchyFromNode(const Data::PrefabNode& node, RuntimeGameObject* parent,
                                                        bool newGuid)
{
    entt::entity newHandle = m_registry.create();
    RuntimeGameObject newGameObject(newHandle, this);


    auto& id = newGameObject.AddComponent<ECS::IDComponent>();
    id.name = node.name;
    id.guid = newGuid ? Guid::NewGuid() : node.localGuid;


    m_guidToEntityMap[id.guid] = newHandle;


    auto& compRegistry = ComponentRegistry::GetInstance();
    for (const auto& [compName, compData] : node.components)
    {
        if (compName == "IDComponent")
        {
            continue;
        }

        const ComponentRegistration* reg = compRegistry.Get(compName);
        if (reg && reg->deserialize)
        {
            if (!reg->has(m_registry, newHandle))
            {
                reg->add(m_registry, newHandle);


                EventBus::GetInstance().Publish(ComponentAddedEvent{m_registry, newHandle, compName});
            }


            reg->deserialize(m_registry, newHandle, compData);
        }
    }


    if (!newGameObject.HasComponent<ECS::Transform>())
    {
        newGameObject.AddComponent<ECS::Transform>();
        EventBus::GetInstance().Publish(ComponentAddedEvent{m_registry, newHandle, "Transform"});
    }


    if (parent)
    {
        newGameObject.SetParent(*parent);
    }
    else
    {
        m_rootGameObjects.push_back(newGameObject);
    }


    EventBus::GetInstance().Publish(GameObjectCreatedEvent{m_registry, newHandle});


    for (const auto& childNode : node.children)
    {
        CreateHierarchyFromNode(childNode, &newGameObject);
    }

    return newGameObject;
}

int RuntimeScene::GetRootSiblingIndex(const RuntimeGameObject& object) const
{
    if (object.HasComponent<ECS::ParentComponent>())
    {
        return -1;
    }


    auto it = std::find(m_rootGameObjects.begin(), m_rootGameObjects.end(), object);

    if (it != m_rootGameObjects.end())
    {
        return static_cast<int>(std::distance(m_rootGameObjects.begin(), it));
    }

    return -1;
}

void RuntimeScene::SetRootSiblingIndex(RuntimeGameObject& object, int newIndex)
{
    if (object.HasComponent<ECS::ParentComponent>()) return;

    auto it = std::find(m_rootGameObjects.begin(), m_rootGameObjects.end(), object);
    if (it == m_rootGameObjects.end()) return;

    m_rootGameObjects.erase(it);

    if (newIndex < 0) newIndex = 0;
    if (newIndex > m_rootGameObjects.size()) newIndex = (int)m_rootGameObjects.size();

    m_rootGameObjects.insert(m_rootGameObjects.begin() + newIndex, object);
}


RuntimeGameObject RuntimeScene::Instantiate(RuntimePrefab& prefab, RuntimeGameObject* parent)
{
    const Data::PrefabNode& rootNode = prefab.GetData().root;

    return CreateHierarchyFromNode(rootNode, parent);
}

void RuntimeScene::InvokeEventFromSerializedArgs(entt::entity entity, const std::string& eventName,
                                                 const std::string& argsAsYaml)
{
    if (!m_registry.all_of<ECS::ScriptsComponent>(entity))
    {
        LogError("实体 {} 没有 ScriptsComponent 组件。", (uint64_t)entity);
        return;
    }

    auto& sourceScriptsComponent = m_registry.get<ECS::ScriptsComponent>(entity);

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

void RuntimeScene::OnEntityDestroyed(entt::registry& registry, entt::entity entity)
{
    if (registry.all_of<ECS::IDComponent>(entity))
    {
        const auto& idComp = registry.get<ECS::IDComponent>(entity);
        m_guidToEntityMap.erase(idComp.guid);
    }
}

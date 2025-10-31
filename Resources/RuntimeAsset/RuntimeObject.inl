#ifndef RUNTIMEOBJECT_INL
#define RUNTIMEOBJECT_INL
#include "AssetManager.h"

template <typename T, typename... Args>
T& RuntimeGameObject::AddComponent(Args&&... args)
{
    if (!CurrentSceneIsEqualToMScene())
    {
        //LogWarn("Attempted to add component to a RuntimeGameObject whose scene is not the current active scene.");
    }
    static_assert(std::is_base_of_v<ECS::IComponent, T>,
                  "T must be derived from ECS::IComponent");

    static_assert(!std::is_same_v<T, ECS::ScriptComponent>,
                  "Do not add ScriptComponent directly. Use AddComponent<ScriptsComponent>() and then the .AddScript() method instead.")
        ;


    auto& registry = m_scene->GetRegistry();
    T& component = registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
    component.Enable = true;

    std::string name = typeid(T).name();
    EventBus::GetInstance().Publish(ComponentAddedEvent{registry, m_entityHandle, name});

    m_componentNames.push_back(name);

    return component;
}

template <typename T>
T& RuntimeGameObject::GetComponent()
{
    if (!CurrentSceneIsEqualToMScene())
    {
        //LogWarn("Attempted to get component from a RuntimeGameObject whose scene is not the current active scene.");
    }

    if (!HasComponent<T>())
    {
        return AddComponent<T>();
    }
    return m_scene->GetRegistry().get<T>(m_entityHandle);
}

template <typename T>
bool RuntimeGameObject::HasComponent() const
{
    if (!CurrentSceneIsEqualToMScene())
    {
        //LogWarn("Attempted to check component from a RuntimeGameObject whose scene is not the current active scene.");
    }
    if (!m_scene)
    {
        return false;
    }
    return m_scene->GetRegistry().all_of<T>(m_entityHandle);
}

template <typename T>
void RuntimeGameObject::RemoveComponent() const
{
    if (!CurrentSceneIsEqualToMScene())
    {
        //LogWarn("Attempted to remove component from a RuntimeGameObject whose scene is not the current active scene.");
    }
    EventBus::GetInstance().Publish(ComponentRemovedEvent{m_scene->GetRegistry(), m_entityHandle, typeid(T).name()});
    m_scene->GetRegistry().remove<T>(m_entityHandle);
}

#endif //RUNTIMEOBJECT_INL

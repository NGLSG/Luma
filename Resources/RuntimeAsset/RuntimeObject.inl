#ifndef RUNTIMEOBJECT_INL
#define RUNTIMEOBJECT_INL
#include "AssetManager.h"

template <typename T, typename... Args>
T& RuntimeGameObject::AddComponent(Args&&... args)
{
    static_assert(std::is_base_of_v<ECS::IComponent, T>,
                  "T must be derived from ECS::IComponent");
    auto& registry = m_scene->GetRegistry();
    T& component = registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
    component.Enable = true;
    std::string name = typeid(T).name();
    EventBus::GetInstance().Publish(ComponentAddedEvent{registry, m_entityHandle, name});
    if constexpr (std::is_same_v<T, ECS::ScriptComponent>)
    {
        std::string scriptName = AssetManager::GetInstance().GetAssetName(component.scriptAsset.assetGuid) +
            " (ScriptComponent)";
        m_componentNames.push_back(scriptName);
    }

    else
        m_componentNames.push_back(name);
    return component;
}

template <typename T>
T& RuntimeGameObject::GetComponent()
{
    return m_scene->GetRegistry().get<T>(m_entityHandle);
}

template <typename T>
bool RuntimeGameObject::HasComponent() const
{
    return m_scene->GetRegistry().all_of<T>(m_entityHandle);
}

template <typename T>
void RuntimeGameObject::RemoveComponent() const
{
    EventBus::GetInstance().Publish(ComponentRemovedEvent{m_scene->GetRegistry(), m_entityHandle, typeid(T).name()});
    m_scene->GetRegistry().remove<T>(m_entityHandle);
}

#endif //RUNTIMEOBJECT_INL

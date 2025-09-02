#include "ComponentRegistry.h"

void ComponentRegistry::Register(const std::string& name, ComponentRegistration registration)
{
    m_registry[name] = std::move(registration);
}

const ComponentRegistration* ComponentRegistry::Get(const std::string& name) const
{
    if (m_registry.contains(name))
    {
        return &m_registry.at(name);
    }
    return nullptr;
}

std::vector<std::string> ComponentRegistry::GetAllRegisteredNames() const
{
    std::vector<std::string> names;
    names.reserve(m_registry.size());
    for (const auto& [name, reg] : m_registry)
    {
        names.push_back(name);
    }
    return names;
}

void ComponentRegistry::CloneAllComponents(const entt::registry& sourceRegistry, entt::entity sourceEntity,
                                           entt::registry& targetRegistry, entt::entity targetEntity) const
{
    for (const auto& [componentName, registration] : m_registry)
    {
        if (registration.has && registration.has(sourceRegistry, sourceEntity))
        {
            if (registration.clone)
            {
                registration.clone(sourceRegistry, sourceEntity, targetRegistry, targetEntity);
            }
        }
    }
}

bool ComponentRegistry::CloneComponent(const std::string& componentName,
                                       const entt::registry& sourceRegistry, entt::entity sourceEntity,
                                       entt::registry& targetRegistry, entt::entity targetEntity) const
{
    auto it = m_registry.find(componentName);
    if (it == m_registry.end())
    {
        return false;
    }

    const auto& registration = it->second;


    if (!registration.has || !registration.has(sourceRegistry, sourceEntity))
    {
        return false;
    }


    if (registration.clone)
    {
        registration.clone(sourceRegistry, sourceEntity, targetRegistry, targetEntity);
        return true;
    }

    return false;
}

#include "RuntimeGameObject.h"
#include "RuntimeScene.h"
#include "../../Components/IDComponent.h"
#include "../../Components/RelationshipComponent.h"
#include "../../Components/LayerComponent.h"
#include <algorithm>

#include "Transform.h"
#include "../../Data/PrefabData.h"

#include <glm/glm.hpp>

#include "ActivityComponent.h"
#include "SceneManager.h"
#include "glm/detail/type_quat.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "Components/ComponentRegistry.h"
#include "../Utils/Logger.h"

RuntimeGameObject::RuntimeGameObject(entt::entity handle, RuntimeScene* scene)
    : m_entityHandle(handle), m_scene(scene)
{
}


std::string RuntimeGameObject::GetName()
{
    return HasComponent<ECS::IDComponent>() ? GetComponent<ECS::IDComponent>().name : "";
}

void RuntimeGameObject::SetName(const std::string& name)
{
    if (HasComponent<ECS::IDComponent>())
    {
        auto& idComp = GetComponent<ECS::IDComponent>();
        idComp.name = name;
    }
    else
    {
        throw std::runtime_error("Cannot set name on GameObject without IDComponent.");
    }
}

Guid RuntimeGameObject::GetGuid()
{
    return HasComponent<ECS::IDComponent>() ? GetComponent<ECS::IDComponent>().guid : Guid();
}

bool RuntimeGameObject::operator==(const RuntimeGameObject& other) const
{
    return m_entityHandle == other.m_entityHandle && m_scene == other.m_scene;
}

bool RuntimeGameObject::IsValid() const
{
    return m_scene != nullptr && m_scene->GetRegistry().valid(m_entityHandle);
}

Data::PrefabNode RuntimeGameObject::SerializeToPrefabData()
{
    Data::PrefabNode node = m_scene->SerializeEntity(m_entityHandle, m_scene->GetRegistry());
    return node;
}

bool RuntimeGameObject::IsDescendantOf(RuntimeGameObject dragged_object)
{
    if (!IsValid() || !dragged_object.IsValid())
        return false;


    RuntimeGameObject current = *this;
    while (current.IsValid())
    {
        if (current == dragged_object)
            return true;

        current = current.GetParent();
    }
    return false;
}

void RuntimeGameObject::SetChildren(std::vector<RuntimeGameObject>& children)
{
    for (auto& child : children)
    {
        child.SetParent(*this);
    }
}

void RuntimeGameObject::SetSiblingIndex(int newIndex)
{
    if (!HasComponent<ECS::ParentComponent>()) return;

    RuntimeGameObject parent = GetParent();
    if (parent.IsValid() && parent.HasComponent<ECS::ChildrenComponent>())
    {
        auto& children = parent.GetComponent<ECS::ChildrenComponent>().children;


        auto it = std::find(children.begin(), children.end(), m_entityHandle);
        if (it == children.end()) return;


        entt::entity selfHandle = *it;
        children.erase(it);


        if (newIndex < 0) newIndex = 0;
        if (newIndex > children.size()) newIndex = children.size();

        children.insert(children.begin() + newIndex, selfHandle);
    }
}


int RuntimeGameObject::GetSiblingIndex()
{
    if (!HasComponent<ECS::ParentComponent>())
    {
        return -1;
    }

    RuntimeGameObject parent = GetParent();
    if (parent.IsValid() && parent.HasComponent<ECS::ChildrenComponent>())
    {
        const auto& children = parent.GetComponent<ECS::ChildrenComponent>().children;


        auto it = std::find(children.begin(), children.end(), m_entityHandle);

        if (it != children.end())
        {
            return static_cast<int>(std::distance(children.begin(), it));
        }
    }

    return -1;
}

std::unordered_map<std::string, const ComponentRegistration*> RuntimeGameObject::GetAllComponents()
{
    return m_scene->GetAllComponents(GetEntityHandle());
}

bool RuntimeGameObject::CurrentSceneIsEqualToMScene() const
{
    auto currentScene = SceneManager::GetInstance().GetCurrentScene();
    return currentScene.get() == m_scene;
}


void RuntimeGameObject::SetParent(RuntimeGameObject parent)
{
    if (!IsValid()) return;

    if (parent == *this)
    {
        LogWarn("SetParent: attempt to set an object as its own parent. Ignored.");
        return;
    }

    if (parent.IsValid() && parent.IsDescendantOf(*this))
    {
        LogWarn("SetParent: attempt to create cyclic hierarchy (new parent is a descendant). Ignored.");
        return;
    }

    if (HasComponent<ECS::ParentComponent>())
    {
        RuntimeGameObject oldParent = GetParent();
        if (oldParent.IsValid() && oldParent.HasComponent<ECS::ChildrenComponent>())
        {
            auto& children = oldParent.GetComponent<ECS::ChildrenComponent>().children;
            std::erase(children, m_entityHandle);
        }
    }

    m_scene->AddToRoot(*this);

    if (parent.IsValid())
    {
        AddComponent<ECS::ParentComponent>().parent = parent;

        if (!parent.HasComponent<ECS::ChildrenComponent>())
        {
            parent.AddComponent<ECS::ChildrenComponent>();
        }
        parent.GetComponent<ECS::ChildrenComponent>().children.push_back(m_entityHandle);

        m_scene->RemoveFromRoot(*this);
    }
    else
    {
        RemoveComponent<ECS::ParentComponent>();
    }

    auto& childTransform = GetComponent<ECS::TransformComponent>();
    auto& parentTransform = parent.GetComponent<ECS::TransformComponent>();


    glm::mat4 parentWorldMatrix = glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(parentTransform.position.x, parentTransform.position.y,
                                                           0.0f));
    parentWorldMatrix = glm::rotate(parentWorldMatrix, parentTransform.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    parentWorldMatrix = glm::scale(parentWorldMatrix,
                                   glm::vec3(parentTransform.scale.x, parentTransform.scale.y, 1.0f));
    glm::mat4 invParentWorldMatrix = glm::inverse(parentWorldMatrix);


    glm::mat4 childWorldMatrix = glm::translate(glm::mat4(1.0f),
                                                glm::vec3(childTransform.position.x, childTransform.position.y, 0.0f));
    childWorldMatrix = glm::rotate(childWorldMatrix, childTransform.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    childWorldMatrix = glm::scale(childWorldMatrix, glm::vec3(childTransform.scale.x, childTransform.scale.y, 1.0f));


    glm::mat4 newLocalMatrix = invParentWorldMatrix * childWorldMatrix;


    glm::vec3 scaleVec, translationVec, skew;
    glm::vec4 perspective;
    glm::quat rotationQuat;
    glm::decompose(newLocalMatrix, scaleVec, rotationQuat, translationVec, skew, perspective);


    childTransform.localPosition = {
        childTransform.position.x - parentTransform.position.x,
        childTransform.position.y - parentTransform.position.y
    };
    childTransform.localScale = {scaleVec.x, scaleVec.y};
    childTransform.localRotation = glm::eulerAngles(rotationQuat).z;
}

void RuntimeGameObject::SetRoot()
{
    if (!IsValid()) return;


    if (HasComponent<ECS::ParentComponent>())
    {
        RuntimeGameObject parent = GetParent();
        if (parent.IsValid() && parent.HasComponent<ECS::ChildrenComponent>())
        {
            auto& children = parent.GetComponent<ECS::ChildrenComponent>().children;
            std::erase(children, m_entityHandle);
        }
        RemoveComponent<ECS::ParentComponent>();
    }


    m_scene->AddToRoot(*this);
}

RuntimeGameObject RuntimeGameObject::GetParent()
{
    if (HasComponent<ECS::ParentComponent>())
    {
        entt::entity parentHandle = GetComponent<ECS::ParentComponent>().parent;
        return {parentHandle, m_scene};
    }
    return {entt::null, m_scene};
}

std::vector<RuntimeGameObject> RuntimeGameObject::GetChildren()
{
    if (HasComponent<ECS::ChildrenComponent>())
    {
        const auto& childrenComp = GetComponent<ECS::ChildrenComponent>();
        std::vector<RuntimeGameObject> children;
        children.reserve(childrenComp.children.size());

        for (const auto& childHandle : childrenComp.children)
        {
            children.emplace_back(childHandle, m_scene);
        }


        return children;
    }


    return {};
}

bool RuntimeGameObject::IsActive()
{
    return GetComponent<ECS::ActivityComponent>().isActive;
}

void RuntimeGameObject::SetActive(bool active)
{
    auto e = InteractScriptEvent();
    e.type = InteractScriptEvent::CommandType::ActivityChange;
    e.entityId = static_cast<uint32_t>(m_entityHandle);
    e.isActive = active;
    EventBus::GetInstance().Publish(e);
    if (HasComponent<ECS::ActivityComponent>())
    {
        GetComponent<ECS::ActivityComponent>().isActive = active;
    }
    else
    {
        AddComponent<ECS::ActivityComponent>(active);
    }
}

LayerMask RuntimeGameObject::GetLayers()
{
    if (HasComponent<ECS::LayerComponent>())
    {
        return GetComponent<ECS::LayerComponent>().layers;
    }
    return LayerMask::Only(0); // Default layer
}

void RuntimeGameObject::SetLayers(LayerMask layers)
{
    if (HasComponent<ECS::LayerComponent>())
    {
        GetComponent<ECS::LayerComponent>().layers = layers;
    }
    else
    {
        AddComponent<ECS::LayerComponent>(layers);
    }
}

uint32_t RuntimeGameObject::GetLayerMask()
{
    return GetLayers().value;
}

bool RuntimeGameObject::IsInLayer(int layer)
{
    return GetLayers().Contains(layer);
}

void RuntimeGameObject::SetInLayer(int layer, bool enabled)
{
    if (HasComponent<ECS::LayerComponent>())
    {
        GetComponent<ECS::LayerComponent>().layers.Set(layer, enabled);
    }
    else
    {
        LayerMask mask = LayerMask::Only(0);
        mask.Set(layer, enabled);
        AddComponent<ECS::LayerComponent>(mask);
    }
}

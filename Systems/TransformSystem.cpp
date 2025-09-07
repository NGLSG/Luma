#include "TransformSystem.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Components/Transform.h"
#include "../Components/RelationshipComponent.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Systems
{
    void TransformSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
    }

    void TransformSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();


        auto rootView = registry.view<ECS::TransformComponent>(entt::exclude<ECS::ParentComponent>);


        for (auto entity : rootView)
        {
            if (!scene->FindGameObjectByEntity(entity).IsActive())
                continue;
            UpdateWorldTransform(entity, registry);
        }
    }

    void TransformSystem::UpdateWorldTransform(entt::entity entity, entt::registry& registry)
    {
        auto& transform = registry.get<ECS::TransformComponent>(entity);
        glm::mat4 worldMatrix(1.0f);

        if (registry.all_of<ECS::ParentComponent>(entity))
        {
            auto& parentComponent = registry.get<ECS::ParentComponent>(entity);
            if (registry.valid(parentComponent.parent))
            {
                auto& parentTransform = registry.get<ECS::TransformComponent>(parentComponent.parent);


                glm::mat4 parentWorldMatrix = glm::translate(glm::mat4(1.0f),
                                                             glm::vec3(parentTransform.position.x,
                                                                       parentTransform.position.y, 0.0f));
                parentWorldMatrix = glm::rotate(parentWorldMatrix, parentTransform.rotation,
                                                glm::vec3(0.0f, 0.0f, 1.0f));
                parentWorldMatrix = glm::scale(parentWorldMatrix,
                                               glm::vec3(parentTransform.scale.x, parentTransform.scale.y, 1.0f));


                glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f),
                                                       glm::vec3(transform.localPosition.x, transform.localPosition.y,
                                                                 0.0f));
                localMatrix = glm::rotate(localMatrix, transform.localRotation, glm::vec3(0.0f, 0.0f, 1.0f));
                localMatrix = glm::scale(localMatrix, glm::vec3(transform.localScale.x, transform.localScale.y, 1.0f));


                worldMatrix = parentWorldMatrix * localMatrix;
            }
        }
        else
        {
        }


        if (registry.all_of<ECS::ParentComponent>(entity))
        {
            glm::vec3 scaleVec, translationVec, skew;
            glm::vec4 perspective;
            glm::quat rotationQuat;
            glm::decompose(worldMatrix, scaleVec, rotationQuat, translationVec, skew, perspective);
            auto& parentComponent = registry.get<ECS::ParentComponent>(entity);
            auto& parentTransform = registry.get<ECS::TransformComponent>(parentComponent.parent);
            transform.position = {
                transform.localPosition.x + parentTransform.position.x,
                transform.localPosition.y + parentTransform.position.y
            };
            transform.scale = {scaleVec.x, scaleVec.y};
            transform.rotation = glm::eulerAngles(rotationQuat).z;
        }


        if (registry.all_of<ECS::ChildrenComponent>(entity))
        {
            auto& childrenComponent = registry.get<ECS::ChildrenComponent>(entity);
            for (auto child : childrenComponent.children)
            {
                UpdateWorldTransform(child, registry);
            }
        }
    }
}

#include "TransformSystem.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Components/Transform.h"
#include "../Components/RelationshipComponent.h"
#include "../Utils/Logger.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_set>

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

    namespace
    {
        static void UpdateWorldTransformImpl(entt::entity entity, entt::registry& registry,
                                             int depth,
                                             std::unordered_set<entt::entity>& visiting)
        {
            if (!registry.valid(entity)) return;
            if (!registry.any_of<ECS::TransformComponent>(entity)) return;

            if (!visiting.insert(entity).second)
            {
                
                return;
            }

            if (depth > 1024)
            {
                LogError("TransformSystem: recursion depth exceeded at entity {}. Possible cyclic hierarchy.",
                         static_cast<uint32_t>(entity));
                visiting.erase(entity);
                return;
            }

            auto& transform = registry.get<ECS::TransformComponent>(entity);
            glm::mat4 worldMatrix(1.0f);

            if (registry.all_of<ECS::ParentComponent>(entity))
            {
                auto& parentComponent = registry.get<ECS::ParentComponent>(entity);
                entt::entity parent = parentComponent.parent;
                if (parent == entity)
                {
                    LogWarn("TransformSystem: entity {} has itself as parent. Ignoring parent.",
                            static_cast<uint32_t>(entity));
                }
                else if (registry.valid(parent) && registry.any_of<ECS::TransformComponent>(parent))
                {
                    auto& parentTransform = registry.get<ECS::TransformComponent>(parent);

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

                    glm::vec3 scaleVec, translationVec, skew;
                    glm::vec4 perspective;
                    glm::quat rotationQuat;
                    glm::decompose(worldMatrix, scaleVec, rotationQuat, translationVec, skew, perspective);

                    
                    transform.position = {
                        transform.localPosition.x + parentTransform.position.x,
                        transform.localPosition.y + parentTransform.position.y
                    };
                    transform.scale = {scaleVec.x, scaleVec.y};
                    transform.rotation = glm::eulerAngles(rotationQuat).z;
                }
            }

            if (registry.all_of<ECS::ChildrenComponent>(entity))
            {
                auto& childrenComponent = registry.get<ECS::ChildrenComponent>(entity);
                for (auto child : childrenComponent.children)
                {
                    if (child == entity) { LogWarn("TransformSystem: child equals parent for entity {}.",
                                                   static_cast<uint32_t>(entity)); continue; }
                    if (!registry.valid(child)) continue;
                    UpdateWorldTransformImpl(child, registry, depth + 1, visiting);
                }
            }

            visiting.erase(entity);
        }
    }

    void TransformSystem::UpdateWorldTransform(entt::entity entity, entt::registry& registry)
    {
        std::unordered_set<entt::entity> visiting;
        UpdateWorldTransformImpl(entity, registry, 0, visiting);
    }
}

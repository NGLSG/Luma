/**
 * @file InputTextSystem.h
 * @brief 声明了InputTextSystem类，用于处理输入文本组件的系统。
 */

#ifndef INPUTTEXTSYSTEM_H
#define INPUTTEXTSYSTEM_H

#include "ISystem.h"
#include "entt/entt.hpp"
#include <string>
#include <vector>

// 前向声明
class RuntimeScene;
struct EngineContext;

namespace ECS
{
    // 前向声明
    struct InputTextComponent;
    struct SerializableEventTarget;
}

namespace Systems
{
    /**
     * @brief 输入文本系统，负责处理场景中的输入文本组件。
     *
     * 该系统继承自ISystem，提供了创建、更新和销毁的生命周期方法，
     * 并处理输入文本组件的焦点事件和文本输入逻辑。
     */
    class InputTextSystem : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;
        void OnDestroy(RuntimeScene* scene) override;

    private:
        void OnFocusGained(RuntimeScene* scene, entt::entity entity, EngineContext& context);
        void OnFocusLost(RuntimeScene* scene, entt::entity entity, EngineContext& context);
        void HandleActiveInput(RuntimeScene* scene, entt::entity entity, EngineContext& context);

        void InvokeTextChangedEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets,
                                    const std::string& newText);
        void InvokeSubmitEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets,
                               const std::string& text);

        entt::entity focusedEntity = entt::null;
        EngineContext* ctx = nullptr;
    };
}
#endif

/**
 * @file InputTextSystem.h
 * @brief 声明了InputTextSystem类，用于处理输入文本组件的系统。
 */

#pragma once

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
    class InputTextSystem final : public ISystem
    {
    public:
        /**
         * @brief 系统创建时调用，用于初始化。
         * @param scene 指向当前运行时场景的指针。
         * @param context 引擎上下文引用。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 系统每帧更新时调用。
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 距离上一帧的时间间隔。
         * @param context 引擎上下文引用。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

        /**
         * @brief 系统销毁时调用，用于清理资源。
         * @param scene 指向当前运行时场景的指针。
         */
        void OnDestroy(RuntimeScene* scene) override;

    private:
        /**
         * @brief 当实体获得焦点时调用。
         * @param scene 指向当前运行时场景的指针。
         * @param entity 获得焦点的实体ID。
         * @param context 引擎上下文引用。
         */
        void OnFocusGained(RuntimeScene* scene, entt::entity entity, EngineContext& context);

        /**
         * @brief 当实体失去焦点时调用。
         * @param scene 指向当前运行时场景的指针。
         * @param entity 失去焦点的实体ID。
         * @param context 引擎上下文引用。
         */
        void OnFocusLost(RuntimeScene* scene, entt::entity entity, EngineContext& context);

        /**
         * @brief 处理当前激活的输入。
         * @param scene 指向当前运行时场景的指针。
         * @param entity 正在处理输入的实体ID。
         * @param context 引擎上下文引用。
         */
        void handleActiveInput(RuntimeScene* scene, entt::entity entity, EngineContext& context);

        /**
         * @brief 触发文本改变事件。
         * @param scene 指向当前运行时场景的指针。
         * @param targets 事件的目标列表。
         * @param newText 改变后的新文本。
         */
        void invokeTextChangedEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets,
                                    const std::string& newText);

        /**
         * @brief 触发提交事件。
         * @param scene 指向当前运行时场景的指针。
         * @param targets 事件的目标列表。
         * @param text 提交的文本。
         */
        void invokeSubmitEvent(RuntimeScene* scene, const std::vector<ECS::SerializableEventTarget>& targets,
                               const std::string& text);

        entt::entity focusedEntity; ///< 当前获得焦点的实体ID。
    };
}
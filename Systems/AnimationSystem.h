#ifndef LUMAENGINE_ANIMATIONSYSTEMS_H
#define LUMAENGINE_ANIMATIONSYSTEMS_H
#include "ISystem.h"
#include "RuntimeAsset/RuntimeAnimationController.h"
#include "entt/entt.hpp"

namespace Systems
{
    /**
     * @brief 动画系统，负责管理和更新场景中的动画。
     *
     * 该系统继承自ISystem接口，提供了动画生命周期管理和每帧更新功能。
     */
    class AnimationSystem : public ISystem
    {
    private:

    public:
        /**
         * @brief 在系统创建时调用，用于初始化动画系统。
         *
         * 此方法在动画系统首次加载或场景初始化时被调用，可以进行资源加载或状态设置。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param engineCtx 引擎上下文，提供引擎核心服务和数据。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 每帧更新时调用，用于处理和更新场景中的动画状态。
         *
         * 此方法根据deltaTime更新所有活动动画的播放进度和状态。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 自上一帧以来的时间间隔（秒）。
         * @param engineCtx 引擎上下文，提供引擎核心服务和数据。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;
    };
}


#endif
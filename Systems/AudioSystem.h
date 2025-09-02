#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Data/EngineContext.h"

namespace Systems
{
    /**
     * @brief 音频系统。
     *
     * 负责管理和处理游戏中的音频相关功能。
     */
    class AudioSystem : public ISystem
    {
    public:
        /**
         * @brief 在系统创建时调用。
         *
         * 用于初始化音频系统及其相关资源。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param context 引擎上下文，提供对核心引擎数据的访问。
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 每帧更新时调用。
         *
         * 用于处理音频系统的运行时逻辑，例如播放、暂停、音量调整等。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 自上一帧以来的时间间隔（秒）。
         * @param context 引擎上下文，提供对核心引擎数据的访问。
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

        /**
         * @brief 在系统销毁时调用。
         *
         * 用于清理音频系统占用的资源。
         *
         * @param scene 指向当前运行时场景的指针。
         */
        void OnDestroy(RuntimeScene* scene) override;
    };
}

#endif
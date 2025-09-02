#ifndef ISYSTEM_H
#define ISYSTEM_H
#include "../Data/EngineContext.h"
class RuntimeScene;

namespace Systems
{
    /**
     * @brief 系统接口基类。
     *
     * 定义了所有系统（System）必须实现的基本生命周期方法。
     */
    class ISystem
    {
    public:
        /**
         * @brief 虚析构函数。
         *
         * 确保派生类析构函数能够正确调用。
         */
        virtual ~ISystem() = default;

        /**
         * @brief 系统创建时调用。
         *
         * 用于初始化系统所需的资源或状态。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param engineCtx 引擎上下文，提供对引擎核心数据的访问。
         */
        virtual void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) =0;

        /**
         * @brief 系统每帧更新时调用。
         *
         * 用于处理系统逻辑，例如更新状态、处理输入等。
         *
         * @param scene 指向当前运行时场景的指针。
         * @param deltaTime 自上一帧以来的时间间隔（秒）。
         * @param engineCtx 引擎上下文，提供对引擎核心数据的访问。
         */
        virtual void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) = 0;

        /**
         * @brief 系统销毁时调用。
         *
         * 用于清理系统占用的资源。
         *
         * @param scene 指向当前运行时场景的指针。
         */
        virtual void OnDestroy(RuntimeScene* scene)
        {
        }
    };
}
#endif
#ifndef INTERACTIONSYSTEM_H
#define INTERACTIONSYSTEM_H
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include <unordered_map>

#include "Camera.h"
#include "ISystem.h"

struct EngineContext;
struct InputState;
class RuntimeScene;

namespace Systems
{
    /**
     * @brief 交互系统，处理用户与游戏对象的交互（鼠标和触摸）
     */
    class InteractionSystem : public ISystem
    {
    public:
        /**
         * @brief 系统创建时调用
         * @param scene 运行时场景指针
         * @param context 引擎上下文引用
         */
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;

        /**
         * @brief 系统更新时调用
         * @param scene 运行时场景指针
         * @param deltaTime 帧时间间隔
         * @param context 引擎上下文引用
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        /**
         * @brief 触摸点信息结构
         */
        struct TouchPointInfo
        {
            ECS::Vector2f position; ///< 触摸点位置（世界坐标）
            entt::entity hoveredEntity; ///< 当前悬停的实体
            entt::entity pressedEntity; ///< 当前按下的实体
        };

        /**
         * @brief 执行几何拾取，找到在指定世界坐标下的最顶层实体
         * @param registry ECS注册表引用
         * @param worldMousePos 世界坐标下的鼠标位置
         * @return 被拾取到的实体，如果没有则返回entt::null
         */
        entt::entity performGeometricPicking(entt::registry& registry, const ECS::Vector2f& worldMousePos);

        /**
         * @brief 处理悬停事件（PointerEnter/Exit）
         * @param registry ECS注册表引用
         * @param currentHoveredEntity 当前悬停的实体
         * @param previousHoveredEntity 之前悬停的实体
         */
        void handleHoverEvents(entt::registry& registry, entt::entity currentHoveredEntity,
                               entt::entity previousHoveredEntity);

        /**
         * @brief 处理点击事件（PointerDown/Up/Click）- 鼠标模式
         * @param registry ECS注册表引用
         * @param input 输入状态引用
         */
        void handleMouseClickEvents(entt::registry& registry, const InputState& input);

        /**
         * @brief 处理多点触摸事件
         * @param scene 运行时场景指针
         * @param context 引擎上下文引用
         */
        void handleMultiTouchEvents(RuntimeScene* scene, EngineContext& context);

        /**
         * @brief 将屏幕坐标转换为世界坐标（居中模式）
         * @param globalScreenPos 全局屏幕坐标
         * @param cameraProps 相机属性
         * @param viewport 视口矩形
         * @return 世界坐标
         */
        static ECS::Vector2f screenToWorld(const ECS::Vector2f& globalScreenPos,
                                           const Camera::CamProperties& cameraProps,
                                           const ECS::RectF& viewport);

    public:
        void OnDestroy(RuntimeScene* scene) override
        {
        }

    private:
        // 鼠标交互状态
        entt::entity m_hoveredEntity = entt::null; ///< 当前悬停的实体（鼠标）
        entt::entity m_pressedEntity = entt::null; ///< 当前按下的实体（鼠标）
        bool m_wasLeftMouseDownLastFrame = false; ///< 上一帧鼠标左键是否按下

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        // 触摸交互状态
        std::unordered_map<SDL_FingerID, TouchPointInfo> m_touchPoints; ///< 所有活动触摸点的信息
#endif
    };
}
#endif

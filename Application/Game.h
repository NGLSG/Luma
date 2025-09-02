#ifndef LUMAENGINEPROJECT_GAME_H
#define LUMAENGINEPROJECT_GAME_H
#include "ApplicationBase.h"
#include "SceneRenderer.h"

/**
 * @brief 游戏类，继承自应用基类，代表了应用程序的核心游戏逻辑。
 * @details 此类是Luma引擎项目的核心游戏实现，负责初始化、更新和渲染游戏内容。
 */
class LUMA_API Game final : public ApplicationBase
{
public:
    /**
     * @brief 构造函数，初始化游戏实例。
     * @param config 应用程序配置。
     */
    Game(ApplicationConfig config);

    /**
     * @brief 析构函数，清理游戏资源。
     */
    ~Game() override = default;

    /**
     * @brief 禁用拷贝构造函数，防止游戏实例被拷贝。
     */
    Game(const Game&) = delete;

    /**
     * @brief 禁用拷贝赋值操作符，防止游戏实例被赋值。
     * @return 对当前对象的引用。
     */
    Game& operator=(const Game&) = delete;

protected:
    /**
     * @brief 初始化派生类的特定资源。
     * @details 此方法在应用程序初始化阶段被调用，用于设置游戏特有的组件。
     */
    void InitializeDerived() override;

    /**
     * @brief 关闭并清理派生类的特定资源。
     * @details 此方法在应用程序关闭阶段被调用，用于释放游戏特有的资源。
     */
    void ShutdownDerived() override;

    /**
     * @brief 更新游戏逻辑。
     * @details 此方法在每一帧被调用，用于处理游戏状态、物理、AI等逻辑。
     * @param deltaTime 自上一帧以来的时间间隔（秒）。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 渲染游戏场景。
     * @details 此方法在每一帧被调用，用于绘制游戏中的所有可见元素。
     */
    void Render() override;

private:
    std::unique_ptr<SceneRenderer> m_sceneRenderer; ///< 场景渲染器，负责管理和绘制游戏场景。
};


#endif
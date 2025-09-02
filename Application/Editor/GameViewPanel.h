#ifndef GAMEVIEWPANEL_H
#define GAMEVIEWPANEL_H

#include "IEditorPanel.h"
#include "../Renderer/RenderTarget.h"

/**
 * @brief 游戏视图面板，用于在编辑器中显示游戏画面。
 *
 * 该面板继承自IEditorPanel，负责初始化、更新、绘制和关闭游戏视图。
 */
class GameViewPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    GameViewPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~GameViewPanel() override = default;

    /**
     * @brief 初始化游戏视图面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新游戏视图面板的逻辑。
     * @param deltaTime 帧之间的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制游戏视图面板的内容。
     */
    void Draw() override;
    /**
     * @brief 关闭游戏视图面板并释放资源。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板的名称。
     * @return 面板名称的C风格字符串。
     */
    const char* GetPanelName() const override { return "游戏视图"; }

private:
    std::shared_ptr<RenderTarget> m_gameViewTarget; ///< 游戏视图的目标渲染器。
};

#endif
#ifndef IEDITORPANEL_H
#define IEDITORPANEL_H

#include "EditorContext.h"

/**
 *  @brief 编辑器面板接口。
 *
 *  定义了所有编辑器面板应实现的基本功能。
 */
class IEditorPanel
{
public:
    /**
     *  @brief 虚析构函数，确保派生类资源正确释放。
     */
    virtual ~IEditorPanel() = default;

    /**
     *  @brief 初始化面板。
     *  @param context 编辑器上下文指针。
     */
    virtual void Initialize(EditorContext* context) = 0;

    /**
     *  @brief 更新面板状态。
     *  @param deltaTime 自上一帧以来的时间间隔。
     */
    virtual void Update(float deltaTime) = 0;

    /**
     *  @brief 绘制面板的用户界面。
     */
    virtual void Draw() = 0;

    /**
     *  @brief 关闭面板，释放相关资源。
     */
    virtual void Shutdown() = 0;

    /**
     *  @brief 获取面板的名称。
     *  @return 面板名称的C字符串。
     */
    virtual const char* GetPanelName() const = 0;

    /**
     *  @brief 检查面板是否可见。
     *  @return 如果面板可见则返回 true，否则返回 false。
     */
    virtual bool IsVisible() const { return m_isVisible; }

    /**
     *  @brief 设置面板的可见性。
     *  @param visible 如果为 true 则使面板可见，否则使其隐藏。
     */
    virtual void SetVisible(bool visible) { m_isVisible = visible; }

    /**
     *  @brief 使面板获得焦点。
     */
    virtual void Focus()
    {
    }

    virtual bool IsFocused() const
    {
        return m_isFocused;
    }

protected:
    bool m_isFocused = false; ///< 面板的焦点状态。
    EditorContext* m_context = nullptr; ///< 编辑器上下文指针。
    bool m_isVisible = true; ///< 面板的可见性状态。
};

#endif

#ifndef IEDITORPANEL_H
#define IEDITORPANEL_H
#include "EditorContext.h"
class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;
    virtual void Initialize(EditorContext* context) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetPanelName() const = 0;
    virtual bool IsVisible() const { return m_isVisible; }
    virtual void SetVisible(bool visible) { m_isVisible = visible; }
    virtual void Focus()
    {
    }
    virtual bool IsFocused() const
    {
        return m_isFocused;
    }
protected:
    bool m_isFocused = false; 
    EditorContext* m_context = nullptr; 
    bool m_isVisible = true; 
};
#endif

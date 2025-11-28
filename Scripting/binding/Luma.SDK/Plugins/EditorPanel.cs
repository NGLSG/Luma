using System;

namespace Luma.SDK.Plugins;




public abstract class EditorPanel
{
    
    
    
    public abstract string Title { get; }

    
    
    
    public bool IsVisible { get; set; } = true;

    
    
    
    public virtual bool IsCloseable => true;

    
    
    
    public EditorPlugin? Plugin { get; internal set; }

    
    
    
    public virtual void OnCreate() { }

    
    
    
    public virtual void OnDestroy() { }

    
    
    
    public virtual void OnFocus() { }

    
    
    
    public virtual void OnLostFocus() { }

    
    
    
    public abstract void OnGUI();

    
    
    
    
    public virtual void OnUpdate(float deltaTime) { }
}

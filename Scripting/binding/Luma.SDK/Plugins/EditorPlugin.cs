using System;
using System.Collections.Generic;

namespace Luma.SDK.Plugins;




public abstract class EditorPlugin : PluginBase
{
    private readonly List<EditorPanel> _panels = new();

    
    
    
    public IReadOnlyList<EditorPanel> Panels => _panels;

    
    
    
    
    
    protected T RegisterPanel<T>() where T : EditorPanel, new()
    {
        var panel = new T();
        panel.Plugin = this;
        _panels.Add(panel);
        return panel;
    }

    
    
    
    
    protected void RegisterPanel(EditorPanel panel)
    {
        panel.Plugin = this;
        _panels.Add(panel);
    }

    
    
    
    
    public virtual void OnEditorUpdate(float deltaTime) { }

    
    
    
    public virtual void OnEditorGUI() { }

    
    
    
    public virtual void OnMenuBarGUI() { }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    public virtual void OnDrawMenuItems(string menuName) { }

    public override void OnUnload()
    {
        base.OnUnload();
        foreach (var panel in _panels)
        {
            panel.OnDestroy();
        }
        _panels.Clear();
    }
}

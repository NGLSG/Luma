using System;

namespace Luma.SDK.Plugins;




public abstract class PluginBase
{
    
    
    
    public abstract string Id { get; }

    
    
    
    public abstract string Name { get; }

    
    
    
    public virtual string Version => "1.0.0";

    
    
    
    public virtual string Author => "Unknown";

    
    
    
    public virtual string Description => "";

    
    
    
    public bool Enabled { get; internal set; } = true;

    
    
    
    public bool Loaded { get; internal set; } = false;

    
    
    
    public virtual void OnLoad() { }

    
    
    
    public virtual void OnUnload() { }

    
    
    
    public virtual void OnEnable() { }

    
    
    
    public virtual void OnDisable() { }
}
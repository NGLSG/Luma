using Luma.SDK.Components;

namespace Luma.SDK;




public abstract class Component : IComponent
{
    
    
    
    public Entity Self { get; internal set; }
}
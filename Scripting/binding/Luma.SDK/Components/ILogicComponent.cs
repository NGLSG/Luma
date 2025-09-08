using Luma.SDK;
using Luma.SDK.Components;




public interface ILogicComponent
{
    
    
    
    Entity Entity { get; set; }
}





public class LogicComponent<T> : ILogicComponent where T : struct, IComponent
{
    
    
    
    
    public T Component
    {
        get => _component;
        
        private set
        {
            _component = value;
            Entity.SetComponentData(value);
        }
    }

    public LogicComponent()
    {
        Entity = Entity.Invalid;
    }

    private T _component;
    public Entity Entity { get; set; }

    public LogicComponent(Entity entity)
    {
        Entity = entity;
        _component = entity.GetComponentData<T>();
    }

    
    
    
    
    
    protected void UpdateComponent(Func<T, T> modifier)
    {
        
        T modifiedComponent = modifier(_component);

        
        Component = modifiedComponent;
    }
}
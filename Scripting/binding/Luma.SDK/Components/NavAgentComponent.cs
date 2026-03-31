using System;

namespace Luma.SDK.Components;

public class NavAgent : ILogicComponent
{
    private const string ComponentName = "NavAgentComponent";

    public Entity Entity { get; set; }

    public NavAgent(Entity entity)
    {
        Entity = entity;
    }

    public NavAgent()
    {
        Entity = Entity.Invalid;
    }

    public Vector2 Destination
    {
        get
        {
            Native.NavAgent_GetDestination(Entity.ScenePtr, Entity.Id, out float x, out float y);
            return new Vector2 { X = x, Y = y };
        }
        set => Native.NavAgent_SetDestination(Entity.ScenePtr, Entity.Id, value.X, value.Y);
    }

    public float Speed
    {
        get => Native.NavAgent_GetSpeed(Entity.ScenePtr, Entity.Id);
        set => Native.NavAgent_SetSpeed(Entity.ScenePtr, Entity.Id, value);
    }

    public bool HasArrived => Native.NavAgent_HasArrived(Entity.ScenePtr, Entity.Id);

    public void SetDestination(Vector2 target)
    {
        Native.NavAgent_SetDestination(Entity.ScenePtr, Entity.Id, target.X, target.Y);
    }

    public void Stop()
    {
        Native.NavAgent_Stop(Entity.ScenePtr, Entity.Id);
    }
}

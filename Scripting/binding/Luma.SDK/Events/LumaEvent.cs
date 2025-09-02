using System;
using YamlDotNet.Serialization;

namespace Luma.SDK;

public class LumaEvent
{
    protected readonly Entity Entity;
    protected readonly string EventName;

    public LumaEvent(Entity entity, string eventName)
    {
        Entity = entity;
        EventName = eventName;
    }

    public void Invoke()
    {
        Entity.InvokeEvent(EventName);
    }
}




public class LumaEvent<T1> : LumaEvent
{
    public LumaEvent(Entity entity, string eventName) : base(entity, eventName)
    {
    }

    public void Invoke(T1 arg1)
    {
        Entity.InvokeEvent(EventName, arg1);
    }
    
}




public class LumaEvent<T1, T2> : LumaEvent
{
    private static readonly ISerializer s_serializer = new SerializerBuilder().Build();

    public LumaEvent(Entity entity, string eventName) : base(entity, eventName)
    {
    }

    public void Invoke(T1 arg1, T2 arg2)
    {
        Entity.InvokeEvent(EventName, arg1, arg2);
    }
}
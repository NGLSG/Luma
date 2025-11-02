using System;
using System.Reflection;

namespace Luma.SDK;


public sealed class SerializedEventBinding
{
    private readonly Entity _owner;
    private readonly Action<Entity> _clear;
    private readonly Func<Entity, int> _getCount;
    private readonly Func<Entity, int, (Guid guid, string componentName, string methodName)> _getAt;
    private readonly Action<Entity, Guid, string, string> _addByGuid; 

    public SerializedEventBinding(Entity owner,
        Func<Entity, int> getCount,
        Func<Entity, int, (Guid guid, string componentName, string methodName)> getAt,
        Action<Entity> clear,
        Action<Entity, Guid, string, string> addByGuid)
    {
        _owner = owner;
        _getCount = getCount;
        _getAt = getAt;
        _clear = clear;
        _addByGuid = addByGuid;
    }

    public void Add(Action subscriber)
    {
        if (subscriber == null) return;
        var targetObj = subscriber.Target;
        var method = subscriber.Method;
        if (targetObj is not Script script)
        {
            Debug.LogWarning("SerializedEventBinding.Add 仅支持绑定到 Script 实例的公开实例方法。请使用方法组形式，例如: Add(MyMethod)");
            return;
        }

        var compName = script.GetType().Name;
        var guid = script.Self.GetComponentProperty<Guid>("IDComponent", "guid");
        _addByGuid(_owner, guid, compName, method.Name);
    }

    public void Clear() => _clear(_owner);

    public void Remove(Action subscriber)
    {
        if (subscriber == null) return;
        var targetObj = subscriber.Target;
        var method = subscriber.Method;
        if (targetObj is not Script script)
        {
            Debug.LogWarning("SerializedEventBinding.Remove 仅支持解绑 Script 实例的方法");
            return;
        }

        var compName = script.GetType().Name;
        var guid = script.Self.GetComponentProperty<Guid>("IDComponent", "guid");
        var methodName = method.Name;

        int count = _getCount(_owner);
        Span<(Guid guid, string componentName, string methodName)> buf = count <= 0 ? Span<(Guid,string,string)>.Empty : new (Guid,string,string)[count];
        for (int i = 0; i < count; i++) buf[i] = _getAt(_owner, i);

        _clear(_owner);
        for (int i = 0; i < buf.Length; i++)
        {
            var it = buf[i];
            if (it.guid.Equals(guid) && string.Equals(it.methodName, methodName, StringComparison.Ordinal) &&
                string.Equals(it.componentName, compName, StringComparison.Ordinal))
            {
                continue; 
            }
            _addByGuid(_owner, it.guid, it.componentName, it.methodName);
        }
    }
}

public sealed class SerializedEventBinding<T1>
{
    private readonly Entity _owner;
    private readonly Action<Entity> _clear;
    private readonly Func<Entity, int> _getCount;
    private readonly Func<Entity, int, (Guid guid, string componentName, string methodName)> _getAt;
    private readonly Action<Entity, Guid, string, string> _addByGuid; 

    public SerializedEventBinding(Entity owner,
        Func<Entity, int> getCount,
        Func<Entity, int, (Guid guid, string componentName, string methodName)> getAt,
        Action<Entity> clear,
        Action<Entity, Guid, string, string> addByGuid)
    {
        _owner = owner;
        _getCount = getCount;
        _getAt = getAt;
        _clear = clear;
        _addByGuid = addByGuid;
    }

    public void Add(Action<T1> subscriber)
    {
        if (subscriber == null) return;
        var targetObj = subscriber.Target;
        var method = subscriber.Method;
        if (targetObj is not Script script)
        {
            Debug.LogWarning("SerializedEventBinding<T>.Add 仅支持绑定到 Script 实例的公开实例方法。请使用方法组形式，例如: Add(OnTextChanged)");
            return;
        }

        var compName = script.GetType().Name;
        var guid = script.Self.GetComponentProperty<Guid>("IDComponent", "guid");
        _addByGuid(_owner, guid, compName, method.Name);
    }

    public void Clear() => _clear(_owner);

    public void Remove(Action<T1> subscriber)
    {
        if (subscriber == null) return;
        var targetObj = subscriber.Target;
        var method = subscriber.Method;
        if (targetObj is not Script script)
        {
            Debug.LogWarning("SerializedEventBinding<T>.Remove 仅支持解绑 Script 实例的方法");
            return;
        }

        var compName = script.GetType().Name;
        var guid = script.Self.GetComponentProperty<Guid>("IDComponent", "guid");
        var methodName = method.Name;

        int count = _getCount(_owner);
        Span<(Guid guid, string componentName, string methodName)> buf = count <= 0 ? Span<(Guid,string,string)>.Empty : new (Guid,string,string)[count];
        for (int i = 0; i < count; i++) buf[i] = _getAt(_owner, i);

        _clear(_owner);
        for (int i = 0; i < buf.Length; i++)
        {
            var it = buf[i];
            if (it.guid.Equals(guid) && string.Equals(it.methodName, methodName, StringComparison.Ordinal) &&
                string.Equals(it.componentName, compName, StringComparison.Ordinal))
            {
                continue;
            }
            _addByGuid(_owner, it.guid, it.componentName, it.methodName);
        }
    }
}

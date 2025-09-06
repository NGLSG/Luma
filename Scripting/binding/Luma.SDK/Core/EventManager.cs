using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace Luma.SDK;

public class Event
{
}

public struct EventHandle
{
    public ulong Id;
    public bool IsValid => Id != 0;
    public static EventHandle Invalid => new() { Id = 0 };
}

public static class EventManager
{
    private interface IListenerCollection { }

    private class ListenerCollection<T> : IListenerCollection where T : Event
    {
        public readonly ConcurrentDictionary<ulong, Action<T>> Listeners = new();
    }

    private static readonly ConcurrentDictionary<Type, IListenerCollection> _eventListeners = new();
    
    private static readonly ConcurrentDictionary<ulong, Action> _unsubscribeActions = new();

    private static long _nextEventId = 1;

    public static EventHandle Subscribe<T>(Action<T> listener) where T : Event
    {
        if (listener == null) throw new ArgumentNullException(nameof(listener));

        var collection = (ListenerCollection<T>)_eventListeners.GetOrAdd(typeof(T), _ => new ListenerCollection<T>());
        var handle = new EventHandle { Id = (ulong)Interlocked.Increment(ref _nextEventId) };

        collection.Listeners[handle.Id] = listener;

        _unsubscribeActions[handle.Id] = () => collection.Listeners.TryRemove(handle.Id, out _);

        return handle;
    }
    
    public static void Unsubscribe(EventHandle handle)
    {
        if (!handle.IsValid) return;

        
        if (_unsubscribeActions.TryRemove(handle.Id, out var unsubscribeAction))
        {
            unsubscribeAction.Invoke();
        }
    }
    
    
    public static void Publish<T>(T evt) where T : Event
    {
        if (evt == null) throw new ArgumentNullException(nameof(evt));

        if (_eventListeners.TryGetValue(typeof(T), out var collection))
        {
            
            var listeners = ((ListenerCollection<T>)collection).Listeners.Values;
            foreach (var listener in listeners)
            {
                try
                {
                    listener.Invoke(evt);
                }
                catch (Exception e)
                {
                    Debug.LogError($"EventManager 发布事件 {typeof(T).Name} 时调用监听器失败: {e.Message}");
                }
            }
        }
    }
}
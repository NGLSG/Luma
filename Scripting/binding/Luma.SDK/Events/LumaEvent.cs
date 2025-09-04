using System;
using System.Collections.Generic;
using System.Reflection;
using YamlDotNet.Serialization;

namespace Luma.SDK;




public class LumaEvent
{
    private readonly List<Action> subscribers = new List<Action>();

    public void Add(Action callback)
    {
        if (callback != null && !subscribers.Contains(callback))
        {
            subscribers.Add(callback);
        }
    }

    public void Remove(Action callback)
    {
        if (callback != null)
        {
            subscribers.Remove(callback);
        }
    }

    public void Clear()
    {
        subscribers.Clear();
    }

    public void Invoke()
    {
        foreach (var callback in subscribers)
        {
            try
            {
                callback?.Invoke();
            }
            catch (Exception e)
            {
                Debug.LogError($"LumaEvent 调用失败: {e.Message}");
            }
        }
    }

    public int SubscriberCount => subscribers.Count;
}




public class LumaEvent<T1>
{
    private readonly List<Action<T1>> subscribers = new List<Action<T1>>();

    public void Add(Action<T1> callback)
    {
        if (callback != null && !subscribers.Contains(callback))
        {
            subscribers.Add(callback);
        }
    }

    public void Remove(Action<T1> callback)
    {
        if (callback != null)
        {
            subscribers.Remove(callback);
        }
    }

    public void Clear()
    {
        subscribers.Clear();
    }

    public void Invoke(T1 arg1)
    {
        foreach (var callback in subscribers)
        {
            try
            {
                callback?.Invoke(arg1);
            }
            catch (Exception e)
            {
                Debug.LogError($"LumaEvent<{typeof(T1).Name}> 调用失败: {e.Message}");
            }
        }
    }

    public int SubscriberCount => subscribers.Count;
}




public class LumaEvent<T1, T2>
{
    private readonly List<Action<T1, T2>> subscribers = new List<Action<T1, T2>>();

    public void Add(Action<T1, T2> callback)
    {
        if (callback != null && !subscribers.Contains(callback))
        {
            subscribers.Add(callback);
        }
    }

    public void Remove(Action<T1, T2> callback)
    {
        if (callback != null)
        {
            subscribers.Remove(callback);
        }
    }

    public void Clear()
    {
        subscribers.Clear();
    }

    public void Invoke(T1 arg1, T2 arg2)
    {
        foreach (var callback in subscribers)
        {
            try
            {
                callback?.Invoke(arg1, arg2);
            }
            catch (Exception e)
            {
                Debug.LogError($"LumaEvent<{typeof(T1).Name}, {typeof(T2).Name}> 调用失败: {e.Message}");
            }
        }
    }

    public int SubscriberCount => subscribers.Count;
}
using System;
using System.Collections.Generic;
using Luma.SDK;

namespace Luma.SDK.Tweening;





public static class DOTween
{
    private static readonly List<Tween> _activeTweens = new();
    private static readonly List<Tween> _tweensToAdd = new();
    private static readonly List<Tween> _tweensToRemove = new();
    private static int _nextId = 1;
    private static bool _isUpdating;

    
    
    
    public static int ActiveTweenCount => _activeTweens.Count;

    
    
    
    
    public static void Update(float deltaTime)
    {
        
        if (_tweensToAdd.Count > 0)
        {
            _activeTweens.AddRange(_tweensToAdd);
            _tweensToAdd.Clear();
        }

        _isUpdating = true;
        
        foreach (var tween in _activeTweens)
        {
            if (!tween.IsKilled)
            {
                float dt = tween.IgnoreTimeScale ? deltaTime : deltaTime; 
                tween.Update(dt);
            }
        }
        
        _isUpdating = false;

        
        if (_tweensToRemove.Count > 0)
        {
            foreach (var tween in _tweensToRemove)
            {
                _activeTweens.Remove(tween);
            }
            _tweensToRemove.Clear();
        }
        
        _activeTweens.RemoveAll(t => t.IsKilled);
    }

    
    
    
    internal static void Add(Tween tween)
    {
        tween.Id = _nextId++;
        
        if (_isUpdating)
            _tweensToAdd.Add(tween);
        else
            _activeTweens.Add(tween);
    }

    
    
    
    internal static void Remove(Tween tween)
    {
        if (_isUpdating)
            _tweensToRemove.Add(tween);
        else
            _activeTweens.Remove(tween);
    }

    #region Factory Methods - Float

    
    
    
    
    
    
    
    public static Tween To(Func<float> getter, Action<float> setter, float endValue, float duration)
    {
        return new FloatTween(getter(), endValue, duration, setter).Play();
    }

    
    
    
    public static Tween To(float startValue, float endValue, float duration, Action<float> setter)
    {
        return new FloatTween(startValue, endValue, duration, setter).Play();
    }

    #endregion

    #region Factory Methods - Vector2

    
    
    
    public static Tween To(Func<Vector2> getter, Action<Vector2> setter, Vector2 endValue, float duration)
    {
        return new Vector2Tween(getter(), endValue, duration, setter).Play();
    }

    
    
    
    public static Tween To(Vector2 startValue, Vector2 endValue, float duration, Action<Vector2> setter)
    {
        return new Vector2Tween(startValue, endValue, duration, setter).Play();
    }

    #endregion

    #region Factory Methods - Vector2Int

    
    
    
    public static Tween To(Func<Vector2Int> getter, Action<Vector2Int> setter, Vector2Int endValue, float duration)
    {
        return new Vector2IntTween(getter(), endValue, duration, setter).Play();
    }

    
    
    
    public static Tween To(Vector2Int startValue, Vector2Int endValue, float duration, Action<Vector2Int> setter)
    {
        return new Vector2IntTween(startValue, endValue, duration, setter).Play();
    }

    #endregion

    #region Factory Methods - Color

    
    
    
    public static Tween To(Func<Color> getter, Action<Color> setter, Color endValue, float duration)
    {
        return new ColorTween(getter(), endValue, duration, setter).Play();
    }

    
    
    
    public static Tween To(Color startValue, Color endValue, float duration, Action<Color> setter)
    {
        return new ColorTween(startValue, endValue, duration, setter).Play();
    }

    #endregion

    #region Utility Methods

    
    
    
    public static Tween Delay(float duration)
    {
        return new DelayTween(duration).Play();
    }

    
    
    
    public static Tween Timer(float duration, Action<float>? onProgress = null)
    {
        return new CallbackTween(duration, onProgress).Play();
    }

    
    
    
    public static void KillAll()
    {
        foreach (var tween in _activeTweens)
        {
            tween.Kill();
        }
        _activeTweens.Clear();
        _tweensToAdd.Clear();
        _tweensToRemove.Clear();
    }

    
    
    
    public static void Kill(object target)
    {
        foreach (var tween in _activeTweens)
        {
            if (tween.Target == target)
                tween.Kill();
        }
    }

    
    
    
    public static void PauseAll()
    {
        foreach (var tween in _activeTweens)
        {
            tween.Pause();
        }
    }

    
    
    
    public static void ResumeAll()
    {
        foreach (var tween in _activeTweens)
        {
            tween.Resume();
        }
    }

    
    
    
    public static void CompleteAll()
    {
        foreach (var tween in _activeTweens)
        {
            tween.Complete();
        }
    }

    #endregion
}

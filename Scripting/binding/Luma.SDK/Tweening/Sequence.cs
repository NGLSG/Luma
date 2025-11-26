using System;
using System.Collections.Generic;

namespace Luma.SDK.Tweening;




public class Sequence : Tween
{
    private readonly List<SequenceItem> _items = new();
    private int _currentIndex;
    private float _currentItemElapsed;
    private bool _sequenceStarted;

    
    
    
    public static Sequence Create()
    {
        return new Sequence();
    }

    private Sequence()
    {
        AutoKill = true;
    }

    
    
    
    public Sequence Append(Tween tween)
    {
        
        tween.Pause();
        
        float startTime = GetSequenceDuration();
        _items.Add(new SequenceItem(tween, startTime, SequenceItemType.Append));
        Duration = startTime + tween.Duration + tween.Delay;
        return this;
    }

    
    
    
    public Sequence Join(Tween tween)
    {
        tween.Pause();
        
        float startTime = _items.Count > 0 ? _items[^1].StartTime : 0f;
        _items.Add(new SequenceItem(tween, startTime, SequenceItemType.Join));
        
        float endTime = startTime + tween.Duration + tween.Delay;
        if (endTime > Duration)
            Duration = endTime;
        
        return this;
    }

    
    
    
    public Sequence Insert(float atTime, Tween tween)
    {
        tween.Pause();
        
        _items.Add(new SequenceItem(tween, atTime, SequenceItemType.Insert));
        
        float endTime = atTime + tween.Duration + tween.Delay;
        if (endTime > Duration)
            Duration = endTime;
        
        return this;
    }

    
    
    
    public Sequence AppendCallback(Action callback)
    {
        float startTime = GetSequenceDuration();
        _items.Add(new SequenceItem(callback, startTime));
        return this;
    }

    
    
    
    public Sequence InsertCallback(float atTime, Action callback)
    {
        _items.Add(new SequenceItem(callback, atTime));
        return this;
    }

    
    
    
    public Sequence AppendInterval(float interval)
    {
        Duration += interval;
        return this;
    }

    
    
    
    public new Sequence Play()
    {
        
        _items.Sort((a, b) => a.StartTime.CompareTo(b.StartTime));
        
        base.Play();
        return this;
    }

    protected override void OnUpdate(float progress)
    {
        float currentTime = progress * Duration;

        foreach (var item in _items)
        {
            if (item.HasTriggered) continue;

            if (currentTime >= item.StartTime)
            {
                item.HasTriggered = true;
                
                if (item.Callback != null)
                {
                    item.Callback();
                }
                else if (item.Tween != null)
                {
                    item.Tween.Play();
                }
            }
        }

        
        foreach (var item in _items)
        {
            if (item.Tween != null && item.HasTriggered && !item.Tween.IsComplete)
            {
                float itemProgress = (currentTime - item.StartTime) / item.Tween.Duration;
                itemProgress = Math.Clamp(itemProgress, 0f, 1f);
                
                
            }
        }
    }

    protected override void OnRestart()
    {
        _currentIndex = 0;
        _currentItemElapsed = 0f;
        _sequenceStarted = false;
        
        foreach (var item in _items)
        {
            item.HasTriggered = false;
            item.Tween?.Restart();
            item.Tween?.Pause();
        }
    }

    private float GetSequenceDuration()
    {
        return Duration;
    }

    private class SequenceItem
    {
        public Tween? Tween { get; }
        public Action? Callback { get; }
        public float StartTime { get; }
        public SequenceItemType Type { get; }
        public bool HasTriggered { get; set; }

        public SequenceItem(Tween tween, float startTime, SequenceItemType type)
        {
            Tween = tween;
            StartTime = startTime;
            Type = type;
        }

        public SequenceItem(Action callback, float startTime)
        {
            Callback = callback;
            StartTime = startTime;
            Type = SequenceItemType.Callback;
        }
    }

    private enum SequenceItemType
    {
        Append,
        Join,
        Insert,
        Callback
    }
}

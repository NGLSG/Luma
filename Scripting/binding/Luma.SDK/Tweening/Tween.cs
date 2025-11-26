using System;

namespace Luma.SDK.Tweening;




public abstract class Tween
{
    
    public float Duration { get; protected set; }
    
    
    public float ElapsedTime { get; protected set; }
    
    
    public Ease EaseType { get; set; } = Ease.OutQuad;
    
    
    public bool IsPlaying { get; protected set; }
    
    
    public bool IsComplete { get; protected set; }
    
    
    public bool IsKilled { get; protected set; }
    
    
    public int Loops { get; set; } = 0;
    
    
    public int CurrentLoop { get; protected set; }
    
    
    public LoopType LoopType { get; set; } = LoopType.Restart;
    
    
    public float Delay { get; set; }
    
    
    public bool AutoKill { get; set; } = true;
    
    
    public bool IgnoreTimeScale { get; set; }
    
    
    public object? Target { get; set; }
    
    
    public int Id { get; internal set; }
    
    
    protected Action? _onStart;
    protected Action? _onUpdate;
    protected Action? _onComplete;
    protected Action? _onKill;
    protected Action<int>? _onLoopComplete;
    
    private float _delayElapsed;
    private bool _started;
    private bool _isYoyo;

    
    
    
    
    internal void Update(float deltaTime)
    {
        if (!IsPlaying || IsComplete || IsKilled) return;

        
        if (_delayElapsed < Delay)
        {
            _delayElapsed += deltaTime;
            return;
        }

        
        if (!_started)
        {
            _started = true;
            _onStart?.Invoke();
        }

        
        ElapsedTime += deltaTime;
        
        
        float progress = Duration > 0 ? Math.Clamp(ElapsedTime / Duration, 0f, 1f) : 1f;
        
        
        if (_isYoyo)
            progress = 1f - progress;
        
        
        float easedProgress = EaseFunc.Evaluate(EaseType, progress);
        
        
        OnUpdate(easedProgress);
        _onUpdate?.Invoke();

        
        if (ElapsedTime >= Duration)
        {
            
            if (Loops == -1 || CurrentLoop < Loops)
            {
                CurrentLoop++;
                ElapsedTime = 0f;
                
                if (LoopType == LoopType.Yoyo)
                    _isYoyo = !_isYoyo;
                else if (LoopType == LoopType.Incremental)
                    OnLoopIncrement();
                
                _onLoopComplete?.Invoke(CurrentLoop);
            }
            else
            {
                Complete();
            }
        }
    }

    
    
    
    public Tween Play()
    {
        IsPlaying = true;
        DOTween.Add(this);
        return this;
    }

    
    
    
    public Tween Pause()
    {
        IsPlaying = false;
        return this;
    }

    
    
    
    public Tween Resume()
    {
        IsPlaying = true;
        return this;
    }

    
    
    
    public Tween Restart()
    {
        ElapsedTime = 0f;
        _delayElapsed = 0f;
        CurrentLoop = 0;
        _started = false;
        _isYoyo = false;
        IsComplete = false;
        IsPlaying = true;
        OnRestart();
        return this;
    }

    
    
    
    public void Complete()
    {
        if (IsComplete || IsKilled) return;
        
        OnUpdate(1f);
        IsComplete = true;
        IsPlaying = false;
        _onComplete?.Invoke();
        
        if (AutoKill)
            Kill();
    }

    
    
    
    public void Kill()
    {
        if (IsKilled) return;
        
        IsKilled = true;
        IsPlaying = false;
        _onKill?.Invoke();
        DOTween.Remove(this);
    }

    
    
    
    public Tween SetEase(Ease ease)
    {
        EaseType = ease;
        return this;
    }

    
    
    
    
    
    public Tween SetLoops(int loops, LoopType loopType = LoopType.Restart)
    {
        Loops = loops;
        LoopType = loopType;
        return this;
    }

    
    
    
    public Tween SetDelay(float delay)
    {
        Delay = delay;
        return this;
    }

    
    
    
    public Tween SetAutoKill(bool autoKill)
    {
        AutoKill = autoKill;
        return this;
    }

    
    
    
    public Tween SetIgnoreTimeScale(bool ignore)
    {
        IgnoreTimeScale = ignore;
        return this;
    }

    
    
    
    public Tween SetTarget(object target)
    {
        Target = target;
        return this;
    }

    
    
    
    public Tween OnStart(Action callback)
    {
        _onStart = callback;
        return this;
    }

    
    
    
    public Tween OnUpdateCallback(Action callback)
    {
        _onUpdate = callback;
        return this;
    }

    
    
    
    public Tween OnComplete(Action callback)
    {
        _onComplete = callback;
        return this;
    }

    
    
    
    public Tween OnKill(Action callback)
    {
        _onKill = callback;
        return this;
    }

    
    
    
    public Tween OnLoopComplete(Action<int> callback)
    {
        _onLoopComplete = callback;
        return this;
    }

    
    
    
    protected abstract void OnUpdate(float progress);
    
    
    
    
    protected virtual void OnRestart() { }
    
    
    
    
    protected virtual void OnLoopIncrement() { }
}




public enum LoopType
{
    
    Restart,
    
    Yoyo,
    
    Incremental
}

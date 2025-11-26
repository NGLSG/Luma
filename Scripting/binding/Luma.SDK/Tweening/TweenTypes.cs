using System;
using Luma.SDK;

namespace Luma.SDK.Tweening;




public class FloatTween : Tween
{
    private float _startValue;
    private float _endValue;
    private readonly float _originalStart;
    private readonly float _originalEnd;
    private readonly Action<float> _setter;

    public FloatTween(float startValue, float endValue, float duration, Action<float> setter)
    {
        _startValue = _originalStart = startValue;
        _endValue = _originalEnd = endValue;
        Duration = duration;
        _setter = setter;
    }

    protected override void OnUpdate(float progress)
    {
        float value = _startValue + (_endValue - _startValue) * progress;
        _setter(value);
    }

    protected override void OnRestart()
    {
        _startValue = _originalStart;
        _endValue = _originalEnd;
    }

    protected override void OnLoopIncrement()
    {
        float diff = _originalEnd - _originalStart;
        _startValue = _endValue;
        _endValue += diff;
    }
}




public class Vector2Tween : Tween
{
    private Vector2 _startValue;
    private Vector2 _endValue;
    private readonly Vector2 _originalStart;
    private readonly Vector2 _originalEnd;
    private readonly Action<Vector2> _setter;

    public Vector2Tween(Vector2 startValue, Vector2 endValue, float duration, Action<Vector2> setter)
    {
        _startValue = _originalStart = startValue;
        _endValue = _originalEnd = endValue;
        Duration = duration;
        _setter = setter;
    }

    protected override void OnUpdate(float progress)
    {
        float x = _startValue.X + (_endValue.X - _startValue.X) * progress;
        float y = _startValue.Y + (_endValue.Y - _startValue.Y) * progress;
        _setter(new Vector2(x, y));
    }

    protected override void OnRestart()
    {
        _startValue = _originalStart;
        _endValue = _originalEnd;
    }

    protected override void OnLoopIncrement()
    {
        Vector2 diff = new Vector2(_originalEnd.X - _originalStart.X, _originalEnd.Y - _originalStart.Y);
        _startValue = _endValue;
        _endValue = new Vector2(_endValue.X + diff.X, _endValue.Y + diff.Y);
    }
}




public class Vector2IntTween : Tween
{
    private Vector2Int _startValue;
    private Vector2Int _endValue;
    private readonly Vector2Int _originalStart;
    private readonly Vector2Int _originalEnd;
    private readonly Action<Vector2Int> _setter;

    public Vector2IntTween(Vector2Int startValue, Vector2Int endValue, float duration, Action<Vector2Int> setter)
    {
        _startValue = _originalStart = startValue;
        _endValue = _originalEnd = endValue;
        Duration = duration;
        _setter = setter;
    }

    protected override void OnUpdate(float progress)
    {
        int x = (int)(_startValue.X + (_endValue.X - _startValue.X) * progress);
        int y = (int)(_startValue.Y + (_endValue.Y - _startValue.Y) * progress);
        _setter(new Vector2Int(x, y));
    }

    protected override void OnRestart()
    {
        _startValue = _originalStart;
        _endValue = _originalEnd;
    }

    protected override void OnLoopIncrement()
    {
        Vector2Int diff = new Vector2Int(_originalEnd.X - _originalStart.X, _originalEnd.Y - _originalStart.Y);
        _startValue = _endValue;
        _endValue = new Vector2Int(_endValue.X + diff.X, _endValue.Y + diff.Y);
    }
}




public class ColorTween : Tween
{
    private Color _startValue;
    private Color _endValue;
    private readonly Color _originalStart;
    private readonly Color _originalEnd;
    private readonly Action<Color> _setter;

    public ColorTween(Color startValue, Color endValue, float duration, Action<Color> setter)
    {
        _startValue = _originalStart = startValue;
        _endValue = _originalEnd = endValue;
        Duration = duration;
        _setter = setter;
    }

    protected override void OnUpdate(float progress)
    {
        float r = _startValue.R + (_endValue.R - _startValue.R) * progress;
        float g = _startValue.G + (_endValue.G - _startValue.G) * progress;
        float b = _startValue.B + (_endValue.B - _startValue.B) * progress;
        float a = _startValue.A + (_endValue.A - _startValue.A) * progress;
        _setter(new Color(r, g, b, a));
    }

    protected override void OnRestart()
    {
        _startValue = _originalStart;
        _endValue = _originalEnd;
    }
}




public class CallbackTween : Tween
{
    private readonly Action<float>? _progressCallback;

    public CallbackTween(float duration, Action<float>? progressCallback = null)
    {
        Duration = duration;
        _progressCallback = progressCallback;
    }

    protected override void OnUpdate(float progress)
    {
        _progressCallback?.Invoke(progress);
    }
}




public class DelayTween : Tween
{
    public DelayTween(float duration)
    {
        Duration = duration;
    }

    protected override void OnUpdate(float progress)
    {
        
    }
}

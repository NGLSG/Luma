using System;
using Luma.SDK;
using Luma.SDK.Components;
using Luma.SDK.Graphics;

namespace Luma.SDK.Tweening;




public static class TweenExtensions
{
    #region Transform Extensions

    
    
    
    public static Tween DOMove(this Transform transform, Vector2 endValue, float duration)
    {
        return DOTween.To(
            () => transform.Position,
            v => transform.Position = v,
            endValue,
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DOMoveX(this Transform transform, float endValue, float duration)
    {
        return DOTween.To(
            () => transform.Position.X,
            v => transform.Position = new Vector2(v, transform.Position.Y),
            endValue,
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DOMoveY(this Transform transform, float endValue, float duration)
    {
        return DOTween.To(
            () => transform.Position.Y,
            v => transform.Position = new Vector2(transform.Position.X, v),
            endValue,
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DOScale(this Transform transform, Vector2 endValue, float duration)
    {
        return DOTween.To(
            () => transform.Scale,
            v => transform.Scale = v,
            endValue,
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DOScale(this Transform transform, float endValue, float duration)
    {
        return DOTween.To(
            () => transform.Scale,
            v => transform.Scale = v,
            new Vector2(endValue, endValue),
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DORotate(this Transform transform, float endValue, float duration)
    {
        return DOTween.To(
            () => transform.Rotation,
            v => transform.Rotation = v,
            endValue,
            duration
        ).SetTarget(transform);
    }

    
    
    
    public static Tween DORotateDegrees(this Transform transform, float endValue, float duration)
    {
        float endRadians = endValue * (MathF.PI / 180f);
        return DOTween.To(
            () => transform.Rotation,
            v => transform.Rotation = v,
            endRadians,
            duration
        ).SetTarget(transform);
    }

    #endregion

    #region Sprite Extensions

    
    
    
    public static Tween DOColor(this Sprite sprite, Color endValue, float duration)
    {
        return DOTween.To(
            () => sprite.Color,
            v => sprite.Color = v,
            endValue,
            duration
        ).SetTarget(sprite);
    }

    
    
    
    public static Tween DOFade(this Sprite sprite, float endValue, float duration)
    {
        return DOTween.To(
            () => sprite.Color.A,
            v => sprite.Color = new Color(sprite.Color.R, sprite.Color.G, sprite.Color.B, v),
            endValue,
            duration
        ).SetTarget(sprite);
    }

    #endregion

    #region Camera Extensions

    
    
    
    public static Tween DOMove(this Type cameraType, Vector2 endValue, float duration)
    {
        if (cameraType != typeof(Camera))
            throw new ArgumentException("此扩展方法仅用于Camera类型");
        
        return DOTween.To(
            () => Camera.Position,
            v => Camera.Position = v,
            endValue,
            duration
        );
    }

    
    
    
    public static Tween DOZoom(float endValue, float duration)
    {
        return DOTween.To(
            () => Camera.Zoom,
            v => Camera.Zoom = v,
            endValue,
            duration
        );
    }

    
    
    
    public static Tween CameraMove(Vector2 endValue, float duration)
    {
        return DOTween.To(
            () => Camera.Position,
            v => Camera.Position = v,
            endValue,
            duration
        );
    }

    
    
    
    public static Tween CameraZoom(float endValue, float duration)
    {
        return DOTween.To(
            () => Camera.Zoom,
            v => Camera.Zoom = v,
            endValue,
            duration
        );
    }

    
    
    
    public static Tween CameraRotate(float endValue, float duration)
    {
        return DOTween.To(
            () => Camera.Rotation,
            v => Camera.Rotation = v,
            endValue,
            duration
        );
    }

    #endregion

    #region Punch & Shake Effects

    
    
    
    public static Tween DOPunchPosition(this Transform transform, Vector2 punch, float duration, int vibrato = 10, float elasticity = 1f)
    {
        var originalPos = transform.Position;
        return DOTween.Timer(duration, progress =>
        {
            float decay = 1f - progress;
            float wave = MathF.Sin(progress * MathF.PI * vibrato) * decay * elasticity;
            transform.Position = new Vector2(
                originalPos.X + punch.X * wave,
                originalPos.Y + punch.Y * wave
            );
        }).OnComplete(() => transform.Position = originalPos).SetTarget(transform);
    }

    
    
    
    public static Tween DOPunchScale(this Transform transform, Vector2 punch, float duration, int vibrato = 10, float elasticity = 1f)
    {
        var originalScale = transform.Scale;
        return DOTween.Timer(duration, progress =>
        {
            float decay = 1f - progress;
            float wave = MathF.Sin(progress * MathF.PI * vibrato) * decay * elasticity;
            transform.Scale = new Vector2(
                originalScale.X + punch.X * wave,
                originalScale.Y + punch.Y * wave
            );
        }).OnComplete(() => transform.Scale = originalScale).SetTarget(transform);
    }

    
    
    
    public static Tween DOShakePosition(this Transform transform, float duration, float strength = 1f, int vibrato = 10)
    {
        var originalPos = transform.Position;
        var random = new Random();
        return DOTween.Timer(duration, progress =>
        {
            float decay = 1f - progress;
            float offsetX = ((float)random.NextDouble() * 2f - 1f) * strength * decay;
            float offsetY = ((float)random.NextDouble() * 2f - 1f) * strength * decay;
            transform.Position = new Vector2(originalPos.X + offsetX, originalPos.Y + offsetY);
        }).OnComplete(() => transform.Position = originalPos).SetTarget(transform);
    }

    
    
    
    public static Tween DOShakeRotation(this Transform transform, float duration, float strength = 0.5f, int vibrato = 10)
    {
        var originalRot = transform.Rotation;
        var random = new Random();
        return DOTween.Timer(duration, progress =>
        {
            float decay = 1f - progress;
            float offset = ((float)random.NextDouble() * 2f - 1f) * strength * decay;
            transform.Rotation = originalRot + offset;
        }).OnComplete(() => transform.Rotation = originalRot).SetTarget(transform);
    }

    #endregion
}

using System;

namespace Luma.SDK.Tweening;




public enum Ease
{
    Linear,
    
    InSine, OutSine, InOutSine,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InQuint, OutQuint, InOutQuint,
    InExpo, OutExpo, InOutExpo,
    InCirc, OutCirc, InOutCirc,
    InElastic, OutElastic, InOutElastic,
    InBack, OutBack, InOutBack,
    InBounce, OutBounce, InOutBounce,
}




public static class EaseFunc
{
    private const float PI = MathF.PI;
    private const float C1 = 1.70158f;
    private const float C2 = C1 * 1.525f;
    private const float C3 = C1 + 1f;
    private const float C4 = (2f * PI) / 3f;
    private const float C5 = (2f * PI) / 4.5f;
    private const float N1 = 7.5625f;
    private const float D1 = 2.75f;

    
    
    
    
    
    
    public static float Evaluate(Ease ease, float t)
    {
        t = Math.Clamp(t, 0f, 1f);
        
        return ease switch
        {
            Ease.Linear => t,
            
            Ease.InSine => 1f - MathF.Cos((t * PI) / 2f),
            Ease.OutSine => MathF.Sin((t * PI) / 2f),
            Ease.InOutSine => -(MathF.Cos(PI * t) - 1f) / 2f,
            
            Ease.InQuad => t * t,
            Ease.OutQuad => 1f - (1f - t) * (1f - t),
            Ease.InOutQuad => t < 0.5f ? 2f * t * t : 1f - MathF.Pow(-2f * t + 2f, 2f) / 2f,
            
            Ease.InCubic => t * t * t,
            Ease.OutCubic => 1f - MathF.Pow(1f - t, 3f),
            Ease.InOutCubic => t < 0.5f ? 4f * t * t * t : 1f - MathF.Pow(-2f * t + 2f, 3f) / 2f,
            
            Ease.InQuart => t * t * t * t,
            Ease.OutQuart => 1f - MathF.Pow(1f - t, 4f),
            Ease.InOutQuart => t < 0.5f ? 8f * t * t * t * t : 1f - MathF.Pow(-2f * t + 2f, 4f) / 2f,
            
            Ease.InQuint => t * t * t * t * t,
            Ease.OutQuint => 1f - MathF.Pow(1f - t, 5f),
            Ease.InOutQuint => t < 0.5f ? 16f * t * t * t * t * t : 1f - MathF.Pow(-2f * t + 2f, 5f) / 2f,
            
            Ease.InExpo => t == 0f ? 0f : MathF.Pow(2f, 10f * t - 10f),
            Ease.OutExpo => t == 1f ? 1f : 1f - MathF.Pow(2f, -10f * t),
            Ease.InOutExpo => t == 0f ? 0f : t == 1f ? 1f : t < 0.5f
                ? MathF.Pow(2f, 20f * t - 10f) / 2f
                : (2f - MathF.Pow(2f, -20f * t + 10f)) / 2f,
            
            Ease.InCirc => 1f - MathF.Sqrt(1f - MathF.Pow(t, 2f)),
            Ease.OutCirc => MathF.Sqrt(1f - MathF.Pow(t - 1f, 2f)),
            Ease.InOutCirc => t < 0.5f
                ? (1f - MathF.Sqrt(1f - MathF.Pow(2f * t, 2f))) / 2f
                : (MathF.Sqrt(1f - MathF.Pow(-2f * t + 2f, 2f)) + 1f) / 2f,
            
            Ease.InElastic => t == 0f ? 0f : t == 1f ? 1f
                : -MathF.Pow(2f, 10f * t - 10f) * MathF.Sin((t * 10f - 10.75f) * C4),
            Ease.OutElastic => t == 0f ? 0f : t == 1f ? 1f
                : MathF.Pow(2f, -10f * t) * MathF.Sin((t * 10f - 0.75f) * C4) + 1f,
            Ease.InOutElastic => t == 0f ? 0f : t == 1f ? 1f : t < 0.5f
                ? -(MathF.Pow(2f, 20f * t - 10f) * MathF.Sin((20f * t - 11.125f) * C5)) / 2f
                : (MathF.Pow(2f, -20f * t + 10f) * MathF.Sin((20f * t - 11.125f) * C5)) / 2f + 1f,
            
            Ease.InBack => C3 * t * t * t - C1 * t * t,
            Ease.OutBack => 1f + C3 * MathF.Pow(t - 1f, 3f) + C1 * MathF.Pow(t - 1f, 2f),
            Ease.InOutBack => t < 0.5f
                ? (MathF.Pow(2f * t, 2f) * ((C2 + 1f) * 2f * t - C2)) / 2f
                : (MathF.Pow(2f * t - 2f, 2f) * ((C2 + 1f) * (t * 2f - 2f) + C2) + 2f) / 2f,
            
            Ease.InBounce => 1f - EvaluateBounceOut(1f - t),
            Ease.OutBounce => EvaluateBounceOut(t),
            Ease.InOutBounce => t < 0.5f
                ? (1f - EvaluateBounceOut(1f - 2f * t)) / 2f
                : (1f + EvaluateBounceOut(2f * t - 1f)) / 2f,
            
            _ => t
        };
    }

    private static float EvaluateBounceOut(float t)
    {
        if (t < 1f / D1)
            return N1 * t * t;
        if (t < 2f / D1)
            return N1 * (t -= 1.5f / D1) * t + 0.75f;
        if (t < 2.5f / D1)
            return N1 * (t -= 2.25f / D1) * t + 0.9375f;
        return N1 * (t -= 2.625f / D1) * t + 0.984375f;
    }
}

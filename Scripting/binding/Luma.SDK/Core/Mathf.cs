namespace Luma.SDK;

public static class Mathf
{
    public const float PI = 3.14159265f;
    public const float Deg2Rad = PI / 180f;
    public const float Rad2Deg = 180f / PI;
    public const float Epsilon = 1e-6f;

    public static float Lerp(float a, float b, float t) =>
        a + (b - a) * Clamp01(t);

    public static float InverseLerp(float a, float b, float value) =>
        System.MathF.Abs(b - a) > Epsilon ? Clamp01((value - a) / (b - a)) : 0f;

    public static float Clamp(float v, float min, float max) =>
        v < min ? min : v > max ? max : v;

    public static float Clamp01(float v) =>
        v < 0f ? 0f : v > 1f ? 1f : v;

    public static float MoveTowards(float current, float target, float maxDelta)
    {
        if (System.MathF.Abs(target - current) <= maxDelta)
            return target;
        return current + Sign(target - current) * maxDelta;
    }

    public static float SmoothDamp(float current, float target, ref float velocity, float smoothTime, float dt)
    {
        float omega = 2f / System.MathF.Max(smoothTime, 0.0001f);
        float x = omega * dt;
        float exp = 1f / (1f + x + 0.48f * x * x + 0.235f * x * x * x);
        float delta = current - target;
        float temp = (velocity + omega * delta) * dt;
        velocity = (velocity - omega * temp) * exp;
        float result = target + (delta + temp) * exp;
        if ((target - current > 0f) == (result > target))
        {
            result = target;
            velocity = 0f;
        }
        return result;
    }

    public static float Repeat(float t, float length) =>
        Clamp(t - System.MathF.Floor(t / length) * length, 0f, length);

    public static float PingPong(float t, float length)
    {
        t = Repeat(t, length * 2f);
        return length - Abs(t - length);
    }

    public static float Sign(float v) =>
        v >= 0f ? 1f : -1f;

    public static float Abs(float v) =>
        System.MathF.Abs(v);

    public static float Min(float a, float b) =>
        a < b ? a : b;

    public static float Max(float a, float b) =>
        a > b ? a : b;

    public static float Floor(float v) =>
        System.MathF.Floor(v);

    public static float Ceil(float v) =>
        System.MathF.Ceiling(v);

    public static float Round(float v) =>
        System.MathF.Round(v);

    public static float Sqrt(float v) =>
        System.MathF.Sqrt(v);

    public static float Sin(float v) =>
        System.MathF.Sin(v);

    public static float Cos(float v) =>
        System.MathF.Cos(v);

    public static float Atan2(float y, float x) =>
        System.MathF.Atan2(y, x);

    public static bool Approximately(float a, float b) =>
        System.MathF.Abs(b - a) < System.MathF.Max(1e-6f * System.MathF.Max(Abs(a), Abs(b)), Epsilon * 8f);
}

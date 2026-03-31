namespace Luma.SDK;

public static class Time
{
    public static float DeltaTime { get; internal set; }
    public static float UnscaledDeltaTime { get; internal set; }
    public static float TimeScale { get; set; } = 1.0f;
    public static float TotalTime { get; internal set; }
    public static int FrameCount { get; internal set; }
    public static float FixedDeltaTime { get; set; } = 1.0f / 60.0f;

    internal static void Update(float dt)
    {
        UnscaledDeltaTime = dt;
        DeltaTime = dt * TimeScale;
        TotalTime += DeltaTime;
        FrameCount++;
    }
}

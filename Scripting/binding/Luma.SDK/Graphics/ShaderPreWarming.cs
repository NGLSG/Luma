using System.Numerics;
using Luma.SDK;

namespace Luma;




public struct ShaderPreWarmingState
{
    
    
    
    public int Total;

    
    
    
    public int Loaded;

    
    
    
    public bool IsRunning;

    
    
    
    public bool IsComplete;

    
    
    
    public float Progress => Total > 0 ? (float)Loaded / Total : 0f;
}





public static class ShaderPreWarming
{
    
    
    
    
    public static void StartAsync()
    {
        Native.Shader_StartPreWarmingAsync();
    }

    
    
    
    public static void Start()
    {
        Native.Shader_PreWarming();
    }

    
    
    
    
    public static void Stop()
    {
        Native.Shader_StopPreWarming();
    }

    
    
    
    public static bool IsRunning => Native.Shader_IsPreWarmingRunning();

    
    
    
    public static bool IsComplete => Native.Shader_IsPreWarmingComplete();

    
    
    
    public static ShaderPreWarmingState GetState()
    {
        Native.Shader_GetPreWarmingState(out int total, out int loaded, out bool isRunning, out bool isComplete);
        return new ShaderPreWarmingState
        {
            Total = total,
            Loaded = loaded,
            IsRunning = isRunning,
            IsComplete = isComplete
        };
    }

    
    
    
    public static float Progress
    {
        get
        {
            var state = GetState();
            return state.Progress;
        }
    }
}
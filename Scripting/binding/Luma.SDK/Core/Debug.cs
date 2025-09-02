namespace Luma.SDK;




public static class Debug
{
    
    
    
    public static void Log(object message)
    {
        Native.Logger_Log(LumaLogLevel.Info, message?.ToString() ?? "null");
    }

    
    
    
    public static void LogWarning(object message)
    {
        Native.Logger_Log(LumaLogLevel.Warning, message?.ToString() ?? "null");
    }

    
    
    
    public static void LogError(object message)
    {
        Native.Logger_Log(LumaLogLevel.Error, message?.ToString() ?? "null");
    }
    
    
    
    
    public static void LogFormat(string format, params object[] args)
    {
        Log(string.Format(format, args));
    }
    
    
    
    
    public static void LogWarningFormat(string format, params object[] args)
    {
        LogWarning(string.Format(format, args));
    }
    
    
    
    
    public static void LogErrorFormat(string format, params object[] args)
    {
        LogError(string.Format(format, args));
    }
}
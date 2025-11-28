using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Plugins;




public static partial class LumaAPI
{
    private const string DllName = "LumaEngine";

    #region Logging

    [LibraryImport(DllName, EntryPoint = "Luma_LogInfo")]
    private static partial void LogInfoNative([MarshalAs(UnmanagedType.LPStr)] string message);

    [LibraryImport(DllName, EntryPoint = "Luma_LogWarn")]
    private static partial void LogWarnNative([MarshalAs(UnmanagedType.LPStr)] string message);

    [LibraryImport(DllName, EntryPoint = "Luma_LogError")]
    private static partial void LogErrorNative([MarshalAs(UnmanagedType.LPStr)] string message);

    [LibraryImport(DllName, EntryPoint = "Luma_LogDebug")]
    private static partial void LogDebugNative([MarshalAs(UnmanagedType.LPStr)] string message);

    
    
    
    public static void LogInfo(string message) => LogInfoNative(message);

    
    
    
    public static void LogWarn(string message) => LogWarnNative(message);

    
    
    
    public static void LogError(string message) => LogErrorNative(message);

    
    
    
    public static void LogDebug(string message) => LogDebugNative(message);

    
    
    
    public static void LogInfo(string format, params object[] args) => LogInfoNative(string.Format(format, args));

    
    
    
    public static void LogWarn(string format, params object[] args) => LogWarnNative(string.Format(format, args));

    
    
    
    public static void LogError(string format, params object[] args) => LogErrorNative(string.Format(format, args));

    #endregion

    #region Editor

    [LibraryImport(DllName, EntryPoint = "Luma_IsEditorMode")]
    [return: MarshalAs(UnmanagedType.I1)]
    public static partial bool IsEditorMode();

    [LibraryImport(DllName, EntryPoint = "Luma_IsPlaying")]
    [return: MarshalAs(UnmanagedType.I1)]
    public static partial bool IsPlaying();

    [LibraryImport(DllName, EntryPoint = "Luma_GetSelectedEntityCount")]
    public static partial int GetSelectedEntityCount();

    [StructLayout(LayoutKind.Sequential)]
    public struct GuidCAPI
    {
        public ulong Data1;
        public ulong Data2;

        public override string ToString() => $"{Data1:X16}{Data2:X16}";
    }

    [LibraryImport(DllName, EntryPoint = "Luma_GetSelectedEntityGuid")]
    public static partial GuidCAPI GetSelectedEntityGuid(int index);

    [LibraryImport(DllName, EntryPoint = "Luma_GetSelectedEntityName")]
    private static partial void GetSelectedEntityNameNative(int index, IntPtr buffer, int bufferSize);

    
    
    
    
    
    public static string GetSelectedEntityName(int index)
    {
        IntPtr buffer = Marshal.AllocHGlobal(256);
        try
        {
            GetSelectedEntityNameNative(index, buffer, 256);
            return Marshal.PtrToStringUTF8(buffer) ?? "";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    #endregion

    #region Project

    [LibraryImport(DllName, EntryPoint = "Luma_IsProjectLoaded")]
    [return: MarshalAs(UnmanagedType.I1)]
    public static partial bool IsProjectLoaded();

    [LibraryImport(DllName, EntryPoint = "Luma_GetProjectName")]
    private static partial void GetProjectNameNative(IntPtr buffer, int bufferSize);

    [LibraryImport(DllName, EntryPoint = "Luma_GetProjectPath")]
    private static partial void GetProjectPathNative(IntPtr buffer, int bufferSize);

    [LibraryImport(DllName, EntryPoint = "Luma_GetAssetsPath")]
    private static partial void GetAssetsPathNative(IntPtr buffer, int bufferSize);

    
    
    
    public static string GetProjectName()
    {
        IntPtr buffer = Marshal.AllocHGlobal(256);
        try
        {
            GetProjectNameNative(buffer, 256);
            return Marshal.PtrToStringUTF8(buffer) ?? "";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    
    
    
    public static string GetProjectPath()
    {
        IntPtr buffer = Marshal.AllocHGlobal(512);
        try
        {
            GetProjectPathNative(buffer, 512);
            return Marshal.PtrToStringUTF8(buffer) ?? "";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    
    
    
    public static string GetAssetsPath()
    {
        IntPtr buffer = Marshal.AllocHGlobal(512);
        try
        {
            GetAssetsPathNative(buffer, 512);
            return Marshal.PtrToStringUTF8(buffer) ?? "";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    #endregion

    #region Scene

    [LibraryImport(DllName, EntryPoint = "Luma_GetEntityCount")]
    public static partial int GetEntityCount();

    [LibraryImport(DllName, EntryPoint = "Luma_GetCurrentSceneName")]
    private static partial void GetCurrentSceneNameNative(IntPtr buffer, int bufferSize);

    
    
    
    public static string GetCurrentSceneName()
    {
        IntPtr buffer = Marshal.AllocHGlobal(256);
        try
        {
            GetCurrentSceneNameNative(buffer, 256);
            return Marshal.PtrToStringUTF8(buffer) ?? "";
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    #endregion

    #region Assets

    [LibraryImport(DllName, EntryPoint = "Luma_GetAssetCount")]
    public static partial int GetAssetCount();

    [LibraryImport(DllName, EntryPoint = "Luma_AssetExists")]
    [return: MarshalAs(UnmanagedType.I1)]
    public static partial bool AssetExists([MarshalAs(UnmanagedType.LPStr)] string path);

    #endregion
}




public static class Log
{
    public static void Info(string message) => LumaAPI.LogInfo(message);
    public static void Info(string format, params object[] args) => LumaAPI.LogInfo(format, args);
    
    public static void Warn(string message) => LumaAPI.LogWarn(message);
    public static void Warn(string format, params object[] args) => LumaAPI.LogWarn(format, args);
    
    public static void Error(string message) => LumaAPI.LogError(message);
    public static void Error(string format, params object[] args) => LumaAPI.LogError(format, args);
    
    public static void Debug(string message) => LumaAPI.LogDebug(message);
}




public static class Project
{
    
    
    
    public static bool IsLoaded => LumaAPI.IsProjectLoaded();

    
    
    
    public static string Name => LumaAPI.GetProjectName();

    
    
    
    public static string Path => LumaAPI.GetProjectPath();

    
    
    
    public static string AssetsPath => LumaAPI.GetAssetsPath();
}




public static class Editor
{
    
    
    
    public static bool IsEditorMode => LumaAPI.IsEditorMode();

    
    
    
    public static bool IsPlaying => LumaAPI.IsPlaying();

    
    
    
    public static int SelectedEntityCount => LumaAPI.GetSelectedEntityCount();

    
    
    
    
    public static LumaAPI.GuidCAPI GetSelectedEntityGuid(int index) => LumaAPI.GetSelectedEntityGuid(index);

    
    
    
    
    public static string GetSelectedEntityName(int index) => LumaAPI.GetSelectedEntityName(index);

    
    
    
    public static string? FirstSelectedEntityName => SelectedEntityCount > 0 ? GetSelectedEntityName(0) : null;
}

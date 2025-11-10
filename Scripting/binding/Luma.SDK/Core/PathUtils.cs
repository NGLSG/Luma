using System.Runtime.InteropServices;

namespace Luma.SDK;

public static class PathUtils
{
    public static string GetExecutableDir()
    {
        return getStringFromCstr(Native.PathUtils_GetExecutableDir());
    }

    public static string GetContentDir()
    {
        return getStringFromCstr(Native.PathUtils_GetContentDir());
    }

    public static string GetPersistentDataDir()
    {
        return getStringFromCstr(Native.PathUtils_GetPersistentDataDir());
    }

    public static string GetCacheDir()
    {
        return getStringFromCstr(Native.PathUtils_GetCacheDir());
    }

    public static string GetAndroidExternalDataDir()
    {
        return getStringFromCstr(Native.PathUtils_GetAndroidExternalDataDir());
    }

    public static string GetAndroidInternalDataDir()
    {
        return getStringFromCstr(Native.PathUtils_GetAndroidInternalDataDir());
    }


    private static string getStringFromCstr(IntPtr strPtr)
    {
        return strPtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(strPtr) ?? string.Empty;
    }
}
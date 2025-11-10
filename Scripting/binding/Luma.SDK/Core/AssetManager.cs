namespace Luma.SDK;

public static class AssetManager
{
    public static bool StartPreload()
    {
        return Native.AssetManager_StartPreload();
    }

    public static void StopPreload()
    {
        Native.AssetManager_StopPreload();
    }

    public static bool IsPreloading()
    {
        return Native.AssetManager_IsPreloadRunning();
    }

    public static bool IsPreloadComplete()
    {
        return Native.AssetManager_IsPreloadComplete();
    }

    public struct Progress
    {
        public int TotalAssets;
        public int LoadedAssets;
        public float Percentage;
    }

    public static Progress GetPreloadProgress()
    {
        Native.AssetManager_GetPreloadProgress(out int totalAssets, out int loadedAssets);
        return new Progress
        {
            TotalAssets = totalAssets,
            LoadedAssets = loadedAssets,
            Percentage = totalAssets > 0 ? (float)loadedAssets / totalAssets : 1.0f
        };
    }

    public static Guid LoadAsset(string path)
    {
        return Native.AssetManager_LoadAsset(path);
    }
}
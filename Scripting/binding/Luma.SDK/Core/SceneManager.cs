using System;
using System.Runtime.InteropServices;

namespace Luma.SDK;

public static class SceneManager
{
    public static bool LoadScene(AssetHandle sceneHandle)
    {
        if (sceneHandle.AssetType != AssetType.Scene || !sceneHandle.IsValid())
        {
            Debug.LogError("LoadScene failed: Provided AssetHandle is not a valid scene.");
            return false;
        }

        return Native.SceneManager_LoadScene(sceneHandle.AssetGuid.ToString());
    }


    public static void LoadSceneAsync(AssetHandle sceneHandle)
    {
        if (sceneHandle.AssetType != AssetType.Scene || !sceneHandle.IsValid())
        {
            Debug.LogError("LoadSceneAsync failed: Provided AssetHandle is not a valid scene.");
            return;
        }

        Native.SceneManager_LoadSceneAsync(sceneHandle.AssetGuid.ToString());
    }


    public static Scene CurrentScene
    {
        get
        {
            IntPtr scenePtr = Native.SceneManager_GetCurrentScene();
            IntPtr guidPtr = Native.SceneManager_GetCurrentSceneGuid();
            if (scenePtr != IntPtr.Zero && guidPtr != IntPtr.Zero)
            {
                string guidStr = Marshal.PtrToStringAnsi(guidPtr) ?? Guid.Empty.ToString();
                if (Guid.TryParse(guidStr, out Guid sceneGuid))
                {
                    return new Scene(sceneGuid, scenePtr);
                }
            }

            return new Scene();
        }
    }
}

public readonly struct Scene
{
    public Guid Guid { get; }
    internal IntPtr ScenePtr { get; }

    internal Scene(Guid guid, IntPtr scenePtr)
    {
        Guid = guid;
        ScenePtr = scenePtr;
    }

    public bool IsValid() => ScenePtr != IntPtr.Zero;

    public Entity FindEntityByGuid(Guid guid)
    {
        if (!IsValid()) return new Entity(0, IntPtr.Zero);
        uint entityId = Native.Scene_FindGameObjectByGuid(ScenePtr, guid.ToString());
        return new Entity(entityId, ScenePtr);
    }

    public Entity CreateEntity(string name = "New Entity")
    {
        if (!IsValid()) return new Entity(0, IntPtr.Zero);
        uint entityId = Native.Scene_CreateGameObject(ScenePtr, name);
        return new Entity(entityId, ScenePtr);
    }
}
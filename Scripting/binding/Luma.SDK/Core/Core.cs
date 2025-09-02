using System;
using System.Runtime.InteropServices;

namespace Luma.SDK;

[StructLayout(LayoutKind.Sequential)]
public struct Vector2
{
    public float X;
    public float Y;
}

[StructLayout(LayoutKind.Sequential)]
public struct Vector2Int
{
    public int X;
    public int Y;
}

[StructLayout(LayoutKind.Sequential)]
public struct Color
{
    public float R;
    public float G;
    public float B;
    public float A;
}

[StructLayout(LayoutKind.Sequential)]
public struct RectF
{
    public float X;
    public float Y;
    public float Width;
    public float Height;
}

public enum AssetType
{
    Unknown = 0,
    Texture,
    Material,
    CSharpScript,
    Scene,
    Prefab,
    Audio,
    Video,
    Animation,
    PhysicsMaterial,
    LocalGameObject,
    Font,
}





[StructLayout(LayoutKind.Sequential)]
public struct AssetHandle
{
    
    
    
    
    public Guid AssetGuid;

    
    
    
    public AssetType AssetType;

    
    
    
    public AssetHandle(Guid guid, AssetType type = AssetType.Unknown)
    {
        AssetGuid = guid;
        AssetType = type;
    }

    
    
    
    public bool IsValid() => AssetGuid != Guid.Empty;

    
    
    
    public static AssetHandle Invalid => new(Guid.Empty, AssetType.Unknown);
}
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using YamlDotNet.Serialization;

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






[StructLayout(LayoutKind.Sequential, Size = 16)]
public readonly struct Guid : IEquatable<Guid>, IComparable<Guid>
{
    
    
    
    public static readonly Guid Empty = new();

    
    
    private readonly ulong data1;
    private readonly ulong data2;

    
    
    
    
    
    public Guid(byte[] bytes)
    {
        if (bytes == null || bytes.Length != 16)
        {
            throw new ArgumentException("Byte array must be 16 bytes long.", nameof(bytes));
        }

        
        var span = new ReadOnlySpan<byte>(bytes);
        data1 = Unsafe.ReadUnaligned<ulong>(ref MemoryMarshal.GetReference(span));
        data2 = Unsafe.ReadUnaligned<ulong>(ref MemoryMarshal.GetReference(span.Slice(8)));
    }

    
    
    
    
    public Guid(string guidString)
    {
        this = Parse(guidString);
    }

    public static bool TryParse(string input, out Guid result)
    {
        try
        {
            result = Parse(input);
            return true;
        }
        catch
        {
            result = Empty;
            return false;
        }
    }

    
    
    
    
    public byte[] ToByteArray()
    {
        var bytes = new byte[16];
        Unsafe.WriteUnaligned(ref bytes[0], data1);
        Unsafe.WriteUnaligned(ref bytes[8], data2);
        return bytes;
    }

    
    
    
    
    
    public override string ToString()
    {
        Span<byte> bytes = stackalloc byte[16];
        Unsafe.WriteUnaligned(ref bytes[0], data1);
        Unsafe.WriteUnaligned(ref bytes[8], data2);

        
        return string.Create(36, bytes, (chars, b) =>
        {
            HexToChars(chars.Slice(0, 8), b.Slice(0, 4));
            chars[8] = '-';
            HexToChars(chars.Slice(9, 4), b.Slice(4, 2));
            chars[13] = '-';
            HexToChars(chars.Slice(14, 4), b.Slice(6, 2));
            chars[18] = '-';
            HexToChars(chars.Slice(19, 4), b.Slice(8, 2));
            chars[23] = '-';
            HexToChars(chars.Slice(24, 12), b.Slice(10, 6));
        });
    }

    
    
    
    
    
    
    
    public static Guid Parse(string input)
    {
        if (input == null)
        {
            throw new ArgumentNullException(nameof(input));
        }

        var span = input.AsSpan().Trim();

        if (span.Length != 36 || span[8] != '-' || span[13] != '-' || span[18] != '-' || span[23] != '-')
        {
            throw new FormatException("Invalid Guid format.");
        }

        Span<byte> bytes = stackalloc byte[16];

        try
        {
            ParseHex(span.Slice(0, 8), bytes.Slice(0, 4));
            ParseHex(span.Slice(9, 4), bytes.Slice(4, 2));
            ParseHex(span.Slice(14, 4), bytes.Slice(6, 2));
            ParseHex(span.Slice(19, 4), bytes.Slice(8, 2));
            ParseHex(span.Slice(24, 12), bytes.Slice(10, 6));
        }
        catch (Exception ex)
        {
            throw new FormatException("Invalid hexadecimal characters in Guid string.", ex);
        }

        ulong d1 = Unsafe.ReadUnaligned<ulong>(ref MemoryMarshal.GetReference(bytes));
        ulong d2 = Unsafe.ReadUnaligned<ulong>(ref MemoryMarshal.GetReference(bytes.Slice(8)));

        return new Guid(d1, d2);
    }

    
    private Guid(ulong d1, ulong d2)
    {
        data1 = d1;
        data2 = d2;
    }

    
    private static void ParseHex(ReadOnlySpan<char> source, Span<byte> destination)
    {
        for (int i = 0; i < destination.Length; i++)
        {
            var high = source[i * 2];
            var low = source[i * 2 + 1];
            destination[i] = (byte)((FromHex(high) << 4) | FromHex(low));
        }
    }

    
    private static void HexToChars(Span<char> destination, ReadOnlySpan<byte> source)
    {
        for (int i = 0; i < source.Length; i++)
        {
            var b = source[i];
            var high = b >> 4;
            var low = b & 0x0F;
            destination[i * 2] = ToHex(high);
            destination[i * 2 + 1] = ToHex(low);
        }
    }

    
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static int FromHex(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw new FormatException("Invalid hex character.");
    }

    
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static char ToHex(int value)
    {
        return (char)(value < 10 ? value + '0' : value - 10 + 'a');
    }

    public override int GetHashCode()
    {
        
        ulong h1 = data1;
        ulong h2 = data2;
        return ((h1 ^ (h2 << 1))).GetHashCode();
    }

    public override bool Equals(object? obj)
    {
        return obj is Guid other && Equals(other);
    }

    public bool Equals(Guid other)
    {
        return data1 == other.data1 && data2 == other.data2;
    }

    public int CompareTo(Guid other)
    {
        int result = data1.CompareTo(other.data1);
        if (result != 0) return result;
        return data2.CompareTo(other.data2);
    }

    public static bool operator ==(Guid left, Guid right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(Guid left, Guid right)
    {
        return !left.Equals(right);
    }
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
    AnimationClip,
    AnimationController,
    PhysicsMaterial,
    LocalGameObject,
    Blueprint,
    Tile,
    Tileset,
    RuleTile,
    Font,
};




[StructLayout(LayoutKind.Sequential)]
public struct AssetHandle : IEquatable<AssetHandle>
{
    [YamlMember(Alias = "guid")]
    public Guid AssetGuid;

    [YamlMember(Alias = "type")]
    public AssetType AssetType;

    public AssetHandle(Guid guid, AssetType type = AssetType.Unknown)
    {
        AssetGuid = guid;
        AssetType = type;
    }

    public bool IsValid() => AssetGuid != Guid.Empty;

    public static AssetHandle Invalid => new(Guid.Empty, AssetType.Unknown);

    #region Static Helpers

    public static AssetHandle TextureHandle(Guid guid) => new(guid, AssetType.Texture);
    public static AssetHandle MaterialHandle(Guid guid) => new(guid, AssetType.Material);
    public static AssetHandle CSharpScriptHandle(Guid guid) => new(guid, AssetType.CSharpScript);
    public static AssetHandle SceneHandle(Guid guid) => new(guid, AssetType.Scene);
    public static AssetHandle PrefabHandle(Guid guid) => new(guid, AssetType.Prefab);
    public static AssetHandle AudioHandle(Guid guid) => new(guid, AssetType.Audio);
    public static AssetHandle VideoHandle(Guid guid) => new(guid, AssetType.Video);
    public static AssetHandle AnimationClipHandle(Guid guid) => new(guid, AssetType.AnimationClip);
    public static AssetHandle AnimationControllerHandle(Guid guid) => new(guid, AssetType.AnimationController);
    public static AssetHandle PhysicsMaterialHandle(Guid guid) => new(guid, AssetType.PhysicsMaterial);
    public static AssetHandle LocalGameObjectHandle(Guid guid) => new(guid, AssetType.LocalGameObject);
    public static AssetHandle BlueprintHandle(Guid guid) => new(guid, AssetType.Blueprint);
    public static AssetHandle TileHandle(Guid guid) => new(guid, AssetType.Tile);
    public static AssetHandle TilesetHandle(Guid guid) => new(guid, AssetType.Tileset);
    public static AssetHandle RuleTileHandle(Guid guid) => new(guid, AssetType.RuleTile);
    public static AssetHandle FontHandle(Guid guid) => new(guid, AssetType.Font);

    #endregion

    #region Equality Members

    public bool Equals(AssetHandle other)
    {
        return AssetGuid.Equals(other.AssetGuid) && AssetType == other.AssetType;
    }

    public override bool Equals(object? obj)
    {
        return obj is AssetHandle other && Equals(other);
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(AssetGuid, (int)AssetType);
    }

    public static bool operator ==(AssetHandle left, AssetHandle right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(AssetHandle left, AssetHandle right)
    {
        return !left.Equals(right);
    }

    #endregion

    public override string ToString()
    {
        return $"AssetHandle(Guid: {AssetGuid}, Type: {AssetType})";
    }
}

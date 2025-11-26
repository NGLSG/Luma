using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Graphics;




public class TextureA : IDisposable
{
    private IntPtr _nativeHandle;
    private bool _disposed;
    private bool _ownsHandle;

    
    
    
    public IntPtr NativeHandle => _nativeHandle;

    
    
    
    
    
    internal TextureA(IntPtr nativeHandle, bool ownsHandle = false)
    {
        _nativeHandle = nativeHandle;
        _ownsHandle = ownsHandle;
    }

    
    
    
    
    
    public static TextureA? Load(Guid assetGuid)
    {
        IntPtr handle = Native.TextureA_Load(assetGuid);
        return handle != IntPtr.Zero ? new TextureA(handle, false) : null;
    }

    
    
    
    
    
    
    public static TextureA? Create(uint width, uint height)
    {
        IntPtr handle = Native.TextureA_Create(width, height);
        return handle != IntPtr.Zero ? new TextureA(handle, true) : null;
    }

    
    
    
    public bool IsValid => _nativeHandle != IntPtr.Zero && Native.TextureA_IsValid(_nativeHandle);

    
    
    
    public uint Width
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_GetWidth(_nativeHandle);
        }
    }

    
    
    
    public uint Height
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_GetHeight(_nativeHandle);
        }
    }

    
    
    
    public uint Depth
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_GetDepth(_nativeHandle);
        }
    }

    
    
    
    public uint MipLevelCount
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_GetMipLevelCount(_nativeHandle);
        }
    }

    
    
    
    public uint SampleCount
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_GetSampleCount(_nativeHandle);
        }
    }

    
    
    
    public bool Is3D
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_Is3D(_nativeHandle);
        }
    }

    
    
    
    public bool IsArray
    {
        get
        {
            ThrowIfDisposed();
            return Native.TextureA_IsArray(_nativeHandle);
        }
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(TextureA));
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_ownsHandle && _nativeHandle != IntPtr.Zero)
            {
                Native.TextureA_Release(_nativeHandle);
            }
            _nativeHandle = IntPtr.Zero;
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~TextureA()
    {
        Dispose();
    }
}

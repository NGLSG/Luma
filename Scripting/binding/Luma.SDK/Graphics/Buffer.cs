using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Graphics;




[Flags]
public enum BufferUsage : uint
{
    None = 0,
    MapRead = 1 << 0,
    MapWrite = 1 << 1,
    CopySrc = 1 << 2,
    CopyDst = 1 << 3,
    Index = 1 << 4,
    Vertex = 1 << 5,
    Uniform = 1 << 6,
    Storage = 1 << 7,
    Indirect = 1 << 8,
    QueryResolve = 1 << 9,
}




public class GpuBuffer : IDisposable
{
    private IntPtr _nativeHandle;
    private bool _disposed;
    private bool _ownsHandle;

    
    
    
    public IntPtr NativeHandle => _nativeHandle;

    
    
    
    internal GpuBuffer(IntPtr nativeHandle, bool ownsHandle = true)
    {
        _nativeHandle = nativeHandle;
        _ownsHandle = ownsHandle;
    }

    
    
    
    
    
    
    public static GpuBuffer? Create(uint size, BufferUsage usage)
    {
        IntPtr handle = Native.GpuBuffer_Create(size, (uint)usage);
        return handle != IntPtr.Zero ? new GpuBuffer(handle, true) : null;
    }

    
    
    
    public static GpuBuffer? CreateUniform(uint size)
    {
        return Create(size, BufferUsage.Uniform | BufferUsage.CopyDst);
    }

    
    
    
    public static GpuBuffer? CreateVertex(uint size)
    {
        return Create(size, BufferUsage.Vertex | BufferUsage.CopyDst);
    }

    
    
    
    public static GpuBuffer? CreateIndex(uint size)
    {
        return Create(size, BufferUsage.Index | BufferUsage.CopyDst);
    }

    
    
    
    public static GpuBuffer? CreateStorage(uint size)
    {
        return Create(size, BufferUsage.Storage | BufferUsage.CopyDst | BufferUsage.CopySrc);
    }

    
    
    
    public bool IsValid => _nativeHandle != IntPtr.Zero && Native.GpuBuffer_IsValid(_nativeHandle);

    
    
    
    public uint Size
    {
        get
        {
            ThrowIfDisposed();
            return Native.GpuBuffer_GetSize(_nativeHandle);
        }
    }

    
    
    
    
    
    
    
    public bool Write(IntPtr data, uint size = 0, uint offset = 0)
    {
        ThrowIfDisposed();
        return Native.GpuBuffer_Write(_nativeHandle, data, size, offset);
    }

    
    
    
    
    public unsafe bool Write<T>(T value, uint offset = 0) where T : unmanaged
    {
        ThrowIfDisposed();
        return Native.GpuBuffer_Write(_nativeHandle, (IntPtr)(&value), (uint)sizeof(T), offset);
    }

    
    
    
    
    public unsafe bool Write<T>(T[] data, uint offset = 0) where T : unmanaged
    {
        ThrowIfDisposed();
        fixed (T* ptr = data)
        {
            return Native.GpuBuffer_Write(_nativeHandle, (IntPtr)ptr, (uint)(data.Length * sizeof(T)), offset);
        }
    }

    
    
    
    
    public unsafe bool Write<T>(ReadOnlySpan<T> data, uint offset = 0) where T : unmanaged
    {
        ThrowIfDisposed();
        fixed (T* ptr = data)
        {
            return Native.GpuBuffer_Write(_nativeHandle, (IntPtr)ptr, (uint)(data.Length * sizeof(T)), offset);
        }
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(GpuBuffer));
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_ownsHandle && _nativeHandle != IntPtr.Zero)
            {
                Native.GpuBuffer_Release(_nativeHandle);
            }
            _nativeHandle = IntPtr.Zero;
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~GpuBuffer()
    {
        Dispose();
    }
}

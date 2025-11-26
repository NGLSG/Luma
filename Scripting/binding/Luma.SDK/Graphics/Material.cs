using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Graphics;




public class Material : IDisposable
{
    private IntPtr _nativeHandle;
    private bool _disposed;

    
    
    
    public IntPtr NativeHandle => _nativeHandle;

    
    
    
    internal Material(IntPtr nativeHandle)
    {
        _nativeHandle = nativeHandle;
    }

    
    
    
    
    
    public static Material? Load(Guid assetGuid)
    {
        IntPtr handle = Native.WGSLMaterial_Load(assetGuid);
        return handle != IntPtr.Zero ? new Material(handle) : null;
    }

    
    
    
    public static Material? GetFromSprite(Entity entity)
    {
        IntPtr handle = Native.WGSLMaterial_GetFromSprite(entity.ScenePtr, entity.Id);
        return handle != IntPtr.Zero ? new Material(handle) : null;
    }

    
    
    
    public bool IsValid => _nativeHandle != IntPtr.Zero && Native.WGSLMaterial_IsValid(_nativeHandle);

    
    
    
    public void SetFloat(string name, float value)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetFloat(_nativeHandle, name, value);
    }

    
    
    
    public void SetInt(string name, int value)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetInt(_nativeHandle, name, value);
    }

    
    
    
    public void SetVec2(string name, float x, float y)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetVec2(_nativeHandle, name, x, y);
    }

    
    
    
    public void SetVec2(string name, Vector2 value)
    {
        SetVec2(name, value.X, value.Y);
    }

    
    
    
    public void SetVec3(string name, float x, float y, float z)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetVec3(_nativeHandle, name, x, y, z);
    }

    
    
    
    public void SetVec4(string name, float r, float g, float b, float a)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetVec4(_nativeHandle, name, r, g, b, a);
    }

    
    
    
    public void SetColor(string name, Color color)
    {
        SetVec4(name, color.R, color.G, color.B, color.A);
    }

    
    
    
    
    
    
    public void SetUniformData(string name, IntPtr data, int size)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetUniformStruct(_nativeHandle, name, data, size);
    }

    
    
    
    
    public unsafe void SetUniformData<T>(string name, T value) where T : unmanaged
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetUniformStruct(_nativeHandle, name, (IntPtr)(&value), sizeof(T));
    }

    
    
    
    
    
    
    
    public void SetTexture(string name, TextureA texture, uint binding = 1, uint group = 0)
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_SetTexture(_nativeHandle, name, texture.NativeHandle, binding, group);
    }

    
    
    
    
    public void UpdateUniformBuffer()
    {
        ThrowIfDisposed();
        Native.WGSLMaterial_UpdateUniformBuffer(_nativeHandle);
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(Material));
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            
            _nativeHandle = IntPtr.Zero;
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~Material()
    {
        Dispose();
    }
}

using System;

namespace Luma.SDK.Graphics;





public static class Camera
{
    
    
    
    public static Vector2 Position
    {
        get => new Vector2(Native.Camera_GetPositionX(), Native.Camera_GetPositionY());
        set => Native.Camera_SetPosition(value.X, value.Y);
    }

    
    
    
    public static float X
    {
        get => Native.Camera_GetPositionX();
        set => Native.Camera_SetPosition(value, Native.Camera_GetPositionY());
    }

    
    
    
    public static float Y
    {
        get => Native.Camera_GetPositionY();
        set => Native.Camera_SetPosition(Native.Camera_GetPositionX(), value);
    }

    
    
    
    
    public static float Zoom
    {
        get => Native.Camera_GetZoom();
        set => Native.Camera_SetZoom(value);
    }

    
    
    
    public static float Rotation
    {
        get => Native.Camera_GetRotation();
        set => Native.Camera_SetRotation(value);
    }

    
    
    
    public static float RotationDegrees
    {
        get => Native.Camera_GetRotation() * (180f / MathF.PI);
        set => Native.Camera_SetRotation(value * (MathF.PI / 180f));
    }

    
    
    
    public static float ViewportWidth => Native.Camera_GetViewportWidth();

    
    
    
    public static float ViewportHeight => Native.Camera_GetViewportHeight();

    
    
    
    public static Vector2 ViewportSize => new Vector2(ViewportWidth, ViewportHeight);

    
    
    
    public static Color ClearColor
    {
        get
        {
            Native.Camera_GetClearColor(out float r, out float g, out float b, out float a);
            return new Color(r, g, b, a);
        }
        set => Native.Camera_SetClearColor(value.R, value.G, value.B, value.A);
    }

    
    
    
    
    
    public static Vector2 ScreenToWorld(Vector2 screenPosition)
    {
        Native.Camera_ScreenToWorld(screenPosition.X, screenPosition.Y, out float worldX, out float worldY);
        return new Vector2(worldX, worldY);
    }

    
    
    
    
    
    
    public static Vector2 ScreenToWorld(float screenX, float screenY)
    {
        Native.Camera_ScreenToWorld(screenX, screenY, out float worldX, out float worldY);
        return new Vector2(worldX, worldY);
    }

    
    
    
    
    
    public static Vector2 WorldToScreen(Vector2 worldPosition)
    {
        Native.Camera_WorldToScreen(worldPosition.X, worldPosition.Y, out float screenX, out float screenY);
        return new Vector2(screenX, screenY);
    }

    
    
    
    
    
    
    public static Vector2 WorldToScreen(float worldX, float worldY)
    {
        Native.Camera_WorldToScreen(worldX, worldY, out float screenX, out float screenY);
        return new Vector2(screenX, screenY);
    }

    
    
    
    
    public static void Move(Vector2 delta)
    {
        var pos = Position;
        Position = new Vector2(pos.X + delta.X, pos.Y + delta.Y);
    }

    
    
    
    
    
    public static void Move(float deltaX, float deltaY)
    {
        var pos = Position;
        Position = new Vector2(pos.X + deltaX, pos.Y + deltaY);
    }

    
    
    
    
    
    public static void Follow(Vector2 targetPosition, float smoothing = 1f)
    {
        var currentPos = Position;
        float newX = currentPos.X + (targetPosition.X - currentPos.X) * smoothing;
        float newY = currentPos.Y + (targetPosition.Y - currentPos.Y) * smoothing;
        Position = new Vector2(newX, newY);
    }
}

namespace Luma.SDK;

public class RayHitResult
{
    public readonly Entity Entity;
    public readonly Vector2 Point;
    public readonly Vector2 Normal;
    public readonly float Fraction;

    public RayHitResult(Scene scene, RaycastHit hit)
    {
        Entity = new Entity(hit.hitEntityHandle, scene.ScenePtr);
        Point = hit.Point;
        Normal = hit.Normal;
        Fraction = hit.Fraction;
    }
}

public static class Physics
{
    public static bool Raycast(Vector2 start, Vector2 end, out RayHitResult hitResult)
    {
        IntPtr sceneHandle = SceneManager.CurrentScene.ScenePtr;
        if (sceneHandle == IntPtr.Zero)
        {
            hitResult = default;
            return false;
        }

        var results = new RaycastHit[1];
        if (Native.Physics_RayCast(sceneHandle, start, end, false, results, 1, out _))
        {
            hitResult = new RayHitResult(SceneManager.CurrentScene, results[0]);
            return true;
        }

        hitResult = default;
        return false;
    }

    public static RayHitResult[] RaycastAll(Vector2 start, Vector2 end)
    {
        IntPtr sceneHandle = SceneManager.CurrentScene.ScenePtr;
        if (sceneHandle == IntPtr.Zero) return Array.Empty<RayHitResult>();

        if (!Native.Physics_RayCast(sceneHandle, start, end, true, null, 0, out int hitCount) || hitCount == 0)
        {
            return Array.Empty<RayHitResult>();
        }

        var cRes = new RaycastHit[hitCount];
        Native.Physics_RayCast(sceneHandle, start, end, true, cRes, hitCount, out _);
        var res = new RayHitResult[hitCount];
        for (int i = 0; i < hitCount; i++)
        {
            res[i] = new RayHitResult(SceneManager.CurrentScene, cRes[i]);
        }

        return res;
    }

    
    
    
    
    
    
    public static Entity[] OverlapCircle(Vector2 center, float radius)
    {
        return OverlapCircleWithTags(center, radius, null);
    }

    
    
    
    
    
    
    
    public static Entity[] OverlapCircleWithTags(Vector2 center, float radius, params string[]? tags)
    {
        IntPtr sceneHandle = SceneManager.CurrentScene.ScenePtr;
        if (sceneHandle == IntPtr.Zero) return Array.Empty<Entity>();

        int tagCount = tags?.Length ?? 0;

        
        if (!Native.Physics_OverlapCircle(sceneHandle, center, radius, tags, tagCount, null!, 0, out int hitCount) || hitCount == 0)
        {
            return Array.Empty<Entity>();
        }

        
        var entityHandles = new uint[hitCount];
        Native.Physics_OverlapCircle(sceneHandle, center, radius, tags, tagCount, entityHandles, hitCount, out _);

        var entities = new Entity[hitCount];
        for (int i = 0; i < hitCount; i++)
        {
            entities[i] = new Entity(entityHandles[i], sceneHandle);
        }

        return entities;
    }

    
    
    
    
    
    
    public static bool OverlapCircleAny(Vector2 center, float radius)
    {
        return OverlapCircleAnyWithTags(center, radius, null);
    }

    
    
    
    
    
    
    
    public static bool OverlapCircleAnyWithTags(Vector2 center, float radius, params string[]? tags)
    {
        IntPtr sceneHandle = SceneManager.CurrentScene.ScenePtr;
        if (sceneHandle == IntPtr.Zero) return false;

        int tagCount = tags?.Length ?? 0;
        return Native.Physics_OverlapCircle(sceneHandle, center, radius, tags, tagCount, null!, 0, out int hitCount) && hitCount > 0;
    }

    
    
    
    
    
    
    
    public static bool OverlapCircleFirst(Vector2 center, float radius, out Entity entity)
    {
        return OverlapCircleFirstWithTags(center, radius, out entity, null);
    }

    
    
    
    
    
    
    
    
    public static bool OverlapCircleFirstWithTags(Vector2 center, float radius, out Entity entity, params string[]? tags)
    {
        IntPtr sceneHandle = SceneManager.CurrentScene.ScenePtr;
        if (sceneHandle == IntPtr.Zero)
        {
            entity = default!;
            return false;
        }

        int tagCount = tags?.Length ?? 0;
        var entityHandles = new uint[1];
        
        if (Native.Physics_OverlapCircle(sceneHandle, center, radius, tags, tagCount, entityHandles, 1, out int hitCount) && hitCount > 0)
        {
            entity = new Entity(entityHandles[0], sceneHandle);
            return true;
        }

        entity = default!;
        return false;
    }
}
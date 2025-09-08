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
}
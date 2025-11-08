using System;

namespace Luma.SDK.Platform;

public static class AndroidPermissions
{
    public static bool HasPermissions(params string[] permissions)
    {
        if (permissions == null || permissions.Length == 0)
        {
            return true;
        }

        return Native.Platform_HasPermissions(permissions, permissions.Length);
    }

    public static bool AcquirePermissions(params string[] permissions)
    {
        if (permissions == null || permissions.Length == 0)
        {
            return true;
        }

        return Native.Platform_RequestPermissions(permissions, permissions.Length);
    }
}

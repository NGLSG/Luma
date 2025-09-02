using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Luma.SDK;






internal static class DomainManager
{
    private static readonly object s_lock = new();
    private static ScriptLoadContext? s_alc;
    private static string? s_baseDirectory;
    private static readonly Dictionary<string, Assembly> s_loadedAssemblies = new(StringComparer.OrdinalIgnoreCase);

    public static bool IsActive
    {
        get { lock (s_lock) return s_alc != null; }
    }

    public static void Initialize(string baseDirectory)
    {
        lock (s_lock)
        {
            if (s_alc != null)
            {
                Unload_NoLock();
            }

            s_baseDirectory = baseDirectory;
            s_alc = new ScriptLoadContext(baseDirectory);
            s_loadedAssemblies.Clear();
        }
    }

    public static void Unload()
    {
        lock (s_lock)
        {
            Unload_NoLock();
        }
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    public static Type? ResolveType(string assemblyName, string typeName)
    {
        lock (s_lock)
        {
            if (s_alc == null || s_baseDirectory == null)
                return null;

            Assembly? asm = LoadAssembly_NoLock(assemblyName);
            return asm?.GetType(typeName, throwOnError: false, ignoreCase: false);
        }
    }

    public static Assembly? LoadAssemblyFromPath(string absolutePath)
    {
        lock (s_lock)
        {
            if (s_alc == null) return null;
            if (!Path.IsPathRooted(absolutePath)) return null;

            try
            {
                string asmName = Path.GetFileNameWithoutExtension(absolutePath);
                if (s_loadedAssemblies.TryGetValue(asmName, out Assembly? cached))
                    return cached;

                var asm = LoadByStream_NoLock(absolutePath);
                if (asm != null)
                {
                    s_loadedAssemblies[asmName] = asm;
                }
                return asm;
            }
            catch
            {
                return null;
            }
        }
    }

    private static void Unload_NoLock()
    {
        if (s_alc == null) return;

        s_loadedAssemblies.Clear();
        s_alc.Unload();
        s_alc = null;
        s_baseDirectory = null;
    }

    private static Assembly? LoadAssembly_NoLock(string assemblyName)
    {
        if (s_alc == null || s_baseDirectory == null) return null;

        if (s_loadedAssemblies.TryGetValue(assemblyName, out Assembly? cached))
            return cached;

        string candidate = Path.Combine(s_baseDirectory, assemblyName + ".dll");
        if (!File.Exists(candidate))
        {

            if (File.Exists(assemblyName))
                candidate = assemblyName;
            else
                return null;
        }

        var asm = LoadByStream_NoLock(candidate);
        if (asm != null)
        {
            s_loadedAssemblies[assemblyName] = asm;
        }
        return asm;
    }

    private static Assembly? LoadByStream_NoLock(string path)
    {
        if (s_alc == null) return null;

        try
        {
            using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            using var ms = new MemoryStream();
            fs.CopyTo(ms);
            ms.Position = 0;

            string pdbPath = Path.ChangeExtension(path, ".pdb");
            if (File.Exists(pdbPath))
            {
                using var fps = new FileStream(pdbPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                using var mps = new MemoryStream();
                fps.CopyTo(mps);
                mps.Position = 0;
                return s_alc.LoadFromStream(ms, mps);
            }

            return s_alc.LoadFromStream(ms);
        }
        catch
        {
            return null;
        }
    }
}
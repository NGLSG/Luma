using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;

namespace Luma.SDK;





internal sealed class ScriptLoadContext : AssemblyLoadContext
{
    private readonly string baseDirectory;

    public ScriptLoadContext(string baseDirectory)
        : base(isCollectible: true)
    {
        this.baseDirectory = baseDirectory;
        Resolving += OnResolving;
    }

    private Assembly? OnResolving(AssemblyLoadContext context, AssemblyName assemblyName)
    {
        try
        {

            if (string.Equals(assemblyName.Name, "Luma.SDK", StringComparison.OrdinalIgnoreCase))
            {
                var shared = AssemblyLoadContext.Default.Assemblies
                    .FirstOrDefault(a => string.Equals(a.GetName().Name, "Luma.SDK", StringComparison.OrdinalIgnoreCase));
                if (shared != null)
                    return shared;
            }


            string candidate = Path.Combine(baseDirectory, assemblyName.Name + ".dll");
            if (File.Exists(candidate))
            {
                return LoadFromFileNoLock(candidate);
            }
        }
        catch
        {

        }
        return null;
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        try
        {

            if (string.Equals(assemblyName.Name, "Luma.SDK", StringComparison.OrdinalIgnoreCase))
            {
                var shared = AssemblyLoadContext.Default.Assemblies
                    .FirstOrDefault(a => string.Equals(a.GetName().Name, "Luma.SDK", StringComparison.OrdinalIgnoreCase));
                if (shared != null)
                    return shared;
            }

            string candidate = Path.Combine(baseDirectory, assemblyName.Name + ".dll");
            if (File.Exists(candidate))
            {
                return LoadFromFileNoLock(candidate);
            }
        }
        catch
        {

        }
        return null;
    }


    private Assembly LoadFromFileNoLock(string path)
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
            return LoadFromStream(ms, mps);
        }
        return LoadFromStream(ms);
    }
}
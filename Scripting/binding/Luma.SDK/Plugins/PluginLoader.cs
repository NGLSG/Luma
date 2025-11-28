using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;

namespace Luma.SDK.Plugins;




public static class PluginLoader
{
    private static readonly Dictionary<string, LoadedPlugin> _loadedPlugins = new();
    private static readonly List<EditorPlugin> _editorPlugins = new();

    
    
    
    public static IReadOnlyList<EditorPlugin> EditorPlugins => _editorPlugins;

    
    
    
    
    
    
    public static bool LoadPlugin(string dllPath, string pluginId)
    {
        if (_loadedPlugins.ContainsKey(pluginId))
        {
            Console.WriteLine($"[PluginLoader] 插件已加载: {pluginId}");
            return true;
        }

        try
        {
            var context = new PluginLoadContext(dllPath);
            var assembly = context.LoadFromAssemblyPath(dllPath);

            var pluginTypes = assembly.GetTypes()
                .Where(t => !t.IsAbstract && typeof(PluginBase).IsAssignableFrom(t))
                .ToList();

            if (pluginTypes.Count == 0)
            {
                Console.WriteLine($"[PluginLoader] 未找到插件类: {dllPath}");
                context.Unload();
                return false;
            }

            var loadedPlugin = new LoadedPlugin
            {
                Id = pluginId,
                Context = context,
                Assembly = assembly,
                Plugins = new List<PluginBase>()
            };

            foreach (var pluginType in pluginTypes)
            {
                try
                {
                    var instance = (PluginBase)Activator.CreateInstance(pluginType)!;
                    instance.Loaded = true;
                    instance.Enabled = true;
                    instance.OnLoad();
                    instance.OnEnable();
                    loadedPlugin.Plugins.Add(instance);

                    if (instance is EditorPlugin editorPlugin)
                    {
                        _editorPlugins.Add(editorPlugin);
                    }

                    Console.WriteLine($"[PluginLoader] 已加载插件: {instance.Name} v{instance.Version}");
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[PluginLoader] 创建插件实例失败: {pluginType.Name} - {e.Message}");
                }
            }

            _loadedPlugins[pluginId] = loadedPlugin;
            return true;
        }
        catch (Exception e)
        {
            Console.WriteLine($"[PluginLoader] 加载插件失败: {dllPath} - {e.Message}");
            return false;
        }
    }

    
    
    
    
    
    public static bool UnloadPlugin(string pluginId)
    {
        if (!_loadedPlugins.TryGetValue(pluginId, out var loadedPlugin))
        {
            return false;
        }

        foreach (var plugin in loadedPlugin.Plugins)
        {
            try
            {
                plugin.OnDisable();
                plugin.OnUnload();
                plugin.Loaded = false;
                plugin.Enabled = false;

                if (plugin is EditorPlugin editorPlugin)
                {
                    _editorPlugins.Remove(editorPlugin);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[PluginLoader] 卸载插件失败: {plugin.Name} - {e.Message}");
            }
        }

        loadedPlugin.Context.Unload();
        _loadedPlugins.Remove(pluginId);

        Console.WriteLine($"[PluginLoader] 已卸载插件: {pluginId}");
        return true;
    }

    
    
    
    public static void UnloadAllPlugins()
    {
        var pluginIds = _loadedPlugins.Keys.ToList();
        foreach (var id in pluginIds)
        {
            UnloadPlugin(id);
        }
    }

    
    
    
    public static void UpdateEditorPlugins(float deltaTime)
    {
        foreach (var plugin in _editorPlugins)
        {
            if (plugin.Enabled)
            {
                try
                {
                    plugin.OnEditorUpdate(deltaTime);
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[PluginLoader] 插件更新错误: {plugin.Name} - {e.Message}");
                }
            }
        }
    }

    
    
    
    public static void DrawEditorPluginPanels()
    {
        foreach (var plugin in _editorPlugins)
        {
            if (!plugin.Enabled) continue;

            try
            {
                plugin.OnEditorGUI();

                foreach (var panel in plugin.Panels)
                {
                    if (!panel.IsVisible) continue;

                    bool isOpen = panel.IsVisible;
                    if (ImGui.Begin(panel.Title, ref isOpen))
                    {
                        panel.OnGUI();
                    }
                    ImGui.End();

                    if (panel.IsCloseable)
                    {
                        panel.IsVisible = isOpen;
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[PluginLoader] 插件 GUI 错误: {plugin.Name} - {e.Message}");
            }
        }
    }

    
    
    
    public static void DrawPluginMenuBar()
    {
        foreach (var plugin in _editorPlugins)
        {
            if (plugin.Enabled)
            {
                try
                {
                    plugin.OnMenuBarGUI();
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[PluginLoader] 插件菜单错误: {plugin.Name} - {e.Message}");
                }
            }
        }
    }

    
    
    
    
    public static void DrawMenuItems(string menuName)
    {
        foreach (var plugin in _editorPlugins)
        {
            if (plugin.Enabled)
            {
                try
                {
                    plugin.OnDrawMenuItems(menuName);
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[PluginLoader] 插件菜单项错误: {plugin.Name} - {e.Message}");
                }
            }
        }
    }

    private class LoadedPlugin
    {
        public string Id { get; set; } = "";
        public AssemblyLoadContext Context { get; set; } = null!;
        public Assembly Assembly { get; set; } = null!;
        public List<PluginBase> Plugins { get; set; } = new();
    }
}




internal class PluginLoadContext : AssemblyLoadContext
{
    private readonly AssemblyDependencyResolver _resolver;

    public PluginLoadContext(string pluginPath) : base(isCollectible: true)
    {
        _resolver = new AssemblyDependencyResolver(pluginPath);
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        
        if (assemblyName.Name == "Luma.SDK")
        {
            return null; 
        }

        string? assemblyPath = _resolver.ResolveAssemblyToPath(assemblyName);
        if (assemblyPath != null)
        {
            return LoadFromAssemblyPath(assemblyPath);
        }

        return null;
    }

    protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
    {
        string? libraryPath = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
        if (libraryPath != null)
        {
            return LoadUnmanagedDllFromPath(libraryPath);
        }

        return IntPtr.Zero;
    }
}

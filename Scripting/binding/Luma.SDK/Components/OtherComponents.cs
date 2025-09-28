using Luma.SDK;
using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct ActivityComponent : IComponent
{
    [MarshalAs(UnmanagedType.I1)] public bool Enable;
    [MarshalAs(UnmanagedType.I1)] public bool IsActive;
}

public sealed class LogicActivityComponent : LogicComponent<ActivityComponent>
{
    private const string ComponentName = "ActivityComponent";

    public LogicActivityComponent(Entity entity) : base(entity)
    {
    }

    public bool IsActive
    {
        get => _component.IsActive;
        set
        {
            _component.IsActive = value;
            Entity.SetComponentProperty(ComponentName, "isActive", value);
        }
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct PrefabComponent : IComponent
{
    [MarshalAs(UnmanagedType.I1)] public bool Enable;
    public AssetHandle SourcePrefab;
}

public sealed class LogicPrefabComponent : LogicComponent<PrefabComponent>
{
    private const string ComponentName = "PrefabComponent";

    public LogicPrefabComponent(Entity entity) : base(entity)
    {
    }

    public AssetHandle SourcePrefab
    {
        get => _component.SourcePrefab;
        set
        {
            _component.SourcePrefab = value;
            Entity.SetComponentProperty(ComponentName, "sourcePrefab", value);
        }
    }
}

public sealed class TilemapComponentLogic : ILogicComponent
{
    private const string ComponentName = "TilemapComponent";

    public Entity Entity { get; set; }

    public TilemapComponentLogic(Entity entity)
    {
        Entity = entity;
    }

    public Vector2 CellSize
    {
        get => Entity.GetComponentProperty<Vector2>(ComponentName, "cellSize");
        set => Entity.SetComponentProperty(ComponentName, "cellSize", value);
    }
}

public sealed class TilemapRendererComponent : ILogicComponent
{
    private const string ComponentName = "TilemapRendererComponent";

    public Entity Entity { get; set; }

    public TilemapRendererComponent(Entity entity)
    {
        Entity = entity;
    }

    public int ZIndex
    {
        get => Entity.GetComponentProperty<int>(ComponentName, "zIndex");
        set => Entity.SetComponentProperty(ComponentName, "zIndex", value);
    }

    public AssetHandle MaterialHandle
    {
        get => Entity.GetComponentProperty<AssetHandle>(ComponentName, "materialHandle");
        set => Entity.SetComponentProperty(ComponentName, "materialHandle", value);
    }
}



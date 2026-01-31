using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

#region Enums

/// <summary>
/// 光源类型
/// </summary>
public enum LightType : byte
{
    Point = 0,
    Spot = 1,
    Directional = 2
}

/// <summary>
/// 光照衰减类型
/// </summary>
public enum AttenuationType : byte
{
    Linear = 0,
    Quadratic = 1,
    InverseSquare = 2
}

/// <summary>
/// 面光源形状
/// </summary>
public enum AreaLightShape : byte
{
    Rectangle = 0,
    Circle = 1
}

/// <summary>
/// 环境光区域形状
/// </summary>
public enum AmbientZoneShape : byte
{
    Rectangle = 0,
    Circle = 1
}

/// <summary>
/// 环境光渐变模式
/// </summary>
public enum AmbientGradientMode : byte
{
    None = 0,
    Vertical = 1,
    Horizontal = 2
}

#endregion

#region Data Components

/// <summary>
/// 方向光组件数据结构 - 与C++ DirectionalLightComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DirectionalLightComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color color;
    public float intensity;
    public Vector2 direction;
    public uint layerMask;
    [MarshalAs(UnmanagedType.U1)] public bool castShadows;
}

/// <summary>
/// 点光源组件数据结构 - 与C++ PointLightComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct PointLightComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color color;
    public float intensity;
    public float radius;
    public AttenuationType attenuation;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;
    public uint layerMask;
    public int priority;
    [MarshalAs(UnmanagedType.U1)] public bool castShadows;
}


/// <summary>
/// 聚光灯组件数据结构 - 与C++ SpotLightComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SpotLightComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color color;
    public float intensity;
    public float radius;
    public float innerAngle;
    public float outerAngle;
    public AttenuationType attenuation;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;
    public uint layerMask;
    public int priority;
    [MarshalAs(UnmanagedType.U1)] public bool castShadows;
}

/// <summary>
/// 面光源组件数据结构 - 与C++ AreaLightComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct AreaLightComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color color;
    public float intensity;
    public AreaLightShape shape;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;
    public float width;
    public float height;
    public float radius;
    public AttenuationType attenuation;
    private byte _pad4;
    private byte _pad5;
    private byte _pad6;
    public uint layerMask;
    public int priority;
    [MarshalAs(UnmanagedType.U1)] public bool castShadows;
    public float shadowSoftness;
}

/// <summary>
/// 光照探针组件数据结构 - 与C++ LightProbeComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct LightProbeComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color sampledColor;
    public float sampledIntensity;
    public float influenceRadius;
    [MarshalAs(UnmanagedType.U1)] public bool isBaked;
    public uint layerMask;
}

#endregion

#region Logic Components

/// <summary>
/// 方向光逻辑组件 - 模拟无限远处的光源（如太阳）
/// </summary>
public sealed class DirectionalLight : LogicComponent<DirectionalLightComponent>
{
    private const string ComponentName = "DirectionalLightComponent";

    public DirectionalLight(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color Color
    {
        get => _component.color;
        set
        {
            _component.color = value;
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    public float Intensity
    {
        get => _component.intensity;
        set
        {
            _component.intensity = value;
            Entity.SetComponentProperty(ComponentName, "intensity", in value);
        }
    }

    public Vector2 Direction
    {
        get => _component.direction;
        set
        {
            _component.direction = value;
            Entity.SetComponentProperty(ComponentName, "direction", in value);
        }
    }

    public uint LayerMask
    {
        get => _component.layerMask;
        set
        {
            _component.layerMask = value;
            Entity.SetComponentProperty(ComponentName, "layerMask", in value);
        }
    }

    public bool CastShadows
    {
        get => _component.castShadows;
        set
        {
            _component.castShadows = value;
            Entity.SetComponentProperty(ComponentName, "castShadows", in value);
        }
    }
}


/// <summary>
/// 点光源逻辑组件 - 从一个点向所有方向发射光线
/// </summary>
public sealed class PointLight : LogicComponent<PointLightComponent>
{
    private const string ComponentName = "PointLightComponent";

    public PointLight(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color Color
    {
        get => _component.color;
        set
        {
            _component.color = value;
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    public float Intensity
    {
        get => _component.intensity;
        set
        {
            _component.intensity = value;
            Entity.SetComponentProperty(ComponentName, "intensity", in value);
        }
    }

    public float Radius
    {
        get => _component.radius;
        set
        {
            _component.radius = value;
            Entity.SetComponentProperty(ComponentName, "radius", in value);
        }
    }

    public AttenuationType Attenuation
    {
        get => _component.attenuation;
        set
        {
            _component.attenuation = value;
            Entity.SetComponentProperty(ComponentName, "attenuation", in value);
        }
    }

    public uint LayerMask
    {
        get => _component.layerMask;
        set
        {
            _component.layerMask = value;
            Entity.SetComponentProperty(ComponentName, "layerMask", in value);
        }
    }

    public int Priority
    {
        get => _component.priority;
        set
        {
            _component.priority = value;
            Entity.SetComponentProperty(ComponentName, "priority", in value);
        }
    }

    public bool CastShadows
    {
        get => _component.castShadows;
        set
        {
            _component.castShadows = value;
            Entity.SetComponentProperty(ComponentName, "castShadows", in value);
        }
    }
}

/// <summary>
/// 聚光灯逻辑组件 - 从一个点向特定方向发射锥形光线
/// </summary>
public sealed class SpotLight : LogicComponent<SpotLightComponent>
{
    private const string ComponentName = "SpotLightComponent";

    public SpotLight(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color Color
    {
        get => _component.color;
        set
        {
            _component.color = value;
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    public float Intensity
    {
        get => _component.intensity;
        set
        {
            _component.intensity = value;
            Entity.SetComponentProperty(ComponentName, "intensity", in value);
        }
    }

    public float Radius
    {
        get => _component.radius;
        set
        {
            _component.radius = value;
            Entity.SetComponentProperty(ComponentName, "radius", in value);
        }
    }

    public float InnerAngle
    {
        get => _component.innerAngle;
        set
        {
            _component.innerAngle = value;
            Entity.SetComponentProperty(ComponentName, "innerAngle", in value);
        }
    }

    public float OuterAngle
    {
        get => _component.outerAngle;
        set
        {
            _component.outerAngle = value;
            Entity.SetComponentProperty(ComponentName, "outerAngle", in value);
        }
    }

    public AttenuationType Attenuation
    {
        get => _component.attenuation;
        set
        {
            _component.attenuation = value;
            Entity.SetComponentProperty(ComponentName, "attenuation", in value);
        }
    }

    public uint LayerMask
    {
        get => _component.layerMask;
        set
        {
            _component.layerMask = value;
            Entity.SetComponentProperty(ComponentName, "layerMask", in value);
        }
    }

    public int Priority
    {
        get => _component.priority;
        set
        {
            _component.priority = value;
            Entity.SetComponentProperty(ComponentName, "priority", in value);
        }
    }

    public bool CastShadows
    {
        get => _component.castShadows;
        set
        {
            _component.castShadows = value;
            Entity.SetComponentProperty(ComponentName, "castShadows", in value);
        }
    }
}


/// <summary>
/// 面光源逻辑组件 - 从矩形或圆形区域发射光线，产生柔和照明效果
/// </summary>
public sealed class AreaLight : LogicComponent<AreaLightComponent>
{
    private const string ComponentName = "AreaLightComponent";

    public AreaLight(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color Color
    {
        get => _component.color;
        set
        {
            _component.color = value;
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    public float Intensity
    {
        get => _component.intensity;
        set
        {
            _component.intensity = value;
            Entity.SetComponentProperty(ComponentName, "intensity", in value);
        }
    }

    public AreaLightShape Shape
    {
        get => _component.shape;
        set
        {
            _component.shape = value;
            Entity.SetComponentProperty(ComponentName, "shape", in value);
        }
    }

    public float Width
    {
        get => _component.width;
        set
        {
            _component.width = value;
            Entity.SetComponentProperty(ComponentName, "width", in value);
        }
    }

    public float Height
    {
        get => _component.height;
        set
        {
            _component.height = value;
            Entity.SetComponentProperty(ComponentName, "height", in value);
        }
    }

    public float Radius
    {
        get => _component.radius;
        set
        {
            _component.radius = value;
            Entity.SetComponentProperty(ComponentName, "radius", in value);
        }
    }

    public AttenuationType Attenuation
    {
        get => _component.attenuation;
        set
        {
            _component.attenuation = value;
            Entity.SetComponentProperty(ComponentName, "attenuation", in value);
        }
    }

    public uint LayerMask
    {
        get => _component.layerMask;
        set
        {
            _component.layerMask = value;
            Entity.SetComponentProperty(ComponentName, "layerMask", in value);
        }
    }

    public int Priority
    {
        get => _component.priority;
        set
        {
            _component.priority = value;
            Entity.SetComponentProperty(ComponentName, "priority", in value);
        }
    }

    public bool CastShadows
    {
        get => _component.castShadows;
        set
        {
            _component.castShadows = value;
            Entity.SetComponentProperty(ComponentName, "castShadows", in value);
        }
    }

    public float ShadowSoftness
    {
        get => _component.shadowSoftness;
        set
        {
            _component.shadowSoftness = value;
            Entity.SetComponentProperty(ComponentName, "shadowSoftness", in value);
        }
    }
}

/// <summary>
/// 光照探针逻辑组件 - 在场景中采样点存储光照信息，用于间接光照计算
/// </summary>
public sealed class LightProbe : LogicComponent<LightProbeComponent>
{
    private const string ComponentName = "LightProbeComponent";

    public LightProbe(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color SampledColor
    {
        get => _component.sampledColor;
        set
        {
            _component.sampledColor = value;
            Entity.SetComponentProperty(ComponentName, "sampledColor", in value);
        }
    }

    public float SampledIntensity
    {
        get => _component.sampledIntensity;
        set
        {
            _component.sampledIntensity = value;
            Entity.SetComponentProperty(ComponentName, "sampledIntensity", in value);
        }
    }

    public float InfluenceRadius
    {
        get => _component.influenceRadius;
        set
        {
            _component.influenceRadius = value;
            Entity.SetComponentProperty(ComponentName, "influenceRadius", in value);
        }
    }

    public bool IsBaked
    {
        get => _component.isBaked;
        set
        {
            _component.isBaked = value;
            Entity.SetComponentProperty(ComponentName, "isBaked", in value);
        }
    }

    public uint LayerMask
    {
        get => _component.layerMask;
        set
        {
            _component.layerMask = value;
            Entity.SetComponentProperty(ComponentName, "layerMask", in value);
        }
    }
}

#endregion


#region Shadow Caster

/// <summary>
/// 阴影形状枚举
/// </summary>
public enum ShadowShape : byte
{
    Auto = 0,
    Rectangle = 1,
    Circle = 2,
    Polygon = 3
}

/// <summary>
/// 阴影投射器组件数据结构 - 与C++ ShadowCasterComponent 匹配
/// 注意: vertices 和 sdfData 是复杂类型，无法直接映射，通过属性API访问
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct ShadowCasterComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public ShadowShape shape;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;
    public float opacity;
    [MarshalAs(UnmanagedType.U1)] public bool selfShadow;
    public float circleRadius;
    public Vector2 rectangleSize;
    public Vector2 offset;

    // SDF 阴影支持
    [MarshalAs(UnmanagedType.U1)] public bool enableSDF;
    public int sdfResolution;
    public float sdfPadding;

    // 阴影缓存支持
    [MarshalAs(UnmanagedType.U1)] public bool enableCache;
    [MarshalAs(UnmanagedType.U1)] public bool isStatic;
}

/// <summary>
/// 阴影投射器逻辑组件 - 标记实体可以投射阴影
/// </summary>
public sealed class ShadowCaster : LogicComponent<ShadowCasterComponent>
{
    private const string ComponentName = "ShadowCasterComponent";

    public ShadowCaster(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public ShadowShape Shape
    {
        get => _component.shape;
        set
        {
            _component.shape = value;
            Entity.SetComponentProperty(ComponentName, "shape", in value);
        }
    }

    public float Opacity
    {
        get => _component.opacity;
        set
        {
            _component.opacity = value;
            Entity.SetComponentProperty(ComponentName, "opacity", in value);
        }
    }

    public bool SelfShadow
    {
        get => _component.selfShadow;
        set
        {
            _component.selfShadow = value;
            Entity.SetComponentProperty(ComponentName, "selfShadow", in value);
        }
    }

    public float CircleRadius
    {
        get => _component.circleRadius;
        set
        {
            _component.circleRadius = value;
            Entity.SetComponentProperty(ComponentName, "circleRadius", in value);
        }
    }

    public Vector2 RectangleSize
    {
        get => _component.rectangleSize;
        set
        {
            _component.rectangleSize = value;
            Entity.SetComponentProperty(ComponentName, "rectangleSize", in value);
        }
    }

    public Vector2 Offset
    {
        get => _component.offset;
        set
        {
            _component.offset = value;
            Entity.SetComponentProperty(ComponentName, "offset", in value);
        }
    }

    public bool EnableSDF
    {
        get => _component.enableSDF;
        set
        {
            _component.enableSDF = value;
            Entity.SetComponentProperty(ComponentName, "enableSDF", in value);
        }
    }

    public int SdfResolution
    {
        get => _component.sdfResolution;
        set
        {
            _component.sdfResolution = value;
            Entity.SetComponentProperty(ComponentName, "sdfResolution", in value);
        }
    }

    public float SdfPadding
    {
        get => _component.sdfPadding;
        set
        {
            _component.sdfPadding = value;
            Entity.SetComponentProperty(ComponentName, "sdfPadding", in value);
        }
    }

    public bool EnableCache
    {
        get => _component.enableCache;
        set
        {
            _component.enableCache = value;
            Entity.SetComponentProperty(ComponentName, "enableCache", in value);
        }
    }

    public bool IsStatic
    {
        get => _component.isStatic;
        set
        {
            _component.isStatic = value;
            Entity.SetComponentProperty(ComponentName, "isStatic", in value);
        }
    }
}

#endregion

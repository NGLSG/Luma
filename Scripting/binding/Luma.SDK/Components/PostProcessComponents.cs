using System.Runtime.InteropServices;

namespace Luma.SDK.Components;

#region Enums

/// <summary>
/// 色调映射模式
/// </summary>
public enum ToneMappingMode : byte
{
    None = 0,
    Reinhard = 1,
    ACES = 2,
    Filmic = 3
}

/// <summary>
/// 雾效模式
/// </summary>
public enum FogMode : byte
{
    Linear = 0,
    Exponential = 1,
    ExponentialSquared = 2
}

/// <summary>
/// 质量等级
/// </summary>
public enum QualityLevel : byte
{
    Low = 0,
    Medium = 1,
    High = 2,
    Ultra = 3,
    Custom = 4
}

/// <summary>
/// 阴影方法
/// </summary>
public enum ShadowMethod : byte
{
    Basic = 0,
    SDF = 1,
    ScreenSpace = 2
}

#endregion


#region Data Components

/// <summary>
/// 后处理设置组件数据结构 - 与C++ PostProcessSettingsComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct PostProcessSettingsComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    // Bloom 设置
    [MarshalAs(UnmanagedType.U1)] public bool enableBloom;
    public float bloomThreshold;
    public float bloomIntensity;
    public float bloomRadius;
    public int bloomIterations;
    public Color bloomTint;

    // 光束设置
    [MarshalAs(UnmanagedType.U1)] public bool enableLightShafts;
    public float lightShaftDensity;
    public float lightShaftDecay;
    public float lightShaftWeight;
    public float lightShaftExposure;

    // 雾效设置
    [MarshalAs(UnmanagedType.U1)] public bool enableFog;
    public FogMode fogMode;
    private byte _pad1;
    private byte _pad2;
    public Color fogColor;
    public float fogDensity;
    public float fogStart;
    public float fogEnd;
    [MarshalAs(UnmanagedType.U1)] public bool enableHeightFog;
    public float heightFogBase;
    public float heightFogDensity;

    // 色调映射设置
    public ToneMappingMode toneMappingMode;
    private byte _pad3;
    private byte _pad4;
    private byte _pad5;
    public float exposure;
    public float contrast;
    public float saturation;
    public float gamma;

    // LUT 颜色分级设置
    [MarshalAs(UnmanagedType.U1)] public bool enableColorGrading;
    public float lutIntensity;
}

/// <summary>
/// 质量设置组件数据结构 - 与C++ QualitySettingsComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct QualitySettingsComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    // 质量等级
    public QualityLevel level;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;

    // 光照设置
    public int maxLightsPerFrame;
    public int maxLightsPerPixel;
    [MarshalAs(UnmanagedType.U1)] public bool enableAreaLights;
    [MarshalAs(UnmanagedType.U1)] public bool enableIndirectLighting;

    // 阴影设置
    public ShadowMethod shadowMethod;
    private byte _pad4;
    private byte _pad5;
    private byte _pad6;
    public int shadowMapResolution;
    [MarshalAs(UnmanagedType.U1)] public bool enableShadowCache;

    // 后处理设置
    [MarshalAs(UnmanagedType.U1)] public bool enableBloom;
    [MarshalAs(UnmanagedType.U1)] public bool enableLightShafts;
    [MarshalAs(UnmanagedType.U1)] public bool enableFog;
    [MarshalAs(UnmanagedType.U1)] public bool enableColorGrading;
    public float renderScale;

    // 自动质量调整
    [MarshalAs(UnmanagedType.U1)] public bool enableAutoQuality;
    public float targetFrameRate;
    public float qualityAdjustThreshold;
}

/// <summary>
/// 光照设置组件数据结构 - 与C++ LightingSettingsComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct LightingSettingsComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public Color ambientColor;
    public float ambientIntensity;
    public int maxLightsPerPixel;
    [MarshalAs(UnmanagedType.U1)] public bool enableShadows;
    public float shadowSoftness;
    [MarshalAs(UnmanagedType.U1)] public bool enableNormalMapping;

    // 阴影贴图配置
    public uint shadowMapResolution;
    public uint maxShadowCasters;
    public float shadowBias;
    public float shadowNormalBias;

    // 间接光照配置
    [MarshalAs(UnmanagedType.U1)] public bool enableIndirectLighting;
    public float indirectIntensity;
    public float bounceDecay;
    public float indirectRadius;
}

/// <summary>
/// 环境区域组件数据结构 - 与C++ AmbientZoneComponent 匹配
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct AmbientZoneComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;

    public AmbientZoneShape shape;
    private byte _pad1;
    private byte _pad2;
    private byte _pad3;
    public float width;
    public float height;
    public Color primaryColor;
    public Color secondaryColor;
    public AmbientGradientMode gradientMode;
    private byte _pad4;
    private byte _pad5;
    private byte _pad6;
    public float intensity;
    public float edgeSoftness;
    public int priority;
    public float blendWeight;
}

#endregion


#region Logic Components

/// <summary>
/// 后处理设置逻辑组件 - 控制场景的后处理效果
/// </summary>
public sealed class PostProcessSettings : LogicComponent<PostProcessSettingsComponent>
{
    private const string ComponentName = "PostProcessSettingsComponent";

    public PostProcessSettings(Entity entity) : base(entity) { }

    #region Enable

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    #endregion

    #region Bloom

    public bool EnableBloom
    {
        get => _component.enableBloom;
        set
        {
            _component.enableBloom = value;
            Entity.SetComponentProperty(ComponentName, "enableBloom", in value);
        }
    }

    public float BloomThreshold
    {
        get => _component.bloomThreshold;
        set
        {
            _component.bloomThreshold = value;
            Entity.SetComponentProperty(ComponentName, "bloomThreshold", in value);
        }
    }

    public float BloomIntensity
    {
        get => _component.bloomIntensity;
        set
        {
            _component.bloomIntensity = value;
            Entity.SetComponentProperty(ComponentName, "bloomIntensity", in value);
        }
    }

    public float BloomRadius
    {
        get => _component.bloomRadius;
        set
        {
            _component.bloomRadius = value;
            Entity.SetComponentProperty(ComponentName, "bloomRadius", in value);
        }
    }

    public int BloomIterations
    {
        get => _component.bloomIterations;
        set
        {
            _component.bloomIterations = value;
            Entity.SetComponentProperty(ComponentName, "bloomIterations", in value);
        }
    }

    public Color BloomTint
    {
        get => _component.bloomTint;
        set
        {
            _component.bloomTint = value;
            Entity.SetComponentProperty(ComponentName, "bloomTint", in value);
        }
    }

    #endregion

    #region Light Shafts

    public bool EnableLightShafts
    {
        get => _component.enableLightShafts;
        set
        {
            _component.enableLightShafts = value;
            Entity.SetComponentProperty(ComponentName, "enableLightShafts", in value);
        }
    }

    public float LightShaftDensity
    {
        get => _component.lightShaftDensity;
        set
        {
            _component.lightShaftDensity = value;
            Entity.SetComponentProperty(ComponentName, "lightShaftDensity", in value);
        }
    }

    public float LightShaftDecay
    {
        get => _component.lightShaftDecay;
        set
        {
            _component.lightShaftDecay = value;
            Entity.SetComponentProperty(ComponentName, "lightShaftDecay", in value);
        }
    }

    public float LightShaftWeight
    {
        get => _component.lightShaftWeight;
        set
        {
            _component.lightShaftWeight = value;
            Entity.SetComponentProperty(ComponentName, "lightShaftWeight", in value);
        }
    }

    public float LightShaftExposure
    {
        get => _component.lightShaftExposure;
        set
        {
            _component.lightShaftExposure = value;
            Entity.SetComponentProperty(ComponentName, "lightShaftExposure", in value);
        }
    }

    #endregion

    #region Fog

    public bool EnableFog
    {
        get => _component.enableFog;
        set
        {
            _component.enableFog = value;
            Entity.SetComponentProperty(ComponentName, "enableFog", in value);
        }
    }

    public FogMode FogMode
    {
        get => _component.fogMode;
        set
        {
            _component.fogMode = value;
            Entity.SetComponentProperty(ComponentName, "fogMode", in value);
        }
    }

    public Color FogColor
    {
        get => _component.fogColor;
        set
        {
            _component.fogColor = value;
            Entity.SetComponentProperty(ComponentName, "fogColor", in value);
        }
    }

    public float FogDensity
    {
        get => _component.fogDensity;
        set
        {
            _component.fogDensity = value;
            Entity.SetComponentProperty(ComponentName, "fogDensity", in value);
        }
    }

    public float FogStart
    {
        get => _component.fogStart;
        set
        {
            _component.fogStart = value;
            Entity.SetComponentProperty(ComponentName, "fogStart", in value);
        }
    }

    public float FogEnd
    {
        get => _component.fogEnd;
        set
        {
            _component.fogEnd = value;
            Entity.SetComponentProperty(ComponentName, "fogEnd", in value);
        }
    }

    public bool EnableHeightFog
    {
        get => _component.enableHeightFog;
        set
        {
            _component.enableHeightFog = value;
            Entity.SetComponentProperty(ComponentName, "enableHeightFog", in value);
        }
    }

    public float HeightFogBase
    {
        get => _component.heightFogBase;
        set
        {
            _component.heightFogBase = value;
            Entity.SetComponentProperty(ComponentName, "heightFogBase", in value);
        }
    }

    public float HeightFogDensity
    {
        get => _component.heightFogDensity;
        set
        {
            _component.heightFogDensity = value;
            Entity.SetComponentProperty(ComponentName, "heightFogDensity", in value);
        }
    }

    #endregion

    #region Tone Mapping

    public ToneMappingMode ToneMappingMode
    {
        get => _component.toneMappingMode;
        set
        {
            _component.toneMappingMode = value;
            Entity.SetComponentProperty(ComponentName, "toneMappingMode", in value);
        }
    }

    public float Exposure
    {
        get => _component.exposure;
        set
        {
            _component.exposure = value;
            Entity.SetComponentProperty(ComponentName, "exposure", in value);
        }
    }

    public float Contrast
    {
        get => _component.contrast;
        set
        {
            _component.contrast = value;
            Entity.SetComponentProperty(ComponentName, "contrast", in value);
        }
    }

    public float Saturation
    {
        get => _component.saturation;
        set
        {
            _component.saturation = value;
            Entity.SetComponentProperty(ComponentName, "saturation", in value);
        }
    }

    public float Gamma
    {
        get => _component.gamma;
        set
        {
            _component.gamma = value;
            Entity.SetComponentProperty(ComponentName, "gamma", in value);
        }
    }

    #endregion

    #region Color Grading

    public bool EnableColorGrading
    {
        get => _component.enableColorGrading;
        set
        {
            _component.enableColorGrading = value;
            Entity.SetComponentProperty(ComponentName, "enableColorGrading", in value);
        }
    }

    public float LutIntensity
    {
        get => _component.lutIntensity;
        set
        {
            _component.lutIntensity = value;
            Entity.SetComponentProperty(ComponentName, "lutIntensity", in value);
        }
    }

    #endregion
}


/// <summary>
/// 质量设置逻辑组件
/// </summary>
public sealed class QualitySettings : LogicComponent<QualitySettingsComponent>
{
    private const string ComponentName = "QualitySettingsComponent";

    public QualitySettings(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public QualityLevel Level
    {
        get => _component.level;
        set
        {
            _component.level = value;
            Entity.SetComponentProperty(ComponentName, "level", in value);
        }
    }

    public int MaxLightsPerFrame
    {
        get => _component.maxLightsPerFrame;
        set
        {
            _component.maxLightsPerFrame = value;
            Entity.SetComponentProperty(ComponentName, "maxLightsPerFrame", in value);
        }
    }

    public int MaxLightsPerPixel
    {
        get => _component.maxLightsPerPixel;
        set
        {
            _component.maxLightsPerPixel = value;
            Entity.SetComponentProperty(ComponentName, "maxLightsPerPixel", in value);
        }
    }

    public bool EnableAreaLights
    {
        get => _component.enableAreaLights;
        set
        {
            _component.enableAreaLights = value;
            Entity.SetComponentProperty(ComponentName, "enableAreaLights", in value);
        }
    }

    public bool EnableIndirectLighting
    {
        get => _component.enableIndirectLighting;
        set
        {
            _component.enableIndirectLighting = value;
            Entity.SetComponentProperty(ComponentName, "enableIndirectLighting", in value);
        }
    }

    public ShadowMethod ShadowMethod
    {
        get => _component.shadowMethod;
        set
        {
            _component.shadowMethod = value;
            Entity.SetComponentProperty(ComponentName, "shadowMethod", in value);
        }
    }

    public int ShadowMapResolution
    {
        get => _component.shadowMapResolution;
        set
        {
            _component.shadowMapResolution = value;
            Entity.SetComponentProperty(ComponentName, "shadowMapResolution", in value);
        }
    }

    public bool EnableShadowCache
    {
        get => _component.enableShadowCache;
        set
        {
            _component.enableShadowCache = value;
            Entity.SetComponentProperty(ComponentName, "enableShadowCache", in value);
        }
    }

    public bool EnableBloom
    {
        get => _component.enableBloom;
        set
        {
            _component.enableBloom = value;
            Entity.SetComponentProperty(ComponentName, "enableBloom", in value);
        }
    }

    public bool EnableLightShafts
    {
        get => _component.enableLightShafts;
        set
        {
            _component.enableLightShafts = value;
            Entity.SetComponentProperty(ComponentName, "enableLightShafts", in value);
        }
    }

    public bool EnableFog
    {
        get => _component.enableFog;
        set
        {
            _component.enableFog = value;
            Entity.SetComponentProperty(ComponentName, "enableFog", in value);
        }
    }

    public bool EnableColorGrading
    {
        get => _component.enableColorGrading;
        set
        {
            _component.enableColorGrading = value;
            Entity.SetComponentProperty(ComponentName, "enableColorGrading", in value);
        }
    }

    public float RenderScale
    {
        get => _component.renderScale;
        set
        {
            _component.renderScale = value;
            Entity.SetComponentProperty(ComponentName, "renderScale", in value);
        }
    }

    public bool EnableAutoQuality
    {
        get => _component.enableAutoQuality;
        set
        {
            _component.enableAutoQuality = value;
            Entity.SetComponentProperty(ComponentName, "enableAutoQuality", in value);
        }
    }

    public float TargetFrameRate
    {
        get => _component.targetFrameRate;
        set
        {
            _component.targetFrameRate = value;
            Entity.SetComponentProperty(ComponentName, "targetFrameRate", in value);
        }
    }

    public float QualityAdjustThreshold
    {
        get => _component.qualityAdjustThreshold;
        set
        {
            _component.qualityAdjustThreshold = value;
            Entity.SetComponentProperty(ComponentName, "qualityAdjustThreshold", in value);
        }
    }
}


/// <summary>
/// 光照设置逻辑组件
/// </summary>
public sealed class LightingSettings : LogicComponent<LightingSettingsComponent>
{
    private const string ComponentName = "LightingSettingsComponent";

    public LightingSettings(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public Color AmbientColor
    {
        get => _component.ambientColor;
        set
        {
            _component.ambientColor = value;
            Entity.SetComponentProperty(ComponentName, "ambientColor", in value);
        }
    }

    public float AmbientIntensity
    {
        get => _component.ambientIntensity;
        set
        {
            _component.ambientIntensity = value;
            Entity.SetComponentProperty(ComponentName, "ambientIntensity", in value);
        }
    }

    public int MaxLightsPerPixel
    {
        get => _component.maxLightsPerPixel;
        set
        {
            _component.maxLightsPerPixel = value;
            Entity.SetComponentProperty(ComponentName, "maxLightsPerPixel", in value);
        }
    }

    public bool EnableShadows
    {
        get => _component.enableShadows;
        set
        {
            _component.enableShadows = value;
            Entity.SetComponentProperty(ComponentName, "enableShadows", in value);
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

    public bool EnableNormalMapping
    {
        get => _component.enableNormalMapping;
        set
        {
            _component.enableNormalMapping = value;
            Entity.SetComponentProperty(ComponentName, "enableNormalMapping", in value);
        }
    }

    public uint ShadowMapResolution
    {
        get => _component.shadowMapResolution;
        set
        {
            _component.shadowMapResolution = value;
            Entity.SetComponentProperty(ComponentName, "shadowMapResolution", in value);
        }
    }

    public uint MaxShadowCasters
    {
        get => _component.maxShadowCasters;
        set
        {
            _component.maxShadowCasters = value;
            Entity.SetComponentProperty(ComponentName, "maxShadowCasters", in value);
        }
    }

    public float ShadowBias
    {
        get => _component.shadowBias;
        set
        {
            _component.shadowBias = value;
            Entity.SetComponentProperty(ComponentName, "shadowBias", in value);
        }
    }

    public float ShadowNormalBias
    {
        get => _component.shadowNormalBias;
        set
        {
            _component.shadowNormalBias = value;
            Entity.SetComponentProperty(ComponentName, "shadowNormalBias", in value);
        }
    }

    public bool EnableIndirectLighting
    {
        get => _component.enableIndirectLighting;
        set
        {
            _component.enableIndirectLighting = value;
            Entity.SetComponentProperty(ComponentName, "enableIndirectLighting", in value);
        }
    }

    public float IndirectIntensity
    {
        get => _component.indirectIntensity;
        set
        {
            _component.indirectIntensity = value;
            Entity.SetComponentProperty(ComponentName, "indirectIntensity", in value);
        }
    }

    public float BounceDecay
    {
        get => _component.bounceDecay;
        set
        {
            _component.bounceDecay = value;
            Entity.SetComponentProperty(ComponentName, "bounceDecay", in value);
        }
    }

    public float IndirectRadius
    {
        get => _component.indirectRadius;
        set
        {
            _component.indirectRadius = value;
            Entity.SetComponentProperty(ComponentName, "indirectRadius", in value);
        }
    }
}

/// <summary>
/// 环境区域逻辑组件
/// </summary>
public sealed class AmbientZone : LogicComponent<AmbientZoneComponent>
{
    private const string ComponentName = "AmbientZoneComponent";

    public AmbientZone(Entity entity) : base(entity) { }

    public bool Enable
    {
        get => _component.Enable;
        set
        {
            _component.Enable = value;
            Entity.SetComponentProperty(ComponentName, "Enable", in value);
        }
    }

    public AmbientZoneShape Shape
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

    public Color PrimaryColor
    {
        get => _component.primaryColor;
        set
        {
            _component.primaryColor = value;
            Entity.SetComponentProperty(ComponentName, "primaryColor", in value);
        }
    }

    public Color SecondaryColor
    {
        get => _component.secondaryColor;
        set
        {
            _component.secondaryColor = value;
            Entity.SetComponentProperty(ComponentName, "secondaryColor", in value);
        }
    }

    public AmbientGradientMode GradientMode
    {
        get => _component.gradientMode;
        set
        {
            _component.gradientMode = value;
            Entity.SetComponentProperty(ComponentName, "gradientMode", in value);
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

    public float EdgeSoftness
    {
        get => _component.edgeSoftness;
        set
        {
            _component.edgeSoftness = value;
            Entity.SetComponentProperty(ComponentName, "edgeSoftness", in value);
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

    public float BlendWeight
    {
        get => _component.blendWeight;
        set
        {
            _component.blendWeight = value;
            Entity.SetComponentProperty(ComponentName, "blendWeight", in value);
        }
    }
}

#endregion

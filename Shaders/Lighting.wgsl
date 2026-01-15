// ============================================================================
// Lighting.wgsl - 2D 光照计算模块
// 提供光源数据结构和光照计算函数
// Feature: 2d-lighting-system
// Feature: 2d-lighting-enhancement (AreaLight support)
// Requirements: 1.1, 2.1, 3.1, 8.2
// ============================================================================
export Lighting;

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;
const LIGHT_TYPE_POINT: u32 = 0u;
const LIGHT_TYPE_SPOT: u32 = 1u;
const LIGHT_TYPE_DIRECTIONAL: u32 = 2u;

const AREA_LIGHT_SHAPE_RECTANGLE: u32 = 0u;
const AREA_LIGHT_SHAPE_CIRCLE: u32 = 1u;

const ATTENUATION_LINEAR: u32 = 0u;
const ATTENUATION_QUADRATIC: u32 = 1u;
const ATTENUATION_INVERSE_SQUARE: u32 = 2u;

const MIN_RADIUS: f32 = 0.001;

// ============ 数据结构 ============

/// 光源数据结构（与 C++ LightData 对应，64 字节）
struct Light {
    position: vec2<f32>,      // 世界坐标位置
    direction: vec2<f32>,     // 方向（聚光灯/方向光）
    color: vec4<f32>,         // RGBA 颜色
    intensity: f32,           // 强度
    radius: f32,              // 影响半径
    innerAngle: f32,          // 内角（聚光灯，弧度）
    outerAngle: f32,          // 外角（聚光灯，弧度）
    lightType: u32,           // 光源类型
    layerMask: u32,           // 光照层掩码
    attenuation: f32,         // 衰减系数（用于标识衰减类型）
    castShadows: u32,         // 是否投射阴影
};

/// 光照全局数据结构（与 C++ LightingGlobalData 对应，32 字节）
struct LightingData {
    ambientColor: vec4<f32>,      // 环境光颜色
    ambientIntensity: f32,        // 环境光强度
    lightCount: u32,              // 当前光源数量
    maxLightsPerPixel: u32,       // 每像素最大光源数
    enableShadows: u32,           // 是否启用阴影
};

/// 面光源数据结构（与 C++ AreaLightData 对应，64 字节）
/// Feature: 2d-lighting-enhancement
struct AreaLight {
    position: vec2<f32>,          // 世界坐标位置（8 字节）
    size: vec2<f32>,              // 宽度和高度（8 字节）
    // -- 16 字节边界 --
    color: vec4<f32>,             // RGBA 颜色（16 字节）
    // -- 16 字节边界 --
    intensity: f32,               // 光照强度（4 字节）
    radius: f32,                  // 影响半径（4 字节）
    shape: u32,                   // 形状类型: 0=Rectangle, 1=Circle（4 字节）
    layerMask: u32,               // 光照层掩码（4 字节）
    // -- 16 字节边界 --
    attenuation: f32,             // 衰减系数（4 字节）
    shadowSoftness: f32,          // 阴影柔和度倍数（4 字节）
    padding1: f32,                // 对齐填充（4 字节）
    padding2: f32,                // 对齐填充（4 字节）
    // -- 16 字节边界 --
};

/// 面光源全局数据结构
struct AreaLightGlobal {
    areaLightCount: u32,          // 面光源数量
    padding1: u32,
    padding2: u32,
    padding3: u32,
};

// ============ 衰减函数 ============

/// 计算线性衰减
/// 公式: attenuation = max(0, 1 - distance / radius)
fn CalculateLinearAttenuation(distance: f32, radius: f32) -> f32 {
    let safeRadius = max(radius, MIN_RADIUS);
    if (distance >= safeRadius) {
        return 0.0;
    }
    return max(0.0, 1.0 - (distance / safeRadius));
}

/// 计算二次衰减
/// 公式: attenuation = max(0, 1 - (distance / radius)^2)
fn CalculateQuadraticAttenuation(distance: f32, radius: f32) -> f32 {
    let safeRadius = max(radius, MIN_RADIUS);
    if (distance >= safeRadius) {
        return 0.0;
    }
    let ratio = distance / safeRadius;
    return max(0.0, 1.0 - (ratio * ratio));
}

/// 计算平方反比衰减
/// 公式: attenuation = 1 / (1 + (distance / radius)^2) * edgeFalloff
fn CalculateInverseSquareAttenuation(distance: f32, radius: f32) -> f32 {
    let safeRadius = max(radius, MIN_RADIUS);
    if (distance >= safeRadius) {
        return 0.0;
    }
    let normalizedDist = distance / safeRadius;
    let factor = normalizedDist * normalizedDist;
    let baseAttenuation = 1.0 / (1.0 + factor * 4.0);
    let edgeFalloff = 1.0 - normalizedDist;
    return baseAttenuation * edgeFalloff;
}

/// 根据衰减类型计算衰减值
fn CalculateAttenuation(distance: f32, radius: f32, attenuationType: u32) -> f32 {
    switch (attenuationType) {
        case ATTENUATION_LINEAR: {
            return CalculateLinearAttenuation(distance, radius);
        }
        case ATTENUATION_QUADRATIC: {
            return CalculateQuadraticAttenuation(distance, radius);
        }
        case ATTENUATION_INVERSE_SQUARE: {
            return CalculateInverseSquareAttenuation(distance, radius);
        }
        default: {
            return CalculateQuadraticAttenuation(distance, radius);
        }
    }
}

// ============ 聚光灯角度衰减 ============

/// 计算聚光灯角度衰减
/// 使用 smoothstep 风格的插值以获得更平滑的过渡
fn CalculateSpotAngleAttenuation(cosAngle: f32, innerAngleCos: f32, outerAngleCos: f32) -> f32 {
    // 如果在内角范围内，完全照亮
    if (cosAngle >= innerAngleCos) {
        return 1.0;
    }
    
    // 如果在外角范围外，完全不照亮
    if (cosAngle <= outerAngleCos) {
        return 0.0;
    }
    
    // 在内外角之间，平滑插值
    let range = innerAngleCos - outerAngleCos;
    if (range <= 0.0) {
        return 0.0;
    }
    
    let t = (cosAngle - outerAngleCos) / range;
    // Smoothstep: 3t^2 - 2t^3
    return t * t * (3.0 - 2.0 * t);
}

// ============ 光照层过滤 ============

/// 检查光源是否影响指定的光照层
fn LightAffectsLayer(lightLayerMask: u32, spriteLayer: u32) -> bool {
    return (lightLayerMask & spriteLayer) != 0u;
}

// ============ 光照计算函数 ============

/// 计算点光源光照贡献
/// @param light 光源数据
/// @param worldPos 世界坐标位置
/// @param spriteLayer 精灵光照层
/// @return 光照颜色贡献 (RGB)
fn CalculatePointLight(light: Light, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 计算到光源的距离
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    // 计算衰减
    let attenuationType = u32(light.attenuation);
    let attenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    // 计算最终光照贡献
    let contribution = light.color.rgb * light.intensity * attenuation;
    
    // 使用 select 代替 if 来保持均匀控制流
    return select(vec3<f32>(0.0), contribution, affectsLayer && attenuation > 0.0);
}

/// 计算聚光灯光照贡献
/// @param light 光源数据
/// @param worldPos 世界坐标位置
/// @param spriteLayer 精灵光照层
/// @return 光照颜色贡献 (RGB)
fn CalculateSpotLight(light: Light, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 计算到光源的方向和距离
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    // 计算距离衰减
    let attenuationType = u32(light.attenuation);
    let distanceAttenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    // 计算从光源到目标点的方向（归一化）
    let lightToTarget = -normalize(toLight);
    
    // 计算与光源方向的夹角余弦值
    let cosAngle = dot(lightToTarget, normalize(light.direction));
    
    // 计算角度衰减
    let innerAngleCos = cos(light.innerAngle);
    let outerAngleCos = cos(light.outerAngle);
    let angleAttenuation = CalculateSpotAngleAttenuation(cosAngle, innerAngleCos, outerAngleCos);
    
    // 计算最终光照贡献
    let contribution = light.color.rgb * light.intensity * distanceAttenuation * angleAttenuation;
    
    // 使用 select 代替 if
    return select(vec3<f32>(0.0), contribution, affectsLayer && distanceAttenuation > 0.0 && angleAttenuation > 0.0);
}

/// 计算方向光光照贡献
/// @param light 光源数据
/// @param worldPos 世界坐标位置（方向光不使用位置）
/// @param spriteLayer 精灵光照层
/// @return 光照颜色贡献 (RGB)
fn CalculateDirectionalLight(light: Light, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 方向光没有距离衰减，直接返回光照颜色
    let contribution = light.color.rgb * light.intensity;
    return select(vec3<f32>(0.0), contribution, affectsLayer);
}

/// 计算单个光源的光照贡献（自动判断光源类型）
/// @param light 光源数据
/// @param worldPos 世界坐标位置
/// @param spriteLayer 精灵光照层
/// @return 光照颜色贡献 (RGB)
fn CalculateLightContribution(light: Light, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    switch (light.lightType) {
        case LIGHT_TYPE_POINT: {
            return CalculatePointLight(light, worldPos, spriteLayer);
        }
        case LIGHT_TYPE_SPOT: {
            return CalculateSpotLight(light, worldPos, spriteLayer);
        }
        case LIGHT_TYPE_DIRECTIONAL: {
            return CalculateDirectionalLight(light, worldPos, spriteLayer);
        }
        default: {
            return vec3<f32>(0.0);
        }
    }
}

/// 计算所有光源的累加光照
/// @param lights 光源数组
/// @param lightingData 光照全局数据
/// @param worldPos 世界坐标位置
/// @param spriteLayer 精灵光照层
/// @return 最终光照颜色 (RGB)
fn CalculateTotalLighting(
    lights: ptr<storage, array<Light>, read>,
    lightingData: LightingData,
    worldPos: vec2<f32>,
    spriteLayer: u32
) -> vec3<f32> {
    // 从环境光开始
    var totalLight = lightingData.ambientColor.rgb * lightingData.ambientIntensity;
    
    // 累加所有光源贡献
    let lightCount = min(lightingData.lightCount, lightingData.maxLightsPerPixel);
    for (var i = 0u; i < lightCount; i = i + 1u) {
        let light = (*lights)[i];
        totalLight = totalLight + CalculateLightContribution(light, worldPos, spriteLayer);
    }
    
    return totalLight;
}

// ============ 法线贴图支持 ============

/// 从法线贴图采样值转换为世界空间法线
/// 法线贴图存储的是切线空间法线，范围 [0, 1]，需要转换到 [-1, 1]
fn UnpackNormal(normalMapValue: vec3<f32>) -> vec3<f32> {
    return normalMapValue * 2.0 - 1.0;
}

/// 计算带法线贴图的点光源光照贡献
/// @param light 光源数据
/// @param worldPos 世界坐标位置
/// @param normal 表面法线（世界空间）
/// @param spriteLayer 精灵光照层
/// @return 光照颜色贡献 (RGB)
fn CalculatePointLightWithNormal(
    light: Light,
    worldPos: vec2<f32>,
    normal: vec3<f32>,
    spriteLayer: u32
) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 计算到光源的方向和距离
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    // 计算衰减
    let attenuationType = u32(light.attenuation);
    let attenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    // 计算光照方向（3D，假设光源在 z=1 的高度）
    let lightDir3D = normalize(vec3<f32>(toLight, 1.0));
    
    // 计算漫反射因子（Lambert）
    let NdotL = max(dot(normal, lightDir3D), 0.0);
    
    let contribution = light.color.rgb * light.intensity * attenuation * NdotL;
    return select(vec3<f32>(0.0), contribution, affectsLayer && attenuation > 0.0);
}

/// 计算带法线贴图的聚光灯光照贡献
fn CalculateSpotLightWithNormal(
    light: Light,
    worldPos: vec2<f32>,
    normal: vec3<f32>,
    spriteLayer: u32
) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 计算到光源的方向和距离
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    // 计算距离衰减
    let attenuationType = u32(light.attenuation);
    let distanceAttenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    // 计算角度衰减
    let lightToTarget = -normalize(toLight);
    let cosAngle = dot(lightToTarget, normalize(light.direction));
    let innerAngleCos = cos(light.innerAngle);
    let outerAngleCos = cos(light.outerAngle);
    let angleAttenuation = CalculateSpotAngleAttenuation(cosAngle, innerAngleCos, outerAngleCos);
    
    // 计算光照方向（3D）
    let lightDir3D = normalize(vec3<f32>(toLight, 1.0));
    
    // 计算漫反射因子
    let NdotL = max(dot(normal, lightDir3D), 0.0);
    
    let contribution = light.color.rgb * light.intensity * distanceAttenuation * angleAttenuation * NdotL;
    return select(vec3<f32>(0.0), contribution, affectsLayer && distanceAttenuation > 0.0 && angleAttenuation > 0.0);
}

/// 计算带法线贴图的方向光光照贡献
fn CalculateDirectionalLightWithNormal(
    light: Light,
    worldPos: vec2<f32>,
    normal: vec3<f32>,
    spriteLayer: u32
) -> vec3<f32> {
    // 使用位运算检查光照层
    let affectsLayer = (light.layerMask & spriteLayer) != 0u;
    
    // 方向光的光照方向（3D，假设从上方照射）
    let lightDir3D = normalize(vec3<f32>(-light.direction, 1.0));
    
    // 计算漫反射因子
    let NdotL = max(dot(normal, lightDir3D), 0.0);
    
    let contribution = light.color.rgb * light.intensity * NdotL;
    return select(vec3<f32>(0.0), contribution, affectsLayer);
}

/// 计算带法线贴图的单个光源贡献
fn CalculateLightContributionWithNormal(
    light: Light,
    worldPos: vec2<f32>,
    normal: vec3<f32>,
    spriteLayer: u32
) -> vec3<f32> {
    switch (light.lightType) {
        case LIGHT_TYPE_POINT: {
            return CalculatePointLightWithNormal(light, worldPos, normal, spriteLayer);
        }
        case LIGHT_TYPE_SPOT: {
            return CalculateSpotLightWithNormal(light, worldPos, normal, spriteLayer);
        }
        case LIGHT_TYPE_DIRECTIONAL: {
            return CalculateDirectionalLightWithNormal(light, worldPos, normal, spriteLayer);
        }
        default: {
            return vec3<f32>(0.0);
        }
    }
}


// ============ 面光源计算函数 ============
// Feature: 2d-lighting-enhancement

/// 计算矩形面光源光照贡献
fn CalculateRectangleAreaLight(areaLight: AreaLight, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    // 检查光照层
    let affectsLayer = (areaLight.layerMask & spriteLayer) != 0u;
    if (!affectsLayer) {
        return vec3<f32>(0.0);
    }
    
    // 计算到面光源中心的距离
    let toLight = areaLight.position - worldPos;
    let distance = length(toLight);
    
    // 如果超出影响半径，返回 0
    if (distance >= areaLight.radius) {
        return vec3<f32>(0.0);
    }
    
    // 计算到矩形面光源表面的最近点
    let halfWidth = areaLight.size.x / 2.0;
    let halfHeight = areaLight.size.y / 2.0;
    
    // 将目标点转换到面光源的局部坐标系
    let localPos = worldPos - areaLight.position;
    
    // 计算到矩形边界的最近点
    let closestX = clamp(localPos.x, -halfWidth, halfWidth);
    let closestY = clamp(localPos.y, -halfHeight, halfHeight);
    let closestPoint = vec2<f32>(closestX, closestY);
    
    // 计算到最近点的距离
    var distToSurface = length(localPos - closestPoint);
    
    // 如果目标点在矩形内部，使用最小距离
    if (distToSurface < MIN_RADIUS) {
        distToSurface = MIN_RADIUS;
    }
    
    // 计算衰减
    let attenuationType = u32(areaLight.attenuation);
    let attenuation = CalculateAttenuation(distToSurface, areaLight.radius, attenuationType);
    
    // 返回光照贡献
    return areaLight.color.rgb * areaLight.intensity * attenuation;
}

/// 计算圆形面光源光照贡献
fn CalculateCircleAreaLight(areaLight: AreaLight, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    // 检查光照层
    let affectsLayer = (areaLight.layerMask & spriteLayer) != 0u;
    if (!affectsLayer) {
        return vec3<f32>(0.0);
    }
    
    // 计算到面光源中心的距离
    let toLight = areaLight.position - worldPos;
    let distance = length(toLight);
    
    // 如果超出影响半径，返回 0
    if (distance >= areaLight.radius) {
        return vec3<f32>(0.0);
    }
    
    // 圆形面光源的半径（使用 width 作为直径）
    let lightRadius = areaLight.size.x / 2.0;
    
    // 计算到圆形表面的距离
    var distToSurface = max(0.0, distance - lightRadius);
    
    // 如果目标点在圆形内部，使用最小距离
    if (distToSurface < MIN_RADIUS) {
        distToSurface = MIN_RADIUS;
    }
    
    // 计算衰减
    let attenuationType = u32(areaLight.attenuation);
    let attenuation = CalculateAttenuation(distToSurface, areaLight.radius, attenuationType);
    
    // 返回光照贡献
    return areaLight.color.rgb * areaLight.intensity * attenuation;
}

/// 计算面光源光照贡献（自动选择形状）
fn CalculateAreaLightContribution(areaLight: AreaLight, worldPos: vec2<f32>, spriteLayer: u32) -> vec3<f32> {
    if (areaLight.shape == AREA_LIGHT_SHAPE_CIRCLE) {
        return CalculateCircleAreaLight(areaLight, worldPos, spriteLayer);
    } else {
        return CalculateRectangleAreaLight(areaLight, worldPos, spriteLayer);
    }
}

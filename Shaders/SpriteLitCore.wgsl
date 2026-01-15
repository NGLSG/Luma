// ============================================================================
// SpriteLitCore.wgsl - 带光照的精灵渲染核心模块
// 提供SpriteLit shader所需的数据结构、绑定和光照计算函数
// Feature: 2d-lighting-system
// Feature: 2d-lighting-enhancement (Emission support)
// Requirements: 4.2, 4.4, 4.5
// ============================================================================
export SpriteLit;
import Std;
import Lighting;

// ============ SpriteLit专用顶点输出 ============

struct SpriteLitVertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) worldPos: vec2<f32>,
    @location(3) @interpolate(flat) lightLayer: u32,
    @location(4) emissionColor: vec4<f32>,      // 自发光颜色
    @location(5) emissionIntensity: f32,        // 自发光强度（支持HDR）
};

// ============ Group 1: 光照数据绑定 ============

@group(1) @binding(0) var<uniform> lightingData: LightingData;
@group(1) @binding(1) var<storage, read> lights: array<Light>;

// 面光源数据绑定（与直接光照同组）
// Feature: 2d-lighting-enhancement

struct AreaLightGlobalData {
    areaLightCount: u32,
    padding1: u32,
    padding2: u32,
    padding3: u32,
};

struct AreaLightData {
    position: vec2<f32>,
    size: vec2<f32>,
    color: vec4<f32>,
    intensity: f32,
    radius: f32,
    shape: u32,
    layerMask: u32,
    attenuation: f32,
    shadowSoftness: f32,
    padding1: f32,
    padding2: f32,
};

@group(1) @binding(2) var<uniform> areaLightGlobal: AreaLightGlobalData;
@group(1) @binding(3) var<storage, read> areaLights: array<AreaLightData>;

// ============ Group 2: 阴影数据绑定 ============

struct ShadowParams {
    edgeCount: u32,
    shadowSoftness: f32,
    shadowBias: f32,
    padding: f32,
};

struct ShadowEdge {
    start: vec2<f32>,
    end: vec2<f32>,
    boundsMin: vec2<f32>,
    boundsMax: vec2<f32>,
    selfShadow: u32,
    opacity: f32,
    padding: vec2<f32>,
};

@group(2) @binding(0) var<uniform> shadowParams: ShadowParams;
@group(2) @binding(1) var<storage, read> shadowEdges: array<ShadowEdge>;

// ============ Group 3: 间接光照数据绑定 ============

struct IndirectLightingGlobal {
    reflectorCount: u32,
    indirectIntensity: f32,
    bounceDecay: f32,
    enableIndirect: u32,
};

struct Reflector {
    position: vec2<f32>,
    size: vec2<f32>,
    color: vec4<f32>,
    intensity: f32,
    radius: f32,
    layerMask: u32,
    padding: f32,
};

@group(3) @binding(0) var<uniform> indirectGlobal: IndirectLightingGlobal;
@group(3) @binding(1) var<storage, read> reflectors: array<Reflector>;

// ============ 自发光全局数据 ============

struct EmissionGlobalData {
    emissionEnabled: u32,
    emissionScale: f32,
    padding1: f32,
    padding2: f32,
};

// ============ 环境光区域数据绑定 ============
// Feature: 2d-lighting-enhancement

// 环境光区域形状常量
const AMBIENT_ZONE_SHAPE_RECTANGLE: u32 = 0u;
const AMBIENT_ZONE_SHAPE_CIRCLE: u32 = 1u;

// 环境光渐变模式常量
const AMBIENT_GRADIENT_NONE: u32 = 0u;
const AMBIENT_GRADIENT_VERTICAL: u32 = 1u;
const AMBIENT_GRADIENT_HORIZONTAL: u32 = 2u;

struct AmbientZoneGlobalData {
    ambientZoneCount: u32,
    padding1: u32,
    padding2: u32,
    padding3: u32,
};

struct AmbientZoneData {
    position: vec2<f32>,
    size: vec2<f32>,
    primaryColor: vec4<f32>,
    secondaryColor: vec4<f32>,
    intensity: f32,
    edgeSoftness: f32,
    gradientMode: u32,
    shape: u32,
    priority: i32,
    blendWeight: f32,
    padding1: f32,
    padding2: f32,
};

@group(3) @binding(2) var<uniform> emissionGlobal: EmissionGlobalData;
@group(3) @binding(3) var<uniform> ambientZoneGlobal: AmbientZoneGlobalData;
@group(3) @binding(4) var<storage, read> ambientZones: array<AmbientZoneData>;

// ============ 阴影常量 ============

const SHADOW_BIAS: f32 = 0.001;

// ============ 顶点变换函数 ============

/// SpriteLit顶点变换
/// Feature: 2d-lighting-enhancement
/// Requirements: 4.2, 4.4, 4.5
fn TransformSpriteLitVertex(input: VertexInput, instanceIndex: u32) -> SpriteLitVertexOutput {
    var output: SpriteLitVertexOutput;
    
    let instance = instanceDatas[instanceIndex];
    
    // 计算局部位置（基于精灵尺寸）
    let localPos = input.position * instance.size;
    
    // 变换到世界空间
    let worldPos = LocalToWorld(localPos, instance);
    
    // 变换到摄像机空间
    let cameraPos = WorldToCamera(worldPos, engineData);
    
    // 变换到裁剪空间
    let clipPos = CameraToClip(cameraPos, engineData);
    
    output.clipPosition = vec4<f32>(clipPos, instance.position.z, 1.0);
    output.uv = TransformUV(input.uv, instance.uvRect);
    output.color = instance.color;
    output.worldPos = worldPos;
    output.lightLayer = instance.lightLayer;
    
    // 从 InstanceData 读取自发光数据
    // Feature: 2d-lighting-enhancement
    output.emissionColor = instance.emissionColor;
    output.emissionIntensity = instance.emissionIntensity;
    
    return output;
}

// ============ 射线-边缘相交检测 ============

/// 检测射线与边缘是否相交
/// 返回相交距离，如果不相交返回负值
fn RayEdgeIntersection(rayOrigin: vec2<f32>, rayDir: vec2<f32>, edgeStart: vec2<f32>, edgeEnd: vec2<f32>) -> f32 {
    let edgeDir = edgeEnd - edgeStart;
    let originToStart = edgeStart - rayOrigin;
    
    // 计算行列式
    let det = rayDir.x * edgeDir.y - rayDir.y * edgeDir.x;
    
    // 平行检测
    if (abs(det) < 0.000001) {
        return -1.0;
    }
    
    let invDet = 1.0 / det;
    
    // 计算射线参数 t
    let t = (originToStart.x * edgeDir.y - originToStart.y * edgeDir.x) * invDet;
    
    // 计算边缘参数 u
    let u = (originToStart.x * rayDir.y - originToStart.y * rayDir.x) * invDet;
    
    // 检查交点是否在射线正方向且在边缘上
    if (t > SHADOW_BIAS && u >= 0.0 && u <= 1.0) {
        return t;
    }
    
    return -1.0;
}

/// 检查点是否在包围盒内
fn IsPointInBounds(point: vec2<f32>, boundsMin: vec2<f32>, boundsMax: vec2<f32>) -> bool {
    return point.x >= boundsMin.x && point.x <= boundsMax.x &&
           point.y >= boundsMin.y && point.y <= boundsMax.y;
}

/// 检查点是否在阴影中（相对于某个光源）
/// 返回阴影因子 [0, 1]，0 表示完全不在阴影中，1 表示完全在阴影中
fn IsPointInShadow(point: vec2<f32>, lightPos: vec2<f32>) -> f32 {
    let toPoint = point - lightPos;
    let distToPoint = length(toPoint);
    
    if (distToPoint < SHADOW_BIAS) {
        return 0.0; // 点在光源位置，不在阴影中
    }
    
    let rayDir = toPoint / distToPoint;
    
    var maxShadowFactor = 0.0;
    
    // 检查是否有边缘阻挡
    for (var i = 0u; i < shadowParams.edgeCount; i = i + 1u) {
        let edge = shadowEdges[i];
        
        // 如果该边缘不允许自阴影，且当前点在该投射器的包围盒内，则跳过
        if (edge.selfShadow == 0u && IsPointInBounds(point, edge.boundsMin, edge.boundsMax)) {
            continue;
        }
        
        let t = RayEdgeIntersection(lightPos, rayDir, edge.start, edge.end);
        
        // 如果交点在光源和目标点之间，则点在阴影中
        if (t > 0.0 && t < distToPoint - SHADOW_BIAS) {
            // 使用边缘的不透明度作为阴影强度
            maxShadowFactor = max(maxShadowFactor, edge.opacity);
            // 如果已经达到最大阴影，可以提前退出
            if (maxShadowFactor >= 1.0) {
                return 1.0;
            }
        }
    }
    
    return maxShadowFactor;
}

// ============ 带阴影的光照计算 ============

/// 计算点光源光照贡献（带阴影）
fn CalculatePointLightWithShadow(light: Light, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    let affectsLayer = (light.layerMask & layerMask) != 0u;
    
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    let attenuationType = u32(light.attenuation);
    let attenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    // 计算阴影
    var shadowFactor = 1.0;
    if (lightingData.enableShadows != 0u && shadowParams.edgeCount > 0u && light.castShadows != 0u) {
        shadowFactor = 1.0 - IsPointInShadow(worldPos, light.position);
    }
    
    let contribution = light.color.rgb * light.intensity * attenuation * shadowFactor;
    return select(vec3<f32>(0.0), contribution, affectsLayer && attenuation > 0.0);
}

/// 计算聚光灯光照贡献（带阴影）
fn CalculateSpotLightWithShadow(light: Light, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    let affectsLayer = (light.layerMask & layerMask) != 0u;
    
    let toLight = light.position - worldPos;
    let distance = length(toLight);
    
    let attenuationType = u32(light.attenuation);
    let distanceAttenuation = CalculateAttenuation(distance, light.radius, attenuationType);
    
    let lightToTarget = -normalize(toLight);
    let cosAngle = dot(lightToTarget, normalize(light.direction));
    let innerAngleCos = cos(light.innerAngle);
    let outerAngleCos = cos(light.outerAngle);
    let angleAttenuation = CalculateSpotAngleAttenuation(cosAngle, innerAngleCos, outerAngleCos);
    
    // 计算阴影
    var shadowFactor = 1.0;
    if (lightingData.enableShadows != 0u && shadowParams.edgeCount > 0u && light.castShadows != 0u) {
        shadowFactor = 1.0 - IsPointInShadow(worldPos, light.position);
    }
    
    let contribution = light.color.rgb * light.intensity * distanceAttenuation * angleAttenuation * shadowFactor;
    return select(vec3<f32>(0.0), contribution, affectsLayer && distanceAttenuation > 0.0 && angleAttenuation > 0.0);
}

/// 计算方向光光照贡献（方向光通常不投射阴影，或使用不同的阴影算法）
fn CalculateDirectionalLightSimple(light: Light, layerMask: u32) -> vec3<f32> {
    let affectsLayer = (light.layerMask & layerMask) != 0u;
    let contribution = light.color.rgb * light.intensity;
    return select(vec3<f32>(0.0), contribution, affectsLayer);
}

/// 计算单个光源的光照贡献（带阴影）
fn CalculateLightContributionWithShadow(light: Light, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    switch (light.lightType) {
        case LIGHT_TYPE_POINT: {
            return CalculatePointLightWithShadow(light, worldPos, layerMask);
        }
        case LIGHT_TYPE_SPOT: {
            return CalculateSpotLightWithShadow(light, worldPos, layerMask);
        }
        case LIGHT_TYPE_DIRECTIONAL: {
            return CalculateDirectionalLightSimple(light, layerMask);
        }
        default: {
            return vec3<f32>(0.0);
        }
    }
}

// ============ 光照累加函数 ============

/// 计算来自单个反射体的间接光照贡献
fn CalculateReflectorContribution(reflector: Reflector, worldPos: vec2<f32>, lightLayer: u32) -> vec3<f32> {
    // 检查层掩码
    let affectsLayer = (reflector.layerMask & lightLayer) != 0u;
    if (!affectsLayer) {
        return vec3<f32>(0.0);
    }
    
    // 计算到反射体中心的距离
    let toReflector = reflector.position - worldPos;
    let distance = length(toReflector);
    
    // 距离衰减
    if (distance > reflector.radius || distance < 0.001) {
        return vec3<f32>(0.0);
    }
    
    // 使用平滑的距离衰减
    let normalizedDist = distance / reflector.radius;
    let attenuation = 1.0 - normalizedDist * normalizedDist;
    
    // 反射体的有效面积影响（调整基准值，让小物体也能有效反射）
    let area = reflector.size.x * reflector.size.y;
    let areaFactor = min(sqrt(area) / 50.0, 1.0);
    
    // 计算反射体接收到的直接光照（简化：使用反射体位置计算）
    var receivedLight = lightingData.ambientColor.rgb * lightingData.ambientIntensity;
    let lightCount = min(lightingData.lightCount, lightingData.maxLightsPerPixel);
    for (var i = 0u; i < lightCount; i = i + 1u) {
        let light = lights[i];
        // 简化计算：不考虑阴影，只计算距离衰减
        if (light.lightType == LIGHT_TYPE_POINT || light.lightType == LIGHT_TYPE_SPOT) {
            let toLightVec = light.position - reflector.position;
            let lightDist = length(toLightVec);
            if (lightDist < light.radius) {
                let lightAtten = 1.0 - (lightDist / light.radius) * (lightDist / light.radius);
                receivedLight = receivedLight + light.color.rgb * light.intensity * lightAtten;
            }
        } else {
            // 方向光
            receivedLight = receivedLight + light.color.rgb * light.intensity;
        }
    }
    
    // 计算间接光照贡献 = 反射体颜色 × 接收到的光 × 衰减 × 面积因子
    let contribution = reflector.color.rgb * receivedLight * attenuation * areaFactor;
    
    return contribution;
}

/// 计算总间接光照
fn CalculateIndirectLighting(worldPos: vec2<f32>, lightLayer: u32) -> vec3<f32> {
    if (indirectGlobal.enableIndirect == 0u || indirectGlobal.reflectorCount == 0u) {
        return vec3<f32>(0.0);
    }
    
    var totalIndirect = vec3<f32>(0.0);
    
    let reflectorCount = min(indirectGlobal.reflectorCount, 64u);
    for (var i = 0u; i < reflectorCount; i = i + 1u) {
        let reflector = reflectors[i];
        totalIndirect = totalIndirect + CalculateReflectorContribution(reflector, worldPos, lightLayer);
    }
    
    // 应用间接光照强度和衰减
    return totalIndirect * indirectGlobal.indirectIntensity * indirectGlobal.bounceDecay;
}

// ============ 面光源光照计算 ============
// Feature: 2d-lighting-enhancement

/// 计算矩形面光源光照贡献
fn CalculateRectangleAreaLightContribution(areaLight: AreaLightData, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    // 检查光照层
    let affectsLayer = (areaLight.layerMask & layerMask) != 0u;
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
    if (distToSurface < 0.001) {
        distToSurface = 0.001;
    }
    
    // 计算衰减
    let attenuationType = u32(areaLight.attenuation);
    let attenuation = CalculateAttenuation(distToSurface, areaLight.radius, attenuationType);
    
    // 返回光照贡献
    return areaLight.color.rgb * areaLight.intensity * attenuation;
}

/// 计算圆形面光源光照贡献
fn CalculateCircleAreaLightContribution(areaLight: AreaLightData, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    // 检查光照层
    let affectsLayer = (areaLight.layerMask & layerMask) != 0u;
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
    if (distToSurface < 0.001) {
        distToSurface = 0.001;
    }
    
    // 计算衰减
    let attenuationType = u32(areaLight.attenuation);
    let attenuation = CalculateAttenuation(distToSurface, areaLight.radius, attenuationType);
    
    // 返回光照贡献
    return areaLight.color.rgb * areaLight.intensity * attenuation;
}

/// 计算单个面光源的光照贡献
fn CalculateSingleAreaLightContribution(areaLight: AreaLightData, worldPos: vec2<f32>, layerMask: u32) -> vec3<f32> {
    if (areaLight.shape == AREA_LIGHT_SHAPE_CIRCLE) {
        return CalculateCircleAreaLightContribution(areaLight, worldPos, layerMask);
    } else {
        return CalculateRectangleAreaLightContribution(areaLight, worldPos, layerMask);
    }
}

/// 计算所有面光源的光照贡献
fn CalculateAreaLighting(worldPos: vec2<f32>, lightLayer: u32) -> vec3<f32> {
    if (areaLightGlobal.areaLightCount == 0u) {
        return vec3<f32>(0.0);
    }
    
    var totalAreaLight = vec3<f32>(0.0);
    
    let areaLightCount = min(areaLightGlobal.areaLightCount, 64u);
    for (var i = 0u; i < areaLightCount; i = i + 1u) {
        let areaLight = areaLights[i];
        totalAreaLight = totalAreaLight + CalculateSingleAreaLightContribution(areaLight, worldPos, lightLayer);
    }
    
    return totalAreaLight;
}

/// 计算总光照（带阴影、间接光照和面光源）
fn CalculateTotalLightingWithShadow(worldPos: vec2<f32>, lightLayer: u32) -> vec3<f32> {
    // 环境光
    var totalLight = lightingData.ambientColor.rgb * lightingData.ambientIntensity;
    
    // 环境光区域贡献（覆盖或混合基础环境光）
    let ambientZoneLight = CalculateAmbientZoneLighting(worldPos);
    totalLight = totalLight + ambientZoneLight;
    
    // 直接光照（点光源、聚光灯、方向光）
    let lightCount = min(lightingData.lightCount, lightingData.maxLightsPerPixel);
    for (var i = 0u; i < lightCount; i = i + 1u) {
        let light = lights[i];
        totalLight = totalLight + CalculateLightContributionWithShadow(light, worldPos, lightLayer);
    }
    
    // 面光源光照
    let areaLight = CalculateAreaLighting(worldPos, lightLayer);
    totalLight = totalLight + areaLight;
    
    // 间接光照（在阴影区域也有效果）
    let indirectLight = CalculateIndirectLighting(worldPos, lightLayer);
    totalLight = totalLight + indirectLight;
    
    return totalLight;
}

// ============ 环境光区域计算 ============
// Feature: 2d-lighting-enhancement

/// 检查点是否在矩形环境光区域内
fn IsPointInRectangleZone(position: vec2<f32>, zonePosition: vec2<f32>, width: f32, height: f32) -> bool {
    let halfWidth = width / 2.0;
    let halfHeight = height / 2.0;
    return position.x >= zonePosition.x - halfWidth &&
           position.x <= zonePosition.x + halfWidth &&
           position.y >= zonePosition.y - halfHeight &&
           position.y <= zonePosition.y + halfHeight;
}

/// 检查点是否在圆形环境光区域内
fn IsPointInCircleZone(position: vec2<f32>, zonePosition: vec2<f32>, radius: f32) -> bool {
    let diff = position - zonePosition;
    return dot(diff, diff) <= radius * radius;
}

/// 检查点是否在环境光区域内
fn IsPointInAmbientZone(position: vec2<f32>, zone: AmbientZoneData) -> bool {
    if (zone.shape == AMBIENT_ZONE_SHAPE_CIRCLE) {
        return IsPointInCircleZone(position, zone.position, zone.size.x / 2.0);
    } else {
        return IsPointInRectangleZone(position, zone.position, zone.size.x, zone.size.y);
    }
}

/// 计算矩形区域的边缘因子
fn CalculateRectangleEdgeFactor(position: vec2<f32>, zonePosition: vec2<f32>, width: f32, height: f32, edgeSoftness: f32) -> f32 {
    let halfWidth = width / 2.0;
    let halfHeight = height / 2.0;
    
    let localPos = position - zonePosition;
    
    let distToEdgeX = halfWidth - abs(localPos.x);
    let distToEdgeY = halfHeight - abs(localPos.y);
    
    if (distToEdgeX < 0.0 || distToEdgeY < 0.0) {
        return 0.0;
    }
    
    let softWidth = halfWidth * edgeSoftness;
    let softHeight = halfHeight * edgeSoftness;
    
    var factorX = 1.0;
    var factorY = 1.0;
    
    if (softWidth > 0.0 && distToEdgeX < softWidth) {
        factorX = distToEdgeX / softWidth;
    }
    
    if (softHeight > 0.0 && distToEdgeY < softHeight) {
        factorY = distToEdgeY / softHeight;
    }
    
    let factor = min(factorX, factorY);
    return factor * factor * (3.0 - 2.0 * factor);
}

/// 计算圆形区域的边缘因子
fn CalculateCircleEdgeFactor(position: vec2<f32>, zonePosition: vec2<f32>, radius: f32, edgeSoftness: f32) -> f32 {
    let distance = length(position - zonePosition);
    
    if (distance >= radius) {
        return 0.0;
    }
    
    let softRadius = radius * edgeSoftness;
    let distToEdge = radius - distance;
    
    var factor = 1.0;
    if (softRadius > 0.0 && distToEdge < softRadius) {
        factor = distToEdge / softRadius;
    }
    
    return factor * factor * (3.0 - 2.0 * factor);
}

/// 计算环境光区域的边缘因子
fn CalculateAmbientZoneEdgeFactor(position: vec2<f32>, zone: AmbientZoneData) -> f32 {
    if (zone.shape == AMBIENT_ZONE_SHAPE_CIRCLE) {
        return CalculateCircleEdgeFactor(position, zone.position, zone.size.x / 2.0, zone.edgeSoftness);
    } else {
        return CalculateRectangleEdgeFactor(position, zone.position, zone.size.x, zone.size.y, zone.edgeSoftness);
    }
}

/// 计算环境光区域的渐变颜色
fn CalculateAmbientZoneGradientColor(zone: AmbientZoneData, localPosition: vec2<f32>) -> vec4<f32> {
    if (zone.gradientMode == AMBIENT_GRADIENT_NONE) {
        return zone.primaryColor;
    }
    
    var t = 0.0;
    
    if (zone.gradientMode == AMBIENT_GRADIENT_VERTICAL) {
        let halfHeight = zone.size.y / 2.0;
        if (halfHeight > 0.0) {
            t = (localPosition.y + halfHeight) / zone.size.y;
            t = clamp(t, 0.0, 1.0);
        }
    } else if (zone.gradientMode == AMBIENT_GRADIENT_HORIZONTAL) {
        let halfWidth = zone.size.x / 2.0;
        if (halfWidth > 0.0) {
            t = (localPosition.x + halfWidth) / zone.size.x;
            t = clamp(t, 0.0, 1.0);
        }
    }
    
    return mix(zone.primaryColor, zone.secondaryColor, t);
}

/// 计算单个环境光区域的贡献
fn CalculateSingleAmbientZoneContribution(zone: AmbientZoneData, worldPos: vec2<f32>) -> vec3<f32> {
    if (!IsPointInAmbientZone(worldPos, zone)) {
        return vec3<f32>(0.0);
    }
    
    let edgeFactor = CalculateAmbientZoneEdgeFactor(worldPos, zone);
    let localPos = worldPos - zone.position;
    let gradientColor = CalculateAmbientZoneGradientColor(zone, localPos);
    
    let weight = edgeFactor * zone.blendWeight * zone.intensity;
    
    return gradientColor.rgb * weight;
}

/// 计算所有环境光区域的光照贡献
fn CalculateAmbientZoneLighting(worldPos: vec2<f32>) -> vec3<f32> {
    if (ambientZoneGlobal.ambientZoneCount == 0u) {
        return vec3<f32>(0.0);
    }
    
    var totalAmbient = vec3<f32>(0.0);
    
    let zoneCount = min(ambientZoneGlobal.ambientZoneCount, 32u);
    for (var i = 0u; i < zoneCount; i = i + 1u) {
        let zone = ambientZones[i];
        totalAmbient = totalAmbient + CalculateSingleAmbientZoneContribution(zone, worldPos);
    }
    
    return totalAmbient;
}

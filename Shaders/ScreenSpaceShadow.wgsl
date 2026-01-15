// ============================================================================
// ScreenSpaceShadow.wgsl - 屏幕空间阴影着色器
// 使用屏幕空间射线行进计算快速近似阴影
// Feature: 2d-lighting-enhancement
// Requirements: 7.2
// ============================================================================

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;
const SHADOW_BIAS: f32 = 0.001;
const MAX_STEPS: i32 = 32;
const STEP_SIZE: f32 = 0.02;

// ============ 数据结构 ============

/// 屏幕空间阴影参数
struct ScreenSpaceShadowParams {
    lightScreenPos: vec2<f32>,   // 光源屏幕位置 [0, 1]
    lightWorldPos: vec2<f32>,    // 光源世界位置
    resolution: vec2<f32>,       // 屏幕分辨率
    lightRadius: f32,            // 光源影响半径
    shadowIntensity: f32,        // 阴影强度 [0, 1]
    shadowSoftness: f32,         // 阴影柔和度 [0, 1]
    depthScale: f32,             // 深度缩放因子
};

// ============ 绑定组 ============

@group(0) @binding(0) var<uniform> params: ScreenSpaceShadowParams;
@group(0) @binding(1) var depthTexture: texture_2d<f32>;
@group(0) @binding(2) var depthSampler: sampler;
@group(0) @binding(3) var occluderTexture: texture_2d<f32>;
@group(0) @binding(4) var occluderSampler: sampler;

// ============ 顶点着色器输入/输出 ============

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) screenPos: vec2<f32>,
};

// ============ 顶点着色器 ============

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // 全屏四边形
    output.clipPosition = vec4<f32>(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.screenPos = input.uv;
    
    return output;
}

// ============ 深度采样函数 ============

/// 采样深度纹理
fn sampleDepth(uv: vec2<f32>) -> f32 {
    // 边界检查
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 0.0;
    }
    return textureSample(depthTexture, depthSampler, uv).r;
}

/// 采样遮挡物纹理（用于判断是否有物体）
fn sampleOccluder(uv: vec2<f32>) -> f32 {
    // 边界检查
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 0.0;
    }
    return textureSample(occluderTexture, occluderSampler, uv).a;
}

// ============ 屏幕空间阴影计算 ============

/// 计算屏幕空间阴影
/// 从当前像素向光源方向进行射线行进
fn calculateScreenSpaceShadow(screenPos: vec2<f32>) -> f32 {
    // 计算到光源的方向
    let toLight = params.lightScreenPos - screenPos;
    let distToLight = length(toLight);
    
    // 如果太近或太远，不计算阴影
    if (distToLight < SHADOW_BIAS) {
        return 1.0; // 完全照亮
    }
    
    let rayDir = toLight / distToLight;
    
    // 射线行进
    var shadow: f32 = 1.0;
    var t: f32 = STEP_SIZE;
    
    for (var i: i32 = 0; i < MAX_STEPS; i = i + 1) {
        if (t >= distToLight) {
            break;
        }
        
        let samplePos = screenPos + rayDir * t;
        
        // 采样遮挡物
        let occluder = sampleOccluder(samplePos);
        
        // 如果有遮挡物，减少光照
        if (occluder > 0.5) {
            // 软阴影：根据距离和柔和度计算
            let shadowFactor = 1.0 - (t / distToLight);
            shadow = shadow * (1.0 - occluder * params.shadowIntensity * shadowFactor);
        }
        
        t = t + STEP_SIZE;
    }
    
    return clamp(shadow, 0.0, 1.0);
}

/// 计算软阴影（使用多次采样）
fn calculateSoftScreenSpaceShadow(screenPos: vec2<f32>) -> f32 {
    // 基础阴影
    var shadow = calculateScreenSpaceShadow(screenPos);
    
    // 如果启用软阴影，进行额外采样
    if (params.shadowSoftness > 0.0) {
        let offset = params.shadowSoftness * 0.01;
        
        // 4 个方向的偏移采样
        shadow = shadow + calculateScreenSpaceShadow(screenPos + vec2<f32>(offset, 0.0));
        shadow = shadow + calculateScreenSpaceShadow(screenPos + vec2<f32>(-offset, 0.0));
        shadow = shadow + calculateScreenSpaceShadow(screenPos + vec2<f32>(0.0, offset));
        shadow = shadow + calculateScreenSpaceShadow(screenPos + vec2<f32>(0.0, -offset));
        
        shadow = shadow / 5.0;
    }
    
    return shadow;
}

// ============ 基于深度的阴影计算 ============

/// 使用深度缓冲区计算阴影
fn calculateDepthBasedShadow(screenPos: vec2<f32>) -> f32 {
    // 当前像素的深度
    let currentDepth = sampleDepth(screenPos);
    
    // 计算到光源的方向
    let toLight = params.lightScreenPos - screenPos;
    let distToLight = length(toLight);
    
    if (distToLight < SHADOW_BIAS) {
        return 1.0;
    }
    
    let rayDir = toLight / distToLight;
    
    // 射线行进，检查深度
    var shadow: f32 = 1.0;
    var t: f32 = STEP_SIZE;
    
    for (var i: i32 = 0; i < MAX_STEPS; i = i + 1) {
        if (t >= distToLight) {
            break;
        }
        
        let samplePos = screenPos + rayDir * t;
        let sampleDepth = sampleDepth(samplePos);
        
        // 如果采样点的深度比当前像素更近（在前面），则有遮挡
        if (sampleDepth > currentDepth + SHADOW_BIAS) {
            // 计算遮挡强度
            let depthDiff = (sampleDepth - currentDepth) * params.depthScale;
            let occlusionFactor = clamp(depthDiff, 0.0, 1.0);
            
            // 软阴影：距离越远，阴影越柔和
            let distanceFactor = 1.0 - (t / distToLight);
            shadow = shadow * (1.0 - occlusionFactor * params.shadowIntensity * distanceFactor);
        }
        
        t = t + STEP_SIZE;
    }
    
    return clamp(shadow, 0.0, 1.0);
}

// ============ 片段着色器 ============

/// 屏幕空间阴影片段着色器
@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // 计算阴影
    let shadow = calculateSoftScreenSpaceShadow(input.screenPos);
    
    // 输出阴影值
    return vec4<f32>(shadow, shadow, shadow, 1.0);
}

/// 基于深度的阴影片段着色器
@fragment
fn depthShadowFragment(input: VertexOutput) -> @location(0) vec4<f32> {
    let shadow = calculateDepthBasedShadow(input.screenPos);
    return vec4<f32>(shadow, shadow, shadow, 1.0);
}

/// 阴影遮罩输出（用于与光照合成）
@fragment
fn shadowMaskFragment(input: VertexOutput) -> @location(0) f32 {
    return calculateSoftScreenSpaceShadow(input.screenPos);
}

// ============ 接触阴影（Contact Shadow） ============

/// 计算接触阴影
/// 用于近距离物体之间的阴影
fn calculateContactShadow(screenPos: vec2<f32>, lightDir: vec2<f32>) -> f32 {
    let currentDepth = sampleDepth(screenPos);
    
    var shadow: f32 = 1.0;
    var t: f32 = 0.001;
    let maxDist: f32 = 0.1; // 接触阴影最大距离
    
    for (var i: i32 = 0; i < 16; i = i + 1) {
        if (t >= maxDist) {
            break;
        }
        
        let samplePos = screenPos + lightDir * t;
        let sampleDepth = sampleDepth(samplePos);
        
        // 检查是否有遮挡
        if (sampleDepth > currentDepth + SHADOW_BIAS) {
            let occlusionFactor = clamp((sampleDepth - currentDepth) * 10.0, 0.0, 1.0);
            shadow = shadow * (1.0 - occlusionFactor * 0.5);
        }
        
        t = t + 0.005;
    }
    
    return shadow;
}

/// 接触阴影片段着色器
@fragment
fn contactShadowFragment(input: VertexOutput) -> @location(0) vec4<f32> {
    let lightDir = normalize(params.lightScreenPos - input.screenPos);
    let shadow = calculateContactShadow(input.screenPos, lightDir);
    return vec4<f32>(shadow, shadow, shadow, 1.0);
}

// ============ 阴影合成 ============

/// 合成多个阴影源
fn blendShadows(shadow1: f32, shadow2: f32) -> f32 {
    // 使用乘法混合
    return shadow1 * shadow2;
}

/// 应用阴影衰减
fn applyShadowAttenuation(shadow: f32, distance: f32, maxDistance: f32) -> f32 {
    let attenuation = 1.0 - clamp(distance / maxDistance, 0.0, 1.0);
    return shadow * attenuation + (1.0 - attenuation);
}

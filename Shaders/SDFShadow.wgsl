// ============================================================================
// SDFShadow.wgsl - SDF 软阴影着色器
// 使用有符号距离场计算高质量软阴影
// Feature: 2d-lighting-enhancement
// Requirements: 7.1, 7.4
// ============================================================================

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;
const SDF_BIAS: f32 = 0.001;
const MAX_MARCH_STEPS: i32 = 64;
const MIN_MARCH_DISTANCE: f32 = 0.01;
const MAX_MARCH_DISTANCE: f32 = 1000.0;

// ============ 数据结构 ============

/// SDF 阴影参数
struct SDFShadowParams {
    lightPosition: vec2<f32>,    // 光源位置
    lightRadius: f32,            // 光源影响半径
    shadowSoftness: f32,         // 软阴影程度 [0, 1]
    resolution: vec2<f32>,       // 屏幕分辨率
    sdfScale: f32,               // SDF 缩放因子
    sdfBias: f32,                // SDF 偏移
};

/// SDF 纹理元数据
struct SDFMetadata {
    origin: vec2<f32>,           // SDF 原点（世界坐标）
    size: vec2<f32>,             // SDF 覆盖区域大小
    cellSize: f32,               // 单元格大小
    textureWidth: u32,           // 纹理宽度
    textureHeight: u32,          // 纹理高度
    padding: f32,                // 对齐填充
};

// ============ 绑定组 ============

@group(0) @binding(0) var<uniform> params: SDFShadowParams;
@group(0) @binding(1) var<uniform> sdfMeta: SDFMetadata;
@group(0) @binding(2) var sdfTexture: texture_2d<f32>;
@group(0) @binding(3) var sdfSampler: sampler;

// ============ 顶点着色器输入/输出 ============

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) worldPosition: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

// ============ 顶点着色器 ============

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // 全屏四边形
    output.clipPosition = vec4<f32>(input.position, 0.0, 1.0);
    output.worldPosition = input.position * params.resolution * 0.5;
    output.uv = input.uv;
    
    return output;
}

// ============ SDF 采样函数 ============

/// 将世界坐标转换为 SDF UV 坐标
fn worldToSDFUV(worldPos: vec2<f32>) -> vec2<f32> {
    let localPos = worldPos - sdfMeta.origin;
    return localPos / sdfMeta.size;
}

/// 采样 SDF 纹理
fn sampleSDF(worldPos: vec2<f32>) -> f32 {
    let uv = worldToSDFUV(worldPos);
    
    // 边界检查
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return MAX_MARCH_DISTANCE;
    }
    
    return textureSample(sdfTexture, sdfSampler, uv).r * params.sdfScale + params.sdfBias;
}

// ============ 软阴影计算 ============

/// 使用球体追踪计算软阴影
/// 基于 Inigo Quilez 的软阴影算法
/// Requirements: 7.4 - 阴影边缘柔和度随距离增加
fn calculateSoftShadow(rayOrigin: vec2<f32>, rayDir: vec2<f32>, maxDist: f32, softness: f32) -> f32 {
    var shadow: f32 = 1.0;
    var t: f32 = MIN_MARCH_DISTANCE;
    var prevDist: f32 = 1e10;
    
    for (var i: i32 = 0; i < MAX_MARCH_STEPS; i = i + 1) {
        if (t >= maxDist) {
            break;
        }
        
        let samplePos = rayOrigin + rayDir * t;
        let dist = sampleSDF(samplePos);
        
        // 如果在物体内部，完全阴影
        if (dist < SDF_BIAS) {
            return 0.0;
        }
        
        // 改进的软阴影计算
        // 使用 Inigo Quilez 的改进公式，避免带状伪影
        let y = dist * dist / (2.0 * prevDist);
        let d = sqrt(dist * dist - y * y);
        
        // softness 控制阴影柔和度
        // 距离越远，阴影越柔和（Requirements: 7.4）
        let penumbra = softness * d / max(0.0, t - y);
        shadow = min(shadow, penumbra);
        
        prevDist = dist;
        t = t + dist;
    }
    
    return clamp(shadow, 0.0, 1.0);
}

/// 简化版软阴影（性能优先）
fn calculateSoftShadowSimple(rayOrigin: vec2<f32>, rayDir: vec2<f32>, maxDist: f32, softness: f32) -> f32 {
    var shadow: f32 = 1.0;
    var t: f32 = MIN_MARCH_DISTANCE;
    
    for (var i: i32 = 0; i < MAX_MARCH_STEPS; i = i + 1) {
        if (t >= maxDist) {
            break;
        }
        
        let samplePos = rayOrigin + rayDir * t;
        let dist = sampleSDF(samplePos);
        
        // 如果在物体内部，完全阴影
        if (dist < SDF_BIAS) {
            return 0.0;
        }
        
        // 简单的软阴影公式
        // 距离越小，阴影越硬；t 越大（距离光源越远），阴影越柔和
        let penumbra = softness * dist / t;
        shadow = min(shadow, penumbra);
        
        t = t + max(dist, MIN_MARCH_DISTANCE);
    }
    
    return clamp(shadow, 0.0, 1.0);
}

// ============ 硬阴影计算 ============

/// 使用球体追踪计算硬阴影
fn calculateHardShadow(rayOrigin: vec2<f32>, rayDir: vec2<f32>, maxDist: f32) -> f32 {
    var t: f32 = MIN_MARCH_DISTANCE;
    
    for (var i: i32 = 0; i < MAX_MARCH_STEPS; i = i + 1) {
        if (t >= maxDist) {
            break;
        }
        
        let samplePos = rayOrigin + rayDir * t;
        let dist = sampleSDF(samplePos);
        
        // 如果在物体内部，完全阴影
        if (dist < SDF_BIAS) {
            return 0.0;
        }
        
        t = t + max(dist, MIN_MARCH_DISTANCE);
    }
    
    return 1.0;
}

// ============ 片段着色器 ============

/// SDF 阴影片段着色器
/// 计算从光源到当前像素的阴影
@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    let worldPos = input.worldPosition;
    let lightPos = params.lightPosition;
    
    // 计算到光源的方向和距离
    let toLight = lightPos - worldPos;
    let distToLight = length(toLight);
    
    // 如果超出光源半径，无光照
    if (distToLight > params.lightRadius) {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
    }
    
    // 归一化方向
    let rayDir = toLight / distToLight;
    
    // 计算阴影
    var shadow: f32;
    if (params.shadowSoftness > 0.0) {
        // 软阴影
        shadow = calculateSoftShadow(worldPos, rayDir, distToLight, params.shadowSoftness);
    } else {
        // 硬阴影
        shadow = calculateHardShadow(worldPos, rayDir, distToLight);
    }
    
    // 输出阴影值（1 = 完全照亮，0 = 完全阴影）
    return vec4<f32>(shadow, shadow, shadow, 1.0);
}

// ============ 阴影遮挡查询着色器 ============

/// 查询指定点的阴影遮挡
/// 用于在光照计算中采样阴影
@fragment
fn shadowQueryFragment(input: VertexOutput) -> @location(0) f32 {
    let worldPos = input.worldPosition;
    let lightPos = params.lightPosition;
    
    // 计算到光源的方向和距离
    let toLight = lightPos - worldPos;
    let distToLight = length(toLight);
    
    // 如果超出光源半径，无阴影（完全黑暗）
    if (distToLight > params.lightRadius) {
        return 0.0;
    }
    
    // 归一化方向
    let rayDir = toLight / distToLight;
    
    // 计算软阴影
    let shadow = calculateSoftShadow(worldPos, rayDir, distToLight, params.shadowSoftness);
    
    return shadow;
}

// ============ 距离场可视化（调试用） ============

/// 可视化 SDF 距离场
@fragment
fn visualizeSDFFragment(input: VertexOutput) -> @location(0) vec4<f32> {
    let dist = sampleSDF(input.worldPosition);
    
    // 内部为红色，外部为蓝色，边缘为白色
    if (dist < 0.0) {
        // 内部：红色渐变
        let intensity = clamp(-dist * 0.1, 0.0, 1.0);
        return vec4<f32>(1.0, 1.0 - intensity, 1.0 - intensity, 1.0);
    } else if (dist < 0.1) {
        // 边缘：白色
        return vec4<f32>(1.0, 1.0, 1.0, 1.0);
    } else {
        // 外部：蓝色渐变
        let intensity = clamp(dist * 0.1, 0.0, 1.0);
        return vec4<f32>(1.0 - intensity, 1.0 - intensity, 1.0, 1.0);
    }
}

// ============ 多光源阴影合成 ============

/// 合成多个光源的阴影
/// 使用最小值混合，确保任何光源照亮的区域都不会完全黑暗
fn blendShadows(shadow1: f32, shadow2: f32) -> f32 {
    return max(shadow1, shadow2);
}

/// 计算阴影衰减
/// 距离光源越远，阴影影响越小
fn shadowAttenuation(shadow: f32, distance: f32, lightRadius: f32) -> f32 {
    let distanceFactor = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);
    return shadow * distanceFactor + (1.0 - distanceFactor);
}

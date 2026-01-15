// ============================================================================
// ShadowCore.wgsl - 2D 阴影渲染核心模块
// 提供阴影体生成、软阴影模糊和阴影计算功能
// Feature: 2d-lighting-system
// ============================================================================
export Shadow;

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;
const SHADOW_BIAS: f32 = 0.001;
const MAX_SHADOW_DISTANCE: f32 = 1000.0;

// ============ 数据结构 ============

/// 阴影投射器顶点数据
struct ShadowVertex {
    position: vec2<f32>,     // 顶点位置
    lightDir: vec2<f32>,     // 光源方向（用于软阴影）
    opacity: f32,            // 阴影不透明度
    padding: vec3<f32>,      // 对齐填充
};

/// 阴影渲染参数
struct ShadowParams {
    lightPosition: vec2<f32>,    // 光源位置
    lightRadius: f32,            // 光源半径
    shadowSoftness: f32,         // 软阴影程度 [0, 1]
    resolution: vec2<f32>,       // 阴影贴图分辨率
    padding: vec2<f32>,          // 对齐填充
};

/// 阴影边缘数据
struct ShadowEdge {
    start: vec2<f32>,    // 边的起点
    end: vec2<f32>,      // 边的终点
};

// ============ 顶点着色器输入/输出 ============

struct ShadowVertexInput {
    @location(0) position: vec2<f32>,
    @location(1) extrudeDir: vec2<f32>,
    @location(2) opacity: f32,
};

struct ShadowVertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) worldPosition: vec2<f32>,
    @location(1) opacity: f32,
};

struct ShadowSampleInput {
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组 ============

@group(0) @binding(0) var<uniform> shadowParams: ShadowParams;
@group(0) @binding(1) var<storage, read> shadowEdges: array<ShadowEdge>;
@group(0) @binding(2) var shadowMapTexture: texture_2d<f32>;
@group(0) @binding(3) var shadowSampler: sampler;

// ============ 软阴影采样偏移（泊松圆盘分布） ============

const POISSON_DISK_16: array<vec2<f32>, 16> = array<vec2<f32>, 16>(
    vec2<f32>(-0.94201624, -0.39906216),
    vec2<f32>(0.94558609, -0.76890725),
    vec2<f32>(-0.094184101, -0.92938870),
    vec2<f32>(0.34495938, 0.29387760),
    vec2<f32>(-0.91588581, 0.45771432),
    vec2<f32>(-0.81544232, -0.87912464),
    vec2<f32>(-0.38277543, 0.27676845),
    vec2<f32>(0.97484398, 0.75648379),
    vec2<f32>(0.44323325, -0.97511554),
    vec2<f32>(0.53742981, -0.47373420),
    vec2<f32>(-0.26496911, -0.41893023),
    vec2<f32>(0.79197514, 0.19090188),
    vec2<f32>(-0.24188840, 0.99706507),
    vec2<f32>(-0.81409955, 0.91437590),
    vec2<f32>(0.19984126, 0.78641367),
    vec2<f32>(0.14383161, -0.14100790)
);

// ============ 阴影体生成函数 ============

/// 阴影体顶点变换
/// 将阴影投射器的边缘向远离光源的方向延伸
fn TransformShadowVolumeVertex(input: ShadowVertexInput, vertexIndex: u32) -> ShadowVertexOutput {
    var output: ShadowVertexOutput;
    
    // 计算从光源到顶点的方向
    let toVertex = input.position - shadowParams.lightPosition;
    let distance = length(toVertex);
    
    // 如果顶点在光源位置，不延伸
    if (distance < SHADOW_BIAS) {
        output.clipPosition = vec4<f32>(input.position / shadowParams.resolution * 2.0 - 1.0, 0.0, 1.0);
        output.worldPosition = input.position;
        output.opacity = input.opacity;
        return output;
    }
    
    // 归一化方向
    let dir = toVertex / distance;
    
    // 根据 extrudeDir 决定是否延伸
    // extrudeDir.x > 0 表示需要延伸到远处
    var finalPosition = input.position;
    if (input.extrudeDir.x > 0.5) {
        // 延伸到光源半径之外
        finalPosition = input.position + dir * MAX_SHADOW_DISTANCE;
    }
    
    // 转换到裁剪空间
    let normalizedPos = finalPosition / shadowParams.resolution * 2.0 - 1.0;
    output.clipPosition = vec4<f32>(normalizedPos, 0.0, 1.0);
    output.worldPosition = finalPosition;
    output.opacity = input.opacity;
    
    return output;
}

// ============ 射线投射阴影计算 ============

/// 计算射线与边缘的交点
/// 返回射线参数 t，如果不相交返回 -1
fn RayEdgeIntersection(rayOrigin: vec2<f32>, rayDir: vec2<f32>, edge: ShadowEdge) -> f32 {
    let edgeDir = edge.end - edge.start;
    let originToStart = edge.start - rayOrigin;
    
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

/// 检查点是否在阴影中
/// 通过从光源向目标点发射射线，检查是否被边缘阻挡
fn IsPointInShadow(point: vec2<f32>, lightPos: vec2<f32>, edgeCount: u32) -> f32 {
    let toPoint = point - lightPos;
    let distToPoint = length(toPoint);
    
    if (distToPoint < SHADOW_BIAS) {
        return 0.0; // 点在光源位置，不在阴影中
    }
    
    let rayDir = toPoint / distToPoint;
    
    // 检查所有边缘
    for (var i = 0u; i < edgeCount; i = i + 1u) {
        let edge = shadowEdges[i];
        let t = RayEdgeIntersection(lightPos, rayDir, edge);
        
        // 如果交点在光源和目标点之间，则点在阴影中
        if (t > 0.0 && t < distToPoint - SHADOW_BIAS) {
            return 1.0;
        }
    }
    
    return 0.0;
}

// ============ 软阴影模糊函数 ============

/// 软阴影采样
/// 使用泊松圆盘采样实现软阴影边缘
fn SampleSoftShadow(uv: vec2<f32>, softness: f32) -> f32 {
    var shadow = 0.0;
    let texelSize = 1.0 / shadowParams.resolution;
    let blurRadius = softness * 4.0; // 模糊半径
    
    // 泊松圆盘采样
    for (var i = 0; i < 16; i = i + 1) {
        let offset = POISSON_DISK_16[i] * texelSize * blurRadius;
        shadow = shadow + textureSample(shadowMapTexture, shadowSampler, uv + offset).r;
    }
    
    return shadow / 16.0;
}

/// 高斯模糊软阴影
/// 使用 5x5 高斯核进行模糊
fn GaussianBlurShadow(uv: vec2<f32>, softness: f32) -> f32 {
    let texelSize = 1.0 / shadowParams.resolution;
    let blurRadius = softness * 2.0;
    
    // 高斯权重（5x5 核，sigma ≈ 1.0）
    let weights = array<f32, 5>(0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    
    var shadow = textureSample(shadowMapTexture, shadowSampler, uv).r * weights[0];
    
    // 水平和垂直方向采样
    for (var i = 1; i < 5; i = i + 1) {
        let offset = f32(i) * texelSize * blurRadius;
        
        // 水平方向
        shadow = shadow + textureSample(shadowMapTexture, shadowSampler, uv + vec2<f32>(offset, 0.0)).r * weights[i];
        shadow = shadow + textureSample(shadowMapTexture, shadowSampler, uv - vec2<f32>(offset, 0.0)).r * weights[i];
        
        // 垂直方向
        shadow = shadow + textureSample(shadowMapTexture, shadowSampler, uv + vec2<f32>(0.0, offset)).r * weights[i];
        shadow = shadow + textureSample(shadowMapTexture, shadowSampler, uv - vec2<f32>(0.0, offset)).r * weights[i];
    }
    
    return shadow;
}

// ============ 阴影采样函数 ============

/// 采样阴影贴图（根据软阴影设置自动选择）
/// 使用 select 代替 if 来保持均匀控制流
fn SampleShadowMap(uv: vec2<f32>) -> f32 {
    // 始终采样两种方式，然后用 select 选择
    let softShadow = SampleSoftShadow(uv, shadowParams.shadowSoftness);
    let hardShadow = textureSample(shadowMapTexture, shadowSampler, uv).r;
    return select(hardShadow, softShadow, shadowParams.shadowSoftness > 0.0);
}

// ============ 阴影遮挡计算 ============

/// 计算阴影遮挡因子
/// 返回 0.0 表示完全照亮，1.0 表示完全阴影
/// 使用 select 代替 if 来保持均匀控制流
fn CalculateShadowOcclusion(worldPos: vec2<f32>, lightPos: vec2<f32>, lightRadius: f32) -> f32 {
    // 计算到光源的距离
    let distance = length(worldPos - lightPos);
    
    // 将世界坐标转换为阴影贴图 UV
    let uv = (worldPos - lightPos + vec2<f32>(lightRadius)) / (lightRadius * 2.0);
    
    // 检查是否在有效范围内
    let inRadius = distance <= lightRadius;
    let inBounds = uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
    let isValid = inRadius && inBounds;
    
    // 钳制UV以避免越界采样
    let clampedUV = clamp(uv, vec2<f32>(0.0), vec2<f32>(1.0));
    
    // 采样阴影贴图
    let shadowValue = SampleShadowMap(clampedUV);
    
    return select(0.0, shadowValue, isValid);
}

// ============ 1D 阴影贴图（用于点光源） ============

/// 将角度转换为 1D 阴影贴图 UV
fn AngleToShadowMapU(angle: f32) -> f32 {
    return (angle + PI) / (2.0 * PI);
}

/// 从 1D 阴影贴图采样
/// 点光源使用 1D 阴影贴图，存储每个角度方向的最近遮挡距离
/// 使用 select 代替 if 来保持均匀控制流
fn Sample1DShadowMap(worldPos: vec2<f32>, lightPos: vec2<f32>) -> f32 {
    let toPoint = worldPos - lightPos;
    let distance = length(toPoint);
    
    // 计算角度（即使距离很小也计算，避免分支）
    let safeToPoint = select(toPoint, vec2<f32>(1.0, 0.0), distance < SHADOW_BIAS);
    let angle = atan2(safeToPoint.y, safeToPoint.x);
    let u = AngleToShadowMapU(angle);
    
    // 采样阴影贴图（存储的是到最近遮挡物的距离）
    let shadowDistance = textureSample(shadowMapTexture, shadowSampler, vec2<f32>(u, 0.5)).r;
    
    // 如果当前点的距离大于阴影距离，则在阴影中
    let inShadow = distance > shadowDistance + SHADOW_BIAS;
    let tooClose = distance < SHADOW_BIAS;
    
    return select(select(0.0, 1.0, inShadow), 0.0, tooClose);
}

// ============ 辅助函数 ============

/// 计算阴影衰减
/// 阴影边缘的衰减，使阴影更自然
fn ShadowAttenuation(shadowValue: f32, distance: f32, lightRadius: f32) -> f32 {
    // 距离衰减：越远阴影越淡
    let distanceFactor = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);
    
    // 应用衰减
    return shadowValue * distanceFactor;
}

/// 混合多个光源的阴影
/// 使用最大值混合，确保任何光源照亮的区域都不会完全黑暗
fn BlendShadows(shadow1: f32, shadow2: f32) -> f32 {
    return min(shadow1, shadow2);
}

// ============================================================================
// LightShaft.wgsl - 光束效果着色器
// 实现屏幕空间径向模糊光束效果（体积光/God Rays）
// Feature: 2d-lighting-enhancement
// Requirements: 6.1, 6.5
// ============================================================================
export LightShaft;

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;

/// 径向模糊采样次数（性能与质量的平衡）
const NUM_SAMPLES: i32 = 64;

/// 最小采样步长（防止采样过密）
const MIN_STEP_SIZE: f32 = 0.001;

// ============ 数据结构 ============

/// 光束参数结构（与 C++ LightShaftParams 对应，64 字节对齐）
struct LightShaftParams {
    // 光源屏幕空间位置（UV 坐标 [0,1]）
    lightScreenPos: vec2<f32>,
    // 光源世界位置
    lightWorldPos: vec2<f32>,
    // -- 16 字节边界 --
    
    // 光束颜色
    lightColor: vec4<f32>,
    // -- 16 字节边界 --
    
    // 光束参数
    density: f32,           ///< 光束密度 [0, 1]
    decay: f32,             ///< 光束衰减 [0, 1]
    weight: f32,            ///< 光束权重 [0, 1]
    exposure: f32,          ///< 光束曝光 [0, ∞)
    // -- 16 字节边界 --
    
    // 额外参数
    numSamples: u32,        ///< 采样次数
    lightRadius: f32,       ///< 光源影响半径
    lightIntensity: f32,    ///< 光源强度
    enableOcclusion: u32,   ///< 是否启用遮挡 (0 = false, 1 = true)
    // -- 16 字节边界 --
};

/// 全屏四边形顶点输出
struct FullscreenVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组定义 ============

// Group 0: 光束参数
@group(0) @binding(0) var<uniform> lightShaftParams: LightShaftParams;

// Group 1: 场景纹理
@group(1) @binding(0) var sceneTexture: texture_2d<f32>;
@group(1) @binding(1) var sceneSampler: sampler;

// Group 2: 阴影/遮挡纹理
@group(2) @binding(0) var occlusionTexture: texture_2d<f32>;
@group(2) @binding(1) var occlusionSampler: sampler;

// ============ 全屏四边形顶点着色器 ============

/// 生成全屏四边形的顶点位置和 UV
/// 使用 3 个顶点绘制覆盖整个屏幕的三角形
@vertex
fn vs_lightshaft(@builtin(vertex_index) vertexIndex: u32) -> FullscreenVertexOutput {
    var output: FullscreenVertexOutput;
    
    // 使用大三角形覆盖整个屏幕
    let x = f32((vertexIndex & 1u) << 2u) - 1.0;
    let y = f32((vertexIndex & 2u) << 1u) - 1.0;
    
    output.position = vec4<f32>(x, y, 0.0, 1.0);
    
    // UV 坐标: Y 轴翻转以匹配纹理坐标系统
    output.uv = vec2<f32>((x + 1.0) * 0.5, (1.0 - y) * 0.5);
    
    return output;
}

// ============ 径向模糊核心函数 ============

/// 计算径向模糊采样位置
/// 从当前像素向光源位置方向采样
fn GetRadialBlurSampleUV(uv: vec2<f32>, lightPos: vec2<f32>, sampleIndex: f32, totalSamples: f32, density: f32) -> vec2<f32> {
    // 计算从当前像素到光源的方向
    let direction = lightPos - uv;
    
    // 计算采样步长（基于密度和采样索引）
    let stepSize = density / totalSamples;
    
    // 计算采样偏移
    let offset = direction * sampleIndex * stepSize;
    
    return uv + offset;
}

/// 径向模糊光束效果
/// 从当前像素向光源方向进行多次采样并累加
fn RadialBlur(uv: vec2<f32>, lightPos: vec2<f32>, density: f32, decay: f32, weight: f32, numSamples: i32) -> vec3<f32> {
    // 计算从当前像素到光源的方向向量
    let deltaTexCoord = (uv - lightPos) * density / f32(numSamples);
    
    var sampleUV = uv;
    var illuminationDecay = 1.0;
    var color = vec3<f32>(0.0);
    
    // 沿光源方向进行采样
    for (var i = 0; i < numSamples; i = i + 1) {
        // 向光源方向移动采样点
        sampleUV = sampleUV - deltaTexCoord;
        
        // 钳制 UV 到有效范围
        let clampedUV = clamp(sampleUV, vec2<f32>(0.0), vec2<f32>(1.0));
        
        // 采样场景颜色
        let sampleColor = textureSample(sceneTexture, sceneSampler, clampedUV).rgb;
        
        // 累加颜色（带衰减）
        color = color + sampleColor * illuminationDecay * weight;
        
        // 应用衰减
        illuminationDecay = illuminationDecay * decay;
    }
    
    return color;
}

/// 带遮挡的径向模糊光束效果
/// 在采样过程中考虑阴影遮挡
fn RadialBlurWithOcclusion(uv: vec2<f32>, lightPos: vec2<f32>, density: f32, decay: f32, weight: f32, numSamples: i32) -> vec3<f32> {
    let deltaTexCoord = (uv - lightPos) * density / f32(numSamples);
    
    var sampleUV = uv;
    var illuminationDecay = 1.0;
    var color = vec3<f32>(0.0);
    
    for (var i = 0; i < numSamples; i = i + 1) {
        sampleUV = sampleUV - deltaTexCoord;
        
        let clampedUV = clamp(sampleUV, vec2<f32>(0.0), vec2<f32>(1.0));
        
        // 采样场景颜色
        let sampleColor = textureSample(sceneTexture, sceneSampler, clampedUV).rgb;
        
        // 采样遮挡值（0 = 完全遮挡，1 = 无遮挡）
        // 阴影贴图中 0 = 无阴影（无遮挡），1 = 完全阴影（完全遮挡）
        let shadowValue = textureSample(occlusionTexture, occlusionSampler, clampedUV).r;
        let occlusion = 1.0 - shadowValue;
        
        // 累加颜色（带衰减和遮挡）
        color = color + sampleColor * illuminationDecay * weight * occlusion;
        
        // 应用衰减
        illuminationDecay = illuminationDecay * decay;
    }
    
    return color;
}

/// 计算遮挡物对光束的影响
/// 基于阴影缓冲区计算光束在该点的遮挡程度
fn CalculateOcclusionFactor(uv: vec2<f32>, lightPos: vec2<f32>, numSamples: i32) -> f32 {
    let deltaTexCoord = (uv - lightPos) / f32(numSamples);
    
    var sampleUV = lightPos;
    var totalOcclusion = 0.0;
    var sampleCount = 0.0;
    
    // 从光源向当前像素采样遮挡
    for (var i = 0; i < numSamples; i = i + 1) {
        sampleUV = sampleUV + deltaTexCoord;
        
        // 检查是否超出当前像素位置
        let dist = length(sampleUV - lightPos);
        let targetDist = length(uv - lightPos);
        if (dist > targetDist) {
            break;
        }
        
        let clampedUV = clamp(sampleUV, vec2<f32>(0.0), vec2<f32>(1.0));
        
        // 采样阴影值
        let shadowValue = textureSample(occlusionTexture, occlusionSampler, clampedUV).r;
        totalOcclusion = totalOcclusion + shadowValue;
        sampleCount = sampleCount + 1.0;
    }
    
    // 返回平均遮挡因子（0 = 无遮挡，1 = 完全遮挡）
    if (sampleCount > 0.0) {
        return totalOcclusion / sampleCount;
    }
    return 0.0;
}

// ============ 光束强度计算 ============

/// 计算光束强度衰减
/// 基于到光源的距离计算衰减
fn CalculateLightShaftIntensity(uv: vec2<f32>, lightPos: vec2<f32>, lightRadius: f32) -> f32 {
    let distance = length(uv - lightPos);
    
    // 使用平滑衰减
    let normalizedDist = distance / max(lightRadius, 0.001);
    let attenuation = 1.0 - clamp(normalizedDist, 0.0, 1.0);
    
    // 应用平滑曲线
    return attenuation * attenuation;
}

/// 计算光源可见性
/// 检查光源是否在屏幕内
fn IsLightVisible(lightPos: vec2<f32>) -> f32 {
    // 扩展边界以允许屏幕外的光源产生部分效果
    let margin = 0.5;
    let inBounds = lightPos.x >= -margin && lightPos.x <= 1.0 + margin &&
                   lightPos.y >= -margin && lightPos.y <= 1.0 + margin;
    
    return select(0.0, 1.0, inBounds);
}

// ============ 片段着色器 ============

/// 光束效果片段着色器（无遮挡）
/// 实现基本的屏幕空间径向模糊光束效果
@fragment
fn fs_lightshaft(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let lightPos = lightShaftParams.lightScreenPos;
    let density = lightShaftParams.density;
    let decay = lightShaftParams.decay;
    let weight = lightShaftParams.weight;
    let exposure = lightShaftParams.exposure;
    let numSamples = i32(lightShaftParams.numSamples);
    
    // 检查光源可见性
    let visibility = IsLightVisible(lightPos);
    if (visibility < 0.5) {
        // 光源不可见，返回原始场景颜色
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 计算径向模糊光束
    let lightShaftColor = RadialBlur(input.uv, lightPos, density, decay, weight, numSamples);
    
    // 应用曝光和光源颜色
    let finalLightShaft = lightShaftColor * exposure * lightShaftParams.lightColor.rgb * lightShaftParams.lightIntensity;
    
    // 采样原始场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv).rgb;
    
    // 叠加光束效果（加法混合）
    let finalColor = sceneColor + finalLightShaft;
    
    return vec4<f32>(finalColor, 1.0);
}

/// 光束效果片段着色器（带遮挡）
/// 实现考虑阴影遮挡的光束效果
@fragment
fn fs_lightshaft_occluded(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let lightPos = lightShaftParams.lightScreenPos;
    let density = lightShaftParams.density;
    let decay = lightShaftParams.decay;
    let weight = lightShaftParams.weight;
    let exposure = lightShaftParams.exposure;
    let numSamples = i32(lightShaftParams.numSamples);
    
    // 检查光源可见性
    let visibility = IsLightVisible(lightPos);
    if (visibility < 0.5) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 计算带遮挡的径向模糊光束
    let lightShaftColor = RadialBlurWithOcclusion(input.uv, lightPos, density, decay, weight, numSamples);
    
    // 应用曝光和光源颜色
    let finalLightShaft = lightShaftColor * exposure * lightShaftParams.lightColor.rgb * lightShaftParams.lightIntensity;
    
    // 采样原始场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv).rgb;
    
    // 叠加光束效果
    let finalColor = sceneColor + finalLightShaft;
    
    return vec4<f32>(finalColor, 1.0);
}

/// 光束提取片段着色器
/// 从场景中提取高亮区域用于光束效果
@fragment
fn fs_lightshaft_extract(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let lightPos = lightShaftParams.lightScreenPos;
    let lightRadius = lightShaftParams.lightRadius;
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 计算到光源的距离衰减
    let intensity = CalculateLightShaftIntensity(input.uv, lightPos, lightRadius);
    
    // 提取高亮区域（基于亮度和距离）
    let luminance = dot(sceneColor.rgb, vec3<f32>(0.2126, 0.7152, 0.0722));
    let threshold = 0.5;
    let brightPass = max(luminance - threshold, 0.0) / max(luminance, 0.001);
    
    // 结合距离衰减和亮度
    let extractedColor = sceneColor.rgb * brightPass * intensity;
    
    return vec4<f32>(extractedColor, 1.0);
}

/// 光束合成片段着色器
/// 将光束效果叠加到最终图像
@fragment
fn fs_lightshaft_composite(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 采样光束颜色（存储在遮挡纹理槽位）
    let lightShaftColor = textureSample(occlusionTexture, occlusionSampler, input.uv);
    
    // 应用曝光
    let exposure = lightShaftParams.exposure;
    let finalLightShaft = lightShaftColor.rgb * exposure;
    
    // 叠加光束效果（加法混合）
    let finalColor = sceneColor.rgb + finalLightShaft;
    
    return vec4<f32>(finalColor, sceneColor.a);
}

// ============ 多光源支持 ============

/// 多光源光束数据
struct MultiLightShaftData {
    lightPositions: array<vec4<f32>, 8>,  ///< 最多 8 个光源位置 (xy = 屏幕位置, zw = 颜色强度)
    lightColors: array<vec4<f32>, 8>,     ///< 光源颜色
    lightCount: u32,                       ///< 实际光源数量
    padding: vec3<u32>,
};

// 注意：多光源版本需要额外的绑定组，这里只提供单光源实现
// 多光源可以通过多次渲染 Pass 实现，每次处理一个光源


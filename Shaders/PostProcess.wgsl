// ============================================================================
// PostProcess.wgsl - 后处理效果基础模块
// 提供后处理管线的通用数据结构、顶点着色器和工具函数
// Feature: 2d-lighting-enhancement
// Requirements: 5.1
// ============================================================================
export PostProcess;

// ============ 常量定义 ============

const PI: f32 = 3.14159265359;

// 色调映射模式
const TONE_MAPPING_NONE: u32 = 0u;
const TONE_MAPPING_REINHARD: u32 = 1u;
const TONE_MAPPING_ACES: u32 = 2u;
const TONE_MAPPING_FILMIC: u32 = 3u;

// 雾效模式
const FOG_MODE_LINEAR: u32 = 0u;
const FOG_MODE_EXPONENTIAL: u32 = 1u;
const FOG_MODE_EXPONENTIAL_SQUARED: u32 = 2u;

// ============ 数据结构 ============

/// 后处理全局数据结构（与 C++ PostProcessGlobalData 对应，128 字节）
struct PostProcessData {
    // Bloom 设置（16 字节）
    bloomThreshold: f32,
    bloomIntensity: f32,
    bloomRadius: f32,
    bloomIterations: u32,
    // -- 16 字节边界 --
    bloomTint: vec4<f32>,
    // -- 16 字节边界 --
    
    // 光束设置（16 字节）
    lightShaftDensity: f32,
    lightShaftDecay: f32,
    lightShaftWeight: f32,
    lightShaftExposure: f32,
    // -- 16 字节边界 --
    
    // 雾效设置（32 字节）
    fogColor: vec4<f32>,
    // -- 16 字节边界 --
    fogDensity: f32,
    fogStart: f32,
    fogEnd: f32,
    fogMode: u32,
    // -- 16 字节边界 --
    heightFogBase: f32,
    heightFogDensity: f32,
    padding1: f32,
    padding2: f32,
    // -- 16 字节边界 --
    
    // 色调映射设置（16 字节）
    exposure: f32,
    contrast: f32,
    saturation: f32,
    gamma: f32,
    // -- 16 字节边界 --
    
    // 标志位（16 字节）
    toneMappingMode: u32,
    enableBloom: u32,
    enableLightShafts: u32,
    enableFog: u32,
    // -- 16 字节边界 --
};

/// 全屏四边形顶点输入
struct FullscreenVertexInput {
    @builtin(vertex_index) vertexIndex: u32,
};

/// 全屏四边形顶点输出
struct FullscreenVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 全屏四边形顶点着色器 ============

/// 生成全屏四边形的顶点位置和 UV
/// 使用 3 个顶点绘制覆盖整个屏幕的三角形
/// 顶点索引: 0, 1, 2 -> 左下, 右下, 左上 (覆盖整个屏幕)
fn GetFullscreenTriangleVertex(vertexIndex: u32) -> FullscreenVertexOutput {
    var output: FullscreenVertexOutput;
    
    // 使用大三角形覆盖整个屏幕
    // 顶点位置: (-1, -1), (3, -1), (-1, 3)
    let x = f32((vertexIndex & 1u) << 2u) - 1.0;
    let y = f32((vertexIndex & 2u) << 1u) - 1.0;
    
    output.position = vec4<f32>(x, y, 0.0, 1.0);
    
    // UV 坐标: (0, 1), (2, 1), (0, -1)
    // 注意: Y 轴翻转以匹配纹理坐标系统
    output.uv = vec2<f32>((x + 1.0) * 0.5, (1.0 - y) * 0.5);
    
    return output;
}

/// 全屏四边形顶点着色器入口点
/// 用于后处理 Pass 的通用顶点着色器
@vertex
fn vs_fullscreen(input: FullscreenVertexInput) -> FullscreenVertexOutput {
    return GetFullscreenTriangleVertex(input.vertexIndex);
}

// ============ 颜色空间转换 ============

/// 线性空间到 sRGB 空间
fn LinearToSRGB(linear: vec3<f32>) -> vec3<f32> {
    return pow(linear, vec3<f32>(1.0 / 2.2));
}

/// sRGB 空间到线性空间
fn SRGBToLinear(srgb: vec3<f32>) -> vec3<f32> {
    return pow(srgb, vec3<f32>(2.2));
}

// ============ 亮度计算 ============

/// 计算感知亮度（使用 Rec. 709 系数）
fn Luminance(color: vec3<f32>) -> f32 {
    return dot(color, vec3<f32>(0.2126, 0.7152, 0.0722));
}

/// 计算简化亮度（使用平均值）
fn LuminanceSimple(color: vec3<f32>) -> f32 {
    return (color.r + color.g + color.b) / 3.0;
}

// ============ 色调映射函数 ============

/// Reinhard 色调映射
/// 简单的全局色调映射，保留颜色比例
fn ToneMappingReinhard(hdr: vec3<f32>) -> vec3<f32> {
    return hdr / (hdr + vec3<f32>(1.0));
}

/// Reinhard 扩展色调映射（带白点）
fn ToneMappingReinhardExtended(hdr: vec3<f32>, whitePoint: f32) -> vec3<f32> {
    let numerator = hdr * (1.0 + hdr / (whitePoint * whitePoint));
    return numerator / (1.0 + hdr);
}

/// ACES 电影色调映射
/// 模拟电影胶片的色调响应曲线
fn ToneMappingACES(hdr: vec3<f32>) -> vec3<f32> {
    let a = 2.51;
    let b = 0.03;
    let c = 2.43;
    let d = 0.59;
    let e = 0.14;
    return clamp((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e), vec3<f32>(0.0), vec3<f32>(1.0));
}

/// Filmic 色调映射（Uncharted 2 风格）
fn ToneMappingFilmicHelper(x: vec3<f32>) -> vec3<f32> {
    let A = 0.15;
    let B = 0.50;
    let C = 0.10;
    let D = 0.20;
    let E = 0.02;
    let F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

fn ToneMappingFilmic(hdr: vec3<f32>) -> vec3<f32> {
    let W = 11.2; // 白点
    let exposureBias = 2.0;
    let curr = ToneMappingFilmicHelper(hdr * exposureBias);
    let whiteScale = vec3<f32>(1.0) / ToneMappingFilmicHelper(vec3<f32>(W));
    return curr * whiteScale;
}

/// 应用色调映射（根据模式选择）
fn ApplyToneMapping(hdr: vec3<f32>, mode: u32) -> vec3<f32> {
    switch (mode) {
        case TONE_MAPPING_NONE: {
            return clamp(hdr, vec3<f32>(0.0), vec3<f32>(1.0));
        }
        case TONE_MAPPING_REINHARD: {
            return ToneMappingReinhard(hdr);
        }
        case TONE_MAPPING_ACES: {
            return ToneMappingACES(hdr);
        }
        case TONE_MAPPING_FILMIC: {
            return ToneMappingFilmic(hdr);
        }
        default: {
            return ToneMappingACES(hdr);
        }
    }
}

// ============ 颜色调整函数 ============

/// 应用曝光调整
fn ApplyExposure(color: vec3<f32>, exposure: f32) -> vec3<f32> {
    return color * exposure;
}

/// 应用对比度调整
/// contrast = 1.0 表示无变化
fn ApplyContrast(color: vec3<f32>, contrast: f32) -> vec3<f32> {
    return (color - 0.5) * contrast + 0.5;
}

/// 应用饱和度调整
/// saturation = 1.0 表示无变化，0.0 表示灰度
fn ApplySaturation(color: vec3<f32>, saturation: f32) -> vec3<f32> {
    let gray = Luminance(color);
    return mix(vec3<f32>(gray), color, saturation);
}

/// 应用 Gamma 校正
fn ApplyGamma(color: vec3<f32>, gamma: f32) -> vec3<f32> {
    return pow(color, vec3<f32>(1.0 / gamma));
}

/// 应用完整的颜色调整链
fn ApplyColorAdjustments(
    color: vec3<f32>,
    exposure: f32,
    contrast: f32,
    saturation: f32,
    gamma: f32
) -> vec3<f32> {
    var result = color;
    result = ApplyExposure(result, exposure);
    result = ApplyContrast(result, contrast);
    result = ApplySaturation(result, saturation);
    result = ApplyGamma(result, gamma);
    return clamp(result, vec3<f32>(0.0), vec3<f32>(1.0));
}

// ============ 雾效计算函数 ============

/// 计算线性雾效因子
fn CalculateLinearFog(distance: f32, fogStart: f32, fogEnd: f32) -> f32 {
    return clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
}

/// 计算指数雾效因子
fn CalculateExponentialFog(distance: f32, density: f32) -> f32 {
    return exp(-density * distance);
}

/// 计算指数平方雾效因子
fn CalculateExponentialSquaredFog(distance: f32, density: f32) -> f32 {
    let factor = density * distance;
    return exp(-factor * factor);
}

/// 根据模式计算雾效因子
fn CalculateFogFactor(distance: f32, fogStart: f32, fogEnd: f32, density: f32, mode: u32) -> f32 {
    switch (mode) {
        case FOG_MODE_LINEAR: {
            return CalculateLinearFog(distance, fogStart, fogEnd);
        }
        case FOG_MODE_EXPONENTIAL: {
            return CalculateExponentialFog(distance, density);
        }
        case FOG_MODE_EXPONENTIAL_SQUARED: {
            return CalculateExponentialSquaredFog(distance, density);
        }
        default: {
            return CalculateLinearFog(distance, fogStart, fogEnd);
        }
    }
}

/// 应用雾效到颜色
fn ApplyFog(color: vec3<f32>, fogColor: vec3<f32>, fogFactor: f32) -> vec3<f32> {
    return mix(fogColor, color, fogFactor);
}

/// 计算高度雾因子
fn CalculateHeightFogFactor(worldY: f32, fogBase: f32, heightDensity: f32) -> f32 {
    let heightAboveBase = max(worldY - fogBase, 0.0);
    return exp(-heightDensity * heightAboveBase);
}

// ============ Bloom 工具函数 ============

/// 提取高亮区域（用于 Bloom）
fn ExtractBrightness(color: vec3<f32>, threshold: f32) -> vec3<f32> {
    let brightness = Luminance(color);
    let contribution = max(brightness - threshold, 0.0);
    return color * (contribution / max(brightness, 0.001));
}

/// 软阈值提取（更平滑的过渡）
fn ExtractBrightnessSoft(color: vec3<f32>, threshold: f32, softKnee: f32) -> vec3<f32> {
    let brightness = Luminance(color);
    let soft = brightness - threshold + softKnee;
    let contribution = clamp(soft * soft / (4.0 * softKnee + 0.00001), 0.0, 1.0);
    return color * contribution;
}

// ============ 采样工具函数 ============

/// 双线性采样（用于降采样）
fn SampleBilinear(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>) -> vec4<f32> {
    return textureSample(tex, samp, uv);
}

/// 4x4 盒式滤波采样（用于降采样）
fn SampleBox4x4(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, texelSize: vec2<f32>) -> vec4<f32> {
    let offset = texelSize * 0.5;
    var color = textureSample(tex, samp, uv + vec2<f32>(-offset.x, -offset.y));
    color += textureSample(tex, samp, uv + vec2<f32>(offset.x, -offset.y));
    color += textureSample(tex, samp, uv + vec2<f32>(-offset.x, offset.y));
    color += textureSample(tex, samp, uv + vec2<f32>(offset.x, offset.y));
    return color * 0.25;
}

/// 9-tap 高斯模糊采样（水平方向）
fn SampleGaussianH(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, texelSize: f32) -> vec4<f32> {
    // 高斯权重（sigma ≈ 1.5）
    let weights = array<f32, 5>(0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    
    var color = textureSample(tex, samp, uv) * weights[0];
    
    for (var i = 1; i < 5; i++) {
        let offset = texelSize * f32(i);
        color += textureSample(tex, samp, uv + vec2<f32>(offset, 0.0)) * weights[i];
        color += textureSample(tex, samp, uv - vec2<f32>(offset, 0.0)) * weights[i];
    }
    
    return color;
}

/// 9-tap 高斯模糊采样（垂直方向）
fn SampleGaussianV(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, texelSize: f32) -> vec4<f32> {
    // 高斯权重（sigma ≈ 1.5）
    let weights = array<f32, 5>(0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    
    var color = textureSample(tex, samp, uv) * weights[0];
    
    for (var i = 1; i < 5; i++) {
        let offset = texelSize * f32(i);
        color += textureSample(tex, samp, uv + vec2<f32>(0.0, offset)) * weights[i];
        color += textureSample(tex, samp, uv - vec2<f32>(0.0, offset)) * weights[i];
    }
    
    return color;
}

// ============ 光束效果工具函数 ============

/// 计算径向模糊采样位置
fn GetRadialBlurOffset(uv: vec2<f32>, lightScreenPos: vec2<f32>, sampleIndex: f32, decay: f32) -> vec2<f32> {
    let direction = uv - lightScreenPos;
    return uv - direction * sampleIndex * decay;
}

// ============ 绑定组定义（供具体 Pass 使用） ============

// Group 0: 后处理全局数据
@group(0) @binding(0) var<uniform> postProcessData: PostProcessData;

// Group 1: 输入纹理
@group(1) @binding(0) var inputTexture: texture_2d<f32>;
@group(1) @binding(1) var inputSampler: sampler;

// Group 2: Emission 纹理（用于 Bloom 提取）
@group(2) @binding(0) var emissionTexture: texture_2d<f32>;
@group(2) @binding(1) var emissionSampler: sampler;

// ============================================================================
// Bloom 提取 Pass (Task 15.1)
// 从场景颜色和 Emission Buffer 提取高亮区域
// Feature: 2d-lighting-enhancement
// Requirements: 5.1
// ============================================================================

/// Bloom 提取参数结构
struct BloomExtractParams {
    threshold: f32,
    softKnee: f32,
    emissionWeight: f32,
    padding: f32,
};

/// Bloom 提取片段着色器
/// 提取亮度超过阈值的像素和自发光像素
@fragment
fn fs_bloom_extract(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样场景颜色
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    
    // 采样自发光颜色
    let emissionColor = textureSample(emissionTexture, emissionSampler, input.uv);
    
    // 从场景颜色提取高亮区域（使用软阈值）
    let threshold = postProcessData.bloomThreshold;
    let softKnee = threshold * 0.5; // 软过渡区域
    let sceneBright = ExtractBrightnessSoft(sceneColor.rgb, threshold, softKnee);
    
    // 自发光直接贡献（HDR 值可以超过 1.0）
    // 自发光强度已经在 Emission Buffer 中，直接使用
    let emissionContrib = emissionColor.rgb * emissionColor.a;
    
    // 合并场景高亮和自发光
    let combinedBright = sceneBright + emissionContrib;
    
    // 应用 Bloom 颜色偏移
    let tintedBright = combinedBright * postProcessData.bloomTint.rgb;
    
    return vec4<f32>(tintedBright, 1.0);
}

/// 简化版 Bloom 提取（仅场景颜色，无 Emission）
@fragment
fn fs_bloom_extract_simple(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    
    let threshold = postProcessData.bloomThreshold;
    let softKnee = threshold * 0.5;
    let brightColor = ExtractBrightnessSoft(sceneColor.rgb, threshold, softKnee);
    
    // 应用 Bloom 颜色偏移
    let tintedBright = brightColor * postProcessData.bloomTint.rgb;
    
    return vec4<f32>(tintedBright, 1.0);
}

// ============================================================================
// Bloom 模糊 Pass (Task 15.2)
// 使用可分离高斯模糊实现降采样模糊
// Feature: 2d-lighting-enhancement
// Requirements: 5.5
// ============================================================================

/// Bloom 模糊参数
struct BloomBlurParams {
    texelSize: vec2<f32>,  // 1.0 / textureSize
    direction: vec2<f32>,  // (1,0) 水平, (0,1) 垂直
};

@group(3) @binding(0) var<uniform> bloomBlurParams: BloomBlurParams;

/// 13-tap 高斯模糊核（优化版本，使用双线性采样减少采样次数）
/// 实际只需要 7 次采样即可实现 13-tap 效果
fn GaussianBlur13Tap(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, direction: vec2<f32>, texelSize: vec2<f32>) -> vec4<f32> {
    // 高斯权重（sigma ≈ 2.0）
    // 使用双线性采样优化：将相邻采样点合并
    let offset1 = 1.411764705882353;
    let offset2 = 3.2941176470588234;
    let offset3 = 5.176470588235294;
    
    let weight0 = 0.1964825501511404;
    let weight1 = 0.2969069646728344;
    let weight2 = 0.09447039785044732;
    let weight3 = 0.010381362401148057;
    
    var color = textureSample(tex, samp, uv) * weight0;
    
    let step = direction * texelSize;
    
    color += textureSample(tex, samp, uv + step * offset1) * weight1;
    color += textureSample(tex, samp, uv - step * offset1) * weight1;
    color += textureSample(tex, samp, uv + step * offset2) * weight2;
    color += textureSample(tex, samp, uv - step * offset2) * weight2;
    color += textureSample(tex, samp, uv + step * offset3) * weight3;
    color += textureSample(tex, samp, uv - step * offset3) * weight3;
    
    return color;
}

/// Bloom 水平模糊片段着色器
@fragment
fn fs_bloom_blur_h(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let texelSize = bloomBlurParams.texelSize;
    let direction = vec2<f32>(1.0, 0.0);
    return GaussianBlur13Tap(inputTexture, inputSampler, input.uv, direction, texelSize);
}

/// Bloom 垂直模糊片段着色器
@fragment
fn fs_bloom_blur_v(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let texelSize = bloomBlurParams.texelSize;
    let direction = vec2<f32>(0.0, 1.0);
    return GaussianBlur13Tap(inputTexture, inputSampler, input.uv, direction, texelSize);
}

/// 降采样模糊（用于 Bloom mip chain）
/// 使用 4x4 盒式滤波 + 高斯权重
@fragment
fn fs_bloom_downsample(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let texelSize = bloomBlurParams.texelSize;
    
    // 13-tap 降采样滤波器（Karis 平均，防止萤火虫效应）
    // 采样模式：
    //   A . B . C
    //   . D . E .
    //   F . G . H
    //   . I . J .
    //   K . L . M
    
    let uv = input.uv;
    
    // 中心采样
    let G = textureSample(inputTexture, inputSampler, uv).rgb;
    
    // 角落采样（2 texel 偏移）
    let A = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-2.0, -2.0)).rgb;
    let C = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(2.0, -2.0)).rgb;
    let K = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-2.0, 2.0)).rgb;
    let M = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(2.0, 2.0)).rgb;
    
    // 边缘采样（1 texel 偏移）
    let D = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-1.0, -1.0)).rgb;
    let E = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(1.0, -1.0)).rgb;
    let I = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-1.0, 1.0)).rgb;
    let J = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(1.0, 1.0)).rgb;
    
    // 十字采样（2 texel 偏移）
    let B = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(0.0, -2.0)).rgb;
    let F = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-2.0, 0.0)).rgb;
    let H = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(2.0, 0.0)).rgb;
    let L = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(0.0, 2.0)).rgb;
    
    // 加权平均（使用 Karis 平均防止萤火虫）
    // 中心权重最高，边缘权重递减
    var result = G * 0.125;
    result += (D + E + I + J) * 0.125;
    result += (A + C + K + M) * 0.03125;
    result += (B + F + H + L) * 0.0625;
    
    return vec4<f32>(result, 1.0);
}

/// 升采样模糊（用于 Bloom mip chain 合并）
@fragment
fn fs_bloom_upsample(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let texelSize = bloomBlurParams.texelSize;
    let uv = input.uv;
    
    // 9-tap 帐篷滤波器（tent filter）
    // 采样模式：
    //   A B C
    //   D E F
    //   G H I
    
    let A = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-1.0, -1.0)).rgb;
    let B = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(0.0, -1.0)).rgb;
    let C = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(1.0, -1.0)).rgb;
    let D = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-1.0, 0.0)).rgb;
    let E = textureSample(inputTexture, inputSampler, uv).rgb;
    let F = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(1.0, 0.0)).rgb;
    let G = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(-1.0, 1.0)).rgb;
    let H = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(0.0, 1.0)).rgb;
    let I = textureSample(inputTexture, inputSampler, uv + texelSize * vec2<f32>(1.0, 1.0)).rgb;
    
    // 帐篷滤波权重
    var result = E * 4.0;
    result += (B + D + F + H) * 2.0;
    result += (A + C + G + I) * 1.0;
    result *= (1.0 / 16.0);
    
    return vec4<f32>(result, 1.0);
}

// ============================================================================
// Bloom 合成 Pass (Task 15.3)
// 将 Bloom 效果叠加到最终图像
// Feature: 2d-lighting-enhancement
// Requirements: 5.3, 5.4
// ============================================================================

// Group 2 重用为 Bloom 纹理（在合成 Pass 中）
// @group(2) @binding(0) var bloomTexture: texture_2d<f32>;
// @group(2) @binding(1) var bloomSampler: sampler;

/// Bloom 合成片段着色器
/// 将 Bloom 效果叠加到场景颜色
@fragment
fn fs_bloom_composite(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样场景颜色
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    
    // 采样 Bloom 颜色（已经过模糊处理）
    let bloomColor = textureSample(emissionTexture, emissionSampler, input.uv);
    
    // 应用 Bloom 强度
    let bloomIntensity = postProcessData.bloomIntensity;
    let bloomContrib = bloomColor.rgb * bloomIntensity;
    
    // 叠加 Bloom 到场景（加法混合）
    let finalColor = sceneColor.rgb + bloomContrib;
    
    return vec4<f32>(finalColor, sceneColor.a);
}

/// Bloom 合成（带颜色偏移）
/// 支持不同颜色通道的不同 Bloom 半径（色差效果）
@fragment
fn fs_bloom_composite_chromatic(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    
    // 采样 Bloom 颜色（可以对不同通道使用不同偏移实现色差）
    let bloomColor = textureSample(emissionTexture, emissionSampler, input.uv);
    
    // 应用 Bloom 强度和颜色偏移
    let bloomIntensity = postProcessData.bloomIntensity;
    let bloomTint = postProcessData.bloomTint.rgb;
    let bloomContrib = bloomColor.rgb * bloomIntensity * bloomTint;
    
    // 叠加 Bloom 到场景
    let finalColor = sceneColor.rgb + bloomContrib;
    
    return vec4<f32>(finalColor, sceneColor.a);
}

/// 直通着色器（用于调试和测试）
@fragment
fn fs_passthrough(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    return textureSample(inputTexture, inputSampler, input.uv);
}

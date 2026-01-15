// ============================================================================
// ToneMapping.wgsl - 色调映射和颜色分级着色器模块
// 实现 HDR 到 LDR 转换和颜色调整效果
// Feature: 2d-lighting-enhancement
// Requirements: 10.1, 10.2, 10.3, 10.4
// ============================================================================

// ============ 常量定义 ============

// 色调映射模式
const TONE_MAPPING_NONE: u32 = 0u;
const TONE_MAPPING_REINHARD: u32 = 1u;
const TONE_MAPPING_ACES: u32 = 2u;
const TONE_MAPPING_FILMIC: u32 = 3u;

// LUT 尺寸常量
const LUT_SIZE: f32 = 32.0;

// ============ 数据结构 ============

/// 色调映射参数结构（与 C++ ToneMappingParams 对应，64 字节）
struct ToneMappingParams {
    // 色调映射参数（16 字节）
    exposure: f32,             ///< 曝光调整 [0, ∞)
    contrast: f32,             ///< 对比度调整 [0, ∞)，1.0 = 无变化
    saturation: f32,           ///< 饱和度调整 [0, ∞)，1.0 = 无变化
    gamma: f32,                ///< Gamma 校正值，通常 2.2
    // -- 16 字节边界 --
    
    // 色调映射模式和标志（16 字节）
    toneMappingMode: u32,      ///< 色调映射模式
    enableToneMapping: u32,    ///< 是否启用色调映射
    enableColorGrading: u32,   ///< 是否启用颜色分级
    enableLUT: u32,            ///< 是否启用 LUT
    // -- 16 字节边界 --
    
    // LUT 参数（16 字节）
    lutIntensity: f32,         ///< LUT 强度 [0, 1]
    lutSize: f32,              ///< LUT 尺寸（通常 32）
    whitePoint: f32,           ///< 白点值（用于 Reinhard 扩展）
    padding1: f32,
    // -- 16 字节边界 --
    
    // 额外颜色调整（16 字节）
    colorBalance: vec3<f32>,   ///< RGB 颜色平衡
    padding2: f32,
    // -- 16 字节边界 --
};

/// 全屏四边形顶点输出
struct FullscreenVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组定义 ============

// Group 0: 色调映射参数
@group(0) @binding(0) var<uniform> toneMappingParams: ToneMappingParams;

// Group 1: 输入纹理（HDR 场景颜色）
@group(1) @binding(0) var inputTexture: texture_2d<f32>;
@group(1) @binding(1) var inputSampler: sampler;

// Group 2: LUT 纹理（3D LUT 展开为 2D）
@group(2) @binding(0) var lutTexture: texture_2d<f32>;
@group(2) @binding(1) var lutSampler: sampler;

// ============ 全屏四边形顶点着色器 ============

/// 生成全屏四边形的顶点位置和 UV
@vertex
fn vs_tonemapping(@builtin(vertex_index) vertexIndex: u32) -> FullscreenVertexOutput {
    var output: FullscreenVertexOutput;
    
    // 使用大三角形覆盖整个屏幕
    let x = f32((vertexIndex & 1u) << 2u) - 1.0;
    let y = f32((vertexIndex & 2u) << 1u) - 1.0;
    
    output.position = vec4<f32>(x, y, 0.0, 1.0);
    output.uv = vec2<f32>((x + 1.0) * 0.5, (1.0 - y) * 0.5);
    
    return output;
}

// ============ 亮度计算函数 ============

/// 计算感知亮度（使用 Rec. 709 系数）
fn Luminance(color: vec3<f32>) -> f32 {
    return dot(color, vec3<f32>(0.2126, 0.7152, 0.0722));
}

// ============ 色调映射算法 ============

/// Reinhard 色调映射
/// 简单的全局色调映射，保留颜色比例
/// Requirements: 10.1, 10.2
fn ToneMappingReinhard(hdr: vec3<f32>) -> vec3<f32> {
    return hdr / (hdr + vec3<f32>(1.0));
}

/// Reinhard 扩展色调映射（带白点）
/// 允许指定最大亮度值（白点）
/// Requirements: 10.1, 10.2
fn ToneMappingReinhardExtended(hdr: vec3<f32>, whitePoint: f32) -> vec3<f32> {
    let wp2 = whitePoint * whitePoint;
    let numerator = hdr * (1.0 + hdr / wp2);
    return numerator / (1.0 + hdr);
}

/// Reinhard 亮度色调映射
/// 基于亮度的 Reinhard，更好地保留颜色
/// Requirements: 10.1, 10.2
fn ToneMappingReinhardLuminance(hdr: vec3<f32>) -> vec3<f32> {
    let lum = Luminance(hdr);
    let lumMapped = lum / (1.0 + lum);
    return hdr * (lumMapped / max(lum, 0.0001));
}

/// ACES 电影色调映射
/// 模拟电影胶片的色调响应曲线，广泛用于电影和游戏
/// Requirements: 10.1, 10.2
fn ToneMappingACES(hdr: vec3<f32>) -> vec3<f32> {
    // ACES 拟合曲线参数
    let a = 2.51;
    let b = 0.03;
    let c = 2.43;
    let d = 0.59;
    let e = 0.14;
    return clamp((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e), vec3<f32>(0.0), vec3<f32>(1.0));
}

/// ACES 近似色调映射（Narkowicz 版本）
/// 更快的 ACES 近似，适合实时渲染
/// Requirements: 10.1, 10.2
fn ToneMappingACESApprox(hdr: vec3<f32>) -> vec3<f32> {
    let v = hdr * 0.6;
    let a = 2.51;
    let b = 0.03;
    let c = 2.43;
    let d = 0.59;
    let e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), vec3<f32>(0.0), vec3<f32>(1.0));
}

/// Filmic 色调映射辅助函数（Uncharted 2 风格）
fn FilmicHelper(x: vec3<f32>) -> vec3<f32> {
    let A = 0.15;  // Shoulder Strength
    let B = 0.50;  // Linear Strength
    let C = 0.10;  // Linear Angle
    let D = 0.20;  // Toe Strength
    let E = 0.02;  // Toe Numerator
    let F = 0.30;  // Toe Denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

/// Filmic 色调映射（Uncharted 2 风格）
/// 提供电影般的色调响应，带有明显的肩部和脚趾区域
/// Requirements: 10.1, 10.2
fn ToneMappingFilmic(hdr: vec3<f32>) -> vec3<f32> {
    let W = 11.2;  // 白点
    let exposureBias = 2.0;
    let curr = FilmicHelper(hdr * exposureBias);
    let whiteScale = vec3<f32>(1.0) / FilmicHelper(vec3<f32>(W));
    return curr * whiteScale;
}

/// Filmic 色调映射（Hejl-Dawson 版本）
/// 更简单的 Filmic 近似，包含 Gamma 校正
fn ToneMappingFilmicHejlDawson(hdr: vec3<f32>) -> vec3<f32> {
    let x = max(vec3<f32>(0.0), hdr - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

/// 根据模式应用色调映射
/// Requirements: 10.1, 10.2
fn ApplyToneMapping(hdr: vec3<f32>, mode: u32, whitePoint: f32) -> vec3<f32> {
    switch (mode) {
        case TONE_MAPPING_NONE: {
            // 无色调映射，直接钳制到 [0, 1]
            return clamp(hdr, vec3<f32>(0.0), vec3<f32>(1.0));
        }
        case TONE_MAPPING_REINHARD: {
            if (whitePoint > 1.0) {
                return ToneMappingReinhardExtended(hdr, whitePoint);
            }
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
/// exposure = 1.0 表示无变化
/// Requirements: 10.4
fn ApplyExposure(color: vec3<f32>, exposure: f32) -> vec3<f32> {
    return color * exposure;
}

/// 应用对比度调整
/// contrast = 1.0 表示无变化
/// 使用中灰点 (0.5) 作为对比度中心
/// Requirements: 10.4
fn ApplyContrast(color: vec3<f32>, contrast: f32) -> vec3<f32> {
    // 以 0.5 为中心进行对比度调整
    return (color - 0.5) * contrast + 0.5;
}

/// 应用对比度调整（使用亮度中心）
/// 更准确的对比度调整，保持整体亮度
fn ApplyContrastLuminance(color: vec3<f32>, contrast: f32) -> vec3<f32> {
    let lum = Luminance(color);
    let adjustedLum = (lum - 0.5) * contrast + 0.5;
    let scale = adjustedLum / max(lum, 0.0001);
    return color * scale;
}

/// 应用饱和度调整
/// saturation = 1.0 表示无变化，0.0 表示灰度
/// Requirements: 10.4
fn ApplySaturation(color: vec3<f32>, saturation: f32) -> vec3<f32> {
    let gray = Luminance(color);
    return mix(vec3<f32>(gray), color, saturation);
}

/// 应用 Gamma 校正
/// 将线性空间转换为显示空间
fn ApplyGamma(color: vec3<f32>, gamma: f32) -> vec3<f32> {
    return pow(max(color, vec3<f32>(0.0)), vec3<f32>(1.0 / gamma));
}

/// 应用颜色平衡
/// colorBalance 是 RGB 乘数
fn ApplyColorBalance(color: vec3<f32>, colorBalance: vec3<f32>) -> vec3<f32> {
    return color * colorBalance;
}

/// 应用完整的颜色调整链
/// Requirements: 10.4
fn ApplyColorAdjustments(
    color: vec3<f32>,
    exposure: f32,
    contrast: f32,
    saturation: f32,
    gamma: f32,
    colorBalance: vec3<f32>
) -> vec3<f32> {
    var result = color;
    
    // 1. 曝光调整（在 HDR 空间）
    result = ApplyExposure(result, exposure);
    
    // 2. 对比度调整
    result = ApplyContrast(result, contrast);
    
    // 3. 饱和度调整
    result = ApplySaturation(result, saturation);
    
    // 4. 颜色平衡
    result = ApplyColorBalance(result, colorBalance);
    
    // 5. Gamma 校正
    result = ApplyGamma(result, gamma);
    
    // 6. 最终钳制
    return clamp(result, vec3<f32>(0.0), vec3<f32>(1.0));
}

// ============ LUT 颜色分级函数 ============

/// 从 3D LUT 纹理采样（LUT 展开为 2D 条带）
/// LUT 纹理格式：水平排列的 N 个 NxN 切片
/// Requirements: 10.3
fn SampleLUT(color: vec3<f32>, lutSize: f32) -> vec3<f32> {
    // 钳制输入颜色到 [0, 1]
    let clampedColor = clamp(color, vec3<f32>(0.0), vec3<f32>(1.0));
    
    // 计算 LUT 坐标
    let lutScale = (lutSize - 1.0) / lutSize;
    let lutOffset = 0.5 / lutSize;
    
    // 计算蓝色通道的切片索引
    let blueSlice = clampedColor.b * (lutSize - 1.0);
    let blueSliceLow = floor(blueSlice);
    let blueSliceHigh = min(blueSliceLow + 1.0, lutSize - 1.0);
    let blueFrac = blueSlice - blueSliceLow;
    
    // 计算 UV 坐标（在展开的 2D LUT 中）
    let uvLow = vec2<f32>(
        (blueSliceLow + clampedColor.r * lutScale + lutOffset) / lutSize,
        clampedColor.g * lutScale + lutOffset
    );
    let uvHigh = vec2<f32>(
        (blueSliceHigh + clampedColor.r * lutScale + lutOffset) / lutSize,
        clampedColor.g * lutScale + lutOffset
    );
    
    // 采样两个切片并插值
    let colorLow = textureSample(lutTexture, lutSampler, uvLow).rgb;
    let colorHigh = textureSample(lutTexture, lutSampler, uvHigh).rgb;
    
    return mix(colorLow, colorHigh, blueFrac);
}

/// 应用 LUT 颜色分级
/// intensity = 0.0 返回原始颜色，1.0 返回完全变换后的颜色
/// Requirements: 10.3
fn ApplyLUT(color: vec3<f32>, lutSize: f32, intensity: f32) -> vec3<f32> {
    let lutColor = SampleLUT(color, lutSize);
    return mix(color, lutColor, intensity);
}

// ============ 片段着色器 ============

/// 完整色调映射和颜色分级片段着色器
/// 执行完整的 HDR 到 LDR 转换管线
/// Requirements: 10.1, 10.2, 10.3, 10.4
@fragment
fn fs_tonemapping_full(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样 HDR 场景颜色
    let hdrColor = textureSample(inputTexture, inputSampler, input.uv);
    var color = hdrColor.rgb;
    
    // 1. 应用曝光（在色调映射之前）
    color = ApplyExposure(color, toneMappingParams.exposure);
    
    // 2. 应用色调映射（HDR -> LDR）
    if (toneMappingParams.enableToneMapping != 0u) {
        color = ApplyToneMapping(color, toneMappingParams.toneMappingMode, toneMappingParams.whitePoint);
    }
    
    // 3. 应用颜色调整（在 LDR 空间）
    if (toneMappingParams.enableColorGrading != 0u) {
        color = ApplyContrast(color, toneMappingParams.contrast);
        color = ApplySaturation(color, toneMappingParams.saturation);
        color = ApplyColorBalance(color, toneMappingParams.colorBalance);
    }
    
    // 4. 应用 LUT 颜色分级
    if (toneMappingParams.enableLUT != 0u) {
        color = ApplyLUT(color, toneMappingParams.lutSize, toneMappingParams.lutIntensity);
    }
    
    // 5. 应用 Gamma 校正
    color = ApplyGamma(color, toneMappingParams.gamma);
    
    // 6. 最终钳制
    color = clamp(color, vec3<f32>(0.0), vec3<f32>(1.0));
    
    return vec4<f32>(color, hdrColor.a);
}

/// 仅色调映射片段着色器（无颜色分级）
/// Requirements: 10.1, 10.2
@fragment
fn fs_tonemapping_only(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样 HDR 场景颜色
    let hdrColor = textureSample(inputTexture, inputSampler, input.uv);
    var color = hdrColor.rgb;
    
    // 应用曝光
    color = ApplyExposure(color, toneMappingParams.exposure);
    
    // 应用色调映射
    color = ApplyToneMapping(color, toneMappingParams.toneMappingMode, toneMappingParams.whitePoint);
    
    // 应用 Gamma 校正
    color = ApplyGamma(color, toneMappingParams.gamma);
    
    return vec4<f32>(color, hdrColor.a);
}

/// 仅颜色调整片段着色器（曝光/对比度/饱和度）
/// Requirements: 10.4
@fragment
fn fs_color_adjustments(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样场景颜色
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    var color = sceneColor.rgb;
    
    // 应用颜色调整
    color = ApplyColorAdjustments(
        color,
        toneMappingParams.exposure,
        toneMappingParams.contrast,
        toneMappingParams.saturation,
        toneMappingParams.gamma,
        toneMappingParams.colorBalance
    );
    
    return vec4<f32>(color, sceneColor.a);
}

/// 仅 LUT 颜色分级片段着色器
/// Requirements: 10.3
@fragment
fn fs_lut_only(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 采样场景颜色
    let sceneColor = textureSample(inputTexture, inputSampler, input.uv);
    var color = sceneColor.rgb;
    
    // 应用 LUT
    color = ApplyLUT(color, toneMappingParams.lutSize, toneMappingParams.lutIntensity);
    
    return vec4<f32>(color, sceneColor.a);
}

// ============ 调试着色器 ============

/// 调试着色器：可视化色调映射曲线
/// 显示不同色调映射算法的响应曲线
@fragment
fn fs_tonemapping_debug(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let uv = input.uv;
    
    // X 轴表示输入 HDR 值 [0, 4]
    let hdrValue = uv.x * 4.0;
    let hdrColor = vec3<f32>(hdrValue);
    
    // 计算各种色调映射结果
    let reinhardResult = ToneMappingReinhard(hdrColor).r;
    let acesResult = ToneMappingACES(hdrColor).r;
    let filmicResult = ToneMappingFilmic(hdrColor).r;
    
    // Y 轴表示输出 LDR 值 [0, 1]
    let y = 1.0 - uv.y;
    
    // 绘制曲线（使用不同颜色）
    let lineWidth = 0.01;
    
    var color = vec3<f32>(0.1); // 背景色
    
    // Reinhard 曲线（红色）
    if (abs(y - reinhardResult) < lineWidth) {
        color = vec3<f32>(1.0, 0.2, 0.2);
    }
    
    // ACES 曲线（绿色）
    if (abs(y - acesResult) < lineWidth) {
        color = vec3<f32>(0.2, 1.0, 0.2);
    }
    
    // Filmic 曲线（蓝色）
    if (abs(y - filmicResult) < lineWidth) {
        color = vec3<f32>(0.2, 0.2, 1.0);
    }
    
    // 绘制网格线
    if (fract(uv.x * 4.0) < 0.02 || fract(uv.y * 4.0) < 0.02) {
        color = vec3<f32>(0.3);
    }
    
    return vec4<f32>(color, 1.0);
}

/// 调试着色器：显示曝光/对比度/饱和度效果
@fragment
fn fs_color_debug(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let uv = input.uv;
    
    // 创建测试图案
    let testColor = vec3<f32>(
        uv.x,
        uv.y,
        0.5
    );
    
    // 应用当前颜色调整
    let adjustedColor = ApplyColorAdjustments(
        testColor,
        toneMappingParams.exposure,
        toneMappingParams.contrast,
        toneMappingParams.saturation,
        toneMappingParams.gamma,
        toneMappingParams.colorBalance
    );
    
    return vec4<f32>(adjustedColor, 1.0);
}

/// 直通着色器（用于测试）
@fragment
fn fs_passthrough(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    return textureSample(inputTexture, inputSampler, input.uv);
}

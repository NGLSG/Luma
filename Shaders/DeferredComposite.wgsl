// ============================================================================
// DeferredComposite.wgsl - 延迟/前向混合合成着色器
// 将延迟渲染结果与前向渲染的透明物体合成
// Feature: 2d-lighting-enhancement
// Requirements: 8.4
// ============================================================================
import Std;

// ============ 合成参数 ============

struct CompositeParams {
    enableDeferred: u32,
    enableForward: u32,
    blendMode: u32,        // 0=Alpha, 1=Additive, 2=Multiply
    debugMode: u32,
    deferredWeight: f32,
    forwardWeight: f32,
    padding1: f32,
    padding2: f32,
};

// ============ 全屏四边形顶点输出 ============

struct CompositeVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组 ============

// Group 5: 合成参数
@group(5) @binding(0) var<uniform> compositeParams: CompositeParams;

// Group 6: 输入纹理
@group(6) @binding(0) var deferredTexture: texture_2d<f32>;
@group(6) @binding(1) var forwardTexture: texture_2d<f32>;
@group(6) @binding(2) var depthTexture: texture_2d<f32>;
@group(6) @binding(3) var compositeSampler: sampler;

// ============ 全屏四边形顶点着色器 ============

@vertex
fn vs_composite(@builtin(vertex_index) vertexIndex: u32) -> CompositeVertexOutput {
    var positions = array<vec2<f32>, 4>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>(-1.0,  1.0),
        vec2<f32>( 1.0,  1.0)
    );
    
    var uvs = array<vec2<f32>, 4>(
        vec2<f32>(0.0, 1.0),
        vec2<f32>(1.0, 1.0),
        vec2<f32>(0.0, 0.0),
        vec2<f32>(1.0, 0.0)
    );
    
    var indices = array<u32, 6>(0u, 1u, 2u, 2u, 1u, 3u);
    let idx = indices[vertexIndex];
    
    var output: CompositeVertexOutput;
    output.position = vec4<f32>(positions[idx], 0.0, 1.0);
    output.uv = uvs[idx];
    return output;
}

// ============ Alpha 混合 ============

fn AlphaBlend(background: vec4<f32>, foreground: vec4<f32>) -> vec4<f32> {
    // 标准 Alpha 混合: result = foreground * alpha + background * (1 - alpha)
    let alpha = foreground.a;
    let rgb = foreground.rgb * alpha + background.rgb * (1.0 - alpha);
    let a = alpha + background.a * (1.0 - alpha);
    return vec4<f32>(rgb, a);
}

// ============ 加法混合 ============

fn AdditiveBlend(background: vec4<f32>, foreground: vec4<f32>) -> vec4<f32> {
    // 加法混合: result = background + foreground * alpha
    let rgb = background.rgb + foreground.rgb * foreground.a;
    let a = max(background.a, foreground.a);
    return vec4<f32>(clamp(rgb, vec3<f32>(0.0), vec3<f32>(1.0)), a);
}

// ============ 乘法混合 ============

fn MultiplyBlend(background: vec4<f32>, foreground: vec4<f32>) -> vec4<f32> {
    // 乘法混合: result = background * foreground
    let rgb = background.rgb * foreground.rgb;
    let a = background.a * foreground.a;
    return vec4<f32>(rgb, a);
}

// ============ 合成片段着色器 ============

@fragment
fn fs_composite(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    // 采样延迟渲染结果
    var deferredColor = vec4<f32>(0.0);
    if (compositeParams.enableDeferred != 0u) {
        deferredColor = textureSample(deferredTexture, compositeSampler, input.uv);
        deferredColor = deferredColor * compositeParams.deferredWeight;
    }
    
    // 采样前向渲染结果（透明物体）
    var forwardColor = vec4<f32>(0.0);
    if (compositeParams.enableForward != 0u) {
        forwardColor = textureSample(forwardTexture, compositeSampler, input.uv);
        forwardColor = forwardColor * compositeParams.forwardWeight;
    }
    
    // 根据混合模式合成
    var finalColor: vec4<f32>;
    switch (compositeParams.blendMode) {
        case 0u: {
            // Alpha 混合：前向渲染结果叠加在延迟渲染结果上
            finalColor = AlphaBlend(deferredColor, forwardColor);
        }
        case 1u: {
            // 加法混合
            finalColor = AdditiveBlend(deferredColor, forwardColor);
        }
        case 2u: {
            // 乘法混合
            finalColor = MultiplyBlend(deferredColor, forwardColor);
        }
        default: {
            finalColor = AlphaBlend(deferredColor, forwardColor);
        }
    }
    
    return finalColor;
}

// ============ 仅延迟渲染输出 ============

@fragment
fn fs_deferred_only(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    return textureSample(deferredTexture, compositeSampler, input.uv);
}

// ============ 仅前向渲染输出 ============

@fragment
fn fs_forward_only(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    return textureSample(forwardTexture, compositeSampler, input.uv);
}

// ============ 调试：显示深度 ============

@fragment
fn fs_debug_depth(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    let depth = textureSample(depthTexture, compositeSampler, input.uv).r;
    return vec4<f32>(depth, depth, depth, 1.0);
}

// ============ 调试：显示混合权重 ============

@fragment
fn fs_debug_blend(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    let deferredColor = textureSample(deferredTexture, compositeSampler, input.uv);
    let forwardColor = textureSample(forwardTexture, compositeSampler, input.uv);
    
    // 红色通道显示延迟渲染贡献，绿色通道显示前向渲染贡献
    let deferredContribution = (deferredColor.r + deferredColor.g + deferredColor.b) / 3.0;
    let forwardContribution = forwardColor.a;
    
    return vec4<f32>(deferredContribution, forwardContribution, 0.0, 1.0);
}

// ============ 基于深度的合成 ============

@fragment
fn fs_depth_composite(input: CompositeVertexOutput) -> @location(0) vec4<f32> {
    let deferredColor = textureSample(deferredTexture, compositeSampler, input.uv);
    let forwardColor = textureSample(forwardTexture, compositeSampler, input.uv);
    let depth = textureSample(depthTexture, compositeSampler, input.uv).r;
    
    // 如果前向渲染有内容且深度更近，使用前向渲染结果
    // 否则使用延迟渲染结果
    if (forwardColor.a > 0.001) {
        // 透明物体始终在延迟渲染结果之上
        return AlphaBlend(deferredColor, forwardColor);
    }
    
    return deferredColor;
}

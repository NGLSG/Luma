// ============================================================================
// Shadow.wgsl - 2D 阴影渲染着色器
// 提供阴影体生成和软阴影模糊功能
// Feature: 2d-lighting-system
// Requirements: 5.4
// ============================================================================
import Shadow;

// ============ 阴影体顶点着色器 ============

/// 阴影体顶点着色器
/// 将阴影投射器的边缘向远离光源的方向延伸
@vertex
fn shadowVolumeVertex(input: ShadowVertexInput, @builtin(vertex_index) vertexIndex: u32) -> ShadowVertexOutput {
    return TransformShadowVolumeVertex(input, vertexIndex);
}

// ============ 阴影体片段着色器 ============

/// 阴影体片段着色器
/// 输出阴影遮挡值
@fragment
fn shadowVolumeFragment(input: ShadowVertexOutput) -> @location(0) f32 {
    // 输出阴影不透明度
    return input.opacity;
}

// ============ 阴影贴图采样片段着色器 ============

/// 阴影贴图采样片段着色器
/// 用于在最终渲染时采样阴影贴图
@fragment
fn shadowSampleFragment(input: ShadowSampleInput) -> @location(0) vec4<f32> {
    var shadow = SampleShadowMap(input.uv);
    
    // 输出阴影值（0 = 无阴影，1 = 完全阴影）
    return vec4<f32>(shadow, shadow, shadow, 1.0);
}

// ============================================================================
// Particle.wgsl - 高性能粒子渲染着色器
// 支持实例化渲染、纹理图集动画、Billboard效果
// ============================================================================
import Std;
import Particle;

// ============ 顶点着色器 ============

@vertex
fn vs_main(
    input: VertexInput,
    @builtin(instance_index) instanceIndex: u32
) -> ParticleVertexOutput {
    return TransformParticleVertex(input, instanceIndex);
}

// ============ 片段着色器 ============

@fragment
fn fs_main(input: ParticleVertexOutput) -> @location(0) vec4<f32> {
    var finalColor = ProcessParticleFragment(input.uv, input.color);
    
    // 丢弃完全透明的像素
    if (finalColor.a < 0.001) {
        discard;
    }
    
    return finalColor;
}

// ============ 加法混合片段着色器变体 ============

@fragment
fn fs_additive(input: ParticleVertexOutput) -> @location(0) vec4<f32> {
    return ProcessParticleFragmentAdditive(input.uv, input.color);
}

// ============ 软粒子片段着色器变体 ============

@fragment
fn fs_soft(input: ParticleVertexOutput) -> @location(0) vec4<f32> {
    var finalColor = ProcessParticleFragmentSoft(input.uv, input.color);
    
    if (finalColor.a < 0.001) {
        discard;
    }
    
    return finalColor;
}

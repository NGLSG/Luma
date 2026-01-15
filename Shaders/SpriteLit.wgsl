// ============================================================================
// SpriteLit.wgsl - 带光照的精灵渲染着色器
// 支持多光源、光照层和自发光
// Feature: 2d-lighting-system
// Feature: 2d-lighting-enhancement (Emission support)
// Requirements: 4.2, 4.4, 4.5, 8.2, 8.3
// ============================================================================
import Std;
import Lighting;
import SpriteLit;

// 注意：EmissionGlobalData 和 emissionGlobal 绑定已在 SpriteLitCore.wgsl 中定义

// ============ 顶点着色器 ============

@vertex
fn vs_main(
    input: VertexInput,
    @builtin(instance_index) instanceIndex: u32
) -> SpriteLitVertexOutput {
    return TransformSpriteLitVertex(input, instanceIndex);
}

// ============ 片段着色器 ============
// Feature: 2d-lighting-enhancement
// Requirements: 4.2, 4.4, 4.5

@fragment
fn fs_main(input: SpriteLitVertexOutput) -> @location(0) vec4<f32> {
    // 采样主纹理
    var texColor = textureSample(mainTexture, mainSampler, input.uv);
    
    // 应用实例颜色
    var baseColor = texColor * input.color;
    
    // 计算光照
    var totalLight = CalculateTotalLightingWithShadow(input.worldPos, input.lightLayer);
    
    // 应用光照到基础颜色
    var litColor = baseColor.rgb * totalLight;
    
    // 计算自发光贡献（独立于场景光照）
    // Requirement 4.4: 自发光独立于场景光照
    // Requirement 4.5: 支持 HDR 自发光值（>1.0）
    var emissionContribution = vec3<f32>(0.0);
    if (emissionGlobal.emissionEnabled != 0u) {
        // 使用顶点传递的自发光颜色和强度
        // 自发光 = 自发光颜色 × 自发光强度 × 全局自发光缩放
        let emissionColor = input.emissionColor.rgb;
        let emissionIntensity = input.emissionIntensity * emissionGlobal.emissionScale;
        emissionContribution = emissionColor * emissionIntensity;
    }
    
    // Requirement 4.2: 自发光颜色添加到最终输出
    // 自发光是加法混合，不受光照影响
    var finalColor = vec4<f32>(litColor + emissionContribution, baseColor.a);
    
    // 丢弃完全透明的像素
    if (finalColor.a < 0.001) {
        discard;
    }
    
    return finalColor;
}

// ============ 带自发光贴图的片段着色器 ============
// 当精灵有自发光贴图时使用此变体
// Feature: 2d-lighting-enhancement
// Requirements: 4.1, 4.2, 4.3, 4.4, 4.5

// 注意：自发光贴图绑定在 Group 4
// 如果需要使用自发光贴图，需要在 SpriteLitCore.wgsl 中启用相应的绑定

// ============ 仅光照调试片段着色器 ============

@fragment
fn fs_debug_lighting(input: SpriteLitVertexOutput) -> @location(0) vec4<f32> {
    // 仅显示光照贡献，不显示纹理
    var totalLight = CalculateTotalLightingWithShadow(input.worldPos, input.lightLayer);
    return vec4<f32>(totalLight, 1.0);
}

// ============ 仅自发光调试片段着色器 ============
// Feature: 2d-lighting-enhancement

@fragment
fn fs_debug_emission(input: SpriteLitVertexOutput) -> @location(0) vec4<f32> {
    // 仅显示自发光贡献
    var emissionContribution = vec3<f32>(0.0);
    if (emissionGlobal.emissionEnabled != 0u) {
        let emissionColor = input.emissionColor.rgb;
        let emissionIntensity = input.emissionIntensity * emissionGlobal.emissionScale;
        emissionContribution = emissionColor * emissionIntensity;
    }
    return vec4<f32>(emissionContribution, 1.0);
}

// ============ 无光照片段着色器（回退） ============

@fragment
fn fs_unlit(input: SpriteLitVertexOutput) -> @location(0) vec4<f32> {
    var texColor = textureSample(mainTexture, mainSampler, input.uv);
    var finalColor = texColor * input.color;
    
    if (finalColor.a < 0.001) {
        discard;
    }
    
    return finalColor;
}

// ============================================================================
// DeferredLighting.wgsl - 延迟光照计算着色器
// 从 G-Buffer 读取几何信息，计算所有光源的光照贡献
// Feature: 2d-lighting-enhancement
// Requirements: 8.1, 8.3
// ============================================================================
import Std;
import Lighting;

// ============ 延迟光照参数 ============

struct DeferredLightingParams {
    lightCount: u32,
    maxLightsPerPixel: u32,
    enableShadows: u32,
    debugMode: u32,
    ambientIntensity: f32,
    padding1: f32,
    padding2: f32,
    padding3: f32,
};

// ============ G-Buffer 全局数据 ============

struct GBufferGlobalData {
    bufferWidth: u32,
    bufferHeight: u32,
    renderMode: u32,
    enableDeferred: u32,
    nearPlane: f32,
    farPlane: f32,
    padding1: f32,
    padding2: f32,
};

// ============ 全屏四边形顶点输出 ============

struct FullscreenVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组 ============

// Group 5: 延迟光照数据
@group(5) @binding(0) var<uniform> deferredParams: DeferredLightingParams;
@group(5) @binding(1) var<uniform> gBufferGlobal: GBufferGlobalData;

// Group 6: G-Buffer 纹理
@group(6) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(6) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(6) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(6) @binding(3) var gBufferMaterial: texture_2d<f32>;
@group(6) @binding(4) var gBufferSampler: sampler;

// Group 1: 光照数据（复用现有绑定）
@group(1) @binding(0) var<uniform> lightingData: LightingData;
@group(1) @binding(1) var<storage, read> lights: array<Light>;

// ============ 全屏四边形顶点着色器 ============

@vertex
fn vs_fullscreen(@builtin(vertex_index) vertexIndex: u32) -> FullscreenVertexOutput {
    // 生成全屏四边形的顶点
    // 顶点顺序: 0=左下, 1=右下, 2=左上, 3=右上
    // 三角形: (0,1,2), (2,1,3)
    var positions = array<vec2<f32>, 4>(
        vec2<f32>(-1.0, -1.0),  // 左下
        vec2<f32>( 1.0, -1.0),  // 右下
        vec2<f32>(-1.0,  1.0),  // 左上
        vec2<f32>( 1.0,  1.0)   // 右上
    );
    
    var uvs = array<vec2<f32>, 4>(
        vec2<f32>(0.0, 1.0),  // 左下
        vec2<f32>(1.0, 1.0),  // 右下
        vec2<f32>(0.0, 0.0),  // 左上
        vec2<f32>(1.0, 0.0)   // 右上
    );
    
    // 使用三角形带索引
    var indices = array<u32, 6>(0u, 1u, 2u, 2u, 1u, 3u);
    let idx = indices[vertexIndex];
    
    var output: FullscreenVertexOutput;
    output.position = vec4<f32>(positions[idx], 0.0, 1.0);
    output.uv = uvs[idx];
    return output;
}

// ============ 从 G-Buffer 解码数据 ============

fn DecodeWorldPosition(uv: vec2<f32>) -> vec3<f32> {
    let data = textureSample(gBufferPosition, gBufferSampler, uv);
    return data.xyz;
}

fn DecodeNormal(uv: vec2<f32>) -> vec3<f32> {
    let data = textureSample(gBufferNormal, gBufferSampler, uv);
    return normalize(data.xyz);
}

fn DecodeAlbedo(uv: vec2<f32>) -> vec4<f32> {
    return textureSample(gBufferAlbedo, gBufferSampler, uv);
}

fn DecodeMaterial(uv: vec2<f32>) -> vec4<f32> {
    return textureSample(gBufferMaterial, gBufferSampler, uv);
}

fn DecodeDepth(uv: vec2<f32>) -> f32 {
    let data = textureSample(gBufferPosition, gBufferSampler, uv);
    return data.w;
}

fn DecodeLightLayer(uv: vec2<f32>) -> u32 {
    let material = textureSample(gBufferMaterial, gBufferSampler, uv);
    return u32(material.a * 255.0);
}

// ============ 延迟光照计算 ============

fn CalculateDeferredLighting(worldPos: vec2<f32>, normal: vec3<f32>, lightLayer: u32) -> vec3<f32> {
    // 从环境光开始
    var totalLight = lightingData.ambientColor.rgb * lightingData.ambientIntensity * deferredParams.ambientIntensity;
    
    // 累加所有光源贡献
    let lightCount = min(deferredParams.lightCount, deferredParams.maxLightsPerPixel);
    for (var i = 0u; i < lightCount; i = i + 1u) {
        let light = lights[i];
        
        // 检查光照层
        if ((light.layerMask & lightLayer) == 0u) {
            continue;
        }
        
        // 根据光源类型计算贡献
        var contribution = vec3<f32>(0.0);
        switch (light.lightType) {
            case LIGHT_TYPE_POINT: {
                contribution = CalculatePointLightWithNormal(light, worldPos, normal, lightLayer);
            }
            case LIGHT_TYPE_SPOT: {
                contribution = CalculateSpotLightWithNormal(light, worldPos, normal, lightLayer);
            }
            case LIGHT_TYPE_DIRECTIONAL: {
                contribution = CalculateDirectionalLightWithNormal(light, worldPos, normal, lightLayer);
            }
            default: {}
        }
        
        totalLight = totalLight + contribution;
    }
    
    return totalLight;
}

// ============ 延迟光照片段着色器 ============

@fragment
fn fs_deferred_lighting(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 从 G-Buffer 读取数据
    let worldPos = DecodeWorldPosition(input.uv);
    let normal = DecodeNormal(input.uv);
    let albedo = DecodeAlbedo(input.uv);
    let material = DecodeMaterial(input.uv);
    let lightLayer = DecodeLightLayer(input.uv);
    
    // 如果 alpha 为 0，表示没有几何体，直接返回透明
    if (albedo.a < 0.001) {
        discard;
    }
    
    // 计算光照
    let lighting = CalculateDeferredLighting(worldPos.xy, normal, lightLayer);
    
    // 应用光照到颜色
    var finalColor = albedo.rgb * lighting;
    
    // 添加自发光
    let emissionIntensity = material.b;
    if (emissionIntensity > 0.0) {
        finalColor = finalColor + albedo.rgb * emissionIntensity;
    }
    
    return vec4<f32>(finalColor, albedo.a);
}

// ============ 调试片段着色器 ============

@fragment
fn fs_debug_gbuffer_position(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let worldPos = DecodeWorldPosition(input.uv);
    // 归一化位置用于可视化
    let normalizedPos = (worldPos.xy / 100.0) * 0.5 + 0.5;
    return vec4<f32>(normalizedPos, worldPos.z * 0.01, 1.0);
}

@fragment
fn fs_debug_gbuffer_normal(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let normal = DecodeNormal(input.uv);
    // 将法线从 [-1,1] 映射到 [0,1]
    let normalVis = normal * 0.5 + 0.5;
    return vec4<f32>(normalVis, 1.0);
}

@fragment
fn fs_debug_gbuffer_albedo(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    return DecodeAlbedo(input.uv);
}

@fragment
fn fs_debug_gbuffer_material(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    return DecodeMaterial(input.uv);
}

@fragment
fn fs_debug_gbuffer_depth(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let depth = DecodeDepth(input.uv);
    return vec4<f32>(depth, depth, depth, 1.0);
}

@fragment
fn fs_debug_gbuffer_lightlayer(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let lightLayer = DecodeLightLayer(input.uv);
    // 将光照层编码为颜色
    let r = f32((lightLayer >> 0u) & 0xFFu) / 255.0;
    let g = f32((lightLayer >> 8u) & 0xFFu) / 255.0;
    let b = f32((lightLayer >> 16u) & 0xFFu) / 255.0;
    return vec4<f32>(r, g, b, 1.0);
}

@fragment
fn fs_debug_lighting_only(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    let worldPos = DecodeWorldPosition(input.uv);
    let normal = DecodeNormal(input.uv);
    let lightLayer = DecodeLightLayer(input.uv);
    let albedo = DecodeAlbedo(input.uv);
    
    if (albedo.a < 0.001) {
        discard;
    }
    
    let lighting = CalculateDeferredLighting(worldPos.xy, normal, lightLayer);
    return vec4<f32>(lighting, 1.0);
}

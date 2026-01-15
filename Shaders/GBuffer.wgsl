// ============================================================================
// GBuffer.wgsl - G-Buffer 写入着色器
// 用于延迟渲染的几何 Pass，将几何信息写入 G-Buffer
// Feature: 2d-lighting-enhancement
// Requirements: 8.2
// ============================================================================
import Std;

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

// ============ G-Buffer 输出结构 ============

struct GBufferOutput {
    @location(0) position: vec4<f32>,   // 世界空间位置 (xyz) + 深度 (w)
    @location(1) normal: vec4<f32>,     // 法线 (xyz) + 材质ID (w)
    @location(2) albedo: vec4<f32>,     // 颜色 (rgb) + alpha (a)
    @location(3) material: vec4<f32>,   // 金属度 (r), 粗糙度 (g), 自发光强度 (b), 光照层 (a)
};

// ============ 顶点输出结构（用于 G-Buffer Pass） ============

struct GBufferVertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) worldPos: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>,
    @location(3) normal: vec3<f32>,
    @location(4) lightLayer: u32,
    @location(5) materialId: u32,
    @location(6) emissionIntensity: f32,
};

// ============ 绑定组 ============

// Group 5: G-Buffer 全局数据
@group(5) @binding(0) var<uniform> gBufferGlobal: GBufferGlobalData;

// ============ 顶点着色器 ============

@vertex
fn vs_gbuffer(
    input: VertexInput,
    @builtin(instance_index) instanceIndex: u32
) -> GBufferVertexOutput {
    let instance = instanceDatas[instanceIndex];
    
    // 计算局部坐标（考虑精灵尺寸）
    let localPos = (input.position - vec2<f32>(0.5)) * instance.size;
    
    // 变换到世界空间
    let worldPos = LocalToWorld(localPos, instance);
    
    // 变换到裁剪空间
    let clipPos = TransformVertex(localPos, instance, engineData);
    
    // 计算 UV
    let uv = TransformUV(input.uv, instance.uvRect);
    
    // 默认法线（2D 精灵朝向 Z 轴正方向）
    let normal = vec3<f32>(0.0, 0.0, 1.0);
    
    var output: GBufferVertexOutput;
    output.clipPosition = clipPos;
    output.worldPos = worldPos;
    output.uv = uv;
    output.color = instance.color;
    output.normal = normal;
    output.lightLayer = instance.lightLayer;
    output.materialId = 0u;  // 默认材质
    output.emissionIntensity = 0.0;  // 默认无自发光
    
    return output;
}

// ============ 片段着色器 ============

@fragment
fn fs_gbuffer(input: GBufferVertexOutput) -> GBufferOutput {
    // 采样主纹理
    let texColor = textureSample(mainTexture, mainSampler, input.uv);
    
    // 应用实例颜色
    let baseColor = texColor * input.color;
    
    // 丢弃完全透明的像素
    if (baseColor.a < 0.001) {
        discard;
    }
    
    // 计算深度（使用裁剪空间 Z）
    let depth = input.clipPosition.z;
    
    // 编码光照层到材质 alpha 通道
    let lightLayerEncoded = f32(input.lightLayer) / 255.0;
    
    var output: GBufferOutput;
    
    // 位置缓冲区：世界空间位置 + 深度
    output.position = vec4<f32>(input.worldPos, 0.0, depth);
    
    // 法线缓冲区：法线 + 材质ID
    output.normal = vec4<f32>(input.normal, f32(input.materialId) / 255.0);
    
    // 颜色缓冲区：基础颜色
    output.albedo = baseColor;
    
    // 材质缓冲区：金属度, 粗糙度, 自发光强度, 光照层
    output.material = vec4<f32>(
        0.0,                        // 金属度（2D 默认为 0）
        1.0,                        // 粗糙度（2D 默认为 1）
        input.emissionIntensity,    // 自发光强度
        lightLayerEncoded           // 光照层
    );
    
    return output;
}

// ============ 带法线贴图的片段着色器 ============

// 注意：如果需要法线贴图支持，需要额外的纹理绑定
// @group(5) @binding(1) var normalTexture: texture_2d<f32>;
// @group(5) @binding(2) var normalSampler: sampler;

// @fragment
// fn fs_gbuffer_normal(input: GBufferVertexOutput) -> GBufferOutput {
//     // 采样法线贴图
//     let normalMapValue = textureSample(normalTexture, normalSampler, input.uv).rgb;
//     let normal = UnpackNormal(normalMapValue);
//     
//     // ... 其余代码与 fs_gbuffer 相同
// }

// ============ 调试片段着色器 ============

@fragment
fn fs_gbuffer_debug_position(input: GBufferVertexOutput) -> @location(0) vec4<f32> {
    // 可视化世界空间位置
    let normalizedPos = (input.worldPos / 100.0) * 0.5 + 0.5;
    return vec4<f32>(normalizedPos, 0.0, 1.0);
}

@fragment
fn fs_gbuffer_debug_normal(input: GBufferVertexOutput) -> @location(0) vec4<f32> {
    // 可视化法线
    let normalVis = input.normal * 0.5 + 0.5;
    return vec4<f32>(normalVis, 1.0);
}

@fragment
fn fs_gbuffer_debug_depth(input: GBufferVertexOutput) -> @location(0) vec4<f32> {
    // 可视化深度
    let depth = input.clipPosition.z;
    return vec4<f32>(depth, depth, depth, 1.0);
}

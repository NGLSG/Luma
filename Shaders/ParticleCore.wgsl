// ============================================================================
// ParticleCore.wgsl - 粒子渲染核心模块
// 提供粒子系统所需的数据结构、绑定和工具函数
// ============================================================================
export Particle;
import Std;

// ============ 粒子数据结构 ============

/// 粒子实例数据 (对应 C++ ParticleGPUData)
struct ParticleInstance {
    positionAndRotation: vec4<f32>,  // xyz: 位置, w: 旋转角度
    color: vec4<f32>,                 // RGBA颜色
    sizeAndUV: vec4<f32>,             // xy: 尺寸, zw: UV偏移
    uvScaleAndIndex: vec4<f32>,       // xy: UV缩放, z: 纹理索引, w: 保留
};

/// 粒子系统配置
struct ParticleConfig {
    textureTiles: vec2<f32>,          // xy: 纹理图集分块数
    billboard: f32,                    // 是否启用Billboard
    softParticle: f32,                 // 软粒子开关
    softDistance: f32,                 // 软粒子距离
    padding: vec3<f32>,
};

// ============ 粒子顶点输出 ============

struct ParticleVertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) worldPos: vec3<f32>,
};

// ============ 粒子绑定组 ============

// Group 0: 引擎核心数据（粒子专用）
@group(0) @binding(0) var<uniform> particleEngineData: EngineData;
@group(0) @binding(1) var<storage, read> particles: array<ParticleInstance>;
@group(0) @binding(2) var particleTexture: texture_2d<f32>;
@group(0) @binding(3) var particleSampler: sampler;

// Group 1: 粒子系统配置 (可选)
@group(1) @binding(0) var<uniform> particleConfig: ParticleConfig;

// ============ 粒子工具函数 ============

/// 2D旋转（粒子专用，使用sin/cos参数）
fn ParticleRotate2D(pos: vec2<f32>, sinR: f32, cosR: f32) -> vec2<f32> {
    return vec2<f32>(
        pos.x * cosR - pos.y * sinR,
        pos.x * sinR + pos.y * cosR
    );
}

/// 计算纹理图集UV
fn ComputeAtlasUV(baseUV: vec2<f32>, textureIndex: f32, tilesX: f32, tilesY: f32) -> vec2<f32> {
    let index = u32(textureIndex);
    let tilesXInt = u32(tilesX);
    let tilesYInt = u32(tilesY);
    
    let tileX = index % tilesXInt;
    let tileY = index / tilesXInt;
    
    let tileSize = vec2<f32>(1.0 / tilesX, 1.0 / tilesY);
    let tileOffset = vec2<f32>(f32(tileX), f32(tileY)) * tileSize;
    
    return tileOffset + baseUV * tileSize;
}

// ============ 粒子顶点变换 ============

/// 粒子顶点变换
fn TransformParticleVertex(input: VertexInput, instanceIndex: u32) -> ParticleVertexOutput {
    var output: ParticleVertexOutput;
    
    let particle = particles[instanceIndex];
    
    // 提取粒子属性
    let particlePos = particle.positionAndRotation.xyz;
    let particleRotation = particle.positionAndRotation.w;
    let particleSize = particle.sizeAndUV.xy;
    let particleColor = particle.color;
    let textureIndex = particle.uvScaleAndIndex.z;
    
    // 计算旋转的sin/cos
    let sinR = sin(particleRotation);
    let cosR = cos(particleRotation);
    
    // 应用粒子尺寸和旋转到顶点
    var localPos = input.position * particleSize;
    localPos = ParticleRotate2D(localPos, sinR, cosR);
    
    // 转换到世界空间
    var worldPos = vec3<f32>(localPos + particlePos.xy, particlePos.z);
    
    // 世界空间到摄像机空间
    var cameraSpace = worldPos.xy - particleEngineData.cameraPosition;
    cameraSpace = ParticleRotate2D(cameraSpace, -particleEngineData.cameraSinR, particleEngineData.cameraCosR);
    cameraSpace = cameraSpace * vec2<f32>(particleEngineData.cameraScaleX, particleEngineData.cameraScaleY);
    
    // 摄像机空间到裁剪空间
    let clipPos = cameraSpace / (particleEngineData.viewportSize * 0.5);
    
    output.clipPosition = vec4<f32>(clipPos, worldPos.z, 1.0);
    output.worldPos = worldPos;
    
    // 计算纹理图集UV
    // sizeAndUV.zw 包含UV偏移, uvScaleAndIndex.xy 包含UV缩放
    let uvOffset = particle.sizeAndUV.zw;
    let uvScale = particle.uvScaleAndIndex.xy;
    output.uv = uvOffset + input.uv * uvScale;
    output.color = particleColor;
    
    return output;
}

// ============ 粒子片段处理函数 ============

/// 标准粒子片段处理
fn ProcessParticleFragment(uv: vec2<f32>, color: vec4<f32>) -> vec4<f32> {
    // 采样纹理
    var texColor = textureSample(particleTexture, particleSampler, uv);
    
    // 应用粒子颜色
    var finalColor = texColor * color;
    
    // 预乘Alpha (用于正确的混合)
    finalColor = vec4<f32>(finalColor.rgb * finalColor.a, finalColor.a);
    
    return finalColor;
}

/// 加法混合粒子片段处理
fn ProcessParticleFragmentAdditive(uv: vec2<f32>, color: vec4<f32>) -> vec4<f32> {
    var texColor = textureSample(particleTexture, particleSampler, uv);
    var finalColor = texColor * color;
    
    // 加法混合：输出RGB会被加到背景上
    // Alpha控制添加强度
    finalColor = vec4<f32>(finalColor.rgb * finalColor.a, 0.0);
    
    return finalColor;
}

/// 软粒子片段处理
fn ProcessParticleFragmentSoft(uv: vec2<f32>, color: vec4<f32>) -> vec4<f32> {
    var texColor = textureSample(particleTexture, particleSampler, uv);
    var finalColor = texColor * color;
    
    // 软粒子效果：基于深度差异淡化
    // 完整实现需要深度纹理比较
    // 这里使用简化的边缘淡化
    let edgeFade = smoothstep(0.0, 0.1, min(
        min(uv.x, 1.0 - uv.x),
        min(uv.y, 1.0 - uv.y)
    ) * 2.0);
    
    finalColor.a *= edgeFade;
    finalColor = vec4<f32>(finalColor.rgb * finalColor.a, finalColor.a);
    
    return finalColor;
}

// ============================================================================
// Common.wgsl - 基础工具和结构定义
// 提供引擎通用的数据结构、变换函数和工具函数
// ============================================================================
export Std;
// ============ 引擎数据结构 ============

/// 引擎全局数据 (对应 C++ EngineData 结构体)
struct EngineData {
    cameraPosition: vec2<f32>,
    cameraScaleX: f32,
    cameraScaleY: f32,
    cameraSinR: f32,
    cameraCosR: f32,
    viewportSize: vec2<f32>,
    timeData: vec2<f32>,
    mousePosition: vec2<f32>,
};

struct InstanceData {
    position: vec4<f32>,
    scaleX: f32,
    scaleY: f32,
    sinr: f32,
    cosr: f32,
    color: vec4<f32>,
    uvRect: vec4<f32>,
    size: vec2<f32>,
    padding: vec2<f32>,
};

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
};


// ============ Group 0: 引擎固有绑定 ============

@group(0) @binding(0) var<uniform> engineData: EngineData;
@group(0) @binding(1) var<storage, read> instanceDatas: array<InstanceData>;
@group(0) @binding(2) var mainTexture: texture_2d<f32>;
@group(0) @binding(3) var mainSampler: sampler;

// ============ 变换工具函数 ============

/// 应用2D旋转变换
fn Rotate2D(pos: vec2<f32>, sinR: f32, cosR: f32) -> vec2<f32> {
    return vec2<f32>(
        pos.x * cosR - pos.y * sinR,
        pos.x * sinR + pos.y * cosR
    );
}

/// 将局部坐标变换到世界坐标
fn LocalToWorld(localPos: vec2<f32>, instance: InstanceData) -> vec2<f32> {
    // 缩放
    var scaled = localPos * vec2<f32>(instance.scaleX, instance.scaleY);
    // 旋转
    var rotated = Rotate2D(scaled, instance.sinr, instance.cosr);
    // 平移
    return rotated + instance.position.xy;
}

/// 将世界坐标变换到摄像机空间
fn WorldToCamera(worldPos: vec2<f32>, engine: EngineData) -> vec2<f32> {
    // 平移到摄像机空间
    var cameraSpace = worldPos - engine.cameraPosition;
    // 应用摄像机旋转
    var rotated = Rotate2D(cameraSpace, -engine.cameraSinR, engine.cameraCosR);
    // 应用摄像机缩放
    return rotated * vec2<f32>(engine.cameraScaleX, engine.cameraScaleY);
}

/// 将摄像机空间坐标变换到裁剪空间 (NDC)
fn CameraToClip(cameraPos: vec2<f32>, engine: EngineData) -> vec2<f32> {
    return cameraPos / (engine.viewportSize * 0.5);
}

/// 完整的顶点变换管线: 局部空间 -> 世界空间 -> 摄像机空间 -> 裁剪空间
fn TransformVertex(localPos: vec2<f32>, instance: InstanceData, engine: EngineData) -> vec4<f32> {
    let worldPos = LocalToWorld(localPos, instance);
    let cameraPos = WorldToCamera(worldPos, engine);
    let clipPos = CameraToClip(cameraPos, engine);
    return vec4<f32>(clipPos, instance.position.z, 1.0);
}

// ============ UV 工具函数 ============

/// 应用UV矩形变换
fn TransformUV(uv: vec2<f32>, uvRect: vec4<f32>) -> vec2<f32> {
    return uvRect.xy + uv * uvRect.zw;
}

/// UV翻转
fn FlipUV(uv: vec2<f32>, flipX: bool, flipY: bool) -> vec2<f32> {
    var result = uv;
    if (flipX) { result.x = 1.0 - result.x; }
    if (flipY) { result.y = 1.0 - result.y; }
    return result;
}

// ============ 颜色工具函数 ============

/// 线性空间到sRGB空间
fn LinearToSRGB(linear_: vec3<f32>) -> vec3<f32> {
    return pow(linear_, vec3<f32>(1.0 / 2.2));
}

/// sRGB空间到线性空间
fn SRGBToLinear(srgb: vec3<f32>) -> vec3<f32> {
    return pow(srgb, vec3<f32>(2.2));
}

/// 预乘Alpha混合
fn PremultiplyAlpha(color: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(color.rgb * color.a, color.a);
}

/// 取消预乘Alpha
fn UnpremultiplyAlpha(color: vec4<f32>) -> vec4<f32> {
    if (color.a > 0.0) {
        return vec4<f32>(color.rgb / color.a, color.a);
    }
    return color;
}

// ============ 数学工具函数 ============

/// 平滑步进函数
fn SmoothStep(edge0: f32, edge1: f32, x: f32) -> f32 {
    let t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/// 重映射值从一个范围到另一个范围
fn Remap(value: f32, fromMin: f32, fromMax: f32, toMin: f32, toMax: f32) -> f32 {
    let t = (value - fromMin) / (fromMax - fromMin);
    return mix(toMin, toMax, t);
}

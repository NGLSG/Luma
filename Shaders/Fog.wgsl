// ============================================================================
// Fog.wgsl - 雾效着色器模块
// 实现距离雾效和高度雾效果，支持光源穿透
// Feature: 2d-lighting-enhancement
// Requirements: 11.1, 11.3, 11.4, 11.5
// ============================================================================

// ============ 常量定义 ============

// 雾效模式
const FOG_MODE_LINEAR: u32 = 0u;
const FOG_MODE_EXPONENTIAL: u32 = 1u;
const FOG_MODE_EXPONENTIAL_SQUARED: u32 = 2u;

// 最大光源数量（用于光源穿透计算）
const MAX_FOG_LIGHTS: u32 = 8u;

// ============ 数据结构 ============

/// 雾效参数结构（与 C++ FogParams 对应，64 字节）
struct FogParams {
    // 雾效颜色（16 字节）
    fogColor: vec4<f32>,
    // -- 16 字节边界 --
    
    // 雾效参数（16 字节）
    fogDensity: f32,
    fogStart: f32,
    fogEnd: f32,
    fogMode: u32,
    // -- 16 字节边界 --
    
    // 高度雾参数（16 字节）
    heightFogBase: f32,
    heightFogDensity: f32,
    enableHeightFog: u32,
    enableFog: u32,
    // -- 16 字节边界 --
    
    // 相机参数（16 字节）
    cameraPosition: vec2<f32>,
    cameraZoom: f32,
    padding: f32,
    // -- 16 字节边界 --
};

/// 光源穿透雾效数据结构（用于光源穿透计算）
struct FogLightData {
    position: vec2<f32>,      ///< 光源世界位置
    radius: f32,              ///< 光源影响半径
    intensity: f32,           ///< 光源强度
    color: vec4<f32>,         ///< 光源颜色
    penetrationStrength: f32, ///< 穿透强度 [0, 1]
    falloff: f32,             ///< 衰减系数
    padding1: f32,
    padding2: f32,
};

/// 光源穿透参数
struct FogLightParams {
    lightCount: u32,
    enableLightPenetration: u32,
    maxPenetration: f32,      ///< 最大穿透量 [0, 1]
    padding: f32,
};

/// 全屏四边形顶点输出
struct FullscreenVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// ============ 绑定组定义 ============

// Group 0: 雾效参数
@group(0) @binding(0) var<uniform> fogParams: FogParams;

// Group 1: 输入纹理（场景颜色）
@group(1) @binding(0) var sceneTexture: texture_2d<f32>;
@group(1) @binding(1) var sceneSampler: sampler;

// Group 2: 深度/位置纹理（用于计算距离）
@group(2) @binding(0) var depthTexture: texture_2d<f32>;
@group(2) @binding(1) var depthSampler: sampler;

// Group 3: 光源穿透数据（可选）
@group(3) @binding(0) var<uniform> fogLightParams: FogLightParams;
@group(3) @binding(1) var<storage, read> fogLights: array<FogLightData>;

// ============ 光源穿透计算函数 ============

/// 计算单个光源对雾效的穿透影响
/// 返回穿透因子 [0, 1]，0 = 无穿透，1 = 完全穿透
/// Requirements: 11.4
fn CalculateLightPenetration(
    worldPos: vec2<f32>,
    lightPos: vec2<f32>,
    lightRadius: f32,
    lightIntensity: f32,
    penetrationStrength: f32,
    falloff: f32
) -> f32 {
    // 计算到光源的距离
    let distance = length(worldPos - lightPos);
    
    // 如果在光源影响范围外，无穿透
    if (distance > lightRadius) {
        return 0.0;
    }
    
    // 计算基于距离的衰减
    let normalizedDist = distance / lightRadius;
    let attenuation = pow(1.0 - normalizedDist, falloff);
    
    // 计算穿透量
    let penetration = attenuation * lightIntensity * penetrationStrength;
    
    return clamp(penetration, 0.0, 1.0);
}

/// 计算所有光源对雾效的总穿透影响
/// Requirements: 11.4
fn CalculateTotalLightPenetration(worldPos: vec2<f32>, maxPenetration: f32) -> f32 {
    var totalPenetration = 0.0;
    
    let lightCount = min(fogLightParams.lightCount, MAX_FOG_LIGHTS);
    
    for (var i = 0u; i < lightCount; i++) {
        let light = fogLights[i];
        let penetration = CalculateLightPenetration(
            worldPos,
            light.position,
            light.radius,
            light.intensity,
            light.penetrationStrength,
            light.falloff
        );
        
        // 累加穿透量（使用 max 而不是 add 以避免过度穿透）
        totalPenetration = max(totalPenetration, penetration);
    }
    
    return clamp(totalPenetration, 0.0, maxPenetration);
}

/// 应用光源穿透到雾效因子
/// 光源穿透会减少雾效强度
fn ApplyLightPenetrationToFog(fogFactor: f32, penetration: f32) -> f32 {
    // 穿透增加雾效因子（减少雾效）
    // fogFactor = 1.0 表示无雾，0.0 表示完全雾化
    // penetration = 1.0 表示完全穿透（无雾），0.0 表示无穿透
    return clamp(fogFactor + penetration * (1.0 - fogFactor), 0.0, 1.0);
}

// ============ 全屏四边形顶点着色器 ============

/// 生成全屏四边形的顶点位置和 UV
@vertex
fn vs_fog(@builtin(vertex_index) vertexIndex: u32) -> FullscreenVertexOutput {
    var output: FullscreenVertexOutput;
    
    // 使用大三角形覆盖整个屏幕
    let x = f32((vertexIndex & 1u) << 2u) - 1.0;
    let y = f32((vertexIndex & 2u) << 1u) - 1.0;
    
    output.position = vec4<f32>(x, y, 0.0, 1.0);
    output.uv = vec2<f32>((x + 1.0) * 0.5, (1.0 - y) * 0.5);
    
    return output;
}

// ============ 距离雾效计算函数 ============

/// 计算线性雾效因子
/// 返回值范围 [0, 1]，0 表示完全雾化，1 表示无雾
/// Requirements: 11.1, 11.3
fn CalculateLinearFog(distance: f32, fogStart: f32, fogEnd: f32) -> f32 {
    // 线性插值：在 fogStart 处雾效为 0，在 fogEnd 处雾效为最大
    return clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
}

/// 计算指数雾效因子
/// 返回值范围 [0, 1]，0 表示完全雾化，1 表示无雾
/// Requirements: 11.1, 11.3
fn CalculateExponentialFog(distance: f32, density: f32) -> f32 {
    // 指数衰减：exp(-density * distance)
    return exp(-density * distance);
}

/// 计算指数平方雾效因子
/// 返回值范围 [0, 1]，0 表示完全雾化，1 表示无雾
/// Requirements: 11.1, 11.3
fn CalculateExponentialSquaredFog(distance: f32, density: f32) -> f32 {
    // 指数平方衰减：exp(-(density * distance)^2)
    let factor = density * distance;
    return exp(-factor * factor);
}

/// 根据模式计算距离雾效因子
/// Requirements: 11.1, 11.3
fn CalculateDistanceFogFactor(distance: f32, fogStart: f32, fogEnd: f32, density: f32, mode: u32) -> f32 {
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

// ============ 高度雾效计算函数 ============

/// 计算高度雾因子
/// 在基准高度以下密度最大，随高度增加密度递减
/// Requirements: 11.5
fn CalculateHeightFogFactor(worldY: f32, fogBase: f32, heightDensity: f32) -> f32 {
    // 计算相对于基准高度的高度差
    let heightAboveBase = max(worldY - fogBase, 0.0);
    // 指数衰减：高度越高，雾效越弱
    return exp(-heightDensity * heightAboveBase);
}

/// 计算组合的高度雾因子（考虑基准高度以下的情况）
/// Requirements: 11.5
fn CalculateHeightFogFactorFull(worldY: f32, fogBase: f32, heightDensity: f32) -> f32 {
    if (worldY <= fogBase) {
        // 在基准高度或以下，雾效最大
        return 1.0;
    } else {
        // 在基准高度以上，雾效随高度递减
        let heightAboveBase = worldY - fogBase;
        return exp(-heightDensity * heightAboveBase);
    }
}

// ============ 雾效应用函数 ============

/// 应用雾效到颜色
/// fogFactor: 1.0 表示无雾（保持原色），0.0 表示完全雾化（使用雾色）
fn ApplyFog(color: vec3<f32>, fogColor: vec3<f32>, fogFactor: f32) -> vec3<f32> {
    return mix(fogColor, color, fogFactor);
}

/// 组合距离雾和高度雾因子
fn CombineFogFactors(distanceFactor: f32, heightFactor: f32) -> f32 {
    // 两种雾效相乘，任一因子为 0 都会导致完全雾化
    return distanceFactor * heightFactor;
}

// ============ 距离计算函数 ============

/// 从深度值计算世界空间距离（2D 简化版本）
/// 对于 2D 游戏，我们使用 UV 坐标和相机参数来估算距离
fn CalculateWorldDistance(uv: vec2<f32>, depth: f32, cameraPos: vec2<f32>, zoom: f32) -> f32 {
    // 将 UV 转换为相对于屏幕中心的坐标
    let screenOffset = (uv - 0.5) * 2.0;
    
    // 考虑相机缩放，计算世界空间偏移
    // 假设标准视口大小为 1920x1080，缩放因子影响可见范围
    let worldOffset = screenOffset / max(zoom, 0.001);
    
    // 使用深度值作为 Z 距离（如果有的话）
    // 对于 2D 游戏，深度通常表示图层深度或伪 3D 距离
    let zDistance = depth * 100.0; // 缩放深度值
    
    // 计算总距离（欧几里得距离）
    let xyDistance = length(worldOffset) * 100.0; // 缩放到世界单位
    return sqrt(xyDistance * xyDistance + zDistance * zDistance);
}

/// 从 UV 坐标计算简化的世界距离（无深度纹理时使用）
fn CalculateSimpleWorldDistance(uv: vec2<f32>, cameraPos: vec2<f32>, zoom: f32) -> f32 {
    // 将 UV 转换为相对于屏幕中心的坐标
    let screenOffset = (uv - 0.5) * 2.0;
    
    // 考虑相机缩放
    let worldOffset = screenOffset / max(zoom, 0.001);
    
    // 返回到屏幕中心的距离（世界单位）
    return length(worldOffset) * 100.0;
}

// ============ 片段着色器 ============

/// 雾效片段着色器（带深度纹理）
/// 使用深度纹理计算精确的世界距离
@fragment
fn fs_fog_with_depth(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 采样深度值
    let depth = textureSample(depthTexture, depthSampler, input.uv).r;
    
    // 计算世界距离
    let worldDistance = CalculateWorldDistance(
        input.uv, 
        depth, 
        fogParams.cameraPosition, 
        fogParams.cameraZoom
    );
    
    // 计算距离雾因子
    let distanceFogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 计算最终雾因子
    var finalFogFactor = distanceFogFactor;
    
    // 如果启用高度雾，计算并组合高度雾因子
    if (fogParams.enableHeightFog != 0u) {
        // 从深度和 UV 估算世界 Y 坐标
        // 对于 2D 游戏，Y 坐标通常与屏幕 Y 相关
        let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
        
        let heightFogFactor = CalculateHeightFogFactorFull(
            worldY,
            fogParams.heightFogBase,
            fogParams.heightFogDensity
        );
        
        finalFogFactor = CombineFogFactors(distanceFogFactor, heightFogFactor);
    }
    
    // 应用雾效
    let foggedColor = ApplyFog(sceneColor.rgb, fogParams.fogColor.rgb, finalFogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

/// 雾效片段着色器（简化版，无深度纹理）
/// 使用屏幕空间距离近似
@fragment
fn fs_fog_simple(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 计算简化的世界距离
    let worldDistance = CalculateSimpleWorldDistance(
        input.uv,
        fogParams.cameraPosition,
        fogParams.cameraZoom
    );
    
    // 计算距离雾因子
    let distanceFogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 计算最终雾因子
    var finalFogFactor = distanceFogFactor;
    
    // 如果启用高度雾
    if (fogParams.enableHeightFog != 0u) {
        // 使用屏幕 Y 坐标估算世界 Y
        let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
        
        let heightFogFactor = CalculateHeightFogFactorFull(
            worldY,
            fogParams.heightFogBase,
            fogParams.heightFogDensity
        );
        
        finalFogFactor = CombineFogFactors(distanceFogFactor, heightFogFactor);
    }
    
    // 应用雾效
    let foggedColor = ApplyFog(sceneColor.rgb, fogParams.fogColor.rgb, finalFogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

/// 仅高度雾片段着色器
/// 只应用基于 Y 坐标的高度雾效果
/// Requirements: 11.5
@fragment
fn fs_height_fog_only(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u || fogParams.enableHeightFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 使用屏幕 Y 坐标估算世界 Y
    // UV.y = 0 是屏幕顶部，UV.y = 1 是屏幕底部
    // 世界 Y 通常是底部为低，顶部为高
    let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
    
    // 计算高度雾因子
    let heightFogFactor = CalculateHeightFogFactorFull(
        worldY,
        fogParams.heightFogBase,
        fogParams.heightFogDensity
    );
    
    // 应用雾效
    let foggedColor = ApplyFog(sceneColor.rgb, fogParams.fogColor.rgb, heightFogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

/// 仅距离雾片段着色器
/// 只应用基于距离的雾效果
/// Requirements: 11.1, 11.3
@fragment
fn fs_distance_fog_only(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 计算简化的世界距离
    let worldDistance = CalculateSimpleWorldDistance(
        input.uv,
        fogParams.cameraPosition,
        fogParams.cameraZoom
    );
    
    // 计算距离雾因子
    let distanceFogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 应用雾效
    let foggedColor = ApplyFog(sceneColor.rgb, fogParams.fogColor.rgb, distanceFogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

// ============ 调试着色器 ============

/// 调试着色器：可视化雾效因子
@fragment
fn fs_fog_debug(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 计算简化的世界距离
    let worldDistance = CalculateSimpleWorldDistance(
        input.uv,
        fogParams.cameraPosition,
        fogParams.cameraZoom
    );
    
    // 计算距离雾因子
    let distanceFogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 计算高度雾因子
    let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
    let heightFogFactor = CalculateHeightFogFactorFull(
        worldY,
        fogParams.heightFogBase,
        fogParams.heightFogDensity
    );
    
    // 可视化：R = 距离雾因子，G = 高度雾因子，B = 组合因子
    let combinedFactor = CombineFogFactors(distanceFogFactor, heightFogFactor);
    
    return vec4<f32>(distanceFogFactor, heightFogFactor, combinedFactor, 1.0);
}

// ============ 带光源穿透的雾效着色器 ============

/// 雾效片段着色器（带光源穿透）
/// 光源可以穿透雾效，在光源附近减少雾效强度
/// Requirements: 11.4
@fragment
fn fs_fog_with_light_penetration(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 计算简化的世界距离
    let worldDistance = CalculateSimpleWorldDistance(
        input.uv,
        fogParams.cameraPosition,
        fogParams.cameraZoom
    );
    
    // 计算世界位置（从 UV 和相机参数）
    let screenOffset = (input.uv - 0.5) * 2.0;
    let worldOffset = screenOffset / max(fogParams.cameraZoom, 0.001) * 100.0;
    let worldPos = fogParams.cameraPosition + worldOffset;
    
    // 计算距离雾因子
    var fogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 如果启用高度雾
    if (fogParams.enableHeightFog != 0u) {
        let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
        let heightFogFactor = CalculateHeightFogFactorFull(
            worldY,
            fogParams.heightFogBase,
            fogParams.heightFogDensity
        );
        fogFactor = CombineFogFactors(fogFactor, heightFogFactor);
    }
    
    // 应用光源穿透
    if (fogLightParams.enableLightPenetration != 0u && fogLightParams.lightCount > 0u) {
        let penetration = CalculateTotalLightPenetration(worldPos, fogLightParams.maxPenetration);
        fogFactor = ApplyLightPenetrationToFog(fogFactor, penetration);
    }
    
    // 应用雾效
    let foggedColor = ApplyFog(sceneColor.rgb, fogParams.fogColor.rgb, fogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

/// 雾效片段着色器（带光源穿透和光源颜色混合）
/// 光源不仅穿透雾效，还会将光源颜色混入雾效
/// Requirements: 11.4
@fragment
fn fs_fog_with_light_color_blend(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 检查雾效是否启用
    if (fogParams.enableFog == 0u) {
        return textureSample(sceneTexture, sceneSampler, input.uv);
    }
    
    // 采样场景颜色
    let sceneColor = textureSample(sceneTexture, sceneSampler, input.uv);
    
    // 计算简化的世界距离
    let worldDistance = CalculateSimpleWorldDistance(
        input.uv,
        fogParams.cameraPosition,
        fogParams.cameraZoom
    );
    
    // 计算世界位置
    let screenOffset = (input.uv - 0.5) * 2.0;
    let worldOffset = screenOffset / max(fogParams.cameraZoom, 0.001) * 100.0;
    let worldPos = fogParams.cameraPosition + worldOffset;
    
    // 计算距离雾因子
    var fogFactor = CalculateDistanceFogFactor(
        worldDistance,
        fogParams.fogStart,
        fogParams.fogEnd,
        fogParams.fogDensity,
        fogParams.fogMode
    );
    
    // 如果启用高度雾
    if (fogParams.enableHeightFog != 0u) {
        let worldY = (1.0 - input.uv.y) * 100.0 / max(fogParams.cameraZoom, 0.001);
        let heightFogFactor = CalculateHeightFogFactorFull(
            worldY,
            fogParams.heightFogBase,
            fogParams.heightFogDensity
        );
        fogFactor = CombineFogFactors(fogFactor, heightFogFactor);
    }
    
    // 计算光源穿透和颜色混合
    var modifiedFogColor = fogParams.fogColor.rgb;
    var totalPenetration = 0.0;
    
    if (fogLightParams.enableLightPenetration != 0u && fogLightParams.lightCount > 0u) {
        let lightCount = min(fogLightParams.lightCount, MAX_FOG_LIGHTS);
        var lightColorContrib = vec3<f32>(0.0);
        var totalWeight = 0.0;
        
        for (var i = 0u; i < lightCount; i++) {
            let light = fogLights[i];
            let penetration = CalculateLightPenetration(
                worldPos,
                light.position,
                light.radius,
                light.intensity,
                light.penetrationStrength,
                light.falloff
            );
            
            if (penetration > 0.0) {
                // 累加光源颜色贡献
                lightColorContrib += light.color.rgb * penetration;
                totalWeight += penetration;
                totalPenetration = max(totalPenetration, penetration);
            }
        }
        
        // 混合光源颜色到雾效颜色
        if (totalWeight > 0.0) {
            let avgLightColor = lightColorContrib / totalWeight;
            modifiedFogColor = mix(modifiedFogColor, avgLightColor, totalPenetration * 0.5);
        }
        
        // 应用穿透
        totalPenetration = clamp(totalPenetration, 0.0, fogLightParams.maxPenetration);
        fogFactor = ApplyLightPenetrationToFog(fogFactor, totalPenetration);
    }
    
    // 应用雾效（使用可能被光源修改的雾效颜色）
    let foggedColor = ApplyFog(sceneColor.rgb, modifiedFogColor, fogFactor);
    
    return vec4<f32>(foggedColor, sceneColor.a);
}

/// 调试着色器：可视化光源穿透
@fragment
fn fs_fog_light_penetration_debug(input: FullscreenVertexOutput) -> @location(0) vec4<f32> {
    // 计算世界位置
    let screenOffset = (input.uv - 0.5) * 2.0;
    let worldOffset = screenOffset / max(fogParams.cameraZoom, 0.001) * 100.0;
    let worldPos = fogParams.cameraPosition + worldOffset;
    
    // 计算光源穿透
    var penetration = 0.0;
    if (fogLightParams.enableLightPenetration != 0u && fogLightParams.lightCount > 0u) {
        penetration = CalculateTotalLightPenetration(worldPos, fogLightParams.maxPenetration);
    }
    
    // 可视化穿透量（红色 = 高穿透，蓝色 = 低穿透）
    return vec4<f32>(penetration, 0.0, 1.0 - penetration, 1.0);
}


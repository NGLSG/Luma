// ============================================================================
// NoiseUtils.wgsl - 噪声函数库
// 提供各种噪声生成算法: Value Noise, Perlin Noise, Simplex Noise, Worley Noise等
// ============================================================================
export Utils.Noise;
import ::Math;

// ============ Value Noise (值噪声) ============

/// 1D Value Noise
fn ValueNoise1D(x: f32) -> f32 {
    let i = floor(x);
    let f = Fract(x);
    
    let a = Hash11(i);
    let b = Hash11(i + 1.0);
    
    let u = f * f * (3.0 - 2.0 * f); // Smoothstep
    return mix(a, b, u);
}

/// 2D Value Noise
fn ValueNoise2D(p: vec2<f32>) -> f32 {
    let i = floor(p);
    let f = Fract2(p);
    
    // 四个角的随机值
    let a = Hash21(i);
    let b = Hash21(i + vec2<f32>(1.0, 0.0));
    let c = Hash21(i + vec2<f32>(0.0, 1.0));
    let d = Hash21(i + vec2<f32>(1.0, 1.0));
    
    // 平滑插值
    let u = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(a, b, u.x),
        mix(c, d, u.x),
        u.y
    );
}

/// 3D Value Noise
fn ValueNoise3D(p: vec3<f32>) -> f32 {
    let i = floor(p);
    let f = Fract3(p);
    
    // 8个角的随机值
    let n000 = Hash31(i);
    let n100 = Hash31(i + vec3<f32>(1.0, 0.0, 0.0));
    let n010 = Hash31(i + vec3<f32>(0.0, 1.0, 0.0));
    let n110 = Hash31(i + vec3<f32>(1.0, 1.0, 0.0));
    let n001 = Hash31(i + vec3<f32>(0.0, 0.0, 1.0));
    let n101 = Hash31(i + vec3<f32>(1.0, 0.0, 1.0));
    let n011 = Hash31(i + vec3<f32>(0.0, 1.0, 1.0));
    let n111 = Hash31(i + vec3<f32>(1.0, 1.0, 1.0));
    
    // 平滑插值
    let u = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
        mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y),
        u.z
    );
}

// ============ Gradient Noise (梯度噪声 / Perlin Noise) ============

/// 2D Perlin Noise
fn PerlinNoise2D(p: vec2<f32>) -> f32 {
    let i = floor(p);
    let f = Fract2(p);
    
    // 四个角的梯度向量
    let g00 = RandomDirection2D(i);
    let g10 = RandomDirection2D(i + vec2<f32>(1.0, 0.0));
    let g01 = RandomDirection2D(i + vec2<f32>(0.0, 1.0));
    let g11 = RandomDirection2D(i + vec2<f32>(1.0, 1.0));
    
    // 计算到四个角的向量
    let d00 = f;
    let d10 = f - vec2<f32>(1.0, 0.0);
    let d01 = f - vec2<f32>(0.0, 1.0);
    let d11 = f - vec2<f32>(1.0, 1.0);
    
    // 计算点积
    let n00 = dot(g00, d00);
    let n10 = dot(g10, d10);
    let n01 = dot(g01, d01);
    let n11 = dot(g11, d11);
    
    // 使用Smootherstep进行插值
    let u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    return mix(
        mix(n00, n10, u.x),
        mix(n01, n11, u.x),
        u.y
    ) * 0.5 + 0.5; // 映射到0-1范围
}

/// 3D Perlin Noise
fn PerlinNoise3D(p: vec3<f32>) -> f32 {
    let i = floor(p);
    let f = Fract3(p);
    
    // 8个角的梯度向量
    let g000 = normalize(Hash33(i) * 2.0 - 1.0);
    let g100 = normalize(Hash33(i + vec3<f32>(1.0, 0.0, 0.0)) * 2.0 - 1.0);
    let g010 = normalize(Hash33(i + vec3<f32>(0.0, 1.0, 0.0)) * 2.0 - 1.0);
    let g110 = normalize(Hash33(i + vec3<f32>(1.0, 1.0, 0.0)) * 2.0 - 1.0);
    let g001 = normalize(Hash33(i + vec3<f32>(0.0, 0.0, 1.0)) * 2.0 - 1.0);
    let g101 = normalize(Hash33(i + vec3<f32>(1.0, 0.0, 1.0)) * 2.0 - 1.0);
    let g011 = normalize(Hash33(i + vec3<f32>(0.0, 1.0, 1.0)) * 2.0 - 1.0);
    let g111 = normalize(Hash33(i + vec3<f32>(1.0, 1.0, 1.0)) * 2.0 - 1.0);
    
    // 计算到8个角的向量
    let d000 = f;
    let d100 = f - vec3<f32>(1.0, 0.0, 0.0);
    let d010 = f - vec3<f32>(0.0, 1.0, 0.0);
    let d110 = f - vec3<f32>(1.0, 1.0, 0.0);
    let d001 = f - vec3<f32>(0.0, 0.0, 1.0);
    let d101 = f - vec3<f32>(1.0, 0.0, 1.0);
    let d011 = f - vec3<f32>(0.0, 1.0, 1.0);
    let d111 = f - vec3<f32>(1.0, 1.0, 1.0);
    
    // 计算点积
    let n000 = dot(g000, d000);
    let n100 = dot(g100, d100);
    let n010 = dot(g010, d010);
    let n110 = dot(g110, d110);
    let n001 = dot(g001, d001);
    let n101 = dot(g101, d101);
    let n011 = dot(g011, d011);
    let n111 = dot(g111, d111);
    
    // Smootherstep插值
    let u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    return mix(
        mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
        mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y),
        u.z
    ) * 0.5 + 0.5;
}

// ============ Simplex Noise (单纯形噪声) ============

/// 2D Simplex Noise (简化版本)
fn SimplexNoise2D(p: vec2<f32>) -> f32 {
    const F2: f32 = 0.366025403784; // (sqrt(3) - 1) / 2
    const G2: f32 = 0.211324865405; // (3 - sqrt(3)) / 6
    
    // 扭曲输入空间
    let s = (p.x + p.y) * F2;
    let i = floor(p + s);
    let t = (i.x + i.y) * G2;
    let X0 = i.x - t;
    let Y0 = i.y - t;
    let x0 = p.x - X0;
    let y0 = p.y - Y0;
    
    // 确定单纯形
    var i1: vec2<f32>;
    if (x0 > y0) {
        i1 = vec2<f32>(1.0, 0.0);
    } else {
        i1 = vec2<f32>(0.0, 1.0);
    }
    
    // 三个角的偏移
    let x1 = x0 - i1.x + G2;
    let y1 = y0 - i1.y + G2;
    let x2 = x0 - 1.0 + 2.0 * G2;
    let y2 = y0 - 1.0 + 2.0 * G2;
    
    // 计算梯度
    let g0 = RandomDirection2D(i);
    let g1 = RandomDirection2D(i + i1);
    let g2 = RandomDirection2D(i + vec2<f32>(1.0, 1.0));
    
    // 计算贡献
    var n0 = 0.0;
    var n1 = 0.0;
    var n2 = 0.0;
    
    let t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 > 0.0) {
        let t02 = t0 * t0;
        n0 = t02 * t02 * dot(g0, vec2<f32>(x0, y0));
    }
    
    let t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 > 0.0) {
        let t12 = t1 * t1;
        n1 = t12 * t12 * dot(g1, vec2<f32>(x1, y1));
    }
    
    let t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 > 0.0) {
        let t22 = t2 * t2;
        n2 = t22 * t22 * dot(g2, vec2<f32>(x2, y2));
    }
    
    return (70.0 * (n0 + n1 + n2)) * 0.5 + 0.5;
}

// ============ Worley Noise (Voronoi / 细胞噪声) ============

/// 2D Worley Noise (返回最近距离)
fn WorleyNoise2D(p: vec2<f32>) -> f32 {
    let i = floor(p);
    let f = Fract2(p);
    
    var minDist = 1.0;
    
    // 检查周围9个格子
    for (var y = -1; y <= 1; y++) {
        for (var x = -1; x <= 1; x++) {
            let neighbor = vec2<f32>(f32(x), f32(y));
            let cellPos = Hash22(i + neighbor);
            let diff = neighbor + cellPos - f;
            let dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    
    return minDist;
}

/// 2D Worley Noise (返回前两个最近距离)
fn WorleyNoise2D_F1F2(p: vec2<f32>) -> vec2<f32> {
    let i = floor(p);
    let f = Fract2(p);
    
    var minDist1 = 1.0;
    var minDist2 = 1.0;
    
    // 检查周围9个格子
    for (var y = -1; y <= 1; y++) {
        for (var x = -1; x <= 1; x++) {
            let neighbor = vec2<f32>(f32(x), f32(y));
            let cellPos = Hash22(i + neighbor);
            let diff = neighbor + cellPos - f;
            let dist = length(diff);
            
            if (dist < minDist1) {
                minDist2 = minDist1;
                minDist1 = dist;
            } else if (dist < minDist2) {
                minDist2 = dist;
            }
        }
    }
    
    return vec2<f32>(minDist1, minDist2);
}

/// 3D Worley Noise
fn WorleyNoise3D(p: vec3<f32>) -> f32 {
    let i = floor(p);
    let f = Fract3(p);
    
    var minDist = 1.0;
    
    // 检查周围27个格子
    for (var z = -1; z <= 1; z++) {
        for (var y = -1; y <= 1; y++) {
            for (var x = -1; x <= 1; x++) {
                let neighbor = vec3<f32>(f32(x), f32(y), f32(z));
                let cellPos = Hash33(i + neighbor);
                let diff = neighbor + cellPos - f;
                let dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    
    return minDist;
}

// ============ Fractal Noise (分形噪声 / FBM) ============

/// Fractal Brownian Motion (FBM) - 2D Value Noise
fn FBM_Value2D(p: vec2<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += ValueNoise2D(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

/// FBM - 2D Perlin Noise
fn FBM_Perlin2D(p: vec2<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += PerlinNoise2D(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

/// FBM - 2D Simplex Noise
fn FBM_Simplex2D(p: vec2<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += SimplexNoise2D(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

/// FBM - 3D Perlin Noise
fn FBM_Perlin3D(p: vec3<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += PerlinNoise3D(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

// ============ Turbulence (湍流噪声) ============

/// 2D Turbulence
fn Turbulence2D(p: vec2<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += abs(PerlinNoise2D(p * frequency) * 2.0 - 1.0) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

/// 3D Turbulence
fn Turbulence3D(p: vec3<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        value += abs(PerlinNoise3D(p * frequency) * 2.0 - 1.0) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

// ============ Ridge Noise (山脊噪声) ============

/// 2D Ridge Noise
fn RidgeNoise2D(p: vec2<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var value = 0.0;
    var amplitude = 1.0;
    var frequency = 1.0;
    var maxValue = 0.0;
    
    for (var i = 0; i < octaves; i++) {
        let n = 1.0 - abs(PerlinNoise2D(p * frequency) * 2.0 - 1.0);
        value += n * n * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

// ============ Domain Warping (域扭曲) ============

/// 2D Domain Warping
fn DomainWarp2D(p: vec2<f32>, amount: f32) -> vec2<f32> {
    let offset = vec2<f32>(
        PerlinNoise2D(p),
        PerlinNoise2D(p + vec2<f32>(5.2, 1.3))
    );
    return p + offset * amount;
}

/// 2D Domain Warping with FBM
fn DomainWarpFBM2D(p: vec2<f32>, octaves: i32, amount: f32) -> vec2<f32> {
    let offset = vec2<f32>(
        FBM_Perlin2D(p, octaves, 2.0, 0.5),
        FBM_Perlin2D(p + vec2<f32>(5.2, 1.3), octaves, 2.0, 0.5)
    );
    return p + offset * amount;
}

// ============ 特殊噪声模式 ============

/// 大理石纹理
fn MarbleNoise(p: vec2<f32>) -> f32 {
    let n = FBM_Perlin2D(p, 4, 2.0, 0.5);
    return sin(p.x + n * 10.0) * 0.5 + 0.5;
}

/// 木纹纹理
fn WoodNoise(p: vec2<f32>) -> f32 {
    let n = FBM_Perlin2D(p, 4, 2.0, 0.5);
    let dist = length(p);
    return Fract(dist + n * 5.0);
}

/// 云朵噪声
fn CloudNoise(p: vec2<f32>) -> f32 {
    let fbm = FBM_Perlin2D(p, 6, 2.0, 0.5);
    return pow(fbm, 2.0);
}

/// 火焰噪声
fn FireNoise(p: vec2<f32>, time: f32) -> f32 {
    let p1 = p + vec2<f32>(0.0, time);
    let turbulence = Turbulence2D(p1 * 2.0, 4, 2.0, 0.5);
    let fbm = FBM_Perlin2D(p1, 3, 2.0, 0.5);
    return pow(turbulence * fbm, 2.0);
}

/// 水波纹噪声
fn WaterNoise(p: vec2<f32>, time: f32) -> f32 {
    let p1 = p + vec2<f32>(time * 0.1, time * 0.05);
    let wave1 = sin(p1.x * 3.0 + time) * 0.5 + 0.5;
    let wave2 = sin(p1.y * 2.0 + time * 1.3) * 0.5 + 0.5;
    let noise = FBM_Perlin2D(p1, 3, 2.0, 0.5);
    return (wave1 + wave2) * 0.5 * noise;
}

/// 闪电噪声
fn LightningNoise(p: vec2<f32>, time: f32) -> f32 {
    let p1 = vec2<f32>(p.x, p.y + time);
    let ridge = RidgeNoise2D(p1 * 5.0, 3, 2.0, 0.5);
    let turbulence = Turbulence2D(p1 * 2.0, 2, 2.0, 0.5);
    return pow(ridge * turbulence, 3.0);
}

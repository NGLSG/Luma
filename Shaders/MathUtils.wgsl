// ============================================================================
// MathUtils.wgsl - 数学工具函数库
// 提供高级数学运算、插值、随机数、几何计算等功能
// ============================================================================
export Utils.Math;
// ============ 常量定义 ============

const PI: f32 = 3.14159265359;
const TWO_PI: f32 = 6.28318530718;
const HALF_PI: f32 = 1.57079632679;
const INV_PI: f32 = 0.31830988618;
const EPSILON: f32 = 0.00001;
const E: f32 = 2.71828182846;
const GOLDEN_RATIO: f32 = 1.61803398875;

// ============ 基础数学函数 ============

/// 快速平方根倒数 (近似)
fn FastInvSqrt(x: f32) -> f32 {
    return 1.0 / sqrt(x);
}

/// 安全除法 (避免除零)
fn SafeDivide(a: f32, b: f32) -> f32 {
    if (abs(b) < EPSILON) {
        return 0.0;
    }
    return a / b;
}

/// 平方函数
fn Sqr(x: f32) -> f32 {
    return x * x;
}

/// 立方函数
fn Cube(x: f32) -> f32 {
    return x * x * x;
}

/// 幂函数 (安全版本)
fn SafePow(x: f32, y: f32) -> f32 {
    return pow(max(x, 0.0), y);
}

/// 符号函数 (-1, 0, 1)
fn Sign(x: f32) -> f32 {
    if (x > 0.0) { return 1.0; }
    if (x < 0.0) { return -1.0; }
    return 0.0;
}

/// 取小数部分
fn Fract(x: f32) -> f32 {
    return x - floor(x);
}

/// 取小数部分 (vec2)
fn Fract2(v: vec2<f32>) -> vec2<f32> {
    return v - floor(v);
}

/// 取小数部分 (vec3)
fn Fract3(v: vec3<f32>) -> vec3<f32> {
    return v - floor(v);
}

// ============ 插值函数 ============

/// 线性插值
fn Lerp(a: f32, b: f32, t: f32) -> f32 {
    return a + (b - a) * t;
}

/// 线性插值 (vec2)
fn Lerp2(a: vec2<f32>, b: vec2<f32>, t: f32) -> vec2<f32> {
    return a + (b - a) * t;
}

/// 线性插值 (vec3)
fn Lerp3(a: vec3<f32>, b: vec3<f32>, t: f32) -> vec3<f32> {
    return a + (b - a) * t;
}

/// 平滑步进 (Hermite插值)
fn SmoothStep(edge0: f32, edge1: f32, x: f32) -> f32 {
    let t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/// 更平滑的步进 (Ken Perlin的改进版本)
fn SmootherStep(edge0: f32, edge1: f32, x: f32) -> f32 {
    let t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

/// 反向插值 (获取插值参数t)
fn InverseLerp(a: f32, b: f32, value: f32) -> f32 {
    return SafeDivide(value - a, b - a);
}

/// 重映射值从一个范围到另一个范围
fn Remap(value: f32, fromMin: f32, fromMax: f32, toMin: f32, toMax: f32) -> f32 {
    let t = InverseLerp(fromMin, fromMax, value);
    return Lerp(toMin, toMax, t);
}

/// 三次Hermite插值
fn CubicHermite(a: f32, b: f32, c: f32, d: f32, t: f32) -> f32 {
    let t2 = t * t;
    let t3 = t2 * t;
    let a0 = d - c - a + b;
    let a1 = a - b - a0;
    let a2 = c - a;
    let a3 = b;
    return a0 * t3 + a1 * t2 + a2 * t + a3;
}

/// Catmull-Rom样条插值
fn CatmullRom(p0: f32, p1: f32, p2: f32, p3: f32, t: f32) -> f32 {
    let t2 = t * t;
    let t3 = t2 * t;
    return 0.5 * (
        (2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
    );
}

// ============ 缓动函数 (Easing Functions) ============

/// 二次缓入
fn EaseInQuad(t: f32) -> f32 {
    return t * t;
}

/// 二次缓出
fn EaseOutQuad(t: f32) -> f32 {
    return t * (2.0 - t);
}

/// 二次缓入缓出
fn EaseInOutQuad(t: f32) -> f32 {
    if (t < 0.5) {
        return 2.0 * t * t;
    }
    return -1.0 + (4.0 - 2.0 * t) * t;
}

/// 三次缓入
fn EaseInCubic(t: f32) -> f32 {
    return t * t * t;
}

/// 三次缓出
fn EaseOutCubic(t: f32) -> f32 {
    let t1 = t - 1.0;
    return t1 * t1 * t1 + 1.0;
}

/// 三次缓入缓出
fn EaseInOutCubic(t: f32) -> f32 {
    if (t < 0.5) {
        return 4.0 * t * t * t;
    }
    let t1 = 2.0 * t - 2.0;
    return 0.5 * t1 * t1 * t1 + 1.0;
}

/// 正弦缓入
fn EaseInSine(t: f32) -> f32 {
    return 1.0 - cos(t * HALF_PI);
}

/// 正弦缓出
fn EaseOutSine(t: f32) -> f32 {
    return sin(t * HALF_PI);
}

/// 正弦缓入缓出
fn EaseInOutSine(t: f32) -> f32 {
    return 0.5 * (1.0 - cos(PI * t));
}

/// 指数缓入
fn EaseInExpo(t: f32) -> f32 {
    if (t <= 0.0) { return 0.0; }
    return pow(2.0, 10.0 * (t - 1.0));
}

/// 指数缓出
fn EaseOutExpo(t: f32) -> f32 {
    if (t >= 1.0) { return 1.0; }
    return 1.0 - pow(2.0, -10.0 * t);
}

/// 弹性缓出
fn EaseOutElastic(t: f32) -> f32 {
    if (t <= 0.0) { return 0.0; }
    if (t >= 1.0) { return 1.0; }
    let p = 0.3;
    return pow(2.0, -10.0 * t) * sin((t - p / 4.0) * TWO_PI / p) + 1.0;
}

/// 回弹缓出
fn EaseOutBounce(t: f32) -> f32 {
    if (t < 1.0 / 2.75) {
        return 7.5625 * t * t;
    } else if (t < 2.0 / 2.75) {
        let t1 = t - 1.5 / 2.75;
        return 7.5625 * t1 * t1 + 0.75;
    } else if (t < 2.5 / 2.75) {
        let t1 = t - 2.25 / 2.75;
        return 7.5625 * t1 * t1 + 0.9375;
    } else {
        let t1 = t - 2.625 / 2.75;
        return 7.5625 * t1 * t1 + 0.984375;
    }
}

// ============ 几何函数 ============

/// 计算两点距离
fn Distance2D(a: vec2<f32>, b: vec2<f32>) -> f32 {
    let d = b - a;
    return sqrt(d.x * d.x + d.y * d.y);
}

/// 计算曼哈顿距离
fn ManhattanDistance(a: vec2<f32>, b: vec2<f32>) -> vec2<f32> {
    return abs(b - a);
}

/// 计算切比雪夫距离
fn ChebyshevDistance(a: vec2<f32>, b: vec2<f32>) -> f32 {
    let d = abs(b - a);
    return max(d.x, d.y);
}

/// 向量旋转 (2D)
fn Rotate2D(v: vec2<f32>, angle: f32) -> vec2<f32> {
    let c = cos(angle);
    let s = sin(angle);
    return vec2<f32>(
        v.x * c - v.y * s,
        v.x * s + v.y * c
    );
}

/// 向量反射
fn Reflect2D(incident: vec2<f32>, normal: vec2<f32>) -> vec2<f32> {
    return incident - 2.0 * dot(incident, normal) * normal;
}

/// 向量折射
fn Refract2D(incident: vec2<f32>, normal: vec2<f32>, eta: f32) -> vec2<f32> {
    let dotNI = dot(normal, incident);
    let k = 1.0 - eta * eta * (1.0 - dotNI * dotNI);
    if (k < 0.0) {
        return vec2<f32>(0.0);
    }
    return eta * incident - (eta * dotNI + sqrt(k)) * normal;
}

/// 计算角度 (弧度)
fn AngleBetween(a: vec2<f32>, b: vec2<f32>) -> f32 {
    return acos(clamp(dot(normalize(a), normalize(b)), -1.0, 1.0));
}

/// 向量投影
fn Project(a: vec2<f32>, b: vec2<f32>) -> vec2<f32> {
    return b * (dot(a, b) / dot(b, b));
}

/// 向量垂直分量
fn Perpendicular(a: vec2<f32>, b: vec2<f32>) -> vec2<f32> {
    return a - Project(a, b);
}

// ============ 范围和约束函数 ============

/// 将值约束在范围内 (循环)
fn Wrap(value: f32, min: f32, max: f32) -> f32 {
    let range = max - min;
    return min + ((value - min) % range + range) % range;
}

/// 将值约束在0-1范围内 (循环)
fn Wrap01(value: f32) -> f32 {
    return Fract(value);
}

/// 将角度约束在-PI到PI范围内
fn WrapAngle(angle: f32) -> f32 {
    return Wrap(angle, -PI, PI);
}

/// Ping-pong效果 (值在0-1之间来回)
fn PingPong(t: f32) -> f32 {
    let t1 = Wrap01(t * 0.5);
    return 1.0 - abs(t1 * 2.0 - 1.0);
}

/// 软约束 (渐进式接近边界)
fn SoftClamp(value: f32, min: f32, max: f32, softness: f32) -> f32 {
    if (value < min) {
        return min - softness * (1.0 - exp((value - min) / softness));
    }
    if (value > max) {
        return max + softness * (1.0 - exp((max - value) / softness));
    }
    return value;
}

// ============ 随机数和哈希函数 ============

/// 简单哈希函数 (float)
fn Hash11(p: f32) -> f32 {
    var p1 = Fract(p * 0.1031);
    p1 *= p1 + 33.33;
    p1 *= p1 + p1;
    return Fract(p1);
}

/// 2D哈希函数 -> 1D
fn Hash21(p: vec2<f32>) -> f32 {
    var p3 = Fract3(vec3<f32>(p.x, p.y, p.x) * 0.1031);
    p3 += dot(p3, vec3<f32>(p3.y, p3.z, p3.x) + 33.33);
    return Fract((p3.x + p3.y) * p3.z);
}

/// 2D哈希函数 -> 2D
fn Hash22(p: vec2<f32>) -> vec2<f32> {
    var p3 = Fract3(vec3<f32>(p.x, p.y, p.x) * vec3<f32>(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, vec3<f32>(p3.y, p3.z, p3.x) + 33.33);
    return Fract2(vec2<f32>((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y));
}

/// 3D哈希函数 -> 1D
fn Hash31(p: vec3<f32>) -> f32 {
    var p3 = Fract3(p * 0.1031);
    p3 += dot(p3, vec3<f32>(p3.y, p3.z, p3.x) + 33.33);
    return Fract((p3.x + p3.y) * p3.z);
}

/// 3D哈希函数 -> 3D
fn Hash33(p: vec3<f32>) -> vec3<f32> {
    var p3 = Fract3(p * vec3<f32>(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, vec3<f32>(p3.y, p3.z, p3.x) + 33.33);
    return Fract3(vec3<f32>(
        (p3.x + p3.y) * p3.z,
        (p3.x + p3.z) * p3.y,
        (p3.y + p3.z) * p3.x
    ));
}

/// 伪随机数生成 (基于种子)
fn Random(seed: f32) -> f32 {
    return Hash11(seed);
}

/// 2D伪随机数生成
fn Random2D(seed: vec2<f32>) -> f32 {
    return Hash21(seed);
}

/// 生成随机方向 (2D单位向量)
fn RandomDirection2D(seed: vec2<f32>) -> vec2<f32> {
    let angle = Hash21(seed) * TWO_PI;
    return vec2<f32>(cos(angle), sin(angle));
}

// ============ 特殊数学函数 ============

/// 阶跃函数 (Heaviside)
fn Step(edge: f32, x: f32) -> f32 {
    if (x < edge) { return 0.0; }
    return 1.0;
}

/// 脉冲函数
fn Pulse(edge0: f32, edge1: f32, x: f32) -> f32 {
    return Step(edge0, x) - Step(edge1, x);
}

/// 三角波
fn TriangleWave(t: f32) -> f32 {
    return abs(Fract(t) * 2.0 - 1.0);
}

/// 方波
fn SquareWave(t: f32) -> f32 {
    return Step(0.5, Fract(t));
}

/// 锯齿波
fn SawtoothWave(t: f32) -> f32 {
    return Fract(t);
}

/// 抛物线函数
fn Parabola(x: f32, k: f32) -> f32 {
    return pow(4.0 * x * (1.0 - x), k);
}

/// 幂曲线
fn PowerCurve(x: f32, a: f32, b: f32) -> f32 {
    let k = pow(a + b, a + b) / (pow(a, a) * pow(b, b));
    return k * pow(x, a) * pow(1.0 - x, b);
}

/// Gain函数 (调整中间值的对比度)
/// <summary>
/// 计算增益函数
/// 注意：WGSL 不支持 ? : 操作符，需使用 select(falseVal, trueVal, cond)
/// </summary>
/// <param name="x">输入值</param>
/// <param name="k">增益系数</param>
fn Gain(x: f32, k: f32) -> f32 {
    let val = select(1.0 - x, x, x < 0.5);

    let a = 0.5 * pow(2.0 * val, k);

    return select(1.0 - a, a, x < 0.5);
}
/// Bias函数 (调整曲线偏向)
fn Bias(x: f32, bias: f32) -> f32 {
    return x / ((1.0 / bias - 2.0) * (1.0 - x) + 1.0);
}

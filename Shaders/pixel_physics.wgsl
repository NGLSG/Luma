struct Pixel {
    pixel_type: u32,
    color: u32,
    lifetime: f32,
    velocity_y: f32,
}

struct SimParams {
    width: u32,
    height: u32,
    frame: u32,
    dt: f32,
}

@group(0) @binding(0) var<storage, read> input: array<Pixel>;
@group(0) @binding(1) var<storage, read_write> output: array<Pixel>;
@group(0) @binding(2) var<uniform> params: SimParams;

const AIR: u32   = 0u;
const SAND: u32  = 1u;
const WATER: u32 = 2u;
const STONE: u32 = 3u;
const FIRE: u32  = 4u;
const STEAM: u32 = 5u;
const OIL: u32   = 6u;
const LAVA: u32  = 7u;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn in_bounds(x: i32, y: i32) -> bool {
    return x >= 0 && x < i32(params.width) && y >= 0 && y < i32(params.height);
}

fn hash(seed: u32) -> u32 {
    var s = seed;
    s ^= s >> 16u;
    s *= 0x45d9f3bu;
    s ^= s >> 16u;
    s *= 0x45d9f3bu;
    s ^= s >> 16u;
    return s;
}

fn get_type(x: i32, y: i32) -> u32 {
    if (!in_bounds(x, y)) { return STONE; }
    return input[idx(u32(x), u32(y))].pixel_type;
}

fn is_empty(x: i32, y: i32) -> bool {
    return get_type(x, y) == AIR;
}

fn is_liquid(t: u32) -> bool {
    return t == WATER || t == OIL || t == LAVA;
}

fn default_color(t: u32) -> u32 {
    switch (t) {
        case 1u: { return 0xFFC8B464u; } // sand
        case 2u: { return 0xFF6464C8u; } // water
        case 3u: { return 0xFF808080u; } // stone
        case 4u: { return 0xFF3264FFu; } // fire
        case 5u: { return 0xFFC8C8C8u; } // steam
        case 6u: { return 0xFF325050u; } // oil
        case 7u: { return 0xFF0040FFu; } // lava
        default: { return 0x00000000u; } // air
    }
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if (x >= params.width || y >= params.height) { return; }

    let checkerboard = (x + y + params.frame) % 2u;
    let i = idx(x, y);
    let me = input[i];

    if (checkerboard != 0u) {
        output[i] = me;
        return;
    }

    let ix = i32(x);
    let iy = i32(y);
    let rng = hash(x * 15823u + y * 9737u + params.frame * 6271u);

    switch (me.pixel_type) {
        case 1u: { // sand
            if (is_empty(ix, iy + 1)) {
                output[idx(x, y + 1u)] = Pixel(SAND, me.color, 0.0, me.velocity_y + 1.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                let dir = select(-1, 1, (rng & 1u) != 0u);
                let nx = ix + dir;
                if (is_empty(nx, iy + 1)) {
                    output[idx(u32(nx), y + 1u)] = Pixel(SAND, me.color, 0.0, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else if (is_liquid(get_type(ix, iy + 1))) {
                    let below = input[idx(x, y + 1u)];
                    output[idx(x, y + 1u)] = me;
                    output[i] = below;
                } else {
                    output[i] = Pixel(SAND, me.color, 0.0, 0.0);
                }
            }
        }
        case 2u: { // water
            if (is_empty(ix, iy + 1)) {
                output[idx(x, y + 1u)] = Pixel(WATER, me.color, 0.0, 0.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                let dir = select(-1, 1, (rng & 1u) != 0u);
                let nx = ix + dir;
                if (is_empty(nx, iy + 1)) {
                    output[idx(u32(nx), y + 1u)] = Pixel(WATER, me.color, 0.0, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else if (is_empty(nx, iy)) {
                    output[idx(u32(nx), y)] = Pixel(WATER, me.color, 0.0, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else {
                    output[i] = me;
                }
            }
        }
        case 4u: { // fire
            let life = me.lifetime + params.dt;
            if (life > 1.5) {
                output[i] = Pixel(STEAM, default_color(STEAM), 0.0, 0.0);
            } else if (is_empty(ix, iy - 1)) {
                let brightness = u32(clamp((1.5 - life) / 1.5, 0.0, 1.0) * 255.0);
                let c = 0xFF000000u | brightness | (brightness / 2u) << 8u;
                output[idx(x, y - 1u)] = Pixel(FIRE, c, life, 0.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                output[i] = Pixel(FIRE, me.color, life, 0.0);
            }
        }
        case 5u: { // steam
            let life = me.lifetime + params.dt;
            if (life > 3.0) {
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else if (is_empty(ix, iy - 1)) {
                output[idx(x, y - 1u)] = Pixel(STEAM, me.color, life, 0.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                let dir = select(-1, 1, (rng & 1u) != 0u);
                if (is_empty(ix + dir, iy)) {
                    output[idx(u32(ix + dir), y)] = Pixel(STEAM, me.color, life, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else {
                    output[i] = Pixel(STEAM, me.color, life, 0.0);
                }
            }
        }
        case 6u: { // oil - similar to water but slower, flammable
            if ((rng % 3u) == 0u && is_empty(ix, iy + 1)) {
                output[idx(x, y + 1u)] = Pixel(OIL, me.color, 0.0, 0.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                let dir = select(-1, 1, (rng & 1u) != 0u);
                if (is_empty(ix + dir, iy)) {
                    output[idx(u32(ix + dir), y)] = Pixel(OIL, me.color, 0.0, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else {
                    output[i] = me;
                }
            }
        }
        case 7u: { // lava - slow liquid, water contact -> stone + steam
            let below_type = get_type(ix, iy + 1);
            if (below_type == WATER) {
                output[idx(x, y + 1u)] = Pixel(STEAM, default_color(STEAM), 0.0, 0.0);
                output[i] = Pixel(STONE, default_color(STONE), 0.0, 0.0);
            } else if ((rng % 4u) == 0u && is_empty(ix, iy + 1)) {
                output[idx(x, y + 1u)] = Pixel(LAVA, me.color, 0.0, 0.0);
                output[i] = Pixel(AIR, 0u, 0.0, 0.0);
            } else {
                let dir = select(-1, 1, (rng & 1u) != 0u);
                if ((rng % 4u) == 0u && is_empty(ix + dir, iy)) {
                    output[idx(u32(ix + dir), y)] = Pixel(LAVA, me.color, 0.0, 0.0);
                    output[i] = Pixel(AIR, 0u, 0.0, 0.0);
                } else {
                    output[i] = me;
                }
            }
        }
        default: { // air (0) and stone (3) stay put
            output[i] = me;
        }
    }
}

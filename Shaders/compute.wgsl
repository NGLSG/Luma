// 输入的顶点数据
struct Vertex {
    position: vec2f,
    uv: vec2f,
};

// 输入的实例数据
struct InstanceData {
    position: vec2f,
    scale: vec2f,
    sinr: f32,
    cosr: f32,
};

// 输出的变换后顶点数据
struct TransformedVertex {
    position: vec2f,
    uv: vec2f,
};

@group(0) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(0) @binding(1) var<storage, read> instances : array<InstanceData>;
@group(0) @binding(2) var<storage, read_write> outputVertices : array<TransformedVertex>;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) id : vec3<u32>) {
    let vertexCount = 4u;  // 每个实例4个顶点
    let totalVertexCount = arrayLength(&instances) * vertexCount;

    if (id.x >= totalVertexCount) {
        return;
    }

    // 计算当前是哪个实例的哪个顶点
    let instanceIndex = id.x / vertexCount;
    let vertexIndex = id.x % vertexCount;

    // 读取顶点和实例数据
    let vertex = vertices[vertexIndex];
    let instance = instances[instanceIndex];

    // 缩放
    var local = vertex.position * instance.scale;

    // 旋转
    var rotated = vec2f(
        local.x * instance.cosr - local.y * instance.sinr,
        local.x * instance.sinr + local.y * instance.cosr
    );

    // 平移
    var world = rotated + instance.position;

    // 写入输出
    outputVertices[id.x].position = world;
    outputVertices[id.x].uv = vertex.uv;
}
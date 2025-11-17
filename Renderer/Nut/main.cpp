#include <format>
#include <future>
#include <iostream>
#include <random>
#include <windows.h>
#include <tchar.h>
#include <chrono>

#include "Buffer.h"
#include "NutContext.h"

struct Vertex
{
    float position[2];
    float uv[2];
};

struct InstanceData
{
    float position[2];
    float scale[2];
    float sinr;
    float cosr;
};

struct TransformedVertex
{
    float position[2];
    float uv[2];
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {1.0f, 1.0f}},
};

// 模拟实体数据
struct SimulationEntity
{
    float position[2];
    float velocity[2];
    float angle;
    float angularVelocity;
    float scale[2];
};

std::shared_ptr<NutContext> graphicsContext;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            graphicsContext->Resize(width, height);
            printf("窗口大小改变: 宽度=%d, 高度=%d\n", width, height);
            break;
        }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// CPU模拟进程 - 60Hz更新
class SimulationThread
{
public:
    SimulationThread(std::vector<SimulationEntity>& entities)
        : entities(entities)
          , running(false)
          , rng(std::random_device{}())
          , distVelocity(-0.3f, 0.3f)
          , distAngular(-1.0f, 1.0f)
    {
    }

    void Start()
    {
        running = true;
        thread = std::jthread([this](std::stop_token stopToken) { this->Run(stopToken); });
    }

    void Stop()
    {
        running = false;
        if (thread.joinable())
        {
            thread.request_stop();
            thread.join();
        }
    }

    std::vector<InstanceData> GetInstanceData()
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        std::vector<InstanceData> instances;
        instances.reserve(entities.size());

        for (const auto& entity : entities)
        {
            InstanceData data{};
            data.position[0] = entity.position[0];
            data.position[1] = entity.position[1];
            data.scale[0] = entity.scale[0];
            data.scale[1] = entity.scale[1];
            data.sinr = std::sin(entity.angle);
            data.cosr = std::cos(entity.angle);
            instances.push_back(data);
        }

        return instances;
    }

private:
    void Run(std::stop_token stopToken)
    {
        const auto frameTime = std::chrono::microseconds(16667); // 60Hz ≈ 16.667ms
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (running && !stopToken.stop_requested())
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // 更新模拟
            UpdateSimulation(deltaTime);

            // 等待下一帧
            std::this_thread::sleep_until(currentTime + frameTime);
        }
    }

    void UpdateSimulation(float deltaTime)
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        for (auto& entity : entities)
        {
            // 更新位置
            entity.position[0] += entity.velocity[0] * deltaTime;
            entity.position[1] += entity.velocity[1] * deltaTime;

            // 边界检测和反弹
            if (entity.position[0] < -1.0f || entity.position[0] > 1.0f)
            {
                entity.velocity[0] = -entity.velocity[0];
                entity.position[0] = std::clamp(entity.position[0], -1.0f, 1.0f);
            }
            if (entity.position[1] < -1.0f || entity.position[1] > 1.0f)
            {
                entity.velocity[1] = -entity.velocity[1];
                entity.position[1] = std::clamp(entity.position[1], -1.0f, 1.0f);
            }

            // 更新角度
            entity.angle += entity.angularVelocity * deltaTime;

            // 随机扰动速度(小幅度)
            if (std::uniform_int_distribution<>(0, 100)(rng) < 5) // 5%概率
            {
                entity.velocity[0] += distVelocity(rng) * 0.1f;
                entity.velocity[1] += distVelocity(rng) * 0.1f;

                // 限制最大速度
                float speedSq = entity.velocity[0] * entity.velocity[0] +
                    entity.velocity[1] * entity.velocity[1];
                const float maxSpeed = 0.5f;
                if (speedSq > maxSpeed * maxSpeed)
                {
                    float speed = std::sqrt(speedSq);
                    entity.velocity[0] = entity.velocity[0] / speed * maxSpeed;
                    entity.velocity[1] = entity.velocity[1] / speed * maxSpeed;
                }
            }
        }
    }

    std::vector<SimulationEntity>& entities;
    std::jthread thread;
    std::atomic<bool> running;
    std::mutex dataMutex;

    std::mt19937 rng;
    std::uniform_real_distribution<float> distVelocity;
    std::uniform_real_distribution<float> distAngular;
};

int WINAPI TESTFUNCTION_WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    const TCHAR CLASS_NAME[] = _T("DawnWin32DemoWindow");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("演示窗口"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
        return 0;

    GraphicsContextDescriptor graphicsDesc;
    graphicsDesc.width = 800;
    graphicsDesc.height = 600;
    graphicsDesc.windowHandle.hWnd = hwnd;
    graphicsDesc.windowHandle.hInst = GetModuleHandle(nullptr);
    graphicsDesc.backendTypePriority = {BackendType::D3D12, BackendType::D3D11, BackendType::Vulkan};
    graphicsContext = NutContext::Create(graphicsDesc);
    ShowWindow(hwnd, nCmdShow);

    // 创建采样器
    Sampler sampler;
    sampler.SetMagFilter(FilterMode::Linear)
           .SetMinFilter(FilterMode::Linear)
           .SetWrapModeU(WrapMode::ClampToEdge)
           .SetWrapModeV(WrapMode::ClampToEdge)
           .Build(graphicsContext);

    auto texture = graphicsContext->LoadTextureFromFile("Test.png");

    // 初始化模拟实体
    std::vector<SimulationEntity> simulationEntities;
    simulationEntities.reserve(10);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist01(-0.8f, 0.8f);
    std::uniform_real_distribution<float> distVelocity(-0.3f, 0.3f);
    std::uniform_real_distribution<float> distAngular(-1.0f, 1.0f);

    for (int i = 0; i < 10; ++i)
    {
        SimulationEntity entity{};
        entity.position[0] = dist01(rng);
        entity.position[1] = dist01(rng);
        entity.velocity[0] = distVelocity(rng);
        entity.velocity[1] = distVelocity(rng);
        entity.angle = dist01(rng) * 3.14159f;
        entity.angularVelocity = distAngular(rng);
        entity.scale[0] = 0.1f;
        entity.scale[1] = 0.1f;
        simulationEntities.push_back(entity);
    }

    // 启动模拟线程
    SimulationThread simThread(simulationEntities);
    simThread.Start();

    // 创建初始实例数据
    auto instances = simThread.GetInstanceData();

    // 创建输入顶点Buffer(用于计算着色器)
    auto inputVertexBuffer = BufferBuilder()
                             .SetUsage(BufferBuilder::GetCommonStorageUsage())
                             .SetData(vertices)
                             .Build(graphicsContext);

    // 创建实例Buffer(用于计算着色器)
    auto instanceBuffer = BufferBuilder()
                          .SetUsage(BufferBuilder::GetCommonStorageUsage())
                          .SetData(instances)
                          .Build(graphicsContext);

    // 创建输出顶点Buffer(计算着色器写入,顶点着色器读取)
    size_t totalVertexCount = vertices.size() * instances.size();
    std::vector<TransformedVertex> outputVertices(totalVertexCount);

    auto outputVertexBuffer = BufferBuilder()
                              .SetUsage(BufferBuilder::GetCommonStorageUsage() | BufferBuilder::GetCommonVertexUsage())
                              .SetData(outputVertices)
                              .Build(graphicsContext);

    // 创建索引Buffer
    std::vector<uint16_t> indices;
    for (size_t i = 0; i < instances.size(); ++i)
    {
        uint16_t baseIndex = static_cast<uint16_t>(i * 4);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    auto ibo = BufferBuilder()
               .SetUsage(BufferBuilder::GetCommonIndexUsage())
               .SetData(indices)
               .Build(graphicsContext);

    // 创建计算着色器Pipeline
    ShaderModule computeModule = ShaderManager::GetFromFile("Shaders/compute.wgsl", graphicsContext);
    ComputePipeline computePipeline({
        .entryPoint = "main",
        .shaderModule = computeModule,
        .context = graphicsContext
    });

    computePipeline.SetBinding("vertices", inputVertexBuffer);
    computePipeline.SetBinding("instances", instanceBuffer);
    computePipeline.SetBinding("outputVertices", outputVertexBuffer);
    computePipeline.BuildBindings(graphicsContext);

    // 创建渲染Pipeline
    ShaderModule renderModule = ShaderManager::GetFromFile("Shaders/normal.wgsl", graphicsContext);

    ColorTargetState colorTarget;
    colorTarget.SetFormat(wgpu::TextureFormat::BGRA8Unorm);

    // 顶点布局(使用变换后的顶点)
    VertexBufferLayout transformedVboLayout;
    transformedVboLayout.arrayStride = sizeof(TransformedVertex);
    transformedVboLayout.attributes = {
        VertexAttribute(&TransformedVertex::position, 0),
        VertexAttribute(&TransformedVertex::uv, 1),
    };
    transformedVboLayout.stepMode = VertexStepMode::Vertex;

    std::vector<VertexBufferLayout> vboLayouts = {transformedVboLayout};

    FragmentState fragment({colorTarget}, renderModule);
    VertexState vertex(vboLayouts, renderModule);

    RenderPipeline pipeline({vertex, fragment, renderModule, graphicsContext});

    pipeline.SetBinding("mySampler", sampler)
            .SetBinding("myTexture", texture)
            .BuildBindings(graphicsContext);

    // 消息循环
    bool running = true;
    MSG msg = {};
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 从模拟线程获取最新的实例数据
        instances = simThread.GetInstanceData();

        // 更新实例Buffer
        instanceBuffer.WriteBuffer(instances.data(), instances.size() * sizeof(InstanceData), 0);

        graphicsContext->ClearCommands();

        // 计算Pass - 预先计算所有顶点变换
        {
            auto computePass = graphicsContext->BeginComputeFrame()
                                              .SetLabel("顶点变换计算")
                                              .Build();

            computePass.SetPipeline(computePipeline);

            // 计算需要的工作组数量
            uint32_t totalCount = static_cast<uint32_t>(totalVertexCount);
            uint32_t groupCount = (totalCount + 63) / 64;
            computePass.Dispatch(groupCount);

            auto cmdCompute = graphicsContext->EndComputeFrame(computePass);
            graphicsContext->Submit({cmdCompute});
        }

        // 离屏渲染到图片
        {
            auto shadowMap = graphicsContext->CreateOrGetRenderTarget("ShadowMap", 100, 100);
            graphicsContext->SetActiveRenderTarget(shadowMap);

            TextureA currentTexture = graphicsContext->GetCurrentTexture();
            if (!currentTexture)
            {
                continue;
            }

            auto pass = graphicsContext->BeginRenderFrame()
                                       .SetLabel("离屏渲染通道")
                                       .AddColorAttachment(
                                           ColorAttachmentBuilder().SetTexture(currentTexture)
                                                                   .SetClearColor({0.f, 0.f, 0.f, 1.0f})
                                                                   .SetLoadOnOpen(LoadOnOpen::Clear)
                                                                   .SetStoreOnOpen(StoreOnOpen::Store)
                                                                   .Build()
                                       )
                                       .Build();
            pass.SetPipeline(pipeline);
            pass.SetVertexBuffer(0, outputVertexBuffer);
            pass.SetIndexBuffer(ibo, wgpu::IndexFormat::Uint16);
            pass.DrawIndexed(indices.size(), 1);
            auto cmdBuffer1 = graphicsContext->EndRenderFrame(pass);

            graphicsContext->Submit({cmdBuffer1});
        }

        // 渲染到窗口
        {
            graphicsContext->SetActiveRenderTarget(nullptr);
            TextureA currentTexture = graphicsContext->GetCurrentTexture();
            if (!currentTexture)
            {
                continue;
            }

            std::vector<wgpu::CommandBuffer> commandBuffers;
            std::jthread testPush([&]()
            {
                auto pass = graphicsContext->BeginRenderFrame()
                                           .SetLabel("主渲染通道")
                                           .AddColorAttachment(
                                               ColorAttachmentBuilder().SetTexture(currentTexture)
                                                                       .SetClearColor({0.2f, 0.3f, 0.5f, 1.0f})
                                                                       .SetLoadOnOpen(LoadOnOpen::Clear)
                                                                       .SetStoreOnOpen(StoreOnOpen::Store)
                                                                       .Build()
                                           )
                                           .Build();
                pass.SetPipeline(pipeline);
                pass.SetVertexBuffer(0, outputVertexBuffer);
                pass.SetIndexBuffer(ibo, wgpu::IndexFormat::Uint16);
                pass.DrawIndexed(indices.size(), 1);
                commandBuffers.emplace_back(graphicsContext->EndRenderFrame(pass));
            });
            testPush.join();

            graphicsContext->Submit(commandBuffers);
            graphicsContext->Present();
        }
    }

    // 停止模拟线程
    simThread.Stop();

    return 0;
}

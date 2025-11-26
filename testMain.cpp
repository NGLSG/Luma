#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <string>


#include "Application/Window.h"
#include "Renderer/GraphicsBackend.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Camera.h"
#include "Renderer/Nut/NutContext.h"
#include "Resources/RuntimeAsset/RuntimeWGSLMaterial.h"
#include "dawn/dawn_proc.h"


#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkGradientShader.h"


using namespace Nut;

TextureAPtr CreateFallbackTexture(std::shared_ptr<NutContext> context, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t pixel[] = {r, g, b, 255};
    return TextureBuilder()
           .SetPixelData(pixel, 1, 1, 4)
           .SetSize(1, 1)
           .SetFormat(wgpu::TextureFormat::RGBA8Unorm)
           .Build(context);
}

int main(int argc, char* argv[])
{
    dawnProcSetProcs(&dawn::native::GetProcs());

    const int WIDTH = 1280;
    const int HEIGHT = 720;
    auto window = PlatformWindow::Create("LumaEngine Hybrid Rendering (Skia + WGPU)", WIDTH, HEIGHT);
    if (!window) return -1;

    GraphicsBackendOptions options;
    options.windowHandle = window->GetNativeWindowHandle();
    options.width = WIDTH;
    options.height = HEIGHT;
    options.backendTypePriority = {BackendType::D3D12, BackendType::Vulkan, BackendType::Metal};
    options.qualityLevel = ::QualityLevel::High;
    options.enableVSync = true;

    auto backend = GraphicsBackend::Create(options);
    if (!backend)
    {
        std::cerr << "GraphicsBackend Init Failed" << std::endl;
        return -1;
    }
    std::cout << "Backend Initialized." << std::endl;

    auto renderSystem = RenderSystem::Create(*backend);
    auto nutContext = backend->GetNutContext();

    auto texture = nutContext->LoadTextureFromFile("./Test.png");
    if (!texture)
    {
        std::cout << "Test.png not found, creating fallback texture." << std::endl;
        texture = CreateFallbackTexture(nutContext, 255, 255, 255);
    }

    Camera::CamProperties camProps;
    camProps.position = {0.0f, 0.0f};
    camProps.viewport = SkRect::MakeXYWH(0, 0, (float)WIDTH, (float)HEIGHT);
    camProps.zoom = {1.0f,1.0f};
    camProps.rotation = 0.0f;
    camProps.clearColor = {0.15f, 0.15f, 0.15f, 1.0f};
    Camera::GetInstance().SetProperties(camProps);

    std::vector<RenderableTransform> transforms;
    const int COUNT = 1;
    for (int i = 0; i < COUNT; ++i)
    {
        
        float x = (i - (COUNT - 1) / 2.0f) * 120.0f;
        transforms.emplace_back(SkPoint{x, 0}, .1f, .1f, 0.0f);
    }

    std::cout << "Starting Render Loop..." << std::endl;
    bool running = true;
    window->OnCloseRequest += [&]() { running = false; };

    float time = 0.0f;
    while (running && !window->ShouldClose())
    {
        window->PollEvents();
        time += 0.02f;

        for (size_t i = 0; i < transforms.size(); ++i)
        {
            float angle = time + i * 0.5f;
            transforms[i].sinR = sin(angle);
            transforms[i].cosR = cos(angle);
        }

        
        if (backend->BeginFrame())
        {
            WGPUSpriteBatch batch;
            batch.image = texture;
            batch.transforms = transforms.data();
            batch.count = transforms.size();
            batch.color = {1.0f, 1.0f, 1.0f, 1.0f};
            batch.filterQuality = 1;
            renderSystem->Submit(batch);

            RawDrawBatch skiaBatch;
            skiaBatch.drawFunc += [WIDTH, HEIGHT](SkCanvas* canvas)
            {
                SkPaint paint;
                paint.setColor(SK_ColorRED);
                SkRect rect = SkRect::MakeXYWH(50, 50, 200, 200);
                canvas->drawRect(rect, paint);
            };
            renderSystem->Submit(skiaBatch);

            renderSystem->Submit(batch);

            renderSystem->Flush();
            backend->Submit();
            backend->PresentFrame();
        }

        SDL_Delay(16);
    }

    return 0;
}

#include "ImGuiRenderer.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_wgpu.h"
#include <stdexcept>
#include "GraphicsBackend.h"
#include "../Utils/Path.h"
#include "../Utils/Logger.h"
ImGuiRenderer::ImGuiRenderer(SDL_Window* window, const wgpu::Device& device, wgpu::TextureFormat renderTargetFormat)
    : m_device(device), m_isInitialized(false)
{
    if (!window)
    {
        throw std::runtime_error("ImGuiRenderer: SDL 窗口指针为空");
    }
    if (!m_device)
    {
        throw std::runtime_error("ImGuiRenderer: WebGPU 设备无效");
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    if (!ImGui_ImplSDL3_InitForOther(window))
    {
        ImGui::DestroyContext();
        throw std::runtime_error("ImGuiRenderer: 初始化 ImGui SDL3 后端失败");
    }
    ApplyEditorStyle();
    ImGui_ImplWGPU_InitInfo initInfo = {};
    initInfo.Device = m_device.Get();  
    initInfo.NumFramesInFlight = 1;
    initInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>(renderTargetFormat);
    initInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;
    if (!ImGui_ImplWGPU_Init(&initInfo))
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("ImGuiRenderer: 初始化 ImGui WGPU 后端失败");
    }
    m_isInitialized = true;
    LogInfo("ImGuiRenderer 初始化成功");
}
ImGuiRenderer::~ImGuiRenderer()
{
    if (m_isInitialized)
    {
        m_textureCache.clear();
        m_activeTexturesInFrame.clear();
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_isInitialized = false;
        LogInfo("ImGuiRenderer 已销毁");
    }
}
void ImGuiRenderer::NewFrame()
{
    if (!m_isInitialized)
    {
        throw std::runtime_error("ImGuiRenderer::NewFrame: 渲染器未初始化");
    }
    if (!m_device)
    {
        throw std::runtime_error("ImGuiRenderer::NewFrame: WebGPU 设备无效");
    }
    if (!m_activeTexturesInFrame.empty())
    {
        for (auto it = m_textureCache.begin(); it != m_textureCache.end();)
        {
            if (std::ranges::find(m_activeTexturesInFrame, it->first) == m_activeTexturesInFrame.end())
            {
                it = m_textureCache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    m_activeTexturesInFrame.clear();
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}
void ImGuiRenderer::Render(wgpu::RenderPassEncoder renderPass)
{
    if (!m_isInitialized)
    {
        LogWarn("ImGuiRenderer::Render: 渲染器未初始化，跳过渲染");
        return;
    }
    ImGui::Render();
    if (renderPass)
    {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
    }
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
void ImGuiRenderer::EndFrame(const GraphicsBackend& backend)
{
    if (!m_isInitialized)
    {
        return;
    }
    wgpu::TextureView frameView = backend.GetCurrentFrameView();
    if (!frameView)
    {
        LogWarn("ImGuiRenderer::EndFrame: 无法获取当前帧视图");
        return;
    }
    wgpu::CommandEncoder encoder = backend.GetDevice().CreateCommandEncoder();
    if (!encoder)
    {
        LogError("ImGuiRenderer::EndFrame: 创建命令编码器失败");
        return;
    }
    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = frameView;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {0.15f, 0.16f, 0.18f, 1.0f};
    wgpu::RenderPassDescriptor passDesc{};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &colorAttachment;
    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&passDesc);
    Render(renderPass);
    renderPass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    backend.GetDevice().GetQueue().Submit(1, &commands);
}
ImTextureID ImGuiRenderer::GetOrCreateTextureIdFor(wgpu::Texture texture)
{
    if (!texture)
    {
        return reinterpret_cast<ImTextureID>(nullptr);
    }
    WGPUTexture textureHandle = texture.Get();
    m_activeTexturesInFrame.push_back(textureHandle);
    auto it = m_textureCache.find(textureHandle);
    if (it != m_textureCache.end())
    {
        return reinterpret_cast<ImTextureID>(it->second.Get());
    }
    wgpu::TextureViewDescriptor viewDesc = {};
    viewDesc.format = texture.GetFormat();
    viewDesc.dimension = wgpu::TextureViewDimension::e2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = wgpu::TextureAspect::All;
    wgpu::TextureView view = texture.CreateView(&viewDesc);
    if (!view)
    {
        LogError("ImGuiRenderer::GetOrCreateTextureIdFor: 创建纹理视图失败");
        return reinterpret_cast<ImTextureID>(nullptr);
    }
    m_textureCache[textureHandle] = view;
    return reinterpret_cast<ImTextureID>(view.Get());
}
void ImGuiRenderer::ProcessEvent(const SDL_Event& event)
{
    ImGui_ImplSDL3_ProcessEvent(&event);
}
void ImGuiRenderer::ApplyEditorStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.56f, 0.80f, 0.26f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.76f, 0.28f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.90f, 0.42f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.52f, 0.96f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.21f, 0.38f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
std::string ImGuiRenderer::LoadFonts(const std::string& fontPath, float dpiScale)
{
    if (!Path::Exists(fontPath))
    {
        LogError("ImGuiRenderer::LoadFonts: 字体文件不存在: {}", fontPath);
        return "";
    }
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig mainConfig;
    mainConfig.FontDataOwnedByAtlas = false;
    mainConfig.SizePixels = 16.0f * dpiScale;
    mainConfig.RasterizerMultiply = dpiScale;
    mainConfig.GlyphRanges = io.Fonts->GetGlyphRangesChineseFull();
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), mainConfig.SizePixels, &mainConfig,
                                                    mainConfig.GlyphRanges);
    if (!mainFont)
    {
        LogError("ImGuiRenderer::LoadFonts: 加载主字体失败");
        return "";
    }
    ImFontConfig emojiConfig;
    emojiConfig.FontDataOwnedByAtlas = false;
    emojiConfig.SizePixels = 16.0f * dpiScale;
    emojiConfig.RasterizerMultiply = dpiScale;
    emojiConfig.MergeMode = true;
    emojiConfig.GlyphMinAdvanceX = 16.0f * dpiScale;
    static const ImWchar emojiRanges[] =
    {
        0x1, 0xFFFF,
        0,
    };
    const char* emojiFontPaths[] =
    {
        "C:/Windows/Fonts/seguiemj.ttf",
        "C:/Windows/Fonts/NotoColorEmoji.ttf",
        "/System/Library/Fonts/Apple Color Emoji.ttc",
        "/usr/share/fonts/truetype/noto-color-emoji/NotoColorEmoji.ttf"
    };
    for (const char* emojiPath : emojiFontPaths)
    {
        if (Path::Exists(emojiPath))
        {
            io.Fonts->AddFontFromFileTTF(emojiPath, emojiConfig.SizePixels, &emojiConfig, emojiRanges);
        }
    }
    ImFontConfig symbolConfig;
    symbolConfig.FontDataOwnedByAtlas = false;
    symbolConfig.SizePixels = 16.0f * dpiScale;
    symbolConfig.RasterizerMultiply = dpiScale;
    symbolConfig.MergeMode = true;
    static const ImWchar basicSymbolRanges[] =
    {
        0x2000, 0x27FF,
        0,
    };
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), symbolConfig.SizePixels, &symbolConfig, basicSymbolRanges);
    io.Fonts->Build();
    auto name = Path::GetFileNameWithoutExtension(fontPath);
    m_fonts[name] = mainFont;
    LogInfo("ImGuiRenderer::LoadFonts: 成功加载字体: {}", name);
    return name;
}
void ImGuiRenderer::SetFont(const std::string& fontName)
{
    auto it = m_fonts.find(fontName);
    if (it != m_fonts.end())
    {
        ImGui::GetIO().FontDefault = it->second;
        LogInfo("ImGuiRenderer::SetFont: 设置默认字体为: {}", fontName);
    }
    else
    {
        throw std::runtime_error("ImGuiRenderer::SetFont: 字体未找到: " + fontName);
    }
}

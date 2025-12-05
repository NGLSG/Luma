#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H
#include <webgpu/webgpu_cpp.h>
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "imgui.h"
class GraphicsBackend;
class ImGuiRenderer
{
public:
    ImGuiRenderer(SDL_Window* window, const wgpu::Device& device, wgpu::TextureFormat renderTargetFormat);
    ~ImGuiRenderer();
    ImGuiRenderer(const ImGuiRenderer&) = delete;
    ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;
    ImGuiRenderer(ImGuiRenderer&&) = delete;
    ImGuiRenderer& operator=(ImGuiRenderer&&) = delete;
    void NewFrame();
    void Render(wgpu::RenderPassEncoder renderPass);
    void EndFrame(const GraphicsBackend& backend);
    ImTextureID GetOrCreateTextureIdFor(wgpu::Texture texture);
    static void ProcessEvent(const SDL_Event& event);
    static void ApplyEditorStyle();
    std::string LoadFonts(const std::string& fontPath, float dpiScale);
    void SetFont(const std::string& fontName);
private:
    wgpu::Device m_device;                                        
    std::unordered_map<WGPUTexture, wgpu::TextureView> m_textureCache;  
    std::vector<WGPUTexture> m_activeTexturesInFrame;             
    std::unordered_map<std::string, ImFont*> m_fonts;             
    bool m_isInitialized;                                         
};
#endif 

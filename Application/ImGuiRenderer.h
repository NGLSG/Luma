#pragma once
#include <webgpu/webgpu_cpp.h>
#include <SDL3/SDL.h>
#include <memory>
#include <unordered_map>

#include "imgui.h"
#include "../Renderer/GraphicsBackend.h"


struct ImFont;


/**
 * @brief ImGui渲染器，负责初始化、管理和渲染ImGui界面。
 */
class ImGuiRenderer
{
public:
    /**
     * @brief 构造ImGui渲染器实例。
     * @param window SDL窗口指针，用于处理输入和上下文。
     * @param device WebGPU设备，用于创建渲染资源。
     * @param renderTargetFormat 渲染目标的纹理格式。
     */
    ImGuiRenderer(SDL_Window* window, wgpu::Device device, wgpu::TextureFormat renderTargetFormat);
    /**
     * @brief 销毁ImGui渲染器实例。
     */
    ~ImGuiRenderer();


    /**
     * @brief 开始一个新的ImGui帧。
     */
    void NewFrame();


    /**
     * @brief 将ImGui的绘制数据渲染到指定的渲染通道编码器。
     * @param renderPass 渲染通道编码器。
     */
    void Render(wgpu::RenderPassEncoder renderPass);


    /**
     * @brief 结束当前的ImGui帧，并处理后端操作。
     * @param backend 图形后端实例。
     */
    void EndFrame(const GraphicsBackend& backend);


    /**
     * @brief 为给定的WebGPU纹理获取或创建一个ImGui纹理ID。
     * @param texture WebGPU纹理。
     * @return ImGui纹理ID。
     */
    ImTextureID GetOrCreateTextureIdFor(wgpu::Texture texture);

    /**
     * @brief 处理SDL事件，供ImGui使用。
     * @param event SDL事件。
     */
    static void ProcessEvent(const SDL_Event& event);


    /**
     * @brief 应用编辑器风格的ImGui样式。
     */
    void ApplyEditorStyle();
    /**
     * @brief 从指定路径加载字体。
     * @param fontPath 字体文件路径。
     * @param dpiScale DPI缩放因子。
     * @return 加载的字体名称。
     */
    std::string LoadFonts(const std::string& fontPath, float dpiScale = 1.0f);
    /**
     * @brief 设置当前使用的字体。
     * @param fontName 字体名称。
     */
    void SetFont(const std::string& fontName);

private:
    wgpu::Device m_device; ///< WebGPU设备实例。
    std::unordered_map<std::string, ImFont*> m_fonts; ///< 已加载的字体映射。


    std::unordered_map<WGPUTexture, wgpu::TextureView> m_textureCache; ///< 纹理缓存，用于将WebGPU纹理映射到ImGui纹理视图。


    std::vector<WGPUTexture> m_activeTexturesInFrame; ///< 当前帧中活跃的WebGPU纹理列表。
};